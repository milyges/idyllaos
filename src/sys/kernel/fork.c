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

#define CLONE_VFORK        0x01

static pid_t do_fork(void * stack, int flags)
{
	int err;
	struct proc * child_proc;
	struct thread * child_thread;
	pid_t pid;
	unsigned intr;

	child_proc = kalloc(sizeof(struct proc));
	proc_init(child_proc, SCHED->current->proc);

	child_proc->uid = SCHED->current->proc->uid;
	child_proc->gid = SCHED->current->proc->gid;
	child_proc->euid = SCHED->current->proc->euid;
	child_proc->egid = SCHED->current->proc->egid;

	/* Klonujemy przestrzen adresowa */
	if ((flags & CLONE_VFORK) != CLONE_VFORK)
	{
		child_proc->vmspace = vm_space_create();
		if (!child_proc->vmspace)
		{
			kfree(child_proc);
			return -ENOMEM;
		}

		err = vm_space_clone(child_proc->vmspace, SCHED->current->proc->vmspace);
		if (err != 0)
		{
			/* Zwolnij pamięć */
			vm_space_destroy(child_proc->vmspace);
			kfree(child_proc);
			return err;
		}
	}
	else
	{
		atomic_inc(&SCHED->current->proc->vmspace->refs);
		child_proc->vmspace = SCHED->current->proc->vmspace;
	}

	/* Klonujemy dane VFS */
	err = vfs_clone(child_proc, SCHED->current->proc);
	if (err != 0)
	{
		/* Zwolnij pamięć */
		vm_space_destroy(child_proc->vmspace);
		kfree(child_proc);
		return err;
	}

	/* Klonujemy aktualny wątek */
	child_thread = kalloc(sizeof(struct thread));
	thread_init(child_thread, child_proc);
	child_proc->main = child_thread;

	/* Klonujemy kontekst wątku */
	ctx_fork(&child_thread->ctx, stack);

	/* Uruchamiamy nowy proces */
	pid = proc_spawn(child_proc);
	if (pid < 0)
	{
		/* TODO: Zwolnij pamięć */
		return pid;
	}

	list_add(&SCHED->current->proc->childs_list, &child_proc->list_siblings);

	if (flags & CLONE_VFORK)
	{
		while(1)
		{
			sched_preempt_disable();

			/* Sprawdzamy czy proces umarl lub wykonal execve */

			if ((child_proc->main->state == THREAD_STATE_ZOMBIE) ||
			    (child_proc->vmspace != SCHED->current->proc->vmspace))
			{
				sched_preempt_enable();
				break;
			}

			/* Blokujemy rodzica */
			SCHED->current->state = THREAD_STATE_BLOCKED;
			SCHED->current->waitpid = pid;
			intr = intr_disable();
			sched_preempt_enable();
			schedule();
			intr_restore(intr);
		}
	}

	return pid;
}

pid_t sys_vfork(void * stack)
{
	return do_fork(stack, 0);
}

pid_t sys_fork(void * stack)
{
	return do_fork(stack, 0);
}
