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
#include <kernel/module.h>
#include <kernel/device.h>
#include <kernel/vfs.h>
#include <kernel/debug.h>
#include <mm/heap.h>
#include <modules/fs/ext2.h>
#include <lib/errno.h>
#include <lib/string.h>
#include <lib/math.h>

int inode_do_read(struct ext2_data * data, struct ext2_inode * inode, ino_t num)
{
	void * buf;
	int err, group_idx, inode_offset;

	/* Sprawdzamy czy numer i-node jest poprawny */
	if ((num < 1) || (num > data->sb.s_inodes_count))
		return -EINVAL;

	/* Obliczamy numer grupy, i przesuniecie i-noda w grupie */
	group_idx = EXT2_GET_INODE_GROUP(data, num);
	inode_offset = (((num - 1) % data->sb.s_inodes_per_group) * sizeof(struct ext2_inode)) / data->blk_size;

	/* Alokujemy bufor */
	buf = kalloc(data->blk_size);

	/* Odczytujemy wymagany blok danych */
	err = bdev_read(data->mp->device, buf, data->spb, EXT2_GET_SECTOR_NUM(data, (data->groups[group_idx].bg_inode_table + inode_offset)));
	if (err != data->spb)
	{
		err = -EIO;
		goto end;
	}
	else
		err = 0;

	memcpy(inode, (buf + ((((num - 1) % data->sb.s_inodes_per_group) * sizeof(struct ext2_inode)) % data->blk_size)), sizeof(struct ext2_inode));
end:
	kfree(buf);
	return err;
}

int inode_do_write(struct ext2_data * data, struct ext2_inode * inode, ino_t num)
{
	void * buf;
	int err, group_idx, inode_offset;

	/* Sprawdzamy czy numer i-node jest poprawny */
	if ((num < 1) || (num > data->sb.s_inodes_count))
		return -EINVAL;

	/* Obliczamy numer grupy, i przesuniecie i-noda w grupie */
	group_idx = EXT2_GET_INODE_GROUP(data, num);
	inode_offset = (((num - 1) % data->sb.s_inodes_per_group) * sizeof(struct ext2_inode)) / data->blk_size;

	/* Alokujemy bufor */
	buf = kalloc(data->blk_size);

	/* Odczytujemy wymagany blok danych */
	err = bdev_read(data->mp->device, buf, data->spb, EXT2_GET_SECTOR_NUM(data, (data->groups[group_idx].bg_inode_table + inode_offset)));
	if (err != data->spb)
	{
		err = -EIO;
		goto end;
	}
	else
		err = 0;

	memcpy((buf + ((((num - 1) % data->sb.s_inodes_per_group) * sizeof(struct ext2_inode)) % data->blk_size)), inode, sizeof(struct ext2_inode));

	/* Zapisujemy zmienony blok danych */
	err = bdev_write(data->mp->device, buf, data->spb, EXT2_GET_SECTOR_NUM(data, (data->groups[group_idx].bg_inode_table + inode_offset)));
	if (err == data->spb)
		err = 0;

end:
	kfree(buf);
	return err;
}

int inode_read(struct vnode * vnode)
{
	int err;
	struct ext2_inode * inode = vnode->data;
	struct ext2_data * data = vnode->mp->data;

	err = inode_do_read(data, inode, vnode->ino);
	if (err != 0)
		return err;

	/* Kopiujemy dane z i-node do v-node */
	vnode->mode = inode->i_mode;
	vnode->nlink = inode->i_links_count;
	//vnode->dev = inode->i_dev;
	vnode->uid = inode->i_uid;
	vnode->gid = inode->i_gid;
	vnode->size = inode->i_size;
	vnode->blocks = inode->i_blocks;
	vnode->block_size = data->blk_size;

	return 0;
}

int inode_write(struct vnode * vnode)
{
	struct ext2_inode * inode = vnode->data;

	/* Kopiujemy dane z vnode do i-node */
	inode->i_mode = vnode->mode;
	inode->i_links_count = vnode->nlink;
	inode->i_uid = vnode->uid;
	inode->i_gid = vnode->gid;
	inode->i_size = vnode->size;

	return inode_do_write(vnode->mp->data, inode, vnode->ino);
}

int inode_link(struct vnode * vnode, char * name, ino_t ino_num, struct ext2_inode * inode, int type)
{
	struct ext2_dir_entry * ent;
	struct ext2_data * data = vnode->mp->data;
	
	
	int tmp, rec_len, name_len, err, blocks = ROUND_UP(vnode->size, vnode->block_size) / vnode->block_size;
	uint32_t offset = 0;
	void * buf;
	
	name_len = strlen(name);
	if (name_len > EXT2_FILE_NAME)
		return -ENOTSUP;
	
	
	/* Zaokrąglamy aby zwiększyć wydajność */
	rec_len = ROUND_UP(sizeof(struct ext2_dir_entry) - sizeof(uint8_t) * EXT2_FILE_NAME + name_len, 4);	
	//kprintf("ext2: inode_link(): insert %s into %d (name_len: %d, rec_len: %d)\n", name, vnode->ino, name_len, rec_len);
	
	buf = kalloc(blocks * data->blk_size);
	err = inode_rw_content(vnode, buf, blocks, 0, EXT2_RW_READ);
	if (err < 0)
		goto end;
	
	err = 0;
	while(offset < vnode->size)
	{
		ent = (struct ext2_dir_entry *)(buf + offset);
		//kprintf("ext2: inode_link(): existing entry at offset %u (rec_len: %d, name_len: %d, inode: %d)\n", offset, ent->rec_len, ent->name_len, ent->inode);
		
		if (ent->rec_len < rec_len)
		{
			offset += ent->rec_len;
			continue;
		}
		else if (ent->inode != 0)
		{
			/* Sprawdzamy czy zmieści się nasz wpis */
			tmp = ROUND_UP(sizeof(struct ext2_dir_entry) - sizeof(uint8_t) * EXT2_FILE_NAME + ent->name_len, 4);
			if (ent->rec_len < (tmp + rec_len))
			{
				offset += ent->rec_len;
				continue;
			}
			
			/* Dzielimy wpis na dwa */
			rec_len = ent->rec_len - tmp;
			ent->rec_len = tmp;
			offset += tmp;
			ent = (struct ext2_dir_entry *)(buf + offset);
			ent->rec_len = rec_len;
		}
		
		ent->inode = ino_num;
		ent->name_len = name_len;
		ent->file_type = type;
		strcpy((char *)ent->name, name);
		
		err = inode_rw_content(vnode, buf, blocks, 0, EXT2_RW_WRITE);
		if (err < 0)
			goto end;
		
		
		err = 0;
		goto end;
	}
	
	TODO("alloc new block");
	err = -ENOSYS;
end:
	kfree(buf);
	
	if (!err)
	{
		/* Aktualizacja liczby odnosień do i-node */
		inode->i_links_count++;
		err = inode_do_write(data, inode, ino_num);
		
		if (err < 0)
		{
			inode_unlink(vnode, name);
		}
	}
	
	return err;
}

int inode_unlink(struct vnode * vnode, char * name)
{
	int pos, err;
	void * buf;
	struct ext2_dir_entry * entry;
	struct ext2_dir_entry * prev = NULL;
	int blocks = EXT2_INODE_BLOCKS(vnode->mp->data, vnode->blocks);

	/* Ładujemy cały katalog do pamięci */
	buf = kalloc(vnode->block_size * blocks);
	err = inode_rw_content(vnode, buf, blocks, 0, EXT2_RW_READ);
	if (err < 0)
		goto end;

	pos = 0;
	err = -ENOENT;
	
	while(pos < vnode->size)
	{
		entry = (struct ext2_dir_entry *)(buf + pos);
		if (!entry->rec_len) /* Koniec katalogu */
			break;

		if ((strlen(name) == entry->name_len) && (!strncmp(name, (char *)entry->name, entry->name_len)))
		{
			/* Jeżeli to nie jest pierwszy wpis, mozemy powiekszyc poprzedni */
			if (prev)
				prev->rec_len += entry->rec_len;
			else
				entry->inode = 0;
			
			err = inode_rw_content(vnode, buf, blocks, 0, EXT2_RW_WRITE);
			if (err > 0)
				err = 0;

			goto end;
		}
		
		pos += entry->rec_len;
		prev = entry;
	}
	
	err = -ENOENT;

end:
	kfree(buf);
	return err;
}

int inode_do_alloc_block(struct vnode * vnode, uint32_t * block, int zeroblock)
{
	int err;
	struct ext2_data * data = vnode->mp->data;
	struct ext2_inode * inode = vnode->data;
	static void * zero = NULL;
	
	err = block_alloc(data, block, EXT2_GET_INODE_GROUP(data, vnode->ino));
	if (err != 0)
		return err;
	
	inode->i_blocks++;
	
	if (zeroblock)
	{
		if (!zero)
		{
			zero = kalloc(vnode->block_size);
			memset(zero, 0x00, vnode->block_size);
		}
		
		err = bdev_write(vnode->mp->device, zero, data->spb, EXT2_GET_SECTOR_NUM(data, *block));		
		if (err < 0)
		{
			TODO("free block");
			return err;
		}
	}
	
	return 0;
}


int32_t inode_rw_content(struct vnode * vnode, void * buf, uint32_t blocks, uint32_t start, int rw)
{
	int idx, err, tmp, bpi; /* Blocks per indirect */
	void * ptr;
	uint32_t * indirect_buf;
	uint32_t * indirect_buf2;
	int indirect_idx, i = 0;

	struct ext2_data * data = vnode->mp->data;
	struct ext2_inode * inode = vnode->data;

	ptr = buf;
	
	if (rw == EXT2_RW_WRITE)
	{
		for(idx = 0; (idx < start) && (idx < 12); idx++)
		{
			if (inode->i_block[idx] < 2)
			{
				err = inode_do_alloc_block(vnode, &inode->i_block[idx], 1);
				if (err != 0)
					return err;
			}
		}
	}
	
	/* Odczytujemy do 12 bezpośrednich bloków */
	for(idx = start; (blocks > 0) && (idx < 12); idx++)
	{
		/* Koniec zawartosci? */
		if (inode->i_block[idx] < 2)
		{
			if (rw == EXT2_RW_READ)
				break;
			
			/* Alokujemy blok */
			err = inode_do_alloc_block(vnode, &inode->i_block[idx], 0);
			if (err != 0)
				return err;
		}
		
		if (rw == EXT2_RW_READ)
			err = bdev_read(data->mp->device, ptr, data->spb, EXT2_GET_SECTOR_NUM(data, inode->i_block[idx]));
		else
			err = bdev_write(data->mp->device, ptr, data->spb, EXT2_GET_SECTOR_NUM(data, inode->i_block[idx]));
		
		if (err != data->spb)
			return -EIO;

		ptr += data->blk_size;
		blocks--;
	}

	bpi = data->blk_size / sizeof(uint32_t);

	/* Czy są jakieś bloki niebezposrednie */
	if (blocks > 0)
	{
		/* Alokujemy bufor */
		indirect_buf = kalloc(data->blk_size);
		indirect_idx = idx - 12;

		/* Sprawdzamy czy mamy jakieś bloki do przeczytania/zapisania na 1 poziomie */
		if ((indirect_idx < bpi) || (rw == EXT2_RW_WRITE))
		{
			if (inode->i_block[12] < 2)
			{
				if (rw == EXT2_RW_READ)
				{
					kfree(indirect_buf);
					return (ptr - buf) / vnode->block_size;
				}
				
				/* Alokujemy blok na indeks */
				err = inode_do_alloc_block(vnode, &inode->i_block[12], 0);
				if (err != 0)
				{
					kfree(indirect_buf);
					return err;
				}
				
				memset(indirect_buf, 0x00, data->blk_size);
			}
			else
			{
				/* Ładujemy indeks */
				err = bdev_read(data->mp->device, indirect_buf, data->spb, EXT2_GET_SECTOR_NUM(data, inode->i_block[12]));
				if (err < 0)
				{
					kfree(indirect_buf);
					return -EIO;
				}
			}

			if (rw == EXT2_RW_WRITE)
			{
				for(i = 0; (i < indirect_idx) && (i < bpi); i++)
				{
					if (indirect_buf[i] < 2)
					{
						err = inode_do_alloc_block(vnode, &indirect_buf[i], 1);
						if (err != 0)
						{
							kfree(indirect_buf);
							return err;
						}
					}
				}
			}
			
			/* Odczytujemy/zapisujemy bloki na 1 poziomie */
			for(;(indirect_idx < bpi) && (blocks > 0); indirect_idx++)
			{
				/* Koniec zawartosci? */
				if (indirect_buf[indirect_idx] < 2)
				{
					if (rw == EXT2_RW_READ)
						break;
					
					err = inode_do_alloc_block(vnode, &indirect_buf[i], 0);
					if (err != 0)
					{
						kfree(indirect_buf);
						return err;
					}
				}
				
				if (rw == EXT2_RW_READ)
					err = bdev_read(data->mp->device, ptr, data->spb, EXT2_GET_SECTOR_NUM(data, indirect_buf[indirect_idx]));
				else
					err = bdev_write(data->mp->device, ptr, data->spb, EXT2_GET_SECTOR_NUM(data, indirect_buf[indirect_idx]));
				
				if (err != data->spb)
				{
					kfree(indirect_buf);
					return -EIO;
				}

				ptr += data->blk_size;
				blocks--;
			}
			
			if (rw == EXT2_RW_WRITE)
			{
				err = bdev_write(data->mp->device, indirect_buf, data->spb, EXT2_GET_SECTOR_NUM(data, inode->i_block[12]));
				if (err < 0)
				{
					kfree(indirect_buf);
					return -EIO;
				}
			}
		}

		/* Bloki na poziomie 2 */
		indirect_idx -= bpi; /* NOTE: indirect_idx jest inkrementowany, czyli jeśli są jakieś bloki to jest on równy conajmniej bpi */
		if ((indirect_idx >= 0) && (indirect_idx < bpi * bpi))
		{
			indirect_buf2 = kalloc(data->blk_size);
			
			if (inode->i_block[13] < 2)
			{
				if (rw == EXT2_RW_READ)
				{
					kfree(indirect_buf2);
					kfree(indirect_buf);
					return (ptr - buf) / vnode->block_size;
				}
				
				/* Alokujemy blok na indeks 1-rzedu*/
				err = inode_do_alloc_block(vnode, &inode->i_block[13], 0);
				if (err != 0)
				{
					kfree(indirect_buf2);
					kfree(indirect_buf);
					return err;
				}
				
				memset(indirect_buf, 0x00, data->blk_size);
			}
			else
			{
				err = bdev_read(data->mp->device, indirect_buf, data->spb, EXT2_GET_SECTOR_NUM(data, inode->i_block[13]));
				if (err < 0)
				{
					kfree(indirect_buf2);
					kfree(indirect_buf);
					return -EIO;
				}
			}

			if (rw == EXT2_RW_READ)
				idx = indirect_idx;
			else
				idx = 0;
			
			while ((idx < bpi * bpi) && (blocks > 0))
			{
				tmp = idx / bpi;
				if (indirect_buf[tmp] < 2)
				{
					if (rw == EXT2_RW_READ)
					{
						kfree(indirect_buf2);
						kfree(indirect_buf);
						return (ptr - buf) / vnode->block_size;
					}
					
					/* Alokujemy blok na indeks 2-rzedu */
					err = inode_do_alloc_block(vnode, &indirect_buf[tmp], 0);
					if (err != 0)
					{
						kfree(indirect_buf2);
						kfree(indirect_buf);
						return err;
					}
					
					memset(indirect_buf2, 0x00, data->blk_size);
				}
				else
				{
					err = bdev_read(data->mp->device, indirect_buf2, data->spb, EXT2_GET_SECTOR_NUM(data, indirect_buf[tmp]));
					if (err != data->spb)
					{
						kfree(indirect_buf2);
						kfree(indirect_buf);
						return -EIO;
					}
				}

				
				for(i = idx % bpi; (i < bpi) && (idx < indirect_idx); i++)
				{
					if (indirect_buf2[i] < 2)
					{
						err = inode_do_alloc_block(vnode, &indirect_buf2[i], 1);
						if (err != 0)
						{
							kfree(indirect_buf2);
							kfree(indirect_buf);
							return err;
						}
					}
					idx++;
				}
				
				for(; (i < bpi) && (blocks > 0); i++)
				{
					if (indirect_buf2[i] < 2)
					{
						if (rw == EXT2_RW_READ)
						{
							kfree(indirect_buf2);
							kfree(indirect_buf);
							return (ptr - buf) / vnode->block_size;
						}
					
						/* Alokujemy blok */
						err = inode_do_alloc_block(vnode, &indirect_buf2[i], 0);
						if (err != 0)
						{
							kfree(indirect_buf2);
							kfree(indirect_buf);
							return err;
						}
					}
				
					if (rw == EXT2_RW_READ)
						err = bdev_read(data->mp->device, ptr, data->spb, EXT2_GET_SECTOR_NUM(data, indirect_buf2[i]));
					else
						err = bdev_write(data->mp->device, ptr, data->spb, EXT2_GET_SECTOR_NUM(data, indirect_buf2[i]));
				
					if (err < 0)
					{
						kfree(indirect_buf2);
						kfree(indirect_buf);
						return -EIO;
					}
				
					ptr += data->blk_size;
					blocks--;
					idx++;
				}
				
				if (rw == EXT2_RW_WRITE)
				{
					err = bdev_write(data->mp->device, indirect_buf2, data->spb, EXT2_GET_SECTOR_NUM(data, indirect_buf[tmp]));
					if (err != data->spb)
					{
						kfree(indirect_buf2);
						kfree(indirect_buf);
						return -EIO;
					}
				}
			}

			if (rw == EXT2_RW_WRITE)
			{
				err = bdev_write(data->mp->device, indirect_buf, data->spb, EXT2_GET_SECTOR_NUM(data, inode->i_block[13]));
				if (err < 0)
				{
					kfree(indirect_buf2);
					kfree(indirect_buf);
					return -EIO;
				}
			}
			
			if ((indirect_idx < bpi * bpi * bpi) && (blocks > 0))
			{
				TODO("triple indirect blocks!");
				while(1);
			}
			kfree(indirect_buf2);
		}

		kfree(indirect_buf);

	}
	
	return (ptr - buf) / vnode->block_size;
}
