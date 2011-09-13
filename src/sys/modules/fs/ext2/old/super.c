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

int ext2_read_superblock(struct mountpoint * mp)
{
	int err;
	struct ext2_data * data = mp->data;

	memset(&data->sb, 0x00, sizeof(struct ext2_superblock));
	err = bdev_read(mp->device, (void *)&data->sb, sizeof(struct ext2_superblock) / data->bps, EXT2_SUPERBLOCK_SECTOR);
	if (err <= 0)
		return -EIO;

	if (data->sb.s_magic != EXT2_MAGIC)
	{
		kprintf(KERN_DEBUG"ext2: Device %04x doesn't contain valid ext2 file system (magic=%x)\n", mp->device, data->sb.s_magic);
		return -EINVAL;
	}
	return 0;
}

int ext2_write_superblock(struct mountpoint * mp)
{
	int err;
	struct ext2_data * data = mp->data;

	err = bdev_write(mp->device, (void *)&data->sb, sizeof(struct ext2_superblock) / data->bps, EXT2_SUPERBLOCK_SECTOR);
	if (err <= 0)
		return -EIO;

	return 0;
}

int ext2_read_groups(struct ext2_data * data)
{
	ssize_t err;

	err = bdev_read(data->mp->device, data->groups, data->gdb * data->spb, EXT2_GET_SECTOR_NUM(data, (data->blk_size == 1024) ? 2 : 1));
	if (err != data->spb * data->gdb)
		return -EIO;

	return 0;
}

int ext2_write_groups(struct ext2_data * data)
{
	ssize_t err;

	err = bdev_write(data->mp->device, data->groups, data->gdb * data->spb, EXT2_GET_SECTOR_NUM(data, (data->blk_size == 1024) ? 2 : 1));
	if (err != data->spb * data->gdb)
		return -EIO;

	return 0;
}

int ext2_mount(struct mountpoint * mp, char * flags)
{
	int err;
	struct ext2_data * data;

	/* Otwieramy urzadzenie */
	err = bdev_open(mp->device);
	if (err != 0)
		return err;

	data = kalloc(sizeof(struct ext2_data));
	memset(data, 0x00, sizeof(struct ext2_data));
	mp->data = data;
	data->mp = mp;

	data->bps = 512; /* TODO: Odczytaj ze sterownika */

	/* Ładujemy super-block */
	err = ext2_read_superblock(mp);
	if (err != 0)
	{
		kfree(data);
		bdev_close(mp->device);
		return err;
	}

	data->blk_size = EXT2_BLOCK_SIZE(&data->sb);
	data->spb = data->blk_size / data->bps;

	if (data->sb.s_inode_size != sizeof(struct ext2_inode))
	{
		kfree(data);
		bdev_close(mp->device);
		return -EINVAL;
	}

	if (data->sb.s_state != EXT2_VALID_FS)
	{
		if (data->sb.s_errors == EXT2_ERRORS_RO)
		{
			mp->flags |= MS_RDONLY;
			kprintf(KERN_WARN"ext2: %s: File system NOT clean, mounting read-only\n", mp->devname);
		}
		else if (data->sb.s_errors == EXT2_ERRORS_PANIC)
			panic("ext2: %s: File system NOT clean\n", mp->devname);
		else
			kprintf(KERN_WARN"ext2: %s: File system NOT clean\n", mp->devname);
	}
	else
	{
		kprintf(KERN_DEBUG"ext2: %s: File system clean\n", mp->devname);
	}

	data->num_groups = ((data->sb.s_blocks_count / data->sb.s_blocks_per_group) +
	                   ((data->sb.s_blocks_count % data->sb.s_blocks_per_group) > 0));

	data->gdb = ROUND_UP(data->num_groups * sizeof(struct ext2_group_desc), data->blk_size) / data->blk_size;
	data->groups = kalloc(data->gdb * data->blk_size);

	err = ext2_read_groups(data);
	if (err != 0)
	{
		kfree(data->groups);
		kfree(data);
		bdev_close(mp->device);
		return err;
	}

	if (data->sb.s_mnt_count >= data->sb.s_max_mnt_count)
	{
		kprintf(KERN_DEBUG"ext2: %s: maximal mount count reached, checking file system is recommended\n", mp->devname);
	}

	if ((mp->flags & MS_RDONLY) != MS_RDONLY)
	{
		data->sb.s_state = EXT2_ERROR_FS;
		data->sb.s_mnt_count++;

		/* TODO: aktualizacja czasu montowania */
		err = ext2_write_superblock(mp);
		if (err != 0)
		{
			kfree(data);
			bdev_close(mp->device);
			return err;
		}
	}

	return 0;
}

int ext2_umount(struct mountpoint * mp)
{
	return -ENOSYS;
}

ino_t ext2_root(struct mountpoint * mp)
{
	return EXT2_ROOT_INO;
}

