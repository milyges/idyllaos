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

#define RAMFS_MAGIC        0xDEADBEEF
#define RAMFS_ROOT_INODE   1
#define RAMFS_DEFAULT_SIZE 0x400000 /* 4MiB */

struct ramfs_superblock
{
	uint32_t magic;

	uint32_t inodes;
	struct ramfs_inode ** inode_table;

	uint32_t size;
	uint32_t used;

	struct mutex mutex;
};

struct ramfs_inode
{
	ino_t num;

	mode_t mode;

	uid_t uid;
	gid_t gid;

	time_t ctime;
	time_t mtime;
	time_t atime;

	nlink_t nlink;
	off_t size;
	dev_t rdev;

	void * data;
	struct mutex mutex;
};

struct ramfs_dentry
{
	char name[NAME_MAX + 1];
	ino_t inode;
};

static int ramfs_put_entry(struct ramfs_inode * parent, struct ramfs_inode * ino, char * name)
{
	struct ramfs_dentry * entry;
	size_t pos = 0;

	mutex_lock(&ino->mutex);
	ino->nlink++;
	mutex_unlock(&ino->mutex);

	mutex_lock(&parent->mutex);
	while(pos < parent->size)
	{
		entry = (struct ramfs_dentry *)(parent->data + pos);
		if (!entry->inode) /* Jest pusty element */
			goto found_free;

		pos += sizeof(struct ramfs_dentry);
	}

	parent->size += sizeof(struct ramfs_dentry);
	parent->data = krealloc(parent->data, parent->size);
	entry = (struct ramfs_dentry *)(parent->data + parent->size - sizeof(struct ramfs_dentry));

found_free:
	entry->inode = ino->num;
	strcpy(entry->name, name);

	mutex_unlock(&parent->mutex);

	return 0;
}

static int ramfs_put_inode(struct ramfs_superblock * super, struct ramfs_inode * ino)
{
	int i;
	mutex_lock(&super->mutex);
	for(i=0;i<super->inodes;i++)
	{
		if (!super->inode_table[i])
		{
			ino->num = i + 1;
			super->inode_table[i] = ino;
			mutex_unlock(&super->mutex);
			return 0;
		}
	}
	super->inodes++;
	super->inode_table = krealloc(super->inode_table, sizeof(struct ramfs_inode *) * super->inodes);
	super->inode_table[super->inodes - 1] = ino;
	ino->num = super->inodes;
	mutex_unlock(&super->mutex);
	return 0;
}

static int ramfs_mount(struct mountpoint * mp, char * flags)
{
	struct ramfs_superblock * super;
	struct ramfs_inode * inode;

	/* Tworzymy superblock */
	super = kalloc(sizeof(struct ramfs_superblock));
	super->magic = RAMFS_MAGIC;
	super->size = RAMFS_DEFAULT_SIZE;
	super->used = 0;
	mutex_init(&super->mutex);

	/* Create root i-node */
	inode = kalloc(sizeof(struct ramfs_inode));
	inode->num = RAMFS_ROOT_INODE;
	inode->mode = S_IFDIR | 0755;
	inode->uid = 0;
	inode->gid = 0;
	inode->ctime = 0; /* sys_time(NULL); */
	inode->mtime = inode->ctime;
	inode->atime = inode->ctime;
	inode->size = 0;
	inode->rdev = 0;
	inode->nlink = 0;
	inode->data = NULL;
	mutex_init(&inode->mutex);

	/* Create . and .. in root directory */
	ramfs_put_entry(inode, inode, ".");
	ramfs_put_entry(inode, inode, "..");
	/* Insert root i-node into inode table */
	super->inode_table = kalloc(sizeof(struct ramfs_inode *));
	super->inode_table[0] = inode;
	super->inodes = 1;

	/* Save super block in mount point struct */
	mp->data = super;
	return 0;
}

static int ramfs_umount(struct mountpoint * mp)
{
	return -ENOSYS;
}

static ino_t ramfs_root(struct mountpoint * mp)
{
	return RAMFS_ROOT_INODE;
}

static int ramfs_flush(struct vnode * vnode)
{
	struct ramfs_inode * inode;

	/* Copy info from ramfs_inode do vfs inode */
	inode = vnode->data;

	mutex_lock(&inode->mutex);
	inode->mode = vnode->mode;
	inode->uid = vnode->uid;
	inode->gid = vnode->gid;
	inode->size = vnode->size;
	inode->nlink = vnode->nlink;
	/*inode->atime = vnode->atime;
	inode->mtime = vnode->mtime;
	inode->ctime = vnode->ctime;*/
	inode->rdev = vnode->rdev;

	if (vnode->size != inode->size)
	{
		inode->size = vnode->size;
		inode->data = krealloc(inode->data, inode->size);
	}

	mutex_unlock(&inode->mutex);
	return 0;
}

static int ramfs_open(struct vnode * vnode)
{
	struct ramfs_superblock * super;
	struct ramfs_inode * inode;

	super = vnode->mp->data;
	if (super->magic != RAMFS_MAGIC)
		return -EFAULT;

	mutex_lock(&super->mutex);
	/* Check for wrong i-node num */
	if (vnode->ino > super->inodes)
	{
		mutex_unlock(&super->mutex);
		return -ENOENT;
	}

	inode = super->inode_table[vnode->ino - 1];
	vnode->data = inode;
	vnode->mode = inode->mode;
	vnode->uid = inode->uid;
	vnode->gid = inode->gid;
	vnode->nlink = inode->nlink;
	/*vnode->atime = inode->atime;
	vnode->mtime = inode->mtime;
	vnode->ctime = inode->ctime;*/
	vnode->rdev = inode->rdev;
	vnode->size = inode->size;

	mutex_unlock(&super->mutex);
	return 0;
}

static int ramfs_close(struct vnode * vnode)
{
	struct ramfs_superblock * super;
	struct ramfs_inode * inode;

	super = vnode->mp->data;
	if (super->magic != RAMFS_MAGIC)
		return -EFAULT;

	inode = vnode->data;

	ramfs_flush(vnode);

	if (!inode->nlink)
	{
		/* Remove i-node */
		mutex_lock(&super->mutex);
		kfree(inode->data);
		super->inode_table[inode->num - 1] = NULL;
		kfree(inode);
		mutex_unlock(&super->mutex);
	}

	return 0;
}

static ino_t ramfs_lookup(struct vnode * vnode, char * name)
{
	struct ramfs_inode * inode = vnode->data;
	struct ramfs_dentry * dir;
	off_t pos = 0;
	mutex_lock(&inode->mutex);
	while(pos < inode->size)
	{
		dir = (struct ramfs_dentry *)(inode->data + pos);
		if ((!strcmp(dir->name, name)) && (dir->inode > 0))
		{
			mutex_unlock(&inode->mutex);
			return dir->inode;
		}
		pos += sizeof(struct ramfs_dentry);
	}
	mutex_unlock(&inode->mutex);
	return 0;
}

static int ramfs_getdents(struct vnode * vnode, struct dirent * dirp, size_t len, loff_t * offset)
{
	struct ramfs_inode * inode;
	struct ramfs_dentry * ent;
	struct dirent * dirent;

	int count = 0;

	inode = vnode->data;

	while((len >= sizeof(struct dirent)) && (*offset < inode->size))
	{
		ent = (struct ramfs_dentry *)(inode->data + *offset);
		dirent = (struct dirent *)((void *)dirp + count);
		if (!ent->inode)
		{
			*offset += sizeof(struct ramfs_dentry);
			continue;
		}

		dirent->d_ino = ent->inode;
		strcpy(dirent->d_name, ent->name);
		dirent->d_reclen = ROUND_UP((addr_t)&dirent->d_name[strlen(dirent->d_name) + 1] - (addr_t)dirent, 4);

		count += dirent->d_reclen;
		len -= sizeof(sizeof(struct dirent));
		*offset += sizeof(struct ramfs_dentry);
		dirent->d_off = *offset;
	}

	return count;
}

int ramfs_creat(struct vnode * vnode, char * name, mode_t mode)
{
	struct ramfs_superblock * super;
	struct ramfs_inode * parent_inode;
	struct ramfs_inode * new_inode;
	parent_inode = vnode->data;
	super = vnode->mp->data;
	if (super->magic != RAMFS_MAGIC)
		return -EFAULT;

	/* Create new i-node */
	new_inode = kalloc(sizeof(struct ramfs_inode));
	new_inode->mode = S_IFREG | (mode & 0777);
	new_inode->uid = 0;
	new_inode->gid = 0;
	new_inode->ctime = 0; /* sys_time(NULL); */
	new_inode->mtime = new_inode->ctime;
	new_inode->atime = new_inode->ctime;
	new_inode->size = 0;
	new_inode->rdev = 0;
	new_inode->data = NULL;
	new_inode->nlink = 0;

	mutex_init(&new_inode->mutex);
	/* Put inode into file system */
	ramfs_put_inode(super, new_inode);
	/* Put new directory into parent */
	ramfs_put_entry(parent_inode, new_inode, name);
	return 0;
}

int ramfs_mknod(struct vnode * vnode, char * name, mode_t mode, dev_t rdev, uid_t uid, gid_t gid)
{
	struct ramfs_superblock * super;
	struct ramfs_inode * parent_inode;
	struct ramfs_inode * new_inode;
	parent_inode = vnode->data;
	super = vnode->mp->data;
	if (super->magic != RAMFS_MAGIC)
		return -EFAULT;

	/* Create new i-node */
	new_inode = kalloc(sizeof(struct ramfs_inode));
	new_inode->mode = mode;
	new_inode->uid = uid;
	new_inode->gid = gid;
	new_inode->ctime = 0; /* sys_time(NULL); */
	new_inode->mtime = new_inode->ctime;
	new_inode->atime = new_inode->ctime;
	new_inode->size = 0;
	new_inode->rdev = rdev;
	new_inode->data = NULL;
	new_inode->nlink = 0;

	mutex_init(&new_inode->mutex);
	/* Put inode into file system */
	ramfs_put_inode(super, new_inode);
	/* Put new directory into parent */
	ramfs_put_entry(parent_inode, new_inode, name);
	return 0;
}

static ssize_t ramfs_write(struct vnode * vnode, void * buf, size_t len, loff_t offset)
{
	struct ramfs_superblock * super;
	struct ramfs_inode * inode;

	inode = vnode->data;
	super = vnode->mp->data;
	if (super->magic != RAMFS_MAGIC)
		return -EFAULT;
	mutex_lock(&inode->mutex);
	mutex_lock(&super->mutex);
	/* Check for free space */
	if (offset + len > inode->size)
	{
		if (super->used + inode->size - (offset + len) >= super->size)
		{
			mutex_unlock(&super->mutex);
			mutex_unlock(&inode->mutex);
			return -ENOSPC;
		}

		inode->data = krealloc(inode->data, offset + len);
		if (offset + len > inode->size)
		{
			super->used += offset + len - inode->size;
			inode->size = offset + len;
			vnode->size = inode->size;
		}
	}
	mutex_unlock(&super->mutex);
	memcpy((void *)(inode->data + offset), buf, len);
	mutex_unlock(&inode->mutex);
	return len;
}

static ssize_t ramfs_read(struct vnode * vnode, void * buf, size_t len, loff_t offset)
{
	struct ramfs_superblock * super;
	struct ramfs_inode * inode;
	inode = vnode->data;
	super = vnode->mp->data;

	if (super->magic != RAMFS_MAGIC)
		return -EFAULT;

	mutex_lock(&inode->mutex);
	if (offset + len > inode->size)
		len = (inode->size - offset > 0) ? inode->size - offset : 0;

	memcpy(buf, (void *)(inode->data + offset), len);
	mutex_unlock(&inode->mutex);

	return len;
}

static int ramfs_mkdir(struct vnode * vnode, char * name, mode_t mode, uid_t uid, gid_t gid)
{
	struct ramfs_superblock * super;
	struct ramfs_inode * parent_inode;
	struct ramfs_inode * new_inode;

	parent_inode = vnode->data;
	super = vnode->mp->data;

	if (super->magic != RAMFS_MAGIC)
		return -EFAULT;

	/* Create new i-node */
	new_inode = kalloc(sizeof(struct ramfs_inode));
	new_inode->mode = S_IFDIR | mode;
	new_inode->uid = uid;
	new_inode->gid = gid;
	new_inode->ctime = 0; /* sys_time(NULL); */
	new_inode->mtime = new_inode->ctime;
	new_inode->atime = new_inode->ctime;
	new_inode->size = 0;
	new_inode->rdev = 0;
	new_inode->nlink = 0;
	new_inode->data = NULL;
	mutex_init(&new_inode->mutex);
	/* Put inode into file system */
	ramfs_put_inode(super, new_inode);
	/* Create . and .. */
	ramfs_put_entry(new_inode, new_inode, ".");
	ramfs_put_entry(new_inode, parent_inode, "..");
	/* Put new directory into parent */
	ramfs_put_entry(parent_inode, new_inode, name);
	return 0;
}

static int ramfs_unlink(struct vnode * vnode, char * name)
{
	struct ramfs_inode * inode = vnode->data;
	struct ramfs_dentry * dir;
	off_t pos = 0;
	mutex_lock(&inode->mutex);
	while(pos < inode->size)
	{
		dir = (struct ramfs_dentry *)(inode->data + pos);
		if (!strcmp(dir->name, name))
		{
			dir->inode = 0;
			mutex_unlock(&inode->mutex);
			return 0;
		}
		pos += sizeof(struct ramfs_dentry);
	}
	mutex_unlock(&inode->mutex);
	return -ENOENT;
}

static struct vnode_ops _ramfs_vnode_ops =
{
	.open = &ramfs_open,
	.close = &ramfs_close,
	.lookup = &ramfs_lookup,
	.getdents = &ramfs_getdents,
	.mknod = &ramfs_mknod,
	.creat = &ramfs_creat,
	.read = &ramfs_read,
	.write = &ramfs_write,
	.mkdir = &ramfs_mkdir,
	.unlink = &ramfs_unlink,
	.flush = &ramfs_flush
};

static struct filesystem_ops _ramfs_fs_ops =
{
	.mount = &ramfs_mount,
	.umount = &ramfs_umount,
	.root = &ramfs_root
};

static struct filesystem _ramfs_fs =
{
	.name = "ramfs",
	.fsid = 0,
	.flags = FS_NODEV,
	.fs_ops = &_ramfs_fs_ops,
	.vnode_ops = &_ramfs_vnode_ops
};


int init(int argc, char * argv[])
{
	fs_register(&_ramfs_fs);
	return 0;
}

int clean(void)
{

	return 0;
}

MODULE_INFO("ramfs", &init, &clean);
