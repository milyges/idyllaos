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
#ifndef __KERNEL_TYPES_H
#define __KERNEL_TYPES_H

#include <arch/types.h>

#define NULL      ((void *)0)
#define PACKED    __attribute__((packed))

/* Typy niezależne od architekury */
typedef unsigned long uint_t;
typedef uint16_t dev_t; /* ID urządzenia */
typedef uint32_t ino_t; /* numer i-węzła */
typedef uint16_t mode_t; /* Tryb */
typedef uint16_t nlink_t; /* ilość dowiązań */
typedef int16_t uid_t; /* ID użytkownika */
typedef int16_t gid_t; /* ID grupy */
typedef int32_t off_t; /* Offset (32bitowy), przy plikach do 2GiB */
typedef int64_t loff_t; /* Offset (64bitowy), przy operacjach na dużych plikach */
typedef int32_t blksize_t; /* Rozmiar bloku */
typedef int32_t blkcnt_t; /* Ilość bloków */
typedef uint64_t time_t; /* Czas */
typedef int32_t pid_t; /* Identyfikator procesu */
typedef int32_t tid_t; /* Identyfikator wątku */
typedef unsigned long size_t; /* Rozmiary obiektów */
typedef signed long ssize_t; /* jw. tylko ze ze znakiem */

struct utsname
{
	char sysname[16];
	char nodename[32];
	char release[32];
	char version[64];
	char machine[16];
};

#endif /* __KERNEL_TYPES_H */

