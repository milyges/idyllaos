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
#include <arch/stacktrace.h>
#include <arch/cpu.h>
#include <arch/asm.h>
#include <kernel/types.h>
#include <kernel/kprintf.h>
#include <kernel/kld.h>
#include <mm/heap.h>

void do_stacktrace(addr_t stacktop, addr_t bp)
{
	reg_t * ptr = (reg_t *)bp;
	addr_t * eip;
	char * name;
	int * offset;
	
	kprintf("Stacktrace:\n");
	while(((addr_t)ptr >= stacktop) && ((addr_t)ptr < (stacktop + __CONFIG_KSTACK_SIZE)))
	{
		eip = (addr_t *)(*(ptr + 1) - 5);
		offset = (int *)((void *)eip + 1);
		name = kld_addr2name((void *)((addr_t)eip + (*offset + 5)));
		kprintf(" <0x%p> call %s\n", eip, (name != NULL) ? name : "?");
		ptr = (reg_t *)(*ptr);
	}
}
