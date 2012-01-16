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
#include <arch/cpu.h>
#include <kernel/types.h>
#include <kernel/kprintf.h>
#include <lib/string.h>
#include <lib/errno.h>
#include <modules/drivers/storage/storage.h>
#include <modules/drivers/storage/ata.h>

static unsigned lba_setup(struct ata_channel * chan, loff_t lba, uint8_t seccount)
{
	uint8_t lba_io[6];
	unsigned lba_mode = ATA_NO_LBA;

	if (lba >= 0x1000000) /* Use LBA48 */
	{
		lba_io[0] = (lba & 0x0000000000FF) >> 0;
		lba_io[1] = (lba & 0x00000000FF00) >> 8;
		lba_io[2] = (lba & 0x000000FF0000) >> 16;
		lba_io[3] = (lba & 0x0000FF000000) >> 24;
		lba_io[4] = (lba & 0x00FF00000000) >> 32;
		lba_io[5] = (lba & 0xFF0000000000) >> 40;
		lba_mode = ATA_LBA48;
	}
	else /* Use LBA28 */
	{
		lba_io[0] = (lba & 0x000000FF) >> 0;
		lba_io[1] = (lba & 0x0000FF00) >> 8;
		lba_io[2] = (lba & 0x00FF0000) >> 16;
		lba_io[3] = 0;
		lba_io[4] = 0;
		lba_io[5] = 0;
		lba_mode = ATA_LBA28;
	}

	/* Select device & enable LBA mode */
	ATA_WRITE_BASE(chan, ATA_REG_HEAD, 0xE0 | ((chan->selected & 0x01) << 4));

	if (lba_mode == ATA_LBA48)
	{
		ATA_WRITE_CTRL(chan, ATA_REG_CTRL, ATA_CTRL_HOB);
		ATA_WRITE_BASE(chan, ATA_REG_SECCOUNT, 0);
		ATA_WRITE_BASE(chan, ATA_REG_SEC, lba_io[3]);
		ATA_WRITE_BASE(chan, ATA_REG_CYLLO, lba_io[4]);
		ATA_WRITE_BASE(chan, ATA_REG_CYLHI, lba_io[5]);
		ATA_WRITE_CTRL(chan, ATA_REG_CTRL, 0);
	}

	ATA_WRITE_BASE(chan, ATA_REG_SECCOUNT, seccount);
	ATA_WRITE_BASE(chan, ATA_REG_SEC, lba_io[0]);
	ATA_WRITE_BASE(chan, ATA_REG_CYLLO, lba_io[1]);
	ATA_WRITE_BASE(chan, ATA_REG_CYLHI, lba_io[2]);

	return lba_mode;
}

static ssize_t pata_do_rw(struct ata_channel * chan, uint8_t device, void * buf, size_t len, loff_t off, unsigned rw)
{
	int err, i, do_sects;
	uint8_t cmd = 0, dma = chan->devices[device].dma;
	ssize_t done = 0;
	unsigned lba_mode;

	/* Wybierz urządzenie */
	ata_device_select(chan, device);

	err = ata_channel_poll(chan, ATA_STAT_BSY, 0);
	if (err != 0)
		return -EBUSY;

	while(len > 0)
	{
		do_sects = len;
		if (do_sects >= 128)
			do_sects = 128;
		len -= do_sects;

		/* Czekamy na urządzenie */
		err = ata_channel_poll(chan, ATA_STAT_RDY, 1);
		if (err != 0)
			return -EBUSY;
		
		if (dma)
		{
			dma_xfer_prepare(chan, device, rw, do_sects * 512);
		}
		
		lba_mode = lba_setup(chan, off, do_sects);
		if ((lba_mode == ATA_LBA28) && (rw == ATA_RW_READ)) cmd = dma ? ATA_CMD_READ_DMA : ATA_CMD_READ_PIO;
		else if ((lba_mode == ATA_LBA48) && (rw == ATA_RW_READ)) cmd = dma ? ATA_CMD_READ_DMA_EXT : ATA_CMD_READ_PIO_EXT;
		else if ((lba_mode == ATA_LBA28) && (rw == ATA_RW_WRITE)) cmd = dma ? ATA_CMD_WRITE_DMA : ATA_CMD_WRITE_PIO;
		else if ((lba_mode == ATA_LBA48) && (rw == ATA_RW_WRITE)) cmd = dma ? ATA_CMD_WRITE_DMA_EXT : ATA_CMD_WRITE_PIO_EXT;

		err = ata_channel_poll(chan, ATA_STAT_BSY, 0);
		if (err != 0)
			return -EBUSY;
		
		ATA_WRITE_BASE(chan, ATA_REG_CMD, cmd);
		DELAY400NS();
	
		if (dma)
		{
			if (rw == ATA_RW_WRITE)
				memcpy(chan->dma_buf, buf, do_sects * 512);
			
			dma_xfer_start(chan);
			err = ata_channel_wait(chan);
			if (err != 0)
				return err;
			
			if (rw == ATA_RW_READ)
				memcpy(buf, chan->dma_buf, do_sects * 512);
			
			done += do_sects;
			
			dma_xfer_finish(chan);
		}
		else
		{
			for(i=0;i<do_sects;i++)
			{
				if (rw == ATA_RW_READ)
				{		
					err = ata_channel_wait(chan);
					if (err != 0)
						return err;
					
					pio_xfer_in(chan, buf, 512);

				}
				else
				{
					pio_xfer_out(chan, buf, 512);

					err = ata_channel_wait(chan);
					if (err != 0)
						return err;
				}
				buf += 512;
				done++;
			}
		}
		
		off += done;
	}

	return done;
}

static ssize_t pata_read(struct storage * storage, void * buf, size_t size, loff_t off)
{
	ssize_t err;
	int dev;
	struct ata_channel * chan = storage->dataptr;
	
	/* Blokujemy kanał */
	mutex_lock(&chan->mutex);
	dev = ata_id2dev(chan, storage->devid);
	if (dev < 0)
		err = -ENODEV;
	else
		err = pata_do_rw(chan, dev, buf, size, off, ATA_RW_READ);
	mutex_unlock(&chan->mutex);
	
	return err;
}

static ssize_t pata_write(struct storage * storage, void * buf, size_t size, loff_t off)
{
	ssize_t err;
	int dev;
	struct ata_channel * chan = storage->dataptr;
	
	/* Blokujemy kanał */
	mutex_lock(&chan->mutex);
	dev = ata_id2dev(chan, storage->devid);
	if (dev < 0)
		err = -ENODEV;
	else
		err = pata_do_rw(chan, dev, buf, size, off, ATA_RW_WRITE);
	mutex_unlock(&chan->mutex);
	
	return err;
}

struct storage_ops __pata_ops = 
{
	.open = NULL,
	.close = NULL,
	.read = pata_read,
	.write = pata_write,
	.ioctl = NULL
};
