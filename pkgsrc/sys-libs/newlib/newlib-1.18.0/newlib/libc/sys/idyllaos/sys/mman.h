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
#ifndef __SYS_MMAN_H
#define __SYS_MMAN_H

#ifdef __cplusplus
extern "C" {
#endif

#define PROT_NONE     0x001
#define PROT_READ     0x002
#define PROT_WRITE    0x004
#define PROT_EXEC     0x008

#define MAP_SHARED    0x010
#define MAP_PRIVATE   0x020
#define MAP_FIXED     0x040
#define MAP_ANONYMOUS 0x080
#define MAP_GROWSDOWN 0x100

#define MS_ASYNC      0x001
#define MS_SYNC       0x002
#define MS_INVALIDATE 0x004

#include <sys/types.h>

void * mmap(void * start, size_t length, int prot, int flags, int fd, off_t offset);
int munmap(void * start, size_t length);

#ifdef __cplusplus
}
#endif


#endif /* __SYS_MMAN_H */
