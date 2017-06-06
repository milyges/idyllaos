/*
 * Idylla Operating System
 * Copyright (C) 2009-2010  Idylla Operating System Team
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
#ifndef __KERNEL_VFS_H
#define __KERNEL_VFS_H

/* mode_t flags */
#define S_IFMT             0170000 /* bitmask for the file type bitfields */
#define S_IFSOCK           0140000 /* socket */
#define S_IFLNK            0120000 /* symbolic link */
#define S_IFREG            0100000 /* regular file */
#define S_IFBLK            0060000 /* block device */
#define S_IFDIR            0040000 /* directory */
#define S_IFCHR            0020000 /* character device */
#define S_IFIFO            0010000 /* FIFO */
#define S_ISUID            0004000 /* set UID bit */
#define S_ISGID            0002000 /* set-group-ID bit */
#define S_ISVTX            0001000 /* sticky bit (see below) */
#define S_IRWXU            00700   /* mask for file owner permissions */
#define S_IRUSR            00400   /* owner has read permission */
#define S_IWUSR            00200   /* owner has write permission */
#define S_IXUSR            00100   /* owner has execute permission */
#define S_IRWXG            00070   /* mask for group permissions */
#define S_IRGRP            00040   /* group has read permission */
#define S_IWGRP            00020   /* group has write permission */
#define S_IXGRP            00010   /* group has execute permission */
#define S_IRWXO            00007   /* mask for permissions for others (not in group) */
#define S_IROTH            00004   /* others have read permission */
#define S_IWOTH            00002   /* others have write permission */
#define S_IXOTH            00001   /* others have execute permission */

/* Mode macros */
#define S_ISREG(m)         (((m) & S_IFMT) == S_IFREG)
#define S_ISDIR(m)         (((m) & S_IFMT) == S_IFDIR)
#define S_ISCHR(m)         (((m) & S_IFMT) == S_IFCHR)
#define S_ISBLK(m)         (((m) & S_IFMT) == S_IFBLK)
#define S_ISLNK(m)         (((m) & S_IFMT) == S_IFLNK)
#define S_ISFIFO(m)        (((m) & S_IFMT) == S_IFIFO)
#define S_ISSOCK(m)        (((m) & S_IFMT) == S_IFSOCK)

/* File flags */
#define _FREAD          0x0001       /* read enabled */
#define _FWRITE         0x0002       /* write enabled */
#define _FAPPEND        0x0008       /* append (writes guaranteed at the end) */
#define _FMARK          0x0010       /* internal; mark during gc() */
#define _FDEFER         0x0020       /* internal; defer for next gc pass */
#define _FASYNC         0x0040       /* signal pgrp when data ready */
#define _FSHLOCK        0x0080       /* BSD flock() shared lock present */
#define _FEXLOCK        0x0100       /* BSD flock() exclusive lock present */
#define _FCREAT         0x0200       /* open with file create */
#define _FTRUNC         0x0400       /* open with truncation */
#define _FEXCL          0x0800       /* error on open if file exists */
#define _FNBIO          0x1000       /* non blocking I/O (sys5 style) */
#define _FSYNC          0x2000       /* do all writes synchronously */
#define _FNONBLOCK      0x4000       /* non blocking I/O (POSIX style) */
#define _FNDELAY        _FNONBLOCK   /* non blocking I/O (4.2 style) */
#define _FNOCTTY        0x8000       /* don't assign a ctty on this open */

#define O_ACCMODE       (O_RDONLY | O_WRONLY | O_RDWR)

/* fnctl() commands */
#define F_DUPFD         0x00
#define F_GETFD         0x01
#define F_SETFD         0x02
#define F_GETFL         0x03
#define F_SETFL         0x04
#define F_GETLK         0x07
#define F_SETLK         0x08
#define F_SETLKW        0x09

/* File descriptor flags */
#define FD_CLOEXEC      1

/*
 * Flag values for open(2) and fcntl(2)
 * The kernel adds 1 to the open modes to turn it into some
 * combination of FREAD and FWRITE.
 */
#define O_RDONLY        0        /* +1 == FREAD */
#define O_WRONLY        1        /* +1 == FWRITE */
#define O_RDWR          2        /* +1 == FREAD|FWRITE */
#define O_APPEND        _FAPPEND
#define O_CREAT         _FCREAT
#define O_TRUNC         _FTRUNC
#define O_EXCL          _FEXCL
#define O_SYNC          _FSYNC
#define O_NONBLOCK      _FNONBLOCK

/* lseek() modes */
#define SEEK_SET        0
#define SEEK_CUR        1
#define SEEK_END        2

/* Mount flags */
#define MS_RDONLY          0x01 /* Mount read only */
#define MS_NOATIME         0x02 /* Dont update access time */
#define MS_NODEV           0x04 /* Ignore deivce fles */
#define MS_NOEXEC          0x08 /* Do not allow programs to be executed from this file system */
#define MS_NOSUID          0x10 /* Ignore Set UID and Set GID bits */
#define MS_REMOUNT         0x20 /* Remount existing fs */

/* File system flags */
#define FS_RDONLY          0x01 /* Read-only file system */
#define FS_NODEV           0x02 /* Don't use block device */

#define VFS_FS_MAX         128
#define PATH_MAX           1024
#define NAME_MAX           255
#define SYMLOOP_MAX        32
#define PIPE_SIZE          0x1000

#define VNODE_HOLD(x)      atomic_inc(&((struct vnode *)(x))->refcount)
#define VNODE_REL(x)       atomic_dec(&((struct vnode *)(x))->refcount)

#include <arch/atomic.h>
#include <kernel/types.h>
#include <kernel/mutex.h>
#include <kernel/proc.h>
#include <lib/list.h>

/* Ścieżka podzielona na katalog i nazwę */
struct vfs_path
{
	char path[PATH_MAX];
	char name[NAME_MAX + 1];
};

/* Wpis w katalogu */
struct dirent
{
	ino_t d_ino;
	uint32_t d_off;
	uint16_t d_reclen;
	char d_name[NAME_MAX + 1];
};

/* Punkt montowania */
struct mountpoint
{
	list_t list; /* Lista montowan */

	dev_t device;
	char * devname;
	struct filesystem * fs;
	unsigned flags;
	void * data;

	struct vnode * root_vnode;
	struct vnode * parent_vnode;
	list_t vnode_list;

	struct mutex mutex;
};

/* Virtual-Node */
struct vnode
{
	list_t list; /* vnodes list */

	ino_t ino;
	mode_t mode;
	nlink_t nlink;
	loff_t size;
	uid_t uid;
	gid_t gid;
	dev_t dev;

	blkcnt_t blocks;
	blksize_t block_size;

	dev_t rdev;

	struct mountpoint * mp;
	struct mountpoint * vfsmountedhere;

	void * data;

	struct vnode_ops * ops;

	atomic_t refcount;
	struct mutex mutex;

	list_t pagecache; /* Używane przez mmap do trzymania listy stron */
};

struct file
{
	struct mutex mutex;
	atomic_t refs;

	int flags;
	int mode;
	loff_t pos;
	
	mode_t type;
	struct vnode * vnode;
	void * dataptr;
};

struct vnode_ops
{
	int (*open)(struct vnode * vnode);
	int (*close)(struct vnode * vnode);
	ino_t (*lookup)(struct vnode * vnode, char * name);
	ssize_t (*read)(struct vnode * vnode, void * buf, size_t len, loff_t offset);
	ssize_t (*write)(struct vnode * vnode, void * buf, size_t len, loff_t offset);
	int (*mkdir)(struct vnode * vnode, char * name, mode_t mode, uid_t uid, gid_t gid);
	int (*creat)(struct vnode * vnode, char * name, mode_t mode, uid_t uid, gid_t gid);
	int (*symlink)(struct vnode * vnode, char * name, char * path);
	int (*readlink)(struct vnode * vnode, char * buf, size_t len);
	int (*getdents)(struct vnode * vnode, struct dirent * dirp, size_t len, loff_t * offset);
	int (*flush)(struct vnode * vnode);
	int (*mknod)(struct vnode * vnode, char * name, mode_t mode, dev_t rdev, uid_t uid, gid_t gid);
	int (*unlink)(struct vnode * vnode, char * name);
};

struct filesystem_ops
{
	int (*mount)(struct mountpoint * mp, char * flags);
	int (*umount)(struct mountpoint * mp);
	ino_t (*root)(struct mountpoint * mp);
};

struct filesystem
{
	list_t list;
	char * name;
	uint8_t fsid;
	unsigned flags;
	struct filesystem_ops * fs_ops;
	struct vnode_ops * vnode_ops;

	atomic_t refcount;
};

struct filedes
{
	int flags;
	struct file * file;
};

struct stat
{
	dev_t st_dev;
	ino_t st_ino;
	mode_t st_mode;
	nlink_t st_nlink;
	uid_t st_uid;
	gid_t st_gid;
	dev_t st_rdev;
	loff_t st_size;
	blksize_t st_blksize;
	blkcnt_t st_blocks;
	time_t st_atime;
	time_t st_mtime;
	time_t st_ctime;
};

int pipe_stat(struct file * file, struct stat * stat);
ssize_t pipe_read(struct file * file, void * buf, size_t bufsz);
ssize_t pipe_write(struct file * file, void * buf, size_t bufsz);
int pipe_free(struct file * file);

int fs_register(struct filesystem * fs);
int fs_unregister(struct filesystem * fs);
void vfs_mount_root(void);
void vfs_setup_console(void);
int vfs_clone(struct proc * child, struct proc * parent);
void vfs_free(struct proc * proc);

int sys_access(char * path, int mode, struct proc * proc);
int sys_chdir(char * path, struct proc * proc);
int sys_chroot(char * path, struct proc * proc);
int sys_open(char * path, int flags, mode_t mode, struct proc * proc);
int sys_close(int fd, struct proc * proc);
int sys_fstat(int fd, struct stat * stat, struct proc * proc);
int sys_stat(char * path, struct stat * stat, struct proc * proc);
int sys_lstat(char * path, struct stat * stat, struct proc * proc);
ssize_t sys_read(int fd, void * buf, size_t len, struct proc * proc);
ssize_t sys_write(int fd, void * buf, size_t len, struct proc * proc);
loff_t sys_lseek(int fd, loff_t off, int whence, struct proc * proc);
int sys_getdents(int fd, struct dirent * buf, unsigned count, struct proc * proc);
int sys_fcntl(int fd, int cmd, void * arg, struct proc * proc);
int sys_dup(int fd, struct proc * proc);
int sys_dup2(int oldfd, int newfd, struct proc * proc);
int sys_ioctl(int fd, int cmd, void * arg, struct proc * proc);
int sys_mount(char * src, char * dest, char * fstype, int flags, void * data, struct proc * proc);
int sys_umount(char * dest, int flags, struct proc * proc);
int sys_mknod(char * path, mode_t mode, dev_t dev, struct proc * proc);
int sys_chmod(char * path, mode_t mode, struct proc * proc);
int sys_mkdir(char * path, mode_t mode, struct proc * proc);
int sys_unlink(char * path, struct proc * proc);
int sys_link(char * oldpath, char * newpath, struct proc * proc);
int sys_truncate(const char *path, loff_t length, struct proc * proc);
int sys_ftruncate(int fd, loff_t length, struct proc * proc);
int sys_pipe(int fds[2], struct proc * proc);

#endif /* __KERNEL_VFS_H */
