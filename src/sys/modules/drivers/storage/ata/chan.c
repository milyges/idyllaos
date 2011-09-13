/*
 * Idylla Operating System
 * Copyright (C) 2009-2010 Idylla Operating System Team
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

static void ata_fix_string(uint8_t * buf, uint16_t wcount)
{
	uint8_t tmp;
	int i;
	for (i=0;i<wcount;i++)
	{
		tmp = buf[i * 2];
		buf[i * 2] = buf[i * 2 + 1];
		buf[i * 2 + 1] = tmp;
	}

	for(i=(wcount * 2)-1;i>=0;i--)
	{
		if (buf[i] == ' ')
			buf[i] = '\0';
		else
			break;
	}
}

void ata_device_select(struct ata_channel * chan, uint8_t dev)
{
	dev &= 0x01;

	if (chan->selected == dev)
		return;

	ATA_WRITE_BASE(chan, ATA_REG_HEAD, 0xA0 | (dev << 4));
	DELAY400NS();

	chan->selected = dev;
}

int ata_channel_reset(struct ata_channel * chan)
{
	uint8_t data;
	unsigned timeout;

	/* Reset programowy */
	ATA_WRITE_CTRL(chan, ATA_REG_CTRL, ATA_CTRL_RESET);
	DELAY400NS();

	ATA_WRITE_CTRL(chan, ATA_REG_CTRL, 0);
	DELAY400NS();

	timeout = 2000; /* 2sec */
	while(--timeout)
	{
		data = ATA_READ_CTRL(chan, ATA_REG_ALTSTAT);
		if ((data & ATA_STAT_BSY) != ATA_STAT_BSY)
			return 0;
		delay(1);
	}

	return -1;
}

int ata_channel_wait(struct ata_channel * chan)
{
	uint8_t data = 0;
	unsigned timeout = 500; /* 5sec */

	while(--timeout)
	{
		data = ATA_READ_CTRL(chan, ATA_REG_ALTSTAT);
		if ((data & ATA_STAT_ERR) == ATA_STAT_ERR) /* Sprawdzamy błąd */
			break;

		if (atomic_get(&chan->irq_counter) > 0)
			break;
		delay(10);
	}

	if (!timeout)
	{
		kprintf(KERN_WARN"ata_wait(): timeout\n");
		return -EIO;
	}

	atomic_dec(&chan->irq_counter);

	data = ATA_READ_BASE(chan, ATA_REG_STAT);
	if ((data & ATA_STAT_ERR) == ATA_STAT_ERR)
	{
		kprintf(KERN_WARN"ata_wait(): I/O Error (mask=0x%X)\n", ATA_READ_BASE(chan, ATA_REG_ERROR));
		return -EIO;
	}

	return 0;
}


int ata_channel_poll(struct ata_channel * chan, unsigned status, unsigned enabled)
{
	int timeout = 500; /* 5sec */
	uint8_t data;
	while(--timeout)
	{
		data = ATA_READ_CTRL(chan, ATA_REG_ALTSTAT);
		if (((enabled) && ((data & status) == status)) || ((!enabled) && ((data & status) != status)))
			return 0;
		delay(10);
	}
	return -EIO;
}

void ata_channel_probe(struct ata_channel * chan)
{
	uint8_t cyllo, cylhi;
	uint8_t buf[512];
	struct ata_identify_info * info;
	int i, err;

	for(i=0;i<ATA_DEVICES;i++)
	{
		/* Wymuszamy wybranie urządzenia */
		chan->selected = 0xFF;
		ata_device_select(chan, i);

		cyllo = ATA_READ_BASE(chan, ATA_REG_CYLLO);
		cylhi = ATA_READ_BASE(chan, ATA_REG_CYLHI);

		if ((cyllo == 0x14) && (cylhi == 0xEB))
		{
			/* Identyfikujemy urządzenie */
			ATA_WRITE_BASE(chan, ATA_REG_CMD, ATA_CMD_PATAPIIDENTIFY);
			DELAY400NS();

			chan->devices[i].type = ATA_TYPE_PATAPI;
		}
		else if ((cyllo == 0x00) && (cylhi == 0x00))
		{
			/* Próbujemy wykonać identyfikacje */
			ATA_WRITE_BASE(chan, ATA_REG_CMD, ATA_CMD_IDENTIFY);
			DELAY400NS();

			if (!ATA_READ_CTRL(chan, ATA_REG_ALTSTAT)) /* Brak urządzenia */
				continue;

			chan->devices[i].type = ATA_TYPE_PATA;
		}
		else
			continue;

		/* Czekamy na zakończenie operacji */
		err = ata_channel_wait(chan);
		if (err != 0)
		{
			chan->devices[i].type = ATA_TYPE_NONE;
			continue;
		}

		/* Pobieramy dane z urządzenia */
		pio_xfer_in(chan, buf, 512);
		info = (struct ata_identify_info *)buf;
		ata_fix_string(info->model_id, 20);

		/* Pokazujemy informacje */
		if (chan->devices[i].type == ATA_TYPE_PATAPI)
		{
			err = patapi_reset(chan, i);
			if (err != 0)
			{
				kprintf("ata%d.%d: ATAPI device: reset failed\n", chan->id, i);
				chan->devices[i].type = ATA_TYPE_NONE;
				continue;
			}

			kprintf("ata%d.%d: ATAPI %s\n", chan->id, i, info->model_id);
		}
		else if (chan->devices[i].type == ATA_TYPE_PATA)
		{
			/*chan->devices[i].geom.sectors = info->lba_sectors;
			chan->devices[i].geom.bps = 512;*/

			kprintf("ata%d.%d: IDE %s (%lu sectors)\n", chan->id, i, info->model_id, info->lba_sectors);
			part_load(chan, i);
		}
	}

}
