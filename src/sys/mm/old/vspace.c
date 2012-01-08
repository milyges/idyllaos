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
#include <mm/heap.h>
#include <mm/vmm.h>
#include <mm/mmap.h>
#include <mm/sma.h>
#include <lib/string.h>
#include <lib/math.h>
#include <lib/errno.h>

struct vspace * vspace_create(void)
{
	struct vspace * vs = kalloc(sizeof(struct vspace));
	mutex_init(&vs->mutex);
	vs->area = sma_area_create(USERMEM_START, USERMEM_SIZE, NULL, NULL);
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

int vspace_destroy(struct vspace * vspace)
{
	return -ENOSYS;
}

void vspace_switch(struct vspace * vspace)
{
	CPU->vspace = vspace;
	if (!vspace)
		paging_switch_aspace(NULL);
	else
		paging_switch_aspace(vspace->dataptr);
}

int vspace_clone(struct vspace ** dest, struct vspace * src)
{
	struct vspace * vspace;
	struct sma_block * block;
	int err;

	vspace = vspace_create();
	if (!vspace)
		return -ENOMEM;

	mutex_lock(&src->mutex);
	/* Dla każdego mapowania */
	block = NULL;

	while((block = sma_get_next_used(src->area, block)) != NULL)
	{
		/* Klonujemy mapowanie */
		err = mmap_clone(block, vspace);
		if (err != 0)
		{
			/* TODO: Zwolnij pamięć */
			TODO("vspace_clone(): failed");
			mutex_unlock(&src->mutex);
			return err;
		}
	}

	mutex_unlock(&src->mutex);

	*dest = vspace;
	return 0;
}
