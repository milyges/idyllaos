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
#include <arch/asm.h>
#include <arch/cpu.h>
#include <kernel/types.h>
#include <kernel/kprintf.h>
#include <kernel/syscall.h>
#include <kernel/proc.h>
#include <lib/errno.h>

void syscall32(struct intr_stack * stack)
{
	intr_enable();
	reg_t (*func)();
	if (stack->ax >= __syscall_table_size)
	{
		kprintf("syscall32: unkown function %d\n", stack->ax);
		return;
	}

	//kprintf("syscall: pid=%d func=%d\n", SCHED->current->proc->pid, stack->ax);
	func = __syscall_table[stack->ax];
	/* Wszystkie funkcje poza fork() i vfork() używają normalnego rozpisania argumentów */
	if ((stack->ax != 2) && (stack->ax != 49))
		stack->ax = func(stack->bx, stack->cx, stack->dx, stack->si, stack->di);
	else
		stack->ax = func(stack);
	intr_disable();
}
