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
#include <arch/atomic.h>
#include <kernel/types.h>
#include <kernel/bitops.h>
#include <kernel/kprintf.h>
#include <kernel/proc.h>
#include <kernel/debug.h>
#include <mm/heap.h>
#include <mm/vmm.h>
#include <mm/sma.h>
#include <mm/phys.h>
#include <lib/string.h>
#include <lib/math.h>
#include <lib/errno.h>

struct vm_space * vm_space_create(void)
{
	struct vm_space * vs = kalloc(sizeof(struct vm_space));
	mutex_init(&vs->mutex);
	atomic_set(&vs->refs, 1);
	vs->area = sma_area_create(USERMEM_START, USERMEM_SIZE, NULL, NULL, &vm_destroy);
	if (!vs->area)
	{
		kfree(vs);
		return NULL;
	}

	vs->dataptr = paging_create_aspace();
	if (!vs->dataptr)
	{
		sma_area_destroy(vs->area);
		kfree(vs);
		return NULL;
	}

	return vs;
}

void vm_space_destroy(struct vm_space * vmspace)
{

	if (CPU->vmspace == vmspace)
		vm_space_switch(NULL);

	/* Sprawdzamy czy inny proces nie uzywa tej przestrzeni adresowej */
	atomic_dec(&vmspace->refs);
	if (atomic_get(&vmspace->refs) > 0)
		return;

	mutex_lock(&vmspace->mutex);

	sma_area_destroy(vmspace->area);

	/* Niszczymy katalog stron */
	paging_destroy_aspace(vmspace->dataptr);

	mutex_unlock(&vmspace->mutex);

	/* Usuwamy strukturę z pamięci */
	kfree(vmspace);
}

void vm_space_switch(struct vm_space * newspace)
{
	CPU->vmspace = newspace;
	if (!newspace)
		paging_switch_aspace(NULL);
	else
		paging_switch_aspace(newspace->dataptr);
}

int vm_space_clone(struct vm_space * dest, struct vm_space * src)
{
	int err;
	struct sma_block * block = NULL;

	mutex_lock(&src->mutex);
	mutex_lock(&dest->mutex);

	/* Klonujemy każde mapowanie */
	while((block = sma_get_next_used(src->area, block)) != NULL)
	{

		err = vm_clone(block->dataptr, src, dest);
		if (err != 0)
		{
			mutex_unlock(&dest->mutex);
			mutex_unlock(&src->mutex);
			return err;
		}

	}

	mutex_unlock(&dest->mutex);
	mutex_unlock(&src->mutex);
	return 0;
}
