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
#include <kernel/debug.h>
#include <mm/heap.h>
#include <mm/vmm.h>
#include <mm/sma.h>
#include <mm/phys.h>
#include <lib/string.h>
#include <lib/math.h>
#include <lib/errno.h>

static int vm_mmap_vfs_init(struct vm_region * region)
{
	return 0;
}

static int vm_mmap_vfs_destroy(struct vm_region * region)
{
	return 0;
}

static struct vm_page * vm_mmap_vfs_getpage(struct vm_mapping * mapping, off_t offset)
{
	struct vm_page * page = NULL;
	ssize_t err;

	offset = ROUND_DOWN(offset, PAGE_SIZE);

	/* W przypadku kiedy mapowanie jest dzielone, szukamy strony w cache dla danego pliku */
	if (mapping->flags & MAP_SHARED)
	{
		TODO("search in file cache");
	}

	if (!page)
	{
		page = kalloc(sizeof(struct vm_page));
		memset(page, 0, sizeof(struct vm_page));
		list_init(&page->list);
		page->offset = mapping->region->offset + offset;
		page->address = phys_alloc(1, PHYS_KERNEL | PHYS_ZONE_HIGH);
		atomic_set(&page->refs, 1);

		/* Mapujemy strone w przestrzeni adresowej */
		if (paging_map_page(mapping->block->start + offset, page->address, PG_FLAG_RW | PG_FLAG_PRESENT, CPU->vmspace->dataptr) != 0)
		{
			phys_free(page->address, 1);
			kfree(page);
			return NULL;
		}
		
		err = mapping->region->vnode->ops->read(mapping->region->vnode, (void *)(mapping->block->start + offset), PAGE_SIZE, mapping->region->offset + offset);
		if (err < 0)
		{
			kprintf("vm_mmap_vfs_getpage(): I/O error (%d)\n", err);
			paging_unmap_page(mapping->block->start + offset, CPU->vmspace->dataptr);
			phys_free(page->address, 1);
			kfree(page);
			return NULL;
		}

		/* Pozostala czesc wypelniamy zerami */
		memset((void *)(mapping->block->start + offset + err), 0x00, PAGE_SIZE - err);

		/* Jeżeli mapowanie jest publiczne, dodajemy stronę do listy mapowan pliku */
		if (mapping->flags & MAP_SHARED)
			list_add(&mapping->region->vnode->pagecache, &page->list);
	}

	return page;
}

struct vm_ops __vm_mmap_vfs_ops =
{
	.init = &vm_mmap_vfs_init,
	.destroy = &vm_mmap_vfs_destroy,
	.getpage = &vm_mmap_vfs_getpage
};
