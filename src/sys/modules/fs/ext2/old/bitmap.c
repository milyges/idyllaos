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
#include <kernel/bitops.h>
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

static int block_bitmap_read(struct ext2_data * data, void * buf, int group_num)
{
	int err;

	if (group_num >= data->num_groups)
		return -EINVAL;

	err = bdev_read(data->mp->device, buf, data->spb, EXT2_GET_SECTOR_NUM(data, data->groups[group_num].bg_block_bitmap));
	if (err != data->spb)
		return -EIO;

	return 0;
}

static int block_bitmap_write(struct ext2_data * data, void * buf, int group_num)
{
	int err;

	if (group_num >= data->num_groups)
		return -EINVAL;

	err = bdev_write(data->mp->device, buf, data->spb, EXT2_GET_SECTOR_NUM(data, data->groups[group_num].bg_block_bitmap));
	if (err != data->spb)
		return -EIO;

	return 0;
}

static int inode_bitmap_read(struct ext2_data * data, void * buf, int group_num)
{
	int err;

	if (group_num >= data->num_groups)
		return -EINVAL;

	err = bdev_read(data->mp->device, buf, data->spb, EXT2_GET_SECTOR_NUM(data, data->groups[group_num].bg_inode_bitmap));
	if (err != data->spb)
		return -EIO;

	return 0;
}

static int inode_bitmap_write(struct ext2_data * data, void * buf, int group_num)
{
	int err;

	if (group_num >= data->num_groups)
		return -EINVAL;

	err = bdev_write(data->mp->device, buf, data->spb, EXT2_GET_SECTOR_NUM(data, data->groups[group_num].bg_inode_bitmap));
	if (err != data->spb)
		return -EIO;

	return 0;
}

uint32_t * ext2_balloc(struct ext2_data * data, int group_num, int blocks)
{
	unsigned long * buf;
	uint32_t * table;
	int err, block_idx = 0, i, j;
	uint32_t block_base;

	/* Spradzamy czy w grupie jest na tyle wolnych blokow */
	if (data->groups[group_num].bg_free_blocks_count < blocks)
		return NULL;

	buf = kalloc(data->spb * data->bps);
	table = kalloc(sizeof(uint32_t) * blocks);

	/* Ladujemy bitmape do pamieci */
	err = block_bitmap_read(data, buf, group_num);
	if (err != 0)
	{
		kfree(buf);
		kfree(table);
		return NULL;
	}

	block_base = group_num * data->sb.s_blocks_per_group + 1;

	for(i = 0; i < data->spb * data->bps / sizeof(unsigned long); i++)
	{
		while(buf[i] != 0xFFFFFFFF)
		{
			j = ffz_bit(buf[i]);
			set_bit(&buf[i], j);
			table[block_idx++] = block_base + i * sizeof(unsigned long) * 8 + j;
			if (block_idx == blocks) /* Mamy wszystkie bloki */
				goto end;
		}
	}

end:
	data->groups[group_num].bg_free_blocks_count -= blocks;
	data->sb.s_free_blocks_count -= blocks;
	/* Zapisujemy nową bitmape na dysk */
	block_bitmap_write(data, buf, group_num);
	/* Zapisujemy grupy na dysk */
	ext2_write_groups(data);
	/* i super block tez... */
	ext2_write_superblock(data->mp);
	kfree(buf);
	return table;

}


int ext2_bfree(struct ext2_data * data, int group_num, uint32_t * blocks, int count)
{
	unsigned long * buf;
	int err, i, tmp;
	uint32_t block_base;


	buf = kalloc(data->spb * data->bps);
	/* Ladujemy bitmape do pamieci */
	err = block_bitmap_read(data, buf, group_num);
	if (err != 0)
	{
		kfree(buf);
		return err;
	}

	block_base = group_num * data->sb.s_blocks_per_group + 1;
	for(i = 0; i < count; i++)
	{
		tmp = blocks[i] - block_base;
		clear_bit(&buf[tmp / (sizeof(unsigned long) * 8)], tmp % (sizeof(unsigned long) * 8));
	}

	data->groups[group_num].bg_free_blocks_count += count;
	data->sb.s_free_blocks_count += count;

	/* Zapisujemy nową bitmape na dysk */
	block_bitmap_write(data, buf, group_num);
	/* Zapisujemy grupy na dysk */
	ext2_write_groups(data);
	/* i super block tez... */
	ext2_write_superblock(data->mp);
	kfree(buf);
	return 0;
}

ino_t ext2_ialloc(struct ext2_data * data, int group_num)
{
	unsigned long * buf;
	ino_t ino_num = 0;
	int err, i, j;

	if (!data->groups[group_num].bg_free_inodes_count)
		return 0;

	buf = kalloc(data->spb * data->bps);
	/* Ladujemy bitmape do pamieci */
	err = inode_bitmap_read(data, buf, group_num);
	if (err != 0)
	{
		kfree(buf);
		return 0;
	}

	for(i = 0; i < data->spb * data->bps / sizeof(unsigned long); i++)
	{
		if (buf[i] != 0xFFFFFFFF)
		{
			j = ffz_bit(buf[i]);
			set_bit(&buf[i], j);
			ino_num = group_num * data->sb.s_inodes_per_group + i * sizeof(unsigned long) * 8 + j + 1;
			break;
		}
	}

	if (ino_num > 0)
	{
		data->groups[group_num].bg_free_inodes_count--;
		data->sb.s_free_inodes_count--;
		/* Zapisujemy nową bitmape na dysk */
		inode_bitmap_write(data, buf, group_num);
		/* Zapisujemy grupy na dysk */
		ext2_write_groups(data);
		/* i super block tez... */
		ext2_write_superblock(data->mp);
	}

	kfree(buf);
	return ino_num;
}

int ext2_ifree(struct ext2_data * data, ino_t inode)
{
	unsigned long * buf;
	ino_t ino_num = 0;
	int err, tmp;
	int group_num = inode / data->sb.s_inodes_per_group;

	buf = kalloc(data->spb * data->bps);
	/* Ladujemy bitmape do pamieci */
	err = inode_bitmap_read(data, buf, group_num);
	if (err != 0)
	{
		kfree(buf);
		return err;
	}

	tmp = inode - group_num * data->sb.s_inodes_per_group - 1;
	clear_bit(&buf[tmp / (sizeof(unsigned long) * 8)], tmp % (sizeof(unsigned long) * 8));

	data->groups[group_num].bg_free_inodes_count++;
	data->sb.s_free_inodes_count++;

	/* Zapisujemy nową bitmape na dysk */
	inode_bitmap_write(data, buf, group_num);
	/* Zapisujemy grupy na dysk */
	ext2_write_groups(data);
	/* i super block tez... */
	ext2_write_superblock(data->mp);

	kfree(buf);
	return ino_num;
}
