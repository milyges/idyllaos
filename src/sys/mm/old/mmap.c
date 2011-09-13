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
#include <kernel/vfs.h>

extern struct mmap_ops __mmap_anon_ops;
extern struct mmap_ops __mmap_vfs_ops;

int sys_mmap(void * start, size_t length, uint16_t flags, int fd, loff_t offset, struct vspace * vspace, void ** addr)
{
	addr_t vaddr;
	int err;
	struct mmap_info * mmap;
	struct sma_block * region;

	vaddr = (addr_t)start;

	if ((vaddr % PAGE_SIZE) || (!length) || (length % PAGE_SIZE))
		return -EINVAL;

	if ((flags & PROT_NONE) && (flags & (PROT_READ | PROT_WRITE | PROT_EXEC)))
		return -EINVAL;

	if (((flags & MAP_ANONYMOUS) != MAP_ANONYMOUS) && ((fd < 0) || (fd >= OPEN_MAX) || (!SCHED->current->proc->filedes[fd])))
		return -EBADF;



	return 0;
}

int mmap_clone(struct sma_block * region, struct vspace * vspace)
{
	return -ENOSYS;
}
