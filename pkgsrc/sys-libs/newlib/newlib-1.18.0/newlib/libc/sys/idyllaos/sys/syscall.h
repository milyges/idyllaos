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
#ifndef __SYS_SYSCALL_H
#define __SYS_SYSCALL_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#define SYS_EXIT          1
#define SYS_FORK          2
#define SYS_READ          3
#define SYS_WRITE         4
#define SYS_OPEN          5
#define SYS_CLOSE         6
#define SYS_WAITPID       7
#define SYS_CREAT         8
#define SYS_LINK          9
#define SYS_UNLINK        10

#define SYS_EXECVE        11
#define SYS_MMAP          12
#define SYS_MUNMAP        13
#define SYS_GETDENTS      14
#define SYS_LSEEK         15
#define SYS_CHDIR         16
#define SYS_FSTAT         17
#define SYS_GETPID        18
#define SYS_KILL          19
#define SYS_STAT          20

#define SYS_LSTAT         21
#define SYS_ACCESS        22
#define SYS_FCHDIR        23
#define SYS_DUP           24
#define SYS_DUP2          25
#define SYS_UMASK         26
#define SYS_CHMOD         27
#define SYS_FCNTL         28
#define SYS_UTIME         29
#define SYS_CHOWN         30

#define SYS_RMDIR         31
#define SYS_MKDIR         32
#define SYS_FTRUNCATE     33
#define SYS_IOCTL         34
#define SYS_SELECT        35
#define SYS_NANOSLEEP     36
#define SYS_SLEEP         37
#define SYS_MKNOD         38
#define SYS_MKFIFO        39
#define SYS_PIPE          40

#define SYS_GETUID        41
#define SYS_GETGID        42
#define SYS_GETEUID       43
#define SYS_GETEGID       44
#define SYS_GETPPID       45
#define SYS_ALARM         46
#define SYS_SETUID        47
#define SYS_SETGID        48
#define SYS_VFORK         49
#define SYS_MOUNT         50

#define SYS_UMOUNT        51
#define SYS_UNAME         52

#ifdef __cplusplus
}
#endif

#include <machine/syscall.h>

#endif /* __SYS_SYSCALL_H */
