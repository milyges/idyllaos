/*
 * Idylla Operating System
 * Copyright (C) 2009  Idylla Operating System Team
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
#include <modules/drivers/storage/ata.h>
#include <lib/errno.h>

static int patapi_send_pkt(struct ata_channel * chan, uint8_t * pkt, uint16_t pkt_len)
{
	int err;

	/* Czekamy na urządzenie */
	err = ata_channel_poll(chan, (ATA_STAT_BSY | ATA_STAT_DRQ), 0);
	if (err != 0)
		return -EBUSY;

	/* Ustawiamy dlugość pakietu */
	ATA_WRITE_BASE(chan, ATA_REG_CYLLO, (pkt_len & 0x00FF));
	ATA_WRITE_BASE(chan, ATA_REG_CYLHI, (pkt_len & 0xFF00) >> 8);

	/* Wysyłamy polecenie PACKET */
	ATA_WRITE_BASE(chan, ATA_REG_CMD, ATA_CMD_PATAPIPACKET);

	/* Czekamy na urządzenie */
	err = ata_channel_poll(chan, ATA_STAT_DRQ, 1);
	if (err != 0)
		return err;

	err = ata_channel_poll(chan, ATA_STAT_BSY, 0);
	if (err != 0)
		return err;

	/* Wysyłamy pakiet */
	pio_xfer_out(chan, pkt, pkt_len);

	return ata_channel_wait(chan);
}

int patapi_reset(struct ata_channel * chan, uint8_t device)
{
	int err;

	/* Wybierz napęd */
	ata_device_select(chan, device);

	/* Poczekaj na urządzenie */
	err = ata_channel_poll(chan, ATA_STAT_BSY, 0);
	if (err != 0)
		return -EBUSY;

	/* Wyślij polecenie RESET */
	ATA_WRITE_BASE(chan, ATA_REG_CMD, ATA_CMD_PATAPIRESET);
	DELAY400NS();

	/* Poczekaj na wykonanie polecenia */
	err = ata_channel_poll(chan, ATA_STAT_BSY, 0);
	if (err != 0)
		return -EBUSY;

	return 0;
}

/* Wsyń napęd */
int patapi_load(struct ata_channel * chan, uint8_t device)
{
	ata_device_select(chan, device);
	return patapi_send_pkt(chan, ATA_PATAPI_PKT_LOAD, 12);
}

/* Wysuń napęd */
int patapi_eject(struct ata_channel * chan, uint8_t device)
{
	ata_device_select(chan, device);
	return patapi_send_pkt(chan, ATA_PATAPI_PKT_EJECT, 12);
}
