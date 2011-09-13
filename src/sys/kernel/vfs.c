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
#include <arch/spinlock.h>
#include <kernel/types.h>
#include <kernel/vfs.h>
#include <kernel/kprintf.h>
#include <kernel/init.h>
#include <kernel/panic.h>
#include <kernel/debug.h>
#include <kernel/device.h>
#include <mm/heap.h>
#include <lib/errno.h>
#include <lib/string.h>
#include <lib/math.h>

static LIST_NEW(_fs_list);
static SPINLOCK_NEW(_fs_list_lock);

static LIST_NEW(_mount_list);
static SPINLOCK_NEW(_mount_list_lock);

static void vfs_path_split(char * path, struct vfs_path * res)
{
	int i, len = strlen(path) - 1;

	if (path[len] == '/')
		len--;

	i = len;

	while ((i > 0) && (path[i] != '/'))
		i--;

	if (path[i] == '/')
		strncpy(res->name, &path[i+1], len - i);
	else
		strncpy(res->name, &path[i], len - i + 1);

	if (i > 0)
		strncpy(res->path, path, i);
	else
		res->path[0] = '\0';
}

static int do_sys_access(struct vnode * vnode, int mode, struct proc * proc)
{
	int res;

	mode &= 0007;
	res = vnode->mode & 0777;

	if (!(proc->euid && proc->uid))
	{
		if (res & 0111)
			res = 0777;
		else
			res = 0666;
	}

	if (proc->euid == vnode->uid)
		res >>= 6;
	else if (proc->egid == vnode->gid)
		res >>= 6;

	if ((res & 0007 & mode) == mode)
		return 0;

	return -EACCES;
}

static inline void do_sys_stat(struct vnode * vnode, struct stat * stat)
{
	stat->st_dev = vnode->dev;
	stat->st_ino = vnode->ino;
	stat->st_mode = vnode->mode;
	stat->st_nlink = vnode->nlink;
	stat->st_uid = vnode->uid;
	stat->st_gid = vnode->gid;
	stat->st_rdev = vnode->rdev;
	stat->st_size = vnode->size;
	stat->st_blksize = vnode->block_size;
	stat->st_blocks = vnode->blocks;
}

static struct filesystem * fs_lookup(char * name)
{
	struct filesystem * fs;
	spinlock_lock(&_fs_list_lock);
	LIST_FOREACH(&_fs_list, fs)
	{
		if (!strcmp(fs->name, name))
		{
			atomic_inc(&fs->refcount);
			spinlock_unlock(&_fs_list_lock);
			return fs;
		}
	}

	spinlock_unlock(&_fs_list_lock);
	return NULL;
}

int fs_register(struct filesystem * fs)
{
	kprintf(KERN_DEBUG"vfs: registering %s file system\n", fs->name);
	spinlock_lock(&_fs_list_lock);
	list_init(&fs->list);
	atomic_set(&fs->refcount, 0);
	list_add(&_fs_list, &fs->list);
	spinlock_unlock(&_fs_list_lock);
	return 0;
}

int fs_unregister(struct filesystem * fs)
{
	return -ENOSYS;
}

static struct vnode * vnode_new(struct mountpoint * mp, ino_t ino)
{
	struct vnode * vnode;
	vnode = kalloc(sizeof(struct vnode));
	memset(vnode, 0, sizeof(struct vnode));

	list_init(&vnode->list);
	list_init(&vnode->pagecache);
	atomic_set(&vnode->refcount, 1);
	mutex_init(&vnode->mutex);
	vnode->ino = ino;
	vnode->mp = mp;
	vnode->ops = mp->fs->vnode_ops;
	vnode->dev = mp->device;

	if (vnode->ops->open(vnode) != 0)
	{
		kfree(vnode);
		return NULL;
	}

	mutex_lock(&mp->mutex);
	list_add(&mp->vnode_list, &vnode->list);
	mutex_unlock(&mp->mutex);

	return vnode;
}

static int vnode_free(struct vnode * vnode)
{
	int err;
	mutex_lock(&vnode->mp->mutex);
	mutex_lock(&vnode->mutex);
	if (atomic_get(&vnode->refcount) > 0)
	{
		mutex_unlock(&vnode->mutex);
		mutex_unlock(&vnode->mp->mutex);
		return -EBUSY;
	}

	list_remove(&vnode->list);
	mutex_unlock(&vnode->mp->mutex);

	err = vnode->ops->close(vnode);
	if (err != 0)
	{
		mutex_lock(&vnode->mp->mutex);
		list_add(&vnode->mp->vnode_list, &vnode->list);
		mutex_unlock(&vnode->mp->mutex);
		mutex_unlock(&vnode->mutex);
		return err;
	}

	kfree(vnode);
	return 0;
}

static struct vnode * vnode_lookup(struct mountpoint * mp, ino_t ino)
{
	struct vnode * vnode;
	mutex_lock(&mp->mutex);
	LIST_FOREACH(&mp->vnode_list, vnode)
	{
		if (vnode->ino == ino)
		{
			VNODE_HOLD(vnode);
			mutex_unlock(&mp->mutex);
			return vnode;
		}
	}
	mutex_unlock(&mp->mutex);
	return NULL;
}

/* TODO: Fix symlink support */
static int vnode_get(char * _path, struct vnode ** result, struct proc * proc, int followlink)
{
	int i = SYMLOOP_MAX, j;
	char * path = strdup(_path);
	char ** table;
	ino_t ino;
	struct vnode * vnode;
	struct vnode * new_vnode;
	int err;

	spinlock_lock(&proc->lock);
	if (path[0] == '/')
		vnode = proc->vnode_root;
	else
		vnode = proc->vnode_current;
	spinlock_unlock(&proc->lock);

	VNODE_HOLD(vnode);

	while(i > 0)
	{
		table = str_explode(path, '/');
		kfree(path);

		if (!table[0])
		{
			*result = vnode;
			str_unexplode(table);
			return 0;
		}

		j = 0;
		while(1)
		{
			/* Jeżeli szukamy .. oraz katalog jest punktem montowania */
			if ((!strcmp(table[j], "..")) && (vnode == vnode->mp->root_vnode) && (vnode->mp->parent_vnode != NULL))
			{
				VNODE_REL(vnode);
				vnode = vnode->mp->parent_vnode;
				VNODE_HOLD(vnode);
			}

			/* Ochrona chroota */
			if ((!strcmp(table[j], "..")) && (vnode == proc->vnode_root))
			{
				if (!table[j + 1])
				{
					*result = vnode;
					return 0;
				}
				j++;
				continue;
			}

			/* Znajdz inode */
			ino = vnode->ops->lookup(vnode, table[j]);
			if (!ino)
			{
				str_unexplode(table);
				*result = NULL;
				VNODE_REL(vnode);
				return -ENOENT;
			}

			/* Szukamy w cache */
			new_vnode = vnode_lookup(vnode->mp, ino);
			/* Jesli nie istnieje to twrzymy nowy wpis */
			if(!new_vnode)
			{
				new_vnode = vnode_new(vnode->mp, ino);
				if (!new_vnode)
				{
					str_unexplode(table);
					*result = NULL;
					VNODE_REL(vnode);
					return -EIO;
				}
			}

			/* Sprawdzamy czy katalog jest punktem montowania */
			if (new_vnode->vfsmountedhere)
			{
				VNODE_HOLD(new_vnode->vfsmountedhere->root_vnode);
				new_vnode = new_vnode->vfsmountedhere->root_vnode;
				VNODE_REL(new_vnode->mp->parent_vnode);
			}

			/* Sprawdzamy czy plik nie jest symlinkiem */
			if ((S_ISLNK(new_vnode->mode)) && (followlink))
			{
				if (!new_vnode->ops->readlink)
				{
					VNODE_REL(vnode);
					VNODE_REL(new_vnode);
					str_unexplode(table);
					*result = NULL;
					return -ENOTSUP;
				}

				/* Odczytujemy dowiazanie */
				path = kalloc(new_vnode->size + 1);
				err = new_vnode->ops->readlink(new_vnode, path, new_vnode->size + 1);
				if (err != 0)
				{
					VNODE_REL(vnode);
					VNODE_REL(new_vnode);
					str_unexplode(table);
					kfree(path);
					*result = NULL;
					return err;
				}

				VNODE_REL(new_vnode);

				if (path[0] == '/')
				{
					VNODE_REL(vnode);
					vnode = proc->vnode_root;
					VNODE_HOLD(vnode);
				}

				break;
			}

			/* Ostatni element ścieżki? */
			if (table[j + 1] == NULL)
			{
				*result = new_vnode;
				str_unexplode(table);
				VNODE_REL(vnode);
				return 0;
			}

			/* Sprawdzamy czy jest to katalog */
			if (!S_ISDIR(vnode->mode))
			{
				VNODE_REL(vnode);
				VNODE_REL(new_vnode);
				str_unexplode(table);
				*result = NULL;
				return -ENOTDIR;
			}

			/* Sprawdzamy prawa dostepu */
			if (do_sys_access(new_vnode, 1, proc) != 0)
			{
				VNODE_REL(vnode);
				VNODE_REL(new_vnode);
				str_unexplode(table);
				*result = NULL;
				return -EACCES;
			}

			VNODE_REL(vnode);
			vnode = new_vnode;
			j++;
		}

		str_unexplode(table);
		i--;
	}

	VNODE_REL(vnode);
	return -ELOOP;
}

/* Funkcje systemowe */
int sys_access(char * path, int mode, struct proc * proc)
{
	struct vnode * vnode;
	int err;

	err = vnode_get(path, &vnode, proc, 1);
	if (err != 0)
		return err;

	err = do_sys_access(vnode, mode, proc);
	VNODE_REL(vnode);
	return err;
}

int sys_chdir(char * path, struct proc * proc)
{
	struct vnode * vnode;
	int err;

	err = vnode_get(path, &vnode, proc, 1);
	if (err != 0)
		return err;

	/* Sprawdzamy czy cel jest katalogiem */
	if (!S_ISDIR(vnode->mode))
	{
		VNODE_REL(vnode);
		return -ENOTDIR;
	}

	/* Sprawdzamy prawa dostepu */
	err = do_sys_access(vnode, 1, proc);
	if (err != 0)
	{
		VNODE_REL(vnode);
		return err;
	}

	if (proc->vnode_current)
		VNODE_REL(proc->vnode_current);
	proc->vnode_current = vnode;
	return 0;
}

int sys_chroot(char * path, struct proc * proc)
{
	struct vnode * vnode;
	int err;

	if (proc->euid)
		return -EPERM;

	err = vnode_get(path, &vnode, proc, 1);
	if (err != 0)
		return err;

	/* Sprawdzamy czy podana ścieżka jest katalogiem */
	if (!S_ISDIR(vnode->mode))
	{
		VNODE_REL(vnode);
		return -ENOTDIR;
	}

	/* Only root can to chroot, so skip access checking */
	if (proc->vnode_root)
		VNODE_REL(proc->vnode_root);
	proc->vnode_root = vnode;
	return 0;
}

int sys_open(char * path, int flags, mode_t mode, struct proc * proc)
{
	struct vnode * vnode;
	struct file * file;
	struct filedes * filedes;
	struct vfs_path target;
	int err, i;

	err = vnode_get(path, &vnode, proc, 1);
	if ((err == -ENOENT) && (flags & _FCREAT))
	{
		/* Tworzymy plik */
		vfs_path_split(path, &target);
		err = vnode_get(target.path, &vnode, proc, 1);
		if (err != 0)
			return err;

		mode &= 0777;
		mode &= ~proc->umask;

		err = do_sys_access(vnode, 3, proc);
		if (err != 0)
		{
			VNODE_REL(vnode);
			return err;
		}

		/* Sprawdzamy czy system plikow nie jest tylko do odczytu */
		if (vnode->mp->flags & MS_RDONLY)
		{
			VNODE_REL(vnode);
			return -EROFS;
		}

		if (vnode->ops->creat)
			err = vnode->ops->creat(vnode, target.name, mode, proc->uid, proc->gid);
		else
			err = -ENOTSUP;

		VNODE_REL(vnode);
		if (err != 0)
			return err;

		err = vnode_get(path, &vnode, proc, 1);
		if (err != 0)
			return err;
	}
	else if ((!err) && (flags & _FEXCL))
	{
		VNODE_REL(vnode);
		return -EEXIST;
	}
	else if (err != 0)
		return err;

	/* Sprawdzamy prawa dostepu */
	err = 0;
	if ((flags + 1) & _FREAD) err |= 4;
	if ((flags + 1) & _FWRITE) err |= 2;
	err = do_sys_access(vnode, err, proc);
	if (err != 0)
	{
		VNODE_REL(vnode);
		return err;
	}

	/* Tworzymy strukture pliku */
	file = kalloc(sizeof(struct file));
	mutex_init(&file->mutex);
	atomic_set(&file->refs, 1);
	file->vnode = vnode;
	file->mode = flags;
	file->pos = 0;


	filedes = kalloc(sizeof(struct filedes));
	filedes->flags = 0;
	filedes->file = file;

	err = -EMFILE;
	/* Add to process file list */
	for(i=0;i<OPEN_MAX;i++)
	{
		if (!proc->filedes[i])
		{
			err = 0;
			if (S_ISCHR(vnode->mode))
				err = cdev_open(vnode->rdev);
			else if (S_ISBLK(vnode->mode))
				err = bdev_open(vnode->rdev);
			if (err < 0)
				break;
			proc->filedes[i] = filedes;
			err = i;
			break;
		}
	}

	if (err < 0)
	{
		kfree(file);
		kfree(filedes);
		VNODE_REL(vnode);
	}
	else if (flags & _FTRUNC)
	{
		sys_ftruncate(err, 0, proc);
	}

	return err;
}

int sys_close(int fd, struct proc * proc)
{
	struct filedes * filedes;
	struct vnode * vnode;

	if ((fd < 0) || (fd >= OPEN_MAX) || (!proc->filedes[fd]))
		return -EBADF;

	filedes = proc->filedes[fd];
	proc->filedes[fd] = NULL;

	atomic_dec(&filedes->file->refs);
	if (!atomic_get(&filedes->file->refs))
	{
		if (S_ISCHR(filedes->file->vnode->mode))
			cdev_close(filedes->file->vnode->rdev);
		else if (S_ISBLK(filedes->file->vnode->mode))
			bdev_close(filedes->file->vnode->rdev);

		vnode = filedes->file->vnode;
		kfree(filedes->file);
		VNODE_REL(vnode);
		vnode_free(vnode);
	}

	kfree(filedes);
	return 0;
}

int sys_fstat(int fd, struct stat * stat, struct proc * proc)
{
	/* Check file descriptor */
	if ((fd < 0) || (fd >= OPEN_MAX) || (!proc->filedes[fd]))
		return -EBADF;
	do_sys_stat(proc->filedes[fd]->file->vnode, stat);
	return 0;
}

int sys_stat(char * path, struct stat * stat, struct proc * proc)
{
	struct vnode * vnode;
	int err;

	err = vnode_get(path, &vnode, proc, 1);
	if (err != 0)
		return err;

	do_sys_stat(vnode, stat);

	VNODE_REL(vnode);
	return 0;
}

int sys_lstat(char * path, struct stat * stat, struct proc * proc)
{
	struct vnode * vnode;
	int err;

	err = vnode_get(path, &vnode, proc, 0);
	if (err != 0)
		return err;

	do_sys_stat(vnode, stat);
	VNODE_REL(vnode);
	return 0;
}


ssize_t sys_read(int fd, void * buf, size_t len, struct proc * proc)
{
	ssize_t err;
	struct file * file;

	/* Check file descriptor */
	if ((fd < 0) || (fd >= OPEN_MAX) || (!proc->filedes[fd]))
		return -EBADF;

	file = proc->filedes[fd]->file;

	/* Check file mode */
	if (((file->mode + 1) & _FREAD) != _FREAD)
		return -EACCES;

	if (S_ISCHR(file->vnode->mode))
		err = cdev_read(file->vnode->rdev, buf, len);
	else if (file->vnode->ops->read)
		err = file->vnode->ops->read(file->vnode, buf, len, file->pos);
	else
		err = -ENOTSUP;

	if (err > 0)
	{
		mutex_lock(&file->mutex);
		file->pos += err;
		mutex_unlock(&file->mutex);
	}

	return err;
}

ssize_t sys_write(int fd, void * buf, size_t len, struct proc * proc)
{
	ssize_t err;
	struct file * file;

	/* Check file descriptor */
	if ((fd < 0) || (fd >= OPEN_MAX) || (!proc->filedes[fd]))
		return -EBADF;

	file = proc->filedes[fd]->file;

	/* Check file mode */
	if (((file->mode + 1) & _FWRITE) != _FWRITE)
		return -EACCES;

	/* If append mode seek to end file */
	if (file->mode & _FAPPEND)
		sys_lseek(fd, 0, SEEK_END, proc);

	if (S_ISCHR(file->vnode->mode))
		err = cdev_write(file->vnode->rdev, buf, len);
	else if (file->vnode->ops->write)
	{
		/* Sprawdzamy czy system plikow nie jest tylko do odczytu */
		if (file->vnode->mp->flags & MS_RDONLY)
			err = -EROFS;
		else
			err = file->vnode->ops->write(file->vnode, buf, len, file->pos);
	}
	else
		err = -ENOTSUP;

	if (err > 0)
	{
		mutex_lock(&file->mutex);
		file->pos += err;
		mutex_unlock(&file->mutex);
	}

	return err;
}

loff_t sys_lseek(int fd, loff_t off, int whence, struct proc * proc)
{
	/* Check file descriptor */
	if ((fd < 0) || (fd >= OPEN_MAX) || (!proc->filedes[fd]))
		return -EBADF;

	mutex_lock(&proc->filedes[fd]->file->mutex);
	switch(whence)
	{
		case SEEK_SET:
		{
			if (off >= 0)
				proc->filedes[fd]->file->pos = off;
			break;
		}
		case SEEK_CUR:
		{
			if (off + proc->filedes[fd]->file->pos >= 0)
				proc->filedes[fd]->file->pos += off;
			break;
		}
		case SEEK_END:
		{
			if (off + proc->filedes[fd]->file->vnode->size >= 0)
				proc->filedes[fd]->file->pos = off + proc->filedes[fd]->file->vnode->size;
			break;
		}
		default:
		{
			mutex_unlock(&proc->filedes[fd]->file->mutex);
			return -EFAULT;
		}
	}
	mutex_unlock(&proc->filedes[fd]->file->mutex);
	return proc->filedes[fd]->file->pos;
}

static int do_sys_dup(int fd, int start, struct proc * proc)
{
	int i;

	/* Check file descriptor */
	if ((fd < 0) || (fd >= OPEN_MAX) || (!proc->filedes[fd]))
		return -EBADF;

	for(i = start; i < OPEN_MAX; i++)
	{
		if (!proc->filedes[i])
		{
			atomic_inc(&proc->filedes[fd]->file->refs);
			proc->filedes[i] = kalloc(sizeof(struct filedes));
			memset(proc->filedes[i], 0, sizeof(struct filedes));
			proc->filedes[i]->file = proc->filedes[fd]->file;
			return i;
		}
	}
	return -EMFILE;
}

int sys_dup(int fd, struct proc * proc)
{
	return do_sys_dup(fd, 0, proc);
}

int sys_dup2(int oldfd, int newfd, struct proc * proc)
{
	/* Check file descriptor */
	if ((oldfd < 0) || (oldfd >= OPEN_MAX) || (!proc->filedes[oldfd]))
		return -EBADF;

	if ((newfd < 0) || (newfd >= OPEN_MAX))
		return -EBADF;

	if (proc->filedes[newfd])
		sys_close(newfd, proc);

	atomic_inc(&proc->filedes[oldfd]->file->refs);
	proc->filedes[newfd] = kalloc(sizeof(struct filedes));
	memset(proc->filedes[newfd], 0, sizeof(struct filedes));
	proc->filedes[newfd]->file = proc->filedes[oldfd]->file;
	return newfd;
}

int sys_getdents(int fd, struct dirent * buf, unsigned count, struct proc * proc)
{
	int err;
	struct file * file;

	/* Check file descriptor */
	if ((fd < 0) || (fd >= OPEN_MAX) || (!proc->filedes[fd]))
		return -EBADF;

	file = proc->filedes[fd]->file;

	if (!S_ISDIR(file->vnode->mode))
		return -ENOTDIR;

	if (file->vnode->ops->getdents)
		err = file->vnode->ops->getdents(file->vnode, buf, count, &file->pos);
	else
		err = -ENOTSUP;

	return err;
}

int sys_ioctl(int fd, int cmd, void * arg, struct proc * proc)
{
	int err;
	struct file * file;

	/* Check file descriptor */
	if ((fd < 0) || (fd >= OPEN_MAX) || (!proc->filedes[fd]))
		return -EBADF;
	file = proc->filedes[fd]->file;

	if (S_ISCHR(file->vnode->mode))
		err = cdev_ioctl(file->vnode->rdev, cmd, arg);
	else
		err = -ENOTSUP;

	return err;
}

int sys_mount(char * src, char * dest, char * fstype, int flags, void * data, struct proc * proc)
{
	struct filesystem * fs;
	struct vnode * vnode;
	struct mountpoint * mp;
	dev_t devid = 0;
	int err;

	/* Tylko root może montować cokolwiek */
	if ((proc->euid != 0) && (proc->uid != 0))
	{
		return -EPERM;
	}

	/* TODO: Remount */

	/* Szukamy wybranego systemu plikow */
	fs = fs_lookup(fstype);
	if (!fs)
		return -ENOTSUP;

	/* Jeżeli system plikow wymaga urządzenia to go szukamy */
	if ((fs->flags & FS_NODEV) != FS_NODEV)
	{
		TODO("get device id");
		return -ENOSYS;
	}

	/* Sprawdzamy czy punkt montowania istnieje i jest katalogiem */
	err = vnode_get(dest, &vnode, proc, 1);
	if (err != 0)
	{
		//fs_release()
		return -ENOENT;
	}

	if (!S_ISDIR(vnode->mode))
	{
		//fs_release()
		VNODE_REL(vnode);
		return -ENOTDIR;
	}

	/* Sprawdzamy czy nie jest już cos zamontowane w tym miejscu */
	if (vnode == vnode->mp->root_vnode)
	{
		VNODE_REL(vnode);
		return -EBUSY;
	}

	/* Tworzymy punkt montowania */
	mp = kalloc(sizeof(struct mountpoint));
	memset(mp, 0, sizeof(struct mountpoint));
	list_init(&mp->list);
	mp->device = devid;
	mp->devname = src;
	mp->fs = fs;
	mp->flags = flags;

	list_init(&mp->vnode_list);
	mutex_init(&mp->mutex);

	/* Wywołujemy funkcję montującą ze sterownika */
	err = mp->fs->fs_ops->mount(mp, NULL);
	if (err != 0)
	{
		//fs_release()
		VNODE_REL(vnode);
		kfree(mp);
		return err;
	}

	mp->parent_vnode = vnode;
	mp->root_vnode = vnode_new(mp, mp->fs->fs_ops->root(mp));
	if (!mp->root_vnode)
	{
		mp->fs->fs_ops->umount(mp);
		//fs_release()
		VNODE_REL(vnode);
		kfree(mp);
		return err;
	}

	vnode->vfsmountedhere = mp;

	/* Dodajemy do listy montowan */
	spinlock_lock(&_mount_list_lock);
	list_add(&_mount_list, &mp->list);
	spinlock_unlock(&_mount_list_lock);

	return 0;
}

int sys_umount(char * dest, int flags, struct proc * proc)
{

	/* Tylko root może montować cokolwiek */
	if ((proc->euid != 0) && (proc->uid != 0))
	{
		return -EPERM;
	}

	return -ENOSYS;
}

int sys_mknod(char * path, mode_t mode, dev_t dev, struct proc * proc)
{
	struct vfs_path target;
	struct vnode * vnode;
	int err;

	/* Tylko root moze tworzyc pliku urzadzen */
	if (((S_ISBLK(mode)) || (S_ISCHR(mode))) && (proc->euid != 0) && (proc->uid != 0))
		return -EPERM;

	/* Sprawdzamy czy plik nie istnieje */
	err = vnode_get(path, &vnode, proc, 1);
	if (!err)
	{
		VNODE_REL(vnode);
		return -EEXIST;
	}

	vfs_path_split(path, &target);
	err = vnode_get(target.path, &vnode, proc, 1);
	if (err != 0)
		return err;

	mode &= ~proc->umask;
	err = do_sys_access(vnode, 3, proc);
	if (err != 0)
	{
		VNODE_REL(vnode);
		return err;
	}

	/* Sprawdzamy czy system plikow nie jest tylko do odczytu */
	if (vnode->mp->flags & MS_RDONLY)
	{
		VNODE_REL(vnode);
		return -EROFS;
	}

	if (vnode->ops->mknod)
		err = vnode->ops->mknod(vnode, target.name, mode, dev, proc->uid, proc->gid);
	else
		err = -ENOTSUP;

	VNODE_REL(vnode);
	if (err != 0)
		return err;

	return 0;
}

int sys_chmod(char * path, mode_t mode, struct proc * proc)
{
	struct vnode * vnode;
	int err;

	mode &= 0777;
	err = vnode_get(path, &vnode, proc, 1);
	if (err != 0)
		return err;

	/* Sprawdzamy czy system plikow nie jest tylko do odczytu */
	if (vnode->mp->flags & MS_RDONLY)
	{
		VNODE_REL(vnode);
		return -EROFS;
	}

	/* Tylko wlasciciel lub root moze zmienic prawa pliku */
	if ((vnode->uid != proc->uid) && (vnode->uid != proc->euid) && (proc->uid != 0) && (proc->euid != 0))
	{
		VNODE_REL(vnode);
		return -EACCES;
	}

	vnode->mode = (vnode->mode & S_IFMT) | mode;
	if (vnode->ops->sync)
		vnode->ops->sync(vnode);

	VNODE_REL(vnode);
	return 0;
}

int sys_mkdir(char * path, mode_t mode, struct proc * proc)
{
	struct vfs_path target;
	struct vnode * vnode;
	int err;

	/* Sprawdzamy czy plik nie istnieje */
	err = vnode_get(path, &vnode, proc, 1);
	if (!err)
	{
		VNODE_REL(vnode);
		return -EEXIST;
	}

	vfs_path_split(path, &target);
	err = vnode_get(target.path, &vnode, proc, 1);
	if (err != 0)
		return err;

	mode &= ~proc->umask;

	err = do_sys_access(vnode, 3, proc);
	if (err != 0)
	{
		VNODE_REL(vnode);
		return err;
	}

	/* Sprawdzamy czy system plikow nie jest tylko do odczytu */
	if (vnode->mp->flags & MS_RDONLY)
	{
		VNODE_REL(vnode);
		return -EROFS;
	}

	if (vnode->ops->mkdir)
		err = vnode->ops->mkdir(vnode, target.name, mode, proc->uid, proc->gid);
	else
		err = -ENOTSUP;

	VNODE_REL(vnode);
	if (err != 0)
		return err;

	return 0;
}

int sys_unlink(char * path, struct proc * proc)
{
	struct vnode * vnode;
	struct vnode * parent_vnode;
	struct vfs_path target;
	int err;

	err = vnode_get(path, &vnode, proc, 1);
	if (err != 0)
		return err;

	/* Sprawdzamy czy system plikow nie jest tylko do odczytu */
	if (vnode->mp->flags & MS_RDONLY)
	{
		VNODE_REL(vnode);
		return -EROFS;
	}

	/* Sprawdzamy czy obiekt nie jest katalogiem */
	if (S_ISDIR(vnode->mode))
	{
		VNODE_REL(vnode);
		return -EISDIR;
	}

	vfs_path_split(path, &target);
	err = vnode_get(target.path, &parent_vnode, proc, 1);
	if (err != 0)
	{
		VNODE_REL(vnode);
		return err;
	}

	/* Sprawdz czy mozemy zapisywac do katalogu nadrzednego */
	err = do_sys_access(parent_vnode, 3, proc);
	if (err != 0)
	{
		VNODE_REL(vnode);
		VNODE_REL(parent_vnode);
		return err;
	}

	/* Wykonujemy unlink */
	if (parent_vnode->ops->unlink)
		err = parent_vnode->ops->unlink(parent_vnode, target.name);
	else
		err = -ENOTSUP;

	if (!err)
	{
		vnode->nlink--;
		VNODE_REL(vnode);
		/* Jeżeli można zwolnoć v-node to go zwalniamy (i tak zostanie zapisany w funkcji close),
		   w innym wypadku zapisujemy tylko zmienione informacje o v-vnodzie na dysk */
		if ((!vnode->nlink) && (!atomic_get(&vnode->refcount)))
			vnode_free(vnode);
		else if (vnode->ops->sync)
			vnode->ops->sync(vnode);
	}
	else
		VNODE_REL(vnode);


	VNODE_REL(parent_vnode);

	return err;
}

int sys_link(char * oldpath, char * newpath, struct proc * proc)
{
	return -ENOSYS;
}

int sys_truncate(const char *path, loff_t length, struct proc * proc)
{
	return -ENOSYS;
}

int sys_ftruncate(int fd, loff_t length, struct proc * proc)
{
	struct file * file;

	TODO("truncate file");
	return -ENOSYS;

	/* Check file descriptor */
	if ((fd < 0) || (fd >= OPEN_MAX) || (!proc->filedes[fd]))
		return -EBADF;

	file = proc->filedes[fd]->file;

	if (((file->mode + 1) & _FWRITE) != _FWRITE)
		return -EACCES;

	/* Sprawdzamy czy system plikow nie jest tylko do odczytu */
	if (file->vnode->mp->flags & MS_RDONLY)
		return -EROFS;

	file->vnode->size = length;
	if (file->vnode->ops->sync)
		file->vnode->ops->sync(file->vnode);

	return 0;
}


#if 0
int sys_readlink(char * path, char * buf, size_t buflen, struct proc * proc)
{
 int err;
 struct vnode * vnode;

 err = vnode_get(path, &vnode, proc, 0);
 if (err != 0)
    return err;

 if (!S_ISLNK(vnode->mode))
    err = -EINVAL;
 else if (vnode->ops->readlink)
    err = vnode->ops->readlink(vnode, buf, buflen);
 else
    err = -ENOTSUP;

 VNODE_REL(vnode);
 return err;
}
#endif

int sys_fcntl(int fd, int cmd, void * arg, struct proc * proc)
{
	/* Check file descriptor */
	if ((fd < 0) || (fd >= OPEN_MAX) || (!proc->filedes[fd]))
		return -EBADF;

	switch(cmd)
	{
		case F_DUPFD: return do_sys_dup(fd, (int)arg, proc);
		case F_GETFD: return proc->filedes[fd]->flags;
		case F_SETFD: proc->filedes[fd]->flags |= FD_CLOEXEC; return 0;
		case F_GETFL: return proc->filedes[fd]->file->flags;
	}

	return -ENOSYS;
}


int sys_pipe(int fds[2], struct proc * proc)
{
	struct filedes * fdw;
	struct filedes * fdr;
	int i;
	struct vnode * vnode;
	extern struct mountpoint __pipefs_mount;

	vnode = vnode_new(&__pipefs_mount, 0);
	if (!vnode)
		return -ENOMEM;

	/* Tworzymy pliki i deskryptory plikow */
	fdr = kalloc(sizeof(struct filedes));
	fdr->flags = 0;
	fdr->file = kalloc(sizeof(struct file));
	mutex_init(&fdr->file->mutex);
	atomic_set(&fdr->file->refs, 1);
	VNODE_HOLD(vnode);
	fdr->file->vnode = vnode;
 	fdr->file->flags = 0;
	fdr->file->mode = O_RDONLY;
	fdr->file->pos = 0;

	fdw = kalloc(sizeof(struct filedes));
	fdw->flags = 0;
	fdw->file = kalloc(sizeof(struct file));
	mutex_init(&fdr->file->mutex);
	atomic_set(&fdr->file->refs, 1);
	VNODE_HOLD(vnode);
	fdw->file->vnode = vnode;
 	fdw->file->flags = 0;
	fdw->file->mode = O_WRONLY;
	fdw->file->pos = 0;

	/* Dodajemy do listy otwartych plików */
	for(i = 0; i < OPEN_MAX; i++)
	{
		if (!proc->filedes[i])
		{
			proc->filedes[i] = fdr;
			fds[0] = i;
			break;
		}
	}

	if (i == OPEN_MAX)
		goto err;

	for( ; i < OPEN_MAX; i++)
	{
		if (!proc->filedes[i])
		{
			proc->filedes[i] = fdw;
			fds[1] = i;
			break;
		}
	}

	if (i != OPEN_MAX)
		return 0;

	proc->filedes[fds[0]] = NULL;

err:
	/* Zwalniamy pamiec */
	VNODE_REL(vnode);
	VNODE_REL(vnode);
	kfree(fdr->file);
	kfree(fdw->file);
	kfree(fdr);
	kfree(fdw);
	vnode_free(vnode);
	return -EMFILE;
}

void vfs_free(struct proc * proc)
{
	int i;
	for(i=0;i<OPEN_MAX;i++)
	{
		if (proc->filedes[i])
			sys_close(i, proc);
	}

	VNODE_REL(proc->vnode_root);
	VNODE_REL(proc->vnode_current);
}

int vfs_clone(struct proc * child, struct proc * parent)
{
	int i;

	VNODE_HOLD(parent->vnode_root);
	child->vnode_root = parent->vnode_root;
	VNODE_HOLD(parent->vnode_current);
	child->vnode_current = parent->vnode_current;

	for(i=0;i<OPEN_MAX;i++)
	{
		if (parent->filedes[i])
		{
			atomic_inc(&parent->filedes[i]->file->refs);
			child->filedes[i] = kalloc(sizeof(struct filedes));
			child->filedes[i]->file = parent->filedes[i]->file;
			child->filedes[i]->flags = parent->filedes[i]->flags;
		}
	}

	return 0;
}

void vfs_setup_console(void)
{
	dev_t devid;
	int err;
	struct vnode * vnode;

	char * tty = kargv_lookup("console=");

	if (tty)
		tty += strlen("console=");
	else
		tty = "tty1";

	/* Open terminal as stdin, out and err for init process */
	devid = device_lookup(DEV_CHAR, tty);
	if (!devid)
		panic("unable to open init console (%s)", tty);

	err = vnode_get("/dev/console", &vnode, SCHED->current->proc, 1);
	if (err != 0)
		panic("unable to open init console (%s)", tty);

	vnode->rdev = devid;

	err = sys_open("/dev/console", O_RDWR, 0, SCHED->current->proc);
	if (err != 0)
		panic("unable to open init console (/dev/console)");
	VNODE_REL(vnode);

	sys_dup(0, SCHED->current->proc);
	sys_dup(0, SCHED->current->proc);
}

void vfs_mount_root(void)
{
	char * rootfs;
	char * root;
	dev_t rootdev = 0;
	struct filesystem * fs = NULL;
	struct mountpoint * mp;
	struct vnode * vnode;
	int err;
	char * rw;

	rw = kargv_lookup("rw");

	kprintf("vfs: mounting root file system %s\n", (!rw) ? "read-only" : "");

	rootfs = kargv_lookup("rootfs=");
	root = kargv_lookup("root=");


	if (rootfs)
	{
		rootfs += strlen("rootfs=");
		fs = fs_lookup(rootfs);
		if (!fs)
			panic("root file system not found");
	}

	if ((!fs) || ((fs) && ((fs->flags & FS_NODEV) != FS_NODEV)))
	{
		if (!root)
			panic("root device not definied (add root= to kernel command line?)");
		root += strlen("root=");
		rootdev = device_lookup(DEV_BLOCK, root);
		if (!rootdev)
			panic("root device not exist");
	}

	if (!fs)
	{
		panic("root file system detection not implemented yet");
	}

	mp = kalloc(sizeof(struct mountpoint));
	memset(mp, 0, sizeof(struct mountpoint));
	list_init(&mp->list);
	mp->device = rootdev;
	mp->devname = "/dev/root";
	mp->fs = fs;
	if (!rw)
		mp->flags = MS_RDONLY;

	list_init(&mp->vnode_list);
	mutex_init(&mp->mutex);

	/* Wywołujemy funkcję montującą ze sterownika */
	err = mp->fs->fs_ops->mount(mp, NULL);
	if (err != 0)
		panic("mounting root file system failed (err=%d)", err);

	mp->root_vnode = vnode_new(mp, mp->fs->fs_ops->root(mp));
	if (!mp->root_vnode)
	{
		mp->fs->fs_ops->umount(mp);
		panic("mounting root file system failed (can not open root vnode)", err);
	}

	/* Dodajemy do listy montowan */
	spinlock_lock(&_mount_list_lock);
	list_add(&_mount_list, &mp->list);
	spinlock_unlock(&_mount_list_lock);

	VNODE_HOLD(mp->root_vnode);
	SCHED->current->proc->vnode_root = mp->root_vnode;
	VNODE_HOLD(mp->root_vnode);
	SCHED->current->proc->vnode_current = mp->root_vnode;

	err = vnode_get("/dev/root", &vnode, SCHED->current->proc, 1);
	if (err != 0)
	{
		kprintf(KERN_WARN"warning: unable to open /dev/root\n");
		return;
	}

	vnode->rdev = rootdev;
	VNODE_REL(vnode);
}
