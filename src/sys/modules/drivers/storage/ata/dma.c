/*
 * Idylla Operating System
 * Copyright (C) 2009-2012 Idylla Operating System Team
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
*/
#include <arch/asm.h>
#include <arch/page.h>
#include <kernel/types.h>
#include <kernel/debug.h>
#include <mm/phys.h>
#include <lib/math.h>
#include <modules/drivers/storage/ata.h>

int dma_xfer_prepare(struct ata_channel * chan, uint8_t device, unsigned rw, uint32_t bytes)
{
	uint32_t addr;
	uint8_t status;
	
	if (!chan->prdtable_phys)
	{
		/* Pierwsze wywołanie, alokujemy pamiec */
		chan->prdtable_phys = phys_alloc(1, PHYS_ZONE_DMA);
		chan->prd = (struct ata_dma_prdtable *)KERNEL_PHYS2VIRT(chan->prdtable_phys);
	}
	
	/* TODO: Odczyt więcej niż 64kb */
	if (!chan->dma_buf_phys)
	{
		chan->dma_buf_phys = phys_alloc(ATA_DMA_BUF_SIZE / PAGE_SIZE, PHYS_ZONE_DMA);
		chan->dma_buf = (void *)KERNEL_PHYS2VIRT(chan->dma_buf_phys);
	}
	
	chan->prd->addr = chan->dma_buf_phys;
	chan->prd->size = bytes;
	chan->prd->last = 0x8000;
	chan->dma_xfer = 1;
	
	/* Ustaw adres PDRT */
	addr = inportd(chan->bdma + ATA_REG_BDMA_PRDT);
	addr &= 0x3;
	addr |= chan->prdtable_phys;
	outportd(chan->bdma + ATA_REG_BDMA_PRDT, addr);
	
	/* Czyscimy bity bledu i IRQ */
	status = ATA_READ_BDMA(chan, ATA_REG_BDMA_STATUS);
	status |= ATA_BDMA_STAT_ERROR | ATA_BDMA_STAT_IRQ;
	ATA_WRITE_BDMA(chan, ATA_REG_BDMA_STATUS, status);
	
	/* Ustawiamy kierunek transferu */
	status = ATA_READ_BDMA(chan, ATA_REG_BDMA_COMMAND);
	if (rw == ATA_RW_READ)
		status |= ATA_BDMA_CMD_READ;
	else
		status &= ~ATA_BDMA_CMD_READ;
	ATA_WRITE_BDMA(chan, ATA_REG_BDMA_COMMAND, status);
	
	return 0;
}

int dma_xfer_start(struct ata_channel * chan)
{
	uint8_t cmd;
	
	cmd = ATA_READ_BDMA(chan, ATA_REG_BDMA_COMMAND);
	cmd |= ATA_BDMA_CMD_START;
	ATA_WRITE_BDMA(chan, ATA_REG_BDMA_COMMAND, cmd);
	return 0;
}

int dma_xfer_finish(struct ata_channel * chan)
{
	uint8_t cmd;
	
	cmd = ATA_READ_BDMA(chan, ATA_REG_BDMA_COMMAND);
	cmd &= ~ATA_BDMA_CMD_START;
	ATA_WRITE_BDMA(chan, ATA_REG_BDMA_COMMAND, cmd);
	chan->dma_xfer = 0;
	
	return 0;
}

