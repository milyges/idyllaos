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
#include <kernel/types.h>
#include <kernel/proc.h>
#include <kernel/kprintf.h>
#include <kernel/debug.h>
#include <kernel/vfs.h>
#include <kernel/elf.h>
#include <mm/heap.h>
#include <mm/vmm.h>
#include <lib/string.h>
#include <lib/errno.h>
#include <lib/math.h>
#include <lib/stddef.h>

extern struct proc * __init_proc;

void sys_exit(int code, struct proc * proc)
{
	struct vm_space * vmspace;
	struct proc * child;

	if (proc->pid == 1)
		panic("attempted to kill init!");

	/* Jeżeli proces ma dzieci, to stają się one teraz dziećmi inita */
	while(!LIST_IS_EMPTY(&proc->childs_list))
	{
		child = (struct proc *)((addr_t)proc->childs_list.next - offsetof(struct proc, list_siblings));
		child->parent = __init_proc;
		list_add(&__init_proc->childs_list, &child->list_siblings);
	}

	/* Zakanczamy wszystkie wątki danego procesu (poza main) */

	/* Zamykamy otwarte pliki */
	vfs_free(proc);

	/* Niszczymy przestrzen adresowa */
	vmspace = proc->vmspace;
	proc->vmspace = NULL;
	vm_space_destroy(vmspace);

	//kprintf("%d: exit()\n", proc->pid);

	/* Budzimy czekajace watki */
	proc->main->exit_code = code;

	sched_preempt_disable();

	proc->main->state = THREAD_STATE_ZOMBIE;

	/* TODO: Sprawdz wszystkie wątki */
	if (((proc->parent->main->state == THREAD_STATE_WAITING) || (proc->parent->main->state == THREAD_STATE_BLOCKED)) &&
	   ((proc->parent->main->waitpid == proc->pid) || (proc->parent->main->waitpid == -1)))
		sched_wakeup(proc->parent->main);

	sched_preempt_enable();
}
