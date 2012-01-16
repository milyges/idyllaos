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
#include <kernel/types.h>
#include <kernel/kprintf.h>
#include <kernel/device.h>
#include <mm/heap.h>
#include <lib/errno.h>
#include <lib/string.h>
#include <lib/printf.h>
#include <modules/drivers/storage/storage.h>

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

void partirion_msdos_detect(struct storage * storage)
{
	struct msdos_ebr ebr;
	struct msdos_mbr mbr;
	ssize_t err;
	int i, tmp = 4;
	loff_t off;
	
	/* Odczytaj MBR */
	err = storage->ops->read(storage, &mbr, 1, 0);
	if (err != 1)
		return;
	
	if ((mbr.magic[0] != 0x55) && (mbr.magic[1] != 0xAA))
	{
		kprintf(KERN_INFO"storage: invalid master boot record signature\n");
		return;
	}
	
	memset(&storage->parts, 0x00, STORAGE_MAX_PARTS * sizeof(struct storage_partition));
	
	for (i=0;i<4;i++)
	{
		if (!mbr.partitions[i].fsid)
			continue;
		
		storage->parts[i].start = mbr.partitions[i].lba_start;
		storage->parts[i].size = mbr.partitions[i].size;
		storage->parts[i].type = mbr.partitions[i].fsid;
		kprintf(" %s%dp%d: start:%u, size:%u, type: %x\n", __storage_names[storage->type], storage->id, i + 1, mbr.partitions[i].lba_start, mbr.partitions[i].size, mbr.partitions[i].fsid);
		
		if ((mbr.partitions[i].fsid == 0x05) || (mbr.partitions[i].fsid == 0x0F))
		{
			off = 0;
			do
			{
				/* Czytamy partycje rozszerzona */
				err = storage->ops->read(storage, &ebr, 1, off);
				if (err != 1)
					break;

				if ((ebr.magic[0] != 0x55) && (ebr.magic[1] != 0xAA))
					break;

				storage->parts[tmp].start = mbr.partitions[i].lba_start + off + ebr.partitions[0].lba_start;
				storage->parts[tmp].size = ebr.partitions[0].size;
				storage->parts[tmp].type = ebr.partitions[0].fsid;

				kprintf(" %s%dp%d: start:%llu, size:%llu, type: %x\n", __storage_names[storage->type], storage->id, tmp + 1, storage->parts[tmp].start, storage->parts[tmp].size, ebr.partitions[0].fsid);
				tmp++;

				off = ebr.partitions[1].lba_start;
			} while((ebr.partitions[1].lba_start != 0) && (ebr.partitions[1].size != 0) && (tmp < 14));
		}
	}
}
