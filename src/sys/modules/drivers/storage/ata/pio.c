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
#include <kernel/types.h>
#include <modules/drivers/storage/ata.h>

void pio_xfer_in(struct ata_channel * chan, void * buf, unsigned len)
{
	unsigned i, words = len / 2;
	uint16_t * buf16 = buf;

	for(i=0;i<words;i++)
		buf16[i] = inportw(chan->base + ATA_REG_DATA);
}

void pio_xfer_out(struct ata_channel * chan, void * buf, unsigned len)
{
	unsigned i, words = len / 2;
	uint16_t * buf16 = buf;

	for(i=0;i<words;i++)
		outportw(chan->base + ATA_REG_DATA, buf16[i]);
}
