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
#include <mm/kmem.h>
#include <lib/string.h>
#include <lib/math.h>
#include <lib/errno.h>

struct vm_page * vm_copy_page(struct vm_mapping * mapping, struct vm_page * oldpage, off_t offset)
{
	struct vm_page * page;
	void * buf;

	/* a) Tworzymy nową stronę
	   b) Mapujemy ją w pamięci tymczawowej jądra
	   c) Kopiujemy jej zawartość
	   d) Odmapowywujemy z pamięci tymczasowej
	*/

	//kprintf("vm_copy_page(): oldpage=0x%x, offset=%x\n", oldpage->address, offset);
	page = kalloc(sizeof(struct vm_page));
	list_init(&page->list);
	atomic_set(&page->refs, 1);
	page->offset = oldpage->offset;
	page->address = phys_alloc(1, PHYS_KERNEL | PHYS_ZONE_HIGH);
	if (!page->address)
	{
		kfree(page);
		return NULL;
	}

	buf = kmem_map_page(page->address);
	//kprintf("buf=%x\n", buf);
	/* Jeżeli stara strona nie jest zamapowana */
	paging_map_page(mapping->block->start + offset, oldpage->address, PG_FLAG_PRESENT, CPU->vmspace->dataptr);
	memcpy(buf, (void *)(mapping->block->start + offset), PAGE_SIZE);
	kmem_unmap_page(buf);

	atomic_dec(&oldpage->refs);
	if (!atomic_get(&oldpage->refs))
	{
		phys_free(oldpage->address, 1);
		kfree(oldpage);
	}

	return page;
}

int vm_getpage(struct vm_mapping * mapping, off_t offset, unsigned rw)
{
	struct vm_region * region = mapping->region;
	int pageidx = offset / PAGE_SIZE;
	struct vm_page * page = NULL;
	uint16_t page_flags = PG_FLAG_USER | PG_FLAG_RW | PG_FLAG_PRESENT;
	//kprintf("pid=%d, getpage\n", SCHED->current->proc->pid);
	//kprintf("flags=%x\n", mapping->flags);
	/* Sprawdzamy czy nie chodzi o zapis w przestrzeni read-only */
	if (((mapping->flags & PROT_WRITE) != PROT_WRITE) && (rw))
	{
		kprintf("getpage: write in read-only memory!\n");
		return -1;
	}

	//mutex_lock(&region->mutex);
	page = region->pages[pageidx];

	if ((page != NULL) && ((mapping->flags & MAP_SHARED) != MAP_SHARED))
	{
		if (rw)
		{
			/* Wykonujemy kopiowanie strony */
			page = vm_copy_page(mapping, page, offset);
			mapping->region->pages[pageidx] = page;
		}
		else
		{
			/* Mapujemy strone read only */
			page_flags &= ~PG_FLAG_RW;
		}
	}
	else
	{
		page = mapping->region->ops->getpage(mapping, offset);
		mapping->region->pages[pageidx] = page;
	}

	//mutex_unlock(&region->mutex);

	if (!page)
	{
		//mutex_unlock(&CPU->vmspace->mutex);
		return -1;
	}

	paging_map_page(mapping->block->start + offset, page->address, page_flags, CPU->vmspace->dataptr);

	//kprintf("getpage exit\n");
	return 0;
}
