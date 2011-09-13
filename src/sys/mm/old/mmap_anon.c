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
#include <kernel/types.h>
#include <lib/string.h>
#include <lib/errno.h>
#include <lib/math.h>
#include <mm/heap.h>
#include <mm/sma.h>
#include <mm/vmm.h>
#include <mm/mmap.h>
#include <mm/phys.h>
#include <kernel/kprintf.h>
#include <kernel/debug.h>
#include <kernel/proc.h>
#include <kernel/vfs.h>

static int mmap_anon_init(struct mmap_info * mmap)
{
	return 0;
}

static int mmap_anon_destroy(struct mmap_info * mmap)
{
	return -ENOSYS;
}

static void * mmap_anon_getpage(struct sma_block * region, struct vspace * vspace, addr_t offset)
{
	int page;
	offset = ROUND_DOWN(offset, PAGE_SIZE);
	page = offset / PAGE_SIZE;
	struct mmap_info * mmap = region->dataptr;

	/* Alokujemy nową stronę */
	if (!mmap->pages[page])
	{
		mmap->pages[page] = phys_alloc(1, PHYS_KERNEL | PHYS_ZONE_HIGH);
		if (!mmap->pages[page])
		{
			kprintf("mmap_anon_getpage(): Out of memory\n");
			return NULL;
		}
	}

	/* Mapujemy strone w przestrzeni adresowej (w trybie rw) */
	if (paging_map_page(region->start + offset, mmap->pages[page], PG_FLAG_USER | PG_FLAG_RW | PG_FLAG_PRESENT, vspace->dataptr) != 0)
		return NULL;

	/* Wypelniamy zerami */
	memset((void *)(region->start + offset), 0x00, PAGE_SIZE);

	/* Zwracamy adres do stworzonej strony */
	return (void *)(region->start + offset);
}

struct mmap_ops __mmap_anon_ops =
{
	.init = &mmap_anon_init,
	.destroy = &mmap_anon_destroy,
	.getpage = &mmap_anon_getpage
};
