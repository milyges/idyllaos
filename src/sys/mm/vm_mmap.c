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

extern struct vm_ops __vm_mmap_anon_ops;
extern struct vm_ops __vm_mmap_vfs_ops;

int vm_mmap(void * start, size_t length, uint16_t flags, int fd, loff_t offset, struct vm_space * vmspace, void ** addr)
{
	addr_t vaddr;
	int err;
	struct file * file = NULL;
	struct vm_region * region;
	struct vm_mapping * mapping;
	struct sma_block * block;

	vaddr = (addr_t)start;

	if ((vaddr % PAGE_SIZE) || (!length) || (length % PAGE_SIZE))
		return -EINVAL;

	if ((flags & PROT_NONE) && (flags & (PROT_READ | PROT_WRITE | PROT_EXEC)))
		return -EINVAL;

	if (((flags & MAP_ANONYMOUS) != MAP_ANONYMOUS) && ((fd < 0) || (fd >= OPEN_MAX) || (!SCHED->current->proc->filedes[fd])))
		return -EBADF;

	if ((flags & MAP_ANONYMOUS) != MAP_ANONYMOUS)
		file = SCHED->current->proc->filedes[fd]->file;

	//mutex_lock(&vmspace->mutex);

	/* Alokujemy nowy blok */
	block = sma_alloc(vmspace->area, (addr_t)start, length, (flags & MAP_FIXED) ? SMA_ALLOC_FIXED : 0);
	if (!block)
	{
		//mutex_unlock(&vmspace->mutex);
		return -ENOMEM;
	}

	/* Tworzymy nowy region */
	region = kalloc(sizeof(struct vm_region));
	memset(region, 0, sizeof(struct vm_region));
	mutex_init(&region->mutex);
	atomic_set(&region->refs, 1);
	region->offset = offset;
	region->size = block->size;
	region->pages = kalloc(sizeof(struct vm_page *) * (block->size / PAGE_SIZE));
	memset(region->pages, 0, sizeof(struct vm_page *) * (block->size / PAGE_SIZE));
	if (file)
	{
		VNODE_HOLD(file->vnode);
		region->vnode = file->vnode;
	}

	/* Tworzymy nowe mapowanie */
	mapping = kalloc(sizeof(struct vm_mapping));
	memset(mapping, 0, sizeof(struct vm_mapping));
	mapping->block = block;
	mapping->vmspace = vmspace;
	mapping->region = region;
	mapping->flags = flags;
	block->dataptr = mapping;

	if (flags & MAP_ANONYMOUS)
		region->ops = &__vm_mmap_anon_ops;
	else
		region->ops = &__vm_mmap_vfs_ops;

	err = region->ops->init(region);
	if (err != 0)
	{
		/* TODO: Zwolnij pamięć */
		//mutex_unlock(&vmspace->mutex);
		return err;
	}

	if (addr)
		*addr = (void *)block->start;

	//mutex_unlock(&vmspace->mutex);
	return 0;
}

int vm_unmap(void * start, size_t length, struct vm_space * vmspace)
{
	int err;
	/* Kontrola wartości */
	if (((addr_t)start % PAGE_SIZE) && (length % PAGE_SIZE))
		return -EINVAL;

	//mutex_lock(&vmspace->mutex);
	//err = sma_free(vmspace->area, (addr_t)start, length);
	//mutex_unlock(&vmspace->mutex);
	return -ENOSYS;
}

static void vm_destroy_region(struct vm_region * region)
{
	int i;
	struct vm_page * page;

	//kprintf("[ %d ] destroy region %x\n", SCHED->current->proc->pid, region);
	for(i = 0; i < region->size / PAGE_SIZE; i++)
	{
		if (!region->pages[i])
			continue;
		page = region->pages[i];
		atomic_dec(&page->refs);

		//kprintf("destroy region, page = %x, refs = %d\n", page->address, atomic_get(&page->refs));

		if (!atomic_get(&page->refs))
		{
			phys_free(page->address, 1);
			kfree(page);
		}
		region->pages[i] = NULL;
	}

	if (region->vnode)
		VNODE_REL(region->vnode);
	kfree(region->pages);
	kfree(region);
}

void vm_destroy(struct sma_block * block)
{
	struct vm_mapping * mapping = block->dataptr;
	struct vm_region * region;

	int i;
	//kprintf("[ %d ] destroy mapping %x -> %x\n", SCHED->current->proc->pid, block->start, block->start + block->size);

	/* Odmapowywujemy pamięć */
	for(i = 0; i < block->size / PAGE_SIZE; i++)
		paging_unmap_page(block->start + i * PAGE_SIZE, mapping->vmspace->dataptr);

	/* Niszczymy niepotrzebne regiony */
	region = mapping->region;
	atomic_dec(&region->refs);
	if (!atomic_get(&region->refs))
	{
		vm_destroy_region(region);
	}

	/* Zwalniamy reszte pamięci */
	kfree(mapping);
}

int vm_clone(struct vm_mapping * mapping, struct vm_space * src, struct vm_space * dest)
{
	struct vm_mapping * newmap;
	struct vm_region * region;
	int i;

	newmap = kalloc(sizeof(struct vm_mapping));
	memset(newmap, 0, sizeof(struct vm_mapping));
	newmap->block = sma_alloc(dest->area, (addr_t)mapping->block->start, mapping->block->size, SMA_ALLOC_FIXED);
	if (!newmap->block)
	{
		kfree(newmap);
		return -ENOMEM;
	}
	newmap->block->dataptr = newmap;
	newmap->vmspace = dest;
	newmap->flags = mapping->flags;
	newmap->offset = mapping->offset;

	/* Na początek kilka optymalizacji, jeśli:
	    - region jest tylko do odczytu,
	    - region dotyczy pliku i jest MAP_SHARED,
	   to tworzymy nowe mapowanie i ustawiamy taki sam region jak w pierwotnym */

	if (((mapping->flags & PROT_WRITE) != PROT_WRITE) || ((mapping->region->vnode != NULL) && (mapping->flags & MAP_SHARED)))
	{
		newmap->region = mapping->region;
		atomic_inc(&mapping->region->refs);
	}
	else
	{
		region = mapping->region;
		//mutex_lock(&region->mutex);

		mapping->region = kalloc(sizeof(struct vm_region));
		newmap->region = kalloc(sizeof(struct vm_region));
		memset(mapping->region, 0, sizeof(struct vm_region));
		memset(newmap->region, 0, sizeof(struct vm_region));

		if (region->vnode)
		{
			VNODE_HOLD(region->vnode);
			VNODE_HOLD(region->vnode);
		}

		mapping->region->pages = kalloc(sizeof(struct vm_page *) * (region->size / PAGE_SIZE));
		newmap->region->pages = kalloc(sizeof(struct vm_page *) * (region->size / PAGE_SIZE));
		memset(mapping->region->pages, 0, sizeof(struct vm_page *) * (region->size / PAGE_SIZE));
		memset(newmap->region->pages, 0, sizeof(struct vm_page *) * (region->size / PAGE_SIZE));
		mutex_init(&mapping->region->mutex);
		mutex_init(&newmap->region->mutex);

		mapping->region->ops = newmap->region->ops = region->ops;
		mapping->region->vnode = newmap->region->vnode = region->vnode;
		mapping->region->size = newmap->region->size = region->size;
		mapping->region->offset = newmap->region->offset = region->offset;

		atomic_set(&mapping->region->refs, 1); /* Nowe regiony maja licznik odwołań równy 1 */
		atomic_set(&newmap->region->refs, 1);
		for (i = 0; i < region->size / PAGE_SIZE; i++)
		{
			if (region->pages[i])
			{
				atomic_inc(&region->pages[i]->refs);
				mapping->region->pages[i] = region->pages[i];
				atomic_inc(&region->pages[i]->refs);
				newmap->region->pages[i] = region->pages[i];
			}
		}

		/* c) */
		for(i = 0; i < mapping->block->size / PAGE_SIZE; i++)
		{
			if (region->pages[i])
				paging_map_page(mapping->block->start + (i * PAGE_SIZE), region->pages[i]->address, PG_FLAG_USER | PG_FLAG_PRESENT, src->dataptr);
		}

		mutex_unlock(&region->mutex);

		atomic_dec(&region->refs);
		if (!atomic_get(&region->refs))
			vm_destroy_region(region);
	}

	return 0;
}
