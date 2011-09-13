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
#include <arch/page.h>
#include <arch/spinlock.h>
#include <kernel/types.h>
#include <kernel/bitops.h>
#include <kernel/kprintf.h>
#include <kernel/proc.h>
#include <kernel/vfs.h>
#include <mm/heap.h>
#include <mm/vmm.h>
#include <mm/sma.h>
#include <mm/phys.h>
#include <lib/string.h>
#include <lib/math.h>
#include <lib/errno.h>

static int vm_mmap_anon_init(struct vm_region * region)
{
	return 0;
}

static int vm_mmap_anon_destroy(struct vm_region * region)
{
	return 0;
}

static struct vm_page * vm_mmap_anon_getpage(struct vm_mapping * mapping, off_t offset)
{
	struct vm_page * page = NULL;

	offset = ROUND_DOWN(offset, PAGE_SIZE);

	page = kalloc(sizeof(struct vm_page));
	memset(page, 0, sizeof(struct vm_page));
	list_init(&page->list);
	atomic_set(&page->refs, 1);
	page->offset = 0;
	page->address = phys_alloc(1, PHYS_KERNEL | PHYS_ZONE_HIGH);

	/* Mapujemy strone w przestrzeni adresowej */
	if (paging_map_page(mapping->block->start + offset, page->address, PG_FLAG_RW | PG_FLAG_PRESENT, CPU->vmspace->dataptr) != 0)
	{
		phys_free(page->address, 1);
		kfree(page);
		return NULL;
	}

	/* Wypelniamy zerami */
	memset((void *)(mapping->block->start + offset), 0x00, PAGE_SIZE);

	return page;
}

struct vm_ops __vm_mmap_anon_ops =
{
	.init = &vm_mmap_anon_init,
	.destroy = &vm_mmap_anon_destroy,
	.getpage = &vm_mmap_anon_getpage
};