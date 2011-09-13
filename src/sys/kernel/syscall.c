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
#include <arch/atomic.h>
#include <arch/cpu.h>
#include <arch/spinlock.h>
#include <arch/ctx.h>
#include <kernel/types.h>
#include <kernel/proc.h>
#include <kernel/kprintf.h>
#include <kernel/debug.h>
#include <kernel/vfs.h>
#include <mm/heap.h>
#include <mm/vmm.h>
#include <lib/string.h>
#include <lib/errno.h>

#define STUB()     kprintf("%s(): This function is a stub!\n", __FUNCTION__);

static int _sys_nosys(void)
{
	return -ENOSYS;
}

static int _sys_exit(int code)
{
	sys_exit((code & 0xFF) << 8, SCHED->current->proc);
	while(1);
	return 0;
}

static pid_t _sys_fork(void * stack)
{
	//kprintf("fork()\n");
	return sys_fork(stack);
}

static int _sys_read(int fd, void * buf, size_t count)
{
	if (!IS_IN_USERMEM(buf))
		return -EFAULT;

	return sys_read(fd, buf, count, SCHED->current->proc);
}

static int _sys_write(int fd, void * buf, size_t count)
{
	if (!IS_IN_USERMEM(buf))
		return -EFAULT;


	return sys_write(fd, buf, count, SCHED->current->proc);
}

static int _sys_open(char * path, int flags, mode_t mode)
{
	if (!IS_IN_USERMEM(path))
		return -EFAULT;

	return sys_open(path, flags, mode, SCHED->current->proc);
}

static int _sys_close(int fd)
{
	return sys_close(fd, SCHED->current->proc);
}

static int _sys_waitpid(pid_t pid, int * status, int options)
{
	if ((status != NULL) && (!IS_IN_USERMEM(status)))
		return -EFAULT;
	//kprintf("waitpid(%d, %x, %d)\n", pid, status, options);
	return sys_waitpid(pid, status, options);
}

static int _sys_creat(char * path, mode_t mode)
{
	if (!IS_IN_USERMEM(path))
		return -EFAULT;

	STUB();
	return -ENOSYS;
}

static int _sys_link(char * oldpath, char * newpath)
{
	if ((!IS_IN_USERMEM(oldpath)) || (!IS_IN_USERMEM(newpath)))
		return -EFAULT;


	return sys_link(oldpath, newpath, SCHED->current->proc);
}

static int _sys_unlink(char * path)
{
	if (!IS_IN_USERMEM(path))
		return -EFAULT;

	return sys_unlink(path, SCHED->current->proc);
}

static int _sys_execve(char * path, char * argv[], char * envp[])
{
	/* TODO: Check argv & envp */
	if (!IS_IN_USERMEM(path))
		return -EFAULT;

	//kprintf("execve(%s, %x, %x)\n", path, argv, envp);
	return sys_execve(path, argv, envp);
}

static void * _sys_mmap(void * start, size_t length, int flags, int fd, off_t offset)
{
	void * addr;
	int err;

	if ((start != NULL) && (!IS_IN_USERMEM(start)))
		return (void *)-EFAULT;

	if (((addr_t)start % PAGE_SIZE) || (length % PAGE_SIZE) || (offset % PAGE_SIZE))
		return (void *)-EINVAL;


	err = vm_mmap(start, length, flags, fd, offset, SCHED->current->proc->vmspace, &addr);
	if (err != 0)
		return (void *)err;
	return addr;
}

static int _sys_munmap(void * start, size_t length)
{
	if (!IS_IN_USERMEM(start))
		return -EFAULT;

	return vm_unmap(start, length, SCHED->current->proc->vmspace);
}

static int _sys_getdents(int fd, struct dirent * buf, unsigned count)
{
	if (!IS_IN_USERMEM(buf))
		return -EFAULT;

	return sys_getdents(fd, buf, count, SCHED->current->proc);
}

static int _sys_lseek(int fd, off_t offset, int whence)
{
	return sys_lseek(fd, offset, whence, SCHED->current->proc);
}

static int _sys_chdir(char * path)
{
	if (!IS_IN_USERMEM(path))
		return -EFAULT;

	return sys_chdir(path, SCHED->current->proc);
}

static int _sys_fstat(int fd, struct stat * stat)
{
	if (!IS_IN_USERMEM(stat))
		return -EFAULT;

	return sys_fstat(fd, stat, SCHED->current->proc);
}

static pid_t _sys_getpid(void)
{
	return SCHED->current->proc->pid;
}

static int _sys_kill(pid_t pid, int sig)
{
	STUB();
	return -ENOSYS;
}

static int _sys_stat(char * path, struct stat * stat)
{
	if ((!IS_IN_USERMEM(path)) || (!IS_IN_USERMEM(stat)))
		return -EFAULT;

	return sys_stat(path, stat, SCHED->current->proc);
}

static int _sys_lstat(char * path, struct stat * stat)
{
	if ((!IS_IN_USERMEM(path)) || (!IS_IN_USERMEM(stat)))
		return -EFAULT;

	return sys_lstat(path, stat, SCHED->current->proc);
}

static int _sys_access(char * path, int mode)
{
	if (!IS_IN_USERMEM(path))
		return -EFAULT;

	return sys_access(path, mode, SCHED->current->proc);
}

static int _sys_fchdir(int fd)
{
	STUB();
	return -ENOSYS;
}

static int _sys_dup(int oldfd)
{
	return sys_dup(oldfd, SCHED->current->proc);
}

static int _sys_dup2(int oldfd, int newfd)
{
	return sys_dup2(oldfd, newfd, SCHED->current->proc);
}

static mode_t _sys_umask(mode_t mode)
{
	mode_t old;
	old = SCHED->current->proc->umask;
	SCHED->current->proc->umask = mode;
	return old;
}

static int _sys_chmod(char * path, mode_t mode)
{
	if (!IS_IN_USERMEM(path))
		return -EFAULT;

	return sys_chmod(path, mode, SCHED->current->proc);
}

static int _sys_fcntl(int fd, int cmd, void * arg)
{
	return sys_fcntl(fd, cmd, arg, SCHED->current->proc);
}

static int _sys_utime()
{
	STUB();
	return -ENOSYS;
}

static int _sys_chown(char * path, uid_t uid, gid_t gid)
{
	STUB();
	return -ENOSYS;
}

static int _sys_rmdir(char * path)
{
	STUB();
	return -ENOSYS;
}

static int _sys_mkdir(char * path, mode_t mode)
{
	if (!IS_IN_USERMEM(path))
		return -EFAULT;

	return sys_mkdir(path, mode, SCHED->current->proc);
}

static int _sys_ftruncate(int fd, off_t len)
{
	STUB();
	return -ENOSYS;
}

static int _sys_ioctl(int fd, int cmd, void * arg)
{
	return sys_ioctl(fd, cmd, arg, SCHED->current->proc);
}

static int _sys_select()
{
	//STUB();
	return -ENOSYS;
}

static int _sys_nanosleep()
{
	STUB();
	return -ENOSYS;
}

static unsigned int _sys_sleep()
{
	STUB();
	return -ENOSYS;
}

static int _sys_mknod(char * path, mode_t mode, dev_t dev)
{
	if (!IS_IN_USERMEM(path))
		return -EFAULT;

	return sys_mknod(path, mode, dev, SCHED->current->proc);
}

static int _sys_mkfifo()
{
	STUB();
	return -ENOSYS;
}

static int _sys_pipe(int * fds)
{
	if (!IS_IN_USERMEM(fds))
		return -EFAULT;

	return sys_pipe(fds, SCHED->current->proc);
}

static uid_t _sys_getuid(void)
{
	return SCHED->current->proc->uid;
}

static gid_t _sys_getgid(void)
{
	return SCHED->current->proc->gid;
}

static uid_t _sys_geteuid(void)
{
	return SCHED->current->proc->euid;
}

static gid_t _sys_getegid(void)
{
	return SCHED->current->proc->egid;
}

static pid_t _sys_getppid(void)
{
	return SCHED->current->proc->parent->pid;
}

static unsigned int _sys_alarm(unsigned int seconds)
{
	return 0;
}

static int _sys_setuid(uid_t uid)
{
	STUB();
	return -ENOSYS;
}

static int _sys_setgid(gid_t dit)
{
	STUB();
	return -ENOSYS;
}

static int _sys_vfork(void * stack)
{
	return sys_vfork(stack);
}

static int _sys_mount(char * src, char * dest, char * fs, int flags, void * data)
{
	if ((!IS_IN_USERMEM(src)) || (!IS_IN_USERMEM(dest)) || (!IS_IN_USERMEM(fs)) ||
	    ((!IS_IN_USERMEM(data)) && (data != NULL)))
		return -EFAULT;

	return sys_mount(src, dest, fs, flags, data, SCHED->current->proc);
}

static int _sys_umount(char * dest, int flags)
{
	if (!IS_IN_USERMEM(dest))
		return -EFAULT;

	return sys_umount(dest, flags, SCHED->current->proc);
}

static int _sys_uname(struct utsname * buf)
{
	extern char __osname[];
	extern char __release[];
	extern char __machine[];
	extern char __version[];

	if (!IS_IN_USERMEM(buf))
		return -EFAULT;

	strcpy(buf->sysname, __osname);
	strcpy(buf->nodename, "localhost"); /* TODO: Obsluga nazwy hosta */
	strcpy(buf->release, __release);
	strcpy(buf->version, __version);
	strcpy(buf->machine, __machine);
	return 0;
}

void * __syscall_table[] =
{
	&_sys_nosys,
	&_sys_exit,
	&_sys_fork,
	&_sys_read,
	&_sys_write,
	&_sys_open,
	&_sys_close,
	&_sys_waitpid,
	&_sys_creat,
	&_sys_link,
	&_sys_unlink,
	&_sys_execve,
	&_sys_mmap,
	&_sys_munmap,
	&_sys_getdents,
	&_sys_lseek,
	&_sys_chdir,
	&_sys_fstat,
	&_sys_getpid,
	&_sys_kill,
	&_sys_stat,
	&_sys_lstat,
	&_sys_access,
	&_sys_fchdir,
	&_sys_dup,
	&_sys_dup2,
	&_sys_umask,
	&_sys_chmod,
	&_sys_fcntl,
	&_sys_utime,
	&_sys_chown,
	&_sys_rmdir,
	&_sys_mkdir,
	&_sys_ftruncate,
	&_sys_ioctl,
	&_sys_select,
	&_sys_nanosleep,
	&_sys_sleep,
	&_sys_mknod,
	&_sys_mkfifo,
	&_sys_pipe,
	&_sys_getuid,
	&_sys_getgid,
	&_sys_geteuid,
	&_sys_getegid,
	&_sys_getppid,
	&_sys_alarm,
	&_sys_setuid,
	&_sys_setgid,
	&_sys_vfork,
	&_sys_mount,
	&_sys_umount,
	&_sys_uname
};

const int __syscall_table_size = sizeof(__syscall_table) / sizeof(void *);