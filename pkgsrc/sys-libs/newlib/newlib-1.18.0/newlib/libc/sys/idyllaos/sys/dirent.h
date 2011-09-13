/*
 * Idylla Operating System
 * Copyright (C) 2009  Idylla Operating System Team
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
#ifndef __SYS_DIRENT_H
#define __SYS_DIRENT_H

#ifdef __cplusplus
extern "C" {
#endif

#define NAME_MAX  255

#ifndef __P
#define __P(args) ()
#endif

typedef struct __dirdesc
{
	int dd_fd;
	long dd_loc;
	long dd_size;
	char *dd_buf;
	int dd_len;
	long dd_seek;
} DIR;

#define __dirfd(dp)	((dp)->dd_fd)

#include <sys/types.h>
#include <stdint.h>

struct dirent
{
	ino_t d_ino;
	off_t d_off;
	uint16_t d_reclen;
	char d_name[NAME_MAX + 1];
};

int _DEFUN(closedir, (dirp), register DIR *dirp);
DIR * _DEFUN(opendir, (name), const char *name);
struct dirent * _DEFUN(readdir, (dirp), register DIR *dirp);
void _DEFUN(rewinddir, (dirp), DIR *dirp);
int _DEFUN(scandir, (dirname, namelist, select, dcomp),	const char *dirname _AND
           struct dirent ***namelist _AND int (*select) __P((const struct dirent *)) _AND
           int (*dcomp) __P((const struct dirent **, const struct dirent **)));
void _DEFUN(seekdir, (dirp, loc), DIR *dirp _AND long loc);
long _DEFUN(telldir, (dirp), DIR *dirp);


#ifdef __cplusplus
}
#endif

#endif /* __SYS_MMAN_H */
