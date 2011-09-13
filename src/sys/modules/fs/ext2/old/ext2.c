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

int ext2_read_indirect_blocks(struct ext2_data * data, uint32_t offset, uint32_t blk, uint32_t * blocks, void ** buf, int level)
{
	ssize_t err = 0;
	unsigned * idx_buf = NULL;
	unsigned count;
	int items = data->blk_size / 4;

	idx_buf = kalloc(data->blk_size);

	//kprintf("ext2: read indirect (level=%d, blk=%u, blocks=%u)\n", level, blk, *blocks);
	err = bdev_read(data->mp->device, (uint8_t *)idx_buf, data->spb, EXT2_GET_SECTOR_NUM(data, blk));
	if (err != data->spb)
	{
		kfree(idx_buf);
		return -EIO;
	}

	/* Now, if the indirection level is 1, this is an index of blocks to read.
	  Otherwise, it is an index of indexes, and we need to recurse */
	if (level > 1)
	{
		//kprintf("ext2: level > 1: offset=%d, blk=%d, blocks=%d\n", offset, blk, *blocks);
		/* Do a recursion for every index in this block */
		for(count = offset / items;((*blocks > 0) && (count < items));count++)
		{
			if (idx_buf[count] < 2)
				return 0;
			//kprintf("count = %d, block = %d\n", count, *blocks);
			err = ext2_read_indirect_blocks(data, (count == offset / items) ? offset : 0, idx_buf[count], blocks, buf, (level - 1));
			if (err < 0)
			{
				kfree(idx_buf);
				return err;
			}
		}


	}
	else
	{
		/* Read the blocks in our index block into the buffer */
		offset = offset % items;
		for (count=offset;((*blocks > 0) && (count < items));count++)
		{
			if (idx_buf[count] < 2)
				return 0;

			//kprintf("ext2: read from %u, %d buf=%x\n", EXT2_GET_SECTOR_NUM(data, idx_buf[count]), data->spb, *buf);
			err = bdev_read(data->mp->device, *buf, data->spb, EXT2_GET_SECTOR_NUM(data, idx_buf[count]));
			if (err != data->spb)
			{
				kfree(idx_buf);
				return -EIO;
			}

			(*blocks)--;
			*buf += data->blk_size;
		}
	}

	kfree(idx_buf);
	return 0;
}

static struct vnode_ops _ext2_vnode_ops =
{
	.open = &ext2_open,
	.close = &ext2_close,
	.lookup = &ext2_lookup,
	.read = &ext2_read,
	.getdents = &ext2_getdents,
	.sync = &ext2_sync,
	.unlink = &ext2_unlink
};

static struct filesystem_ops _ext2_fs_ops =
{
	.mount = &ext2_mount,
	.umount = &ext2_umount,
	.root = &ext2_root
};

static struct filesystem _ext2_fs =
{
	.name = "ext2",
	.fsid = 0x83,
	.flags = 0,
	.fs_ops = &_ext2_fs_ops,
	.vnode_ops = &_ext2_vnode_ops
};


int init(int argc, char * argv[])
{
	fs_register(&_ext2_fs);
	return 0;
}

int clean(void)
{

	return 0;
}

MODULE_INFO("ext2", &init, &clean);
