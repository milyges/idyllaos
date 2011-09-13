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
#include <kernel/debug.h>
#include <mm/heap.h>
#include <modules/fs/ext2.h>
#include <lib/errno.h>
#include <lib/string.h>
#include <lib/math.h>

static int ext2_inode_read(struct ext2_data * data, ino_t num, struct ext2_inode * inode)
{
	int group_num, inode_table_block;
	ssize_t err;
	uint8_t * buf;

	if ((num < 1) || (num > data->sb.s_inodes_count))
		return -EINVAL;

	num--;
	group_num = num / data->sb.s_inodes_per_group;
	inode_table_block = (((num % data->sb.s_inodes_per_group) * sizeof(struct ext2_inode)) / data->blk_size);

	buf = kalloc(data->blk_size);

	err = bdev_read(data->mp->device, buf, data->spb, EXT2_GET_SECTOR_NUM(data, (data->groups[group_num].bg_inode_table + inode_table_block)));
	if (err != data->spb)
	{
		kfree(buf);
		return -EIO;
	}

	memcpy(inode, (buf + (((num % data->sb.s_inodes_per_group) * sizeof(struct ext2_inode)) % data->blk_size)), sizeof(struct ext2_inode));
	kfree(buf);
	return 0;
}

static int ext2_inode_write(struct ext2_data * data, ino_t num, struct ext2_inode * inode)
{
	int group_num, inode_table_block;
	ssize_t err;
	uint8_t * buf;

	if ((num < 1) || (num > data->sb.s_inodes_count))
		return -EINVAL;

	num--;
	group_num = num / data->sb.s_inodes_per_group;
	inode_table_block = (((num % data->sb.s_inodes_per_group) * sizeof(struct ext2_inode)) / data->blk_size);

	buf = kalloc(data->blk_size);

	/* Musimy odczytać cały blok, ale docelowo nie powinno być to odczuwalne
	   po wprowadzieniu buforów dyskowych */
	err = bdev_read(data->mp->device, buf, data->spb,  EXT2_GET_SECTOR_NUM(data, (data->groups[group_num].bg_inode_table + inode_table_block)));
	if (err != data->spb)
	{
		kfree(buf);
		return -EIO;
	}

	memcpy((buf + (((num % data->sb.s_inodes_per_group) * sizeof(struct ext2_inode)) % data->blk_size)), inode, sizeof(struct ext2_inode));

	err = bdev_write(data->mp->device, buf, data->spb,  EXT2_GET_SECTOR_NUM(data, (data->groups[group_num].bg_inode_table + inode_table_block)));
	if (err != data->spb)
		err = -EIO;


	kfree(buf);
	return err;
}


int read_inode_content(struct ext2_data * data, struct ext2_inode * inode, uint32_t start, uint32_t blocks, void * buf)
{
	ssize_t err = 0;
	void * ptr = NULL;
	int count, items;

	/* If block is zero, that means read the whole file */
	if (!blocks)
	{
		blocks = inode->i_blocks;
	}

	ptr = buf;

	//kprintf("start = %d, blocks = %d\n", start, blocks);

	/* Read (up to) the first 12 direct blocks */
	for (count=start;((blocks > 0) && (count < 12) && (ptr < (buf + inode->i_size)));count++)
	{
		if (inode->i_block[count] < 2)
			return 0;

		err = bdev_read(data->mp->device, ptr, data->spb, EXT2_GET_SECTOR_NUM(data, inode->i_block[count]));
		if (err != data->spb)
			return -EIO;
		blocks--;
		ptr += data->blk_size;
	}

	count -= 12;
	items = data->blk_size / 4;

	/* Indirect blocks */
	if ((blocks > 0) && (count < items) && (inode->i_block[12]))
	{
		err = ext2_read_indirect_blocks(data, count, inode->i_block[12], &blocks, &ptr, 1);
		if (err < 0)
			return err;
	}

	/* Double-indirect blocks */
	if ((blocks > 0) && (count < items * items) && (inode->i_block[13]))
	{
		err = ext2_read_indirect_blocks(data, (count < items) ? 0 : (count - items) % (items * items), inode->i_block[13], &blocks, &ptr, 2);
		if (err < 0)
			return err;
	}

	return 0;
 #if 0

 /* Triple-indirect blocks */
 if ((blocks > 0) && (inode->i_block[14]))
 {
  err = ext2_read_indirect_blocks(data, (level < 2) ? 0 : count % (data->blk_size / 4), inode->i_block[13], &blocks, &ptr, 3);
  if (err < 0)
     return err;
 }
#endif
}

int ext2_write_inode_content(struct ext2_data * data, struct ext2_inode * inode, uint32_t start, uint32_t blocks, void * buf)
{
	ssize_t err = 0;
	void * ptr = NULL;
	int count, items;

	/* If block is zero, that means write the whole file */
	if (!blocks)
	{
		blocks = inode->i_blocks;
	}

	ptr = buf;


	/* Write (up to) the first 12 direct blocks */
	for (count=start;((blocks > 0) && (count < 12) && (ptr < (buf + inode->i_size)));count++)
	{
		if (inode->i_block[count] < 2)
		{
			kprintf("ext2: ext2_write_inode_content(): block should be allocated!\n");
			return -EIO; /* Cos nie tak, bloki mialy byc zaalokowane! */
		}

		err = bdev_write(data->mp->device, ptr, data->spb, EXT2_GET_SECTOR_NUM(data, inode->i_block[count]));
		if (err != data->spb)
			return -EIO;
		blocks--;
		ptr += data->blk_size;
	}

	count -= 12;
	items = data->blk_size / 4;
#if 0
	/* Indirect blocks */
	if ((blocks > 0) && (count < items) && (inode->i_block[12]))
	{
		err = ext2_read_indirect_blocks(data, count, inode->i_block[12], &blocks, &ptr, 1);
		if (err < 0)
			return err;
	}

	/* Double-indirect blocks */
	if ((blocks > 0) && (count < items * items) && (inode->i_block[13]))
	{
		err = ext2_read_indirect_blocks(data, (count < items) ? 0 : (count - items) % (items * items), inode->i_block[13], &blocks, &ptr, 2);
		if (err < 0)
			return err;
	}
#endif
	return 0;
}

int ext2_sync(struct vnode * vnode)
{
	struct ext2_inode * inode = vnode->data;
	struct ext2_data * data = vnode->mp->data;

	inode->i_mode = vnode->mode;
	inode->i_links_count = vnode->nlink;
	//vnode->dev = inode->i_dev;
	inode->i_uid = vnode->uid;
	inode->i_gid = vnode->gid;

	if (inode->i_size > vnode->size)
	{
		TODO("free blocks");
	}
	else if (inode->i_size < vnode->size)
	{
		TODO("Allocate blocks!");
	}

	return ext2_inode_write(data, vnode->ino, inode);
}

int ext2_open(struct vnode * vnode)
{
	int err;
	struct ext2_inode * inode;
	struct ext2_data * data = vnode->mp->data;

	inode = kalloc(sizeof(struct ext2_inode));

	err = ext2_inode_read(data, vnode->ino, inode);
	if (err != 0)
	{
		kfree(inode);
		return err;
	}

	//kprintf("ext2_open(): inode=%d, mode=0%o, nlink=%d, size=%u\n", vnode->ino, inode->i_mode, inode->i_links_count, inode->i_size);

	vnode->data = inode;
	vnode->mode = inode->i_mode;
	vnode->nlink = inode->i_links_count;
	//vnode->dev = inode->i_dev;
	vnode->uid = inode->i_uid;
	vnode->gid = inode->i_gid;
	vnode->size = inode->i_size;

	return 0;
}

int ext2_close(struct vnode * vnode)
{
	struct ext2_data * data = vnode->mp->data;
	if ((vnode->mp->flags & MS_RDONLY) != MS_RDONLY)
	{
		ext2_sync(vnode);
		//if (!vnode->nlink)
		//	ext2_inode_free(data, vnode->dataptr);
	}

	return 0;
}

ino_t ext2_lookup(struct vnode * vnode, char * name)
{
	int pos, err;
	void * buf;
	ino_t ino = 0;
	struct ext2_dir_entry * entry;
	struct ext2_inode * inode;
	struct ext2_data * data = vnode->mp->data;

	inode = vnode->data;

	buf = kalloc(inode->i_blocks * data->blk_size);
	err = read_inode_content(data, inode, 0, 0, buf);
	if (err != 0)
	{
		kfree(buf);
		return 0;
	}

	pos = 0;
	while(pos < inode->i_size)
	{
		entry = (struct ext2_dir_entry *)(buf + pos);

		if (!entry->rec_len)
			break;

		if ((strlen(name) == entry->name_len) && (!strncmp(name, (char *)entry->name, entry->name_len)))
		{
			ino = entry->inode;
			break;
		}
		pos += entry->rec_len;
	}

	kfree(buf);
	return ino;
}

ssize_t ext2_read(struct vnode * vnode, void * buf, size_t len, loff_t offset)
{
	struct ext2_data * data = vnode->mp->data;
	struct ext2_inode * inode = vnode->data;
	unsigned start, blocks;
	void * tmp;
	int err;

	if (offset + len >= inode->i_size)
	{
		if (inode->i_size - offset < 0)
			return 0;
		else
			len = inode->i_size - offset;
	}

	start = offset / data->blk_size;
	blocks = (len / data->blk_size) + ((len % data->blk_size) > 0);
	//kprintf("start=%d, blocks=%d\n", start, blocks);

	if (!blocks)
		return 0;

	tmp = kalloc(data->blk_size * blocks);

	err = read_inode_content(data, inode, start, blocks, tmp);
	if (err != 0)
	{
		kfree(tmp);
		return -EIO;
	}

	memcpy(buf, (void *)(tmp + (offset % data->blk_size)), len);
	kfree(tmp);
	return len;
}

int ext2_getdents(struct vnode * vnode, struct dirent * dirp, size_t len, loff_t * offset)
{
	struct ext2_dir_entry * entry;
	struct ext2_inode * inode = vnode->data;
	struct ext2_data * data = vnode->mp->data;
	struct dirent * dirent;

	void * buf;
	int count = 0, err;
	int reclen;


	buf = kalloc(inode->i_blocks * data->blk_size);
	err = read_inode_content(data, inode, 0, 0, buf);
	if (err != 0)
	{
		kfree(buf);
		return err;
	}

	while (*offset < inode->i_size)
	{
		entry = (struct ext2_dir_entry *)(buf + *offset);
		dirent = (struct dirent *)((void *)dirp + count);

		if (!entry->rec_len)
			break;

		reclen = ROUND_UP((addr_t)&dirent->d_name[entry->name_len + 1] - (addr_t)dirent, 4);
		if (count + reclen >= len)
			break;

		dirent->d_ino = entry->inode;
		memcpy(dirent->d_name, entry->name, entry->name_len);
		dirent->d_name[entry->name_len] = '\0';
		dirent->d_reclen = reclen;

		*offset += entry->rec_len;
		dirent->d_off = *offset;
		count += dirent->d_reclen;
	}

	kfree(buf);
	return count;
}

int ext2_unlink(struct vnode * vnode, char * name)
{
	int pos, err;
	void * buf;
	struct ext2_dir_entry * entry;
	struct ext2_dir_entry * prev;
	struct ext2_inode * inode;
	struct ext2_data * data = vnode->mp->data;
	return -ENOSYS;

	inode = vnode->data;
	pos = 0;
	buf = kalloc(inode->i_blocks * data->blk_size);
	err = read_inode_content(data, inode, 0, 0, buf);
	if (err != 0)
	{
		kfree(buf);
		return 0;
	}

	err = -ENOENT;
	prev = NULL;
	while(pos < inode->i_size)
	{
		entry = (struct ext2_dir_entry *)(buf + pos);

		if (!entry->rec_len)
			break;

		if ((strlen(name) == entry->name_len) && (!strncmp(name, (char *)entry->name, entry->name_len)))
		{
			/* Nie mozna unlinkowac katalogow */
			if (entry->file_type == EXT2_FT_DIR)
				err = -EISDIR;
			else
			{
				if (!prev)
					panic("ext2: Oops... tried to unlink first entry!");

				prev->rec_len += entry->rec_len;
				err = 0;
			}
			break;
		}

		pos += entry->rec_len;
		prev = entry;
	}

	/* Jezeli nie ma bledu, zapisz zmieniony katalog na dysk */
	if (!err)
		err = ext2_write_inode_content(data, inode, 0, 0, buf);

	kfree(buf);
	return err;
}
