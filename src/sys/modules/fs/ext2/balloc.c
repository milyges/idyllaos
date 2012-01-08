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
#include <kernel/bitops.h>
#include <kernel/kprintf.h>
#include <kernel/module.h>
#include <kernel/device.h>
#include <kernel/debug.h>
#include <kernel/vfs.h>
#include <mm/heap.h>
#include <modules/fs/ext2.h>
#include <lib/errno.h>
#include <lib/string.h>
#include <lib/math.h>

static int block_bitmap_read(struct ext2_data * data, void * buf, int group)
{
	int err;

	if (group >= data->groups_count)
		return -EINVAL;

	err = bdev_read(data->mp->device, buf, data->spb, EXT2_GET_SECTOR_NUM(data, data->groups[group].bg_block_bitmap));
	if (err != data->spb)
		return -EIO;

	return 0;
}

static int block_bitmap_write(struct ext2_data * data, void * buf, int group)
{
	int err;

	if (group >= data->groups_count)
		return -EINVAL;

	err = bdev_write(data->mp->device, buf, data->spb, EXT2_GET_SECTOR_NUM(data, data->groups[group].bg_block_bitmap));
	if (err != data->spb)
		return -EIO;

	return 0;
}


int block_alloc(struct ext2_data * data, uint32_t * block, int group)
{
	int err, i, j;
	unsigned long * buf;
	uint32_t free_blocks = 0;
	
	if (group < 0)
	{
		for(i = 0; i < data->groups_count; i++)
		{
			if (data->groups[i].bg_free_blocks_count > free_blocks)
			{
				free_blocks = data->groups[i].bg_free_blocks_count;
				group = i;
			}
		}
	}
	
	/* Blokujemy daną grupę */
	EXT2_GROUP_LOCK(data, group);

	/* Sprawdzamy czy są w niej w ogóle jakieś wolne bloki */
	if (!data->groups[group].bg_free_blocks_count)
	{
		err = -ENOSPC;
		goto end;
	}

	/* Ładujemy bitmape blokow do pamieci */
	buf = kalloc(data->blk_size);
	err = block_bitmap_read(data, buf, group);
	if (err != 0)
		goto end;

	/* Szukamy wolnego bloku */
	err = -ENOSPC;
	for(i = 0; i < data->blk_size / sizeof(unsigned long); i++)
	{
		if (buf[i] != ~0UL)
		{
			j = ffz_bit(buf[i]);
			set_bit(&buf[i], j);
			*block = group * data->sb.s_blocks_per_group + i * sizeof(unsigned long) * 8 + j + 1;
			err = 0;
			break;
		}
	}

	/* Znalezlismy i zaalokowalismy nowy blok */
	if (!err)
	{
		data->groups[group].bg_free_blocks_count--;
		data->sb.s_free_blocks_count--;
		/* Zapisz bitmape na dysk */
		err = block_bitmap_write(data, buf, group);

		/* TODO: Jeżeli MS_SYNC to zapisz tez grupy i sb */
	}

	kfree(buf);
end:
	EXT2_GROUP_UNLOCK(data, group);
	return err;
}

int block_free(struct ext2_data * data, uint32_t block)
{
	int group, err, i, j;
	unsigned long * buf;
	group = EXT2_GET_BLOCK_GROUP(data, block);
	
	/* Blokujemy daną grupę */
	EXT2_GROUP_LOCK(data, group);
	
	/* Ładujemy bitmape blokow do pamieci */
	buf = kalloc(data->blk_size);
	err = block_bitmap_read(data, buf, group);
	if (err != 0)
		goto end;
	
	/* Sprawdzamy czy i-node jest zajęty */
	i = (block - group * data->sb.s_blocks_per_group - 1) / (sizeof(unsigned long) * 8);
	j = block % (sizeof(unsigned long) * 8) - 1;
	
	err = 0;
	if (test_bit(buf[i], j))
	{
		clear_bit(&buf[i], j);
		data->groups[group].bg_free_blocks_count++;
		data->sb.s_free_blocks_count++;
		err = block_bitmap_write(data, buf, group);
		
		/* TODO: Jeżeli MS_SYNC to zapisz tez grupy i sb */
	}
	
	kfree(buf);
end:
	EXT2_GROUP_UNLOCK(data, group);
	return err;
}
