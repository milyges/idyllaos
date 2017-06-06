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

static int ext2_open(struct vnode * vnode)
{
	int err;

	vnode->data = kalloc(sizeof(struct ext2_inode));

	/* Wczytujemy i-node */
	err = inode_read(vnode);
	vnode->dev = vnode->mp->device;
	
	if (err != 0)
	{
		kfree(vnode->data);
		vnode->data = NULL;
	}

	return err;
}

static int ext2_close(struct vnode * vnode)
{	
	if ((vnode->mp->flags & MS_RDONLY) != MS_RDONLY)
	{
		inode_write(vnode);
		
		if (!vnode->nlink)
		{
			/* Zwalniamy i-node */
			TODO("free i-node");
		}
	}
	kfree(vnode->data);
	return 0;
}

static ino_t ext2_lookup(struct vnode * vnode, char * name)
{
	int pos, err;
	void * buf;
	ino_t ino = 0;
	struct ext2_dir_entry * entry;
	int blocks = EXT2_INODE_BLOCKS(vnode->mp->data, vnode->blocks);

	/* Ładujemy cały katalog do pamięci */
	buf = kalloc(vnode->block_size * blocks);
	err = inode_rw_content(vnode, buf, blocks, 0, EXT2_RW_READ);
	if (err < 0)
		goto end;

	pos = 0;
	while(pos < vnode->size)
	{
		entry = (struct ext2_dir_entry *)(buf + pos);
		if (!entry->rec_len) /* Koniec katalogu */
			break;

		if ((strlen(name) == entry->name_len) && (!strncmp(name, (char *)entry->name, entry->name_len)))
		{
			ino = entry->inode;
			break;
		}

		pos += entry->rec_len;
	}

end:
	kfree(buf);
	return ino;
}

static ssize_t ext2_read(struct vnode * vnode, void * buf, size_t len, loff_t offset)
{
	size_t left, bytes;
	void * tmpbuf = NULL;
	uint32_t start;
	int err;
	
	//kprintf("ext2_read(%d, %x, %u, %llu)\n", vnode->ino, buf, len, offset);
	
	/* Przycinamy do rozmiaru pliku */
	if (offset + len >= vnode->size)
	{
		if (vnode->size < offset)
			return 0;
		else
			len = vnode->size - offset;
	}
	
	left = len;
	
	/* Obliczamy pierwszy blok */
	start = offset / vnode->block_size;
	
	/* Sprawdzamy czy offset jest wyrównany do bloku */
	if (offset % vnode->block_size)
	{
		/* Jeśli nie, alokujemy bufor */
		tmpbuf = kalloc(vnode->block_size);
		
		err = inode_rw_content(vnode, tmpbuf, 1, start, EXT2_RW_READ);
		if (err < 0)
			goto end;
		
		/* Kopiujemy dane z bufora tymczasowego do docelowego */
		bytes = MIN(vnode->block_size - (offset % vnode->block_size), len);
		memcpy(buf, (void *)(tmpbuf + (offset % vnode->block_size)), bytes);
		left -= bytes;
		buf += bytes;
		start++;
	}

	if (left >= vnode->block_size)
	{
		err = inode_rw_content(vnode, buf, left / vnode->block_size, start, EXT2_RW_READ);
		if (err < 0)
			goto end;
		
		bytes = ROUND_DOWN(left, vnode->block_size); 
		buf += bytes;
		left -= bytes;
		start += bytes / vnode->block_size;
	}
	
	if (left > 0)
	{
		if (!tmpbuf)
			tmpbuf = kalloc(vnode->block_size);
		
		err = inode_rw_content(vnode, tmpbuf, 1, start, EXT2_RW_READ);
		if (err < 0)
			goto end;
		

		memcpy(buf, tmpbuf, left);
		left = 0;
	}
	
	err = len - left;
end:	
	if (tmpbuf)
		kfree(tmpbuf);
	
	return err;
#if 0

	uint32_t start, count;
	void * tmpbuf;
	int err;

	if (offset + len >= vnode->size)
	{
		if (vnode->size < offset)
			return 0;
		else
			len = vnode->size - offset;
	}

	start = offset / vnode->block_size;
	count = ROUND_UP(len, vnode->block_size) / vnode->block_size;

	tmpbuf = kalloc(count * vnode->block_size);
	err = inode_read_content(vnode, tmpbuf, count, start);
	if (err < 0)
		goto end;

	memcpy(buf, (void *)(tmpbuf + (offset % vnode->block_size)), len);
	err = len;

end:
	kfree(tmpbuf);
	return err;
#endif
}

static ssize_t ext2_write(struct vnode * vnode, void * buf, size_t len, loff_t offset)
{
	size_t left, bytes;
	void * tmpbuf = NULL;
	uint32_t start;
	int err;
	
	//kprintf("ext2_write(%d, %x, %u, %llu)\n", vnode->ino, buf, len, offset);
	
	left = len;
	
	/* Obliczamy pierwszy blok */
	start = offset / vnode->block_size;

	/* Sprawdzamy czy offset jest wyrównany do bloku */
	if (offset % vnode->block_size)
	{
		/* Jeśli nie, alokujemy bufor */
		tmpbuf = kalloc(vnode->block_size);
		
		if (inode_rw_content(vnode, tmpbuf, 1, start, EXT2_RW_READ) <= 0)
			memset(tmpbuf, 0, vnode->block_size);
		
		/* Kopiujemy dane z bufora zródłowego do tymczasowego */
		bytes = MIN(vnode->block_size - (offset % vnode->block_size), len);
		memcpy((void *)(tmpbuf + (offset % vnode->block_size)), buf, bytes);
		
		/* Zapisujemy zmiany */
		err = inode_rw_content(vnode, tmpbuf, 1, start, EXT2_RW_WRITE);
		if (err < 0)
			goto end;
		
		left -= bytes;
		buf += bytes;
		start++;
	}

	if (left >= vnode->block_size)
	{
		err = inode_rw_content(vnode, buf, left / vnode->block_size, start, EXT2_RW_WRITE);
		if (err < 0)
			goto end;
		buf += ROUND_DOWN(left, vnode->block_size);
		left -= ROUND_DOWN(left, vnode->block_size);
		start += left / vnode->block_size;
	}
	
	if ((left > 0) && ((offset + len) % vnode->block_size))
	{
		if (!tmpbuf)
			tmpbuf = kalloc(vnode->block_size);
		
		if (inode_rw_content(vnode, tmpbuf, 1, start, EXT2_RW_READ) <= 0)
			memset(tmpbuf, 0, vnode->block_size);
		
		memcpy(tmpbuf, buf, left);
		err = inode_rw_content(vnode, tmpbuf, 1, start, EXT2_RW_WRITE);
		if (err < 0)
			goto end;
		left = 0;
	}
	
	err = len - left;
	
	//kprintf(" len = %d, left = %d\n", len, left);
	
	vnode->size = MAX(vnode->size, offset + err);
	inode_write(vnode);
	
end:
	if (tmpbuf)
		kfree(tmpbuf);
	
	return err;
}

int ext2_getdents(struct vnode * vnode, struct dirent * dirp, size_t len, loff_t * offset)
{
	struct ext2_dir_entry * entry;
	struct ext2_data * data = vnode->mp->data;
	struct dirent * dirent;
	int blocks = ROUND_UP(vnode->size, vnode->block_size) / vnode->block_size;
	void * buf;
	int err;
	int reclen;


	buf = kalloc(blocks * data->blk_size);
	err = inode_rw_content(vnode, buf, blocks, 0, EXT2_RW_READ);
	if (err < 0)
		goto end;

	err = 0;
	while (*offset < vnode->size)
	{
		entry = (struct ext2_dir_entry *)(buf + *offset);
		dirent = (struct dirent *)((void *)dirp + err);

		if (!entry->rec_len)
			break;

		reclen = ROUND_UP((addr_t)&dirent->d_name[entry->name_len + 1] - (addr_t)dirent, 4);
		if (err + reclen >= len)
			break;

		dirent->d_ino = entry->inode;
		memcpy(dirent->d_name, entry->name, entry->name_len);
		dirent->d_name[entry->name_len] = '\0';
		dirent->d_reclen = reclen;

		*offset += entry->rec_len;
		dirent->d_off = *offset;
		err += dirent->d_reclen;
	}

end:
	kfree(buf);
	return err;
}

/* Funkcja zapisuje v-node na dysk */
int ext2_flush(struct vnode * vnode)
{
	return inode_write(vnode);
}

int ext2_creat(struct vnode * vnode, char * name, mode_t mode, uid_t uid, gid_t gid)
{
	ino_t ino_num;
	int err;
	struct ext2_inode ino;

	/* Alokujemy nowy i-node:
	   - Na początek próbujemy w tej samej grupe co i-node rodzic
	   - Jesli się nie da to wtedy w grupie która ma najwięcej wolnego miejsca */
	err = inode_alloc(vnode->mp->data, &ino_num, EXT2_GET_INODE_GROUP(vnode->mp->data, vnode->ino));
	if (err == -ENOSPC)
		err = inode_alloc(vnode->mp->data, &ino_num, -1);

	if (err != 0)
		goto end;

	/* Zapisujemy i-node na dysk */
	memset(&ino, 0, sizeof(struct ext2_inode));
	ino.i_mode = S_IFREG | (mode & 0777);
	ino.i_uid = uid;
	ino.i_gid = gid;
	ino.i_links_count = 0;
	
	/* TODO: Czasy */

	err = inode_link(vnode, name, ino_num, &ino, EXT2_FT_REG_FILE);
	if (err != 0)
 		inode_free(vnode->mp->data, ino_num);
	
end:
	return err;
}

static int ext2_unlink(struct vnode * vnode, char * name)
{
	return -ENOSYS; //inode_unlink(vnode, name);	
}

static int ext2_mount(struct mountpoint * mp, char * flags)
{
	int err, i;
	struct ext2_data * data;

	/* Otwieramy urzadzenie */
	err = bdev_open(mp->device);
	if (err != 0)
		return err;

	/* Inicjujemy strukturę która będzie przechowywać dane o zamontowanym systemie plikow */
	data = kalloc(sizeof(struct ext2_data));
	memset(data, 0x00, sizeof(struct ext2_data));
	mutex_init(&data->sb_mutex);
	data->bps = 512; /* TODO: Odczytaj ze sterownika */
	data->mp = mp;
	mp->data = data;

	/* Ładujemy super block */
	err = super_read(data);
	if (err != 0)
	{
		kfree(data);
		return err;
	}

	data->blk_size = EXT2_BLOCK_SIZE(&data->sb);
	data->spb = data->blk_size / data->bps;

	if (data->sb.s_inode_size != sizeof(struct ext2_inode))
	{
		kprintf("ext2: %s: Wrong i-node size\n", mp->devname);
		kfree(data);
		bdev_close(mp->device);
		return -EINVAL;
	}

	/* Sprawdzamy czy system plików był poprawnie odmontowany */
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
		kprintf(KERN_DEBUG"ext2: %s: File system clean\n", mp->devname);


	/* Obliczamy ilość grup bloków i ilość bloków na informacje o grupach */
	data->groups_count = ROUND_UP(data->sb.s_blocks_count, data->sb.s_blocks_per_group) / data->sb.s_blocks_per_group;
	data->groups_blocks = ROUND_UP(data->groups_count * sizeof(struct ext2_group_desc), data->blk_size) / data->blk_size;

	/* Alokujemy pamięć na informacje o grupach oraz muteksy dla grup */
	data->groups = kalloc(data->groups_blocks * data->blk_size);
	data->groups_mutex = kalloc(sizeof(struct mutex) * data->groups_count);

	/* Inicjujemy muteksy */
	for(i = 0; i < data->groups_count; i++)
		mutex_init(&data->groups_mutex[i]);

	/* Odczytujemy informacje o grupach */
	err = super_read_groups(data);
	if (err != 0)
	{
		kfree(data->groups);
		kfree(data->groups_mutex);
		kfree(data);
		return err;
	}

	/* Jezeli osiagnelismy limit montowan, wyswietlamy komunikat */
	if (data->sb.s_mnt_count >= data->sb.s_max_mnt_count)
		kprintf(KERN_DEBUG"ext2: %s: maximal mount count reached, checking file system is recommended\n", mp->devname);

	/* Jeżeli nie montujemy read-only */
	if ((mp->flags & MS_RDONLY) != MS_RDONLY)
	{
		/* Aktualizujemy super block */
		data->sb.s_state = EXT2_ERROR_FS;
		data->sb.s_mnt_count++;
		/* TODO: Czas montowania */

		/* Zapisujemy super-block */
		err = super_write(data);
		if (err != 0)
		{
			kfree(data->groups);
			kfree(data->groups_mutex);
			kfree(data);
			return err;
		}
	}

	return 0;
}

static int ext2_umount(struct mountpoint * mp)
{
	return -ENOSYS;
}

static ino_t ext2_root(struct mountpoint * mp)
{
	return EXT2_ROOT_INO;
}


static struct vnode_ops _ext2_vnode_ops =
{
	.open = &ext2_open,
	.close = &ext2_close,
	.lookup = &ext2_lookup,
	.read = &ext2_read,
	.write = &ext2_write,
	.getdents = &ext2_getdents,
	.creat = &ext2_creat,
	.flush = &ext2_flush,
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


static int init(int argc, char * argv[])
{
	fs_register(&_ext2_fs);
	return 0;
}

static int clean(void)
{

	return -ENOSYS;
}

MODULE_INFO("ext2", &init, &clean);
