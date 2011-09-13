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
#include <kernel/module.h>
#include <kernel/device.h>
#include <kernel/vfs.h>
#include <mm/heap.h>
#include <modules/fs/ext2.h>
#include <lib/errno.h>
#include <lib/string.h>
#include <lib/math.h>

int super_read(struct ext2_data * data)
{
	int err;
	memset(&data->sb, 0x00, sizeof(struct ext2_superblock));
	err = bdev_read(data->mp->device, (void *)&data->sb, sizeof(struct ext2_superblock) / data->bps, EXT2_SUPERBLOCK_SECTOR);

	if (err != (sizeof(struct ext2_superblock) / data->bps))
		return -EIO;

	if (data->sb.s_magic != EXT2_MAGIC)
	{
		kprintf(KERN_DEBUG"ext2: %s: Device doesn't contain valid ext2 file system (magic=%x)\n", data->mp->devname, data->sb.s_magic);
		return -EINVAL;
	}

	return 0;
}

int super_write(struct ext2_data * data)
{
	int err;
	err = bdev_write(data->mp->device, (void *)&data->sb, sizeof(struct ext2_superblock) / data->bps, EXT2_SUPERBLOCK_SECTOR);

	if (err != (sizeof(struct ext2_superblock) / data->bps))
		return -EIO;

	return 0;
}

int super_read_groups(struct ext2_data * data)
{
	ssize_t err;

	err = bdev_read(data->mp->device, data->groups, data->groups_blocks * data->spb, EXT2_GET_SECTOR_NUM(data, (data->blk_size == 1024) ? 2 : 1));
	if (err != data->spb * data->groups_blocks)
		return -EIO;

	return 0;
}

int super_write_groups(struct ext2_data * data)
{
	ssize_t err;

	err = bdev_write(data->mp->device, data->groups, data->groups_blocks * data->spb, EXT2_GET_SECTOR_NUM(data, (data->blk_size == 1024) ? 2 : 1));
	if (err != data->spb * data->groups_blocks)
		return -EIO;

	return 0;
}
