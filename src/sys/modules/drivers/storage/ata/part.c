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
#include <kernel/types.h>
#include <kernel/kprintf.h>
#include <kernel/device.h>
#include <mm/heap.h>
#include <lib/errno.h>
#include <lib/string.h>
#include <lib/printf.h>
#include <modules/drivers/storage/ata.h>


struct msdos_partition
{
	uint8_t flags;
	uint8_t shead;
	uint8_t ssector;
	uint8_t scylinder;
	uint8_t fsid;
	uint8_t ehead;
	uint8_t esector;
	uint8_t ecylinder;
	uint32_t lba_start;
	uint32_t size;
} PACKED;

struct msdos_mbr
{
	uint8_t bootcode[0x1BE];
	struct msdos_partition partitions[4];
	uint8_t magic[2];
} PACKED;

struct msdos_ebr
{
	uint8_t bootcode[0x1BE];
	struct msdos_partition partitions[2];
	uint8_t reserved[32];
	uint8_t magic[2];
};

void part_load(struct ata_channel * chan, uint8_t device)
{
	struct msdos_ebr ebr;
	struct msdos_mbr mbr;
	ssize_t err;
	int i, tmp = 4;
	loff_t off;

	/* Load mbr */
	err = pata_read(chan, device, &mbr, 1, 0);
	if (err != 1)
		return;

	if ((mbr.magic[0] != 0x55) && (mbr.magic[1] != 0xAA))
	{
		kprintf(KERN_INFO"ata%d.%d: invalid master boot record signature\n", chan->id, device);
		return;
	}

	memset(&chan->devices[device].parts, 0, sizeof(&chan->devices[device].parts));

	for (i=0;i<4;i++)
	{
		if (!mbr.partitions[i].fsid)
			continue;

		chan->devices[device].parts[i].start = mbr.partitions[i].lba_start;
		chan->devices[device].parts[i].size = mbr.partitions[i].size;
		chan->devices[device].parts[i].type = mbr.partitions[i].fsid;
		kprintf(" p%d: start:%u, size:%u, type: %x\n", i, mbr.partitions[i].lba_start, mbr.partitions[i].size, mbr.partitions[i].fsid);

		if ((mbr.partitions[i].fsid == 0x05) || (mbr.partitions[i].fsid == 0x0F))
		{
			off = 0;
			do
			{
				/* Czytamy partycje rozszerzona */
				part_read(chan, device, i, &ebr, 1, off);

				if ((ebr.magic[0] != 0x55) && (ebr.magic[1] != 0xAA))
					break;

				chan->devices[device].parts[tmp].start = mbr.partitions[i].lba_start + off + ebr.partitions[0].lba_start;
				chan->devices[device].parts[tmp].size = ebr.partitions[0].size;
				chan->devices[device].parts[tmp].type = ebr.partitions[0].fsid;

				kprintf(" p%d: start:%llu, size:%llu, type: %x\n", tmp, chan->devices[device].parts[tmp].start, chan->devices[device].parts[tmp].size, ebr.partitions[0].fsid);
				tmp++;

				off = ebr.partitions[1].lba_start;
			} while((ebr.partitions[1].lba_start != 0) && (ebr.partitions[1].size != 0) && (tmp < 14));


		}
	}
}

ssize_t part_read(struct ata_channel * chan, uint8_t device, uint8_t part, void * dest, size_t len, loff_t off)
{
	if (!chan->devices[device].parts[part].type)
		return -ENODEV;

	if (off >= chan->devices[device].parts[part].size)
		return -EINVAL;

	if (off + len > chan->devices[device].parts[part].size)
		len = chan->devices[device].parts[part].size - off;

	return pata_read(chan, device, dest, len, chan->devices[device].parts[part].start + off);
}

ssize_t part_write(struct ata_channel * chan, uint8_t device, uint8_t part, void * dest, size_t len, loff_t off)
{
	if (!chan->devices[device].parts[part].type)
		return -ENODEV;

	if (off >= chan->devices[device].parts[part].size)
		return -EINVAL;

	if (off + len > chan->devices[device].parts[part].size)
		len = chan->devices[device].parts[part].size - off;

	return pata_write(chan, device, dest, len, chan->devices[device].parts[part].start + off);
}
