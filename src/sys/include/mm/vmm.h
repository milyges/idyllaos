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
#ifndef __MM_VMM_H
#define __MM_VMM_H

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

#include <kernel/mutex.h>
#include <mm/sma.h>

/* Struktura opisująca stronę pamięci fizycznej */
struct vm_page
{
	list_t list; /* Lista stron dla danego pliku */
	atomic_t refs; /* Ilość odwołań */

	off_t offset; /* Offset w pliku */
	paddr_t address; /* Adres fizyczny */
};

struct vm_space
{
	void * dataptr; /* Dane specyficzne dla architektóry (np. katalog stron) */
	struct mutex mutex;
	struct sma_area * area;
	atomic_t refs; /* Uzywane przy vfork() */
};

struct vm_region
{
	atomic_t refs; /* Licznik odwołań */
	struct vm_ops * ops;
	struct vm_page ** pages;
	struct mutex mutex;
	size_t size;

	/* Używane przy mapowaniu pliku */
	struct vnode * vnode;
	loff_t offset;
};

struct vm_mapping
{
	struct sma_block * block; /* Potrzebne do określania rozmiaru */
	struct vm_space * vmspace; /* Przestrzen adresowa mapowania */
	uint16_t flags; /* Flagi */
	struct vm_region * region; /* Region pamięci */
	off_t offset; /* Przesunięcie w regionie */
};

struct vm_ops
{
	int (*init)(struct vm_region * region);
	int (*destroy)(struct vm_region * region);
	struct vm_page * (*getpage)(struct vm_mapping * mapping, off_t offset);
};

int vm_pagefault(addr_t address, unsigned rw);

struct vm_space * vm_space_create(void);
void vm_space_destroy(struct vm_space * vmspace);
void vm_space_switch(struct vm_space * newspace);
int vm_space_clone(struct vm_space * dest, struct vm_space * src);

int vm_mmap(void * start, size_t length, uint16_t flags, int fd, loff_t offset, struct vm_space * vmspace, void ** addr);
int vm_unmap(void * start, size_t length, struct vm_space * vmspace);
void vm_destroy(struct sma_block * block);

int vm_clone(struct vm_mapping * mapping, struct vm_space * src, struct vm_space * dest);

int vm_getpage(struct vm_mapping * mapping, off_t offset, unsigned rw);

#endif /* __MM_VMM_H */
