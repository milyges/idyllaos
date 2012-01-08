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
#include <kernel/debug.h>
#include <mm/heap.h>
#include <mm/vmm.h>
#include <mm/sma.h>
#include <lib/string.h>
#include <lib/math.h>
#include <lib/errno.h>

int vm_pagefault(addr_t address, unsigned rw)
{
	struct vm_space * vmspace = CPU->vmspace;
	struct sma_block * block;
	int err;
	
	if (!vmspace)
		return -1;

	if (SCHED->current->proc->pid > 3)
	{		
		//kprintf("vm_pagefault: pid=%d, addr=%p, rw=%d\n", SCHED->current->proc->pid, address, rw);
		//kprintf("vmspace=%x\n", vmspace);
		//mutex_dump(&vmspace->mutex);
	}
	
	mutex_lock(&vmspace->mutex);

	/* Pobieramy blok */
	block = sma_getblock(vmspace->area, address);

	/* Sprawdzamy czy mapowanie istnieje */
	if ((!block) || ((block->flags & SMA_FLAGS_USED) != SMA_FLAGS_USED))
		err = -2;
	else
		err = vm_getpage(block->dataptr, ROUND_DOWN(address - block->start, PAGE_SIZE), rw);
	
	mutex_unlock(&vmspace->mutex);
	return err;
}
