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

static int mmap_vfs_init(struct mmap_info * mmap)
{
	return 0;
}

static int mmap_vfs_destroy(struct mmap_info * mmap)
{
	return -ENOSYS;
}

static void * mmap_vfs_getpage(struct sma_block * region, struct vspace * vspace, addr_t offset)
{
	int page;
	ssize_t err;
	offset = ROUND_DOWN(offset, PAGE_SIZE);
	page = offset / PAGE_SIZE;
	struct mmap_info * mmap = region->dataptr;

	mutex_lock(&mmap->mutex);

	/* Jeżeli strona już istnieje, tylko ją mapujemy */
	if (mmap->pages[page])
	{
		if (paging_map_page(region->start + offset, mmap->pages[page], PG_FLAG_USER | PG_FLAG_RW | PG_FLAG_PRESENT, vspace->dataptr) != 0)
			return NULL;
	}
	else
	{
		mmap->pages[page] = phys_alloc(1, PHYS_KERNEL | PHYS_ZONE_HIGH);
		if (!mmap->pages[page])
		{
			kprintf("mmap_vfs_getpage(): Out of memory\n");
			return NULL;
		}


		/* Mapujemy strone w przestrzeni adresowej */
		if (paging_map_page(region->start + offset, mmap->pages[page], PG_FLAG_USER | PG_FLAG_RW | PG_FLAG_PRESENT, vspace->dataptr) != 0)
			return NULL;

		/* Ładujemy ile się da z pliku */
		err = mmap->file->vnode->ops->read(mmap->file->vnode, (void *)(region->start + offset), PAGE_SIZE, mmap->offset + offset);
		if (err < 0)
		{
			kprintf("mmap_vfs_getpage(): I/O error (%d)\n", err);
			return NULL;
		}
		//kprintf("mmap_vfs_getpage(): loaded %d bytes from 0x%X\n", err, mmap->offset + offset);

		/* Pozostala czesc wypelniamy zerami */
		memset((void *)(region->start + offset + err), 0x00, PAGE_SIZE - err);
	}

	mutex_unlock(&mmap->mutex);

	/* Zwracamy adres do stworzonej strony */
	return (void *)(region->start + offset);
}

struct mmap_ops __mmap_vfs_ops =
{
	.init = &mmap_vfs_init,
	.destroy = &mmap_vfs_destroy,
	.getpage = &mmap_vfs_getpage
};
