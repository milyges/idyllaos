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
#include <kernel/kprintf.h>
#include <kernel/debug.h>
#include <kernel/proc.h>

int vmm_page_fault(void * addr, unsigned rw)
{
	struct vspace * vspace = CPU->vspace;
	struct sma_block * block;
	struct mmap_info * mmap;
	if (!vspace)
		return -1;

	block = sma_getblock(vspace->area, (addr_t)addr);

	//kprintf("mm: page fault at %x\n", addr);

	/* Sprawdzamy czy blok istnieje i czy jest zajety */
	if ((!block) || ((block->flags & SMA_FLAGS_USED) != SMA_FLAGS_USED))
		return -1;

	mmap = block->dataptr;
	/* Próbujemy pobrac strone */
	if (!mmap->ops->getpage(block, vspace, (addr_t)addr - (block->start)))
		return -1;

	return 0;
}
