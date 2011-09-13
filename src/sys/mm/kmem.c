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
#include <lib/string.h>
#include <lib/errno.h>
#include <lib/math.h>
#include <mm/heap.h>
#include <mm/sma.h>
#include <mm/kmem.h>
#include <kernel/kprintf.h>
#include <kernel/debug.h>

static struct sma_area * _kmem_area;

void * kmem_alloc(size_t len)
{
	struct sma_block * block;
	block = sma_alloc(_kmem_area, 0, len, 0);
	if (!block)
		return NULL;
	return (void *)block->start;
}

void kmem_free(void * ptr, size_t len)
{
	sma_free(_kmem_area, (addr_t)ptr, len);
}

void kmem_init(void)
{
	_kmem_area = sma_area_create(KMEM_START, KMEM_SIZE, NULL, NULL, NULL);
}

void * kmem_map_page(paddr_t phys)
{
	void * ptr;
	ptr = kmem_alloc(PAGE_SIZE);
	if (!ptr)
		return NULL;

	paging_map_page((addr_t)ptr, phys, PG_FLAG_RW | PG_FLAG_PRESENT, NULL);
	return ptr;
}

void kmem_unmap_page(void * ptr)
{
	paging_unmap_page((addr_t)ptr, NULL);
	kmem_free(ptr, PAGE_SIZE);
}
