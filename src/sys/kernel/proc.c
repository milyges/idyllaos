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
#include <arch/atomic.h>
#include <arch/cpu.h>
#include <arch/spinlock.h>
#include <arch/ctx.h>
#include <kernel/types.h>
#include <kernel/proc.h>
#include <kernel/kprintf.h>
#include <kernel/debug.h>
#include <mm/heap.h>
#include <lib/string.h>
#include <lib/list.h>
#include <lib/errno.h>
#include <lib/stddef.h>

static LIST_NEW(_proc_list);
static SPINLOCK_NEW(_proc_list_lock);
static pid_t _next_pid = 1;
static SPINLOCK_NEW(_next_pid_lock);

struct proc * get_proc(pid_t pid)
{
	struct proc * proc;
	
	sched_preempt_disable();
	spinlock_lock(&_proc_list_lock);
	LIST_FOREACH(&_proc_list, proc)
	{
		if (proc->pid == pid)
		{
			spinlock_unlock(&_proc_list_lock);
			sched_preempt_enable();
			return proc;
		}
	}
	spinlock_unlock(&_proc_list_lock);
	sched_preempt_enable();
	return NULL;
}

void proctab_dump(void)
{
	struct proc * proc;
	sched_preempt_disable();
	spinlock_lock(&_proc_list_lock);
	kprintf("\n   pid | parent | state\n");
	LIST_FOREACH(&_proc_list, proc)
	{
		kprintf(" %5d |  %5d | %d\n", proc->pid, proc->parent->pid, proc->main->state);
	}
	spinlock_unlock(&_proc_list_lock);
	sched_preempt_enable();
}

void proc_init(struct proc * proc, struct proc * parent)
{
	memset(proc, 0, sizeof(struct proc));
	list_init(&proc->list);
	list_init(&proc->list_siblings);
	list_init(&proc->thread_list);
	list_init(&proc->childs_list);

	spinlock_init(&proc->lock);
	proc->parent = parent;
}

pid_t proc_spawn(struct proc * proc)
{
	tid_t err;
	spinlock_lock(&_next_pid_lock);
	proc->pid = _next_pid++;
	spinlock_unlock(&_next_pid_lock);

	/* Dodajemy proces do listy */
	spinlock_lock(&_proc_list_lock);
	list_add(&_proc_list, &proc->list);
	spinlock_unlock(&_proc_list_lock);

	/* Uruchamiamy główny wątek */
	err = thread_spawn(proc->main);
	if (err < 0)
	{
		/* Błąd */
		spinlock_lock(&_proc_list_lock);
		list_remove(&proc->list);
		spinlock_unlock(&_proc_list_lock);
		return err;
	}

	return proc->pid;
}

void proc_free(struct proc * proc)
{
	/* Usuwamy z listy procesów */
	spinlock_lock(&_proc_list_lock);
	list_remove(&proc->list);
	spinlock_unlock(&_proc_list_lock);

	/* Usuwamy z listy rodzenstwa */
	list_remove(&proc->list_siblings);

	/* Kasujemy glowny watek */
	thread_free(proc->main);

	/* Kasujemy proces */
	kfree(proc);
}

int sys_waitpid(pid_t pid, int * status, int options)
{
	struct proc * proc;
	list_t * iter;
	int ret, intr;

	if (LIST_IS_EMPTY(&SCHED->current->proc->childs_list))
		return -EAGAIN;

	//kprintf("wait(%d, %x, %d)\n", pid, status, options);

	if ((pid > 0) || (pid == -1))
	{
		while(1)
		{
			sched_preempt_disable();
			/* Przeglądamy listę dzieci, szukając martwego */
			LIST_FOREACH(&SCHED->current->proc->childs_list, iter)
			{
				proc = (struct proc *)((addr_t)iter - offsetof(struct proc, list_siblings));
				if ((proc->main->state == THREAD_STATE_ZOMBIE) && ((proc->pid == pid) || (pid == -1)))
				{
					sched_preempt_enable();
					if (status)
						*status = proc->main->exit_code;
					ret = proc->pid;
					proc_free(proc);
					return ret;
				}
			}

			if (options & WNOHANG)
			{
				sched_preempt_enable();
				return -EAGAIN;
			}

			SCHED->current->state = THREAD_STATE_WAITING;
			SCHED->current->waitpid = pid;
			intr = intr_disable();
			sched_preempt_enable();
			schedule();
			intr_restore(intr);
		}
	}

	return -ENOSYS;
}
