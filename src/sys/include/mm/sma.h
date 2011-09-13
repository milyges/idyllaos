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
#ifndef __MM_SMA_H
#define __MM_SMA_H

#include <kernel/types.h>
#include <lib/list.h>
#include <kernel/mutex.h>

#define SMA_FLAGS_USED   0x01
#define SMA_ALLOC_FIXED  0x02

/* Struktura przechowująca informacje o pustym/zajętym bloku */
struct sma_block
{
	list_t list;
	unsigned flags;

	addr_t start;
	size_t size;

	void * dataptr;
};

/* Struktura przechowująca informacje o obszarze do alokacji */
struct sma_area
{
	struct mutex mutex;

	list_t used_list;
	list_t free_list;

	addr_t start;
	size_t size;

	void (*do_alloc)(struct sma_block * block);
	void (*do_resize)(struct sma_block * block, addr_t newstart, size_t newsize);
	void (*do_free)(struct sma_block * block);
};

/* Area operations */
struct sma_area * sma_area_create(addr_t start, size_t size, void * do_alloc, void * do_resize, void * do_free);
int sma_area_destroy(struct sma_area * area);
void sma_area_dump(struct sma_area * area);

/* Block operations */
struct sma_block * sma_alloc(struct sma_area * area, addr_t addr, size_t size, unsigned flags);
int sma_free(struct sma_area * area, addr_t start, size_t size);
struct sma_block * sma_getblock(struct sma_area * area, addr_t addr);
struct sma_block * sma_get_next_used(struct sma_area * area, struct sma_block * current);

#endif /* __MM_SMA_H */
