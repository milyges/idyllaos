/*
 * Idylla Operating System
 * Copyright (C) 2009  Idylla Operating System Team
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
#include <kernel/panic.h>
#include <kernel/kprintf.h>
#include <kernel/debug.h>
#include <mm/phys.h>
#include <lib/string.h>

#define LACKS_UNISTD_H
#define LACKS_STRING_H
#define LACKS_SYS_TYPES_H
#define LACKS_STDLIB_H
#define LACKS_ERRNO_H

#define HAVE_MMAP               0
#define HAVE_MORECORE           1
#define USE_LOCKS               1
#define NO_MALLINFO             1

#define ABORT                   panic("mm_heap: kmalloc abort\n")
#define USAGE_ERROR_ACTION(m,p) panic("mm_heap: kmalloc usage error (%s:%d)\n", m, p)
#define malloc_getpagesize      ((size_t)PAGE_SIZE)

#define MALLOC_FAILURE_ACTION   kprintf(KERN_ERR"mm_heap: kmalloc failed\n")
#define MORECORE                ksbrk
#define stderr			NULL
#define fprintf(t, fmt...)	kprintf(fmt)

#define malloc                  kalloc
#define free                    kfree
#define calloc                  kcalloc
#define realloc                 krealloc
#define memalign                kmemalign
#define valloc                  kvalloc

static addr_t _heap_top = KHEAP_START;
static size_t _heap_size = 0;

/* Zmieniamy rozmiar sterty jądra */
void * ksbrk(ssize_t incr)
{
	void * ptr = (void *)_heap_top;
	paddr_t frame;
	
	if (!incr)
		return ptr;
	
	//kprintf("ksbrk(%d)\n", incr);
	
	if (incr < 0)
	{
		TODO("Free some heap memory");
	}
	else
	{
		while(incr + _heap_top >= KHEAP_START + _heap_size)
		{
			if (_heap_size >= KHEAP_SIZE)
				panic("End of kernel virtual memory");
			
			frame = phys_alloc(1, PHYS_KERNEL | PHYS_ZONE_HIGH);
			if (!frame)
				panic("End of kernel memory!");
			
			paging_map_page(KHEAP_START + _heap_size, frame, PG_FLAG_PRESENT | PG_FLAG_RW, NULL);
			_heap_size += PAGE_SIZE;
		}
	}
	_heap_top += incr;
	
	return ptr;
}

#include "malloc.c"
