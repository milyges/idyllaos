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

static tid_t _next_tid = 1;
static SPINLOCK_NEW(_next_tid_lock);

void thread_init(struct thread * thread, struct proc * parent)
{
	memset(thread, 0, sizeof(struct thread));
	list_init(&thread->list);
	list_init(&thread->t_list);
	list_init(&thread->s_list);
	thread->proc = parent;
	thread->state = THREAD_STATE_NEW;
}

tid_t thread_spawn(struct thread * thread)
{
	spinlock_lock(&_next_tid_lock);
	thread->tid = _next_tid++;
	spinlock_unlock(&_next_tid_lock);

	thread->state = THREAD_STATE_NEW;

	/* Dodaj do listy wątków procesu */
	spinlock_lock(&thread->proc->lock);
	list_add(&thread->proc->thread_list, &thread->t_list);
	atomic_inc(&thread->proc->threads);
	spinlock_unlock(&thread->proc->lock);

	thread->timeout = thread->prio;

	/* Dodajemy do kolejki zadań */
	sched_wakeup(thread);

	return thread->tid;
}

void thread_exit(struct thread * thread, int code)
{
	kprintf("thread_exit(%d, %d)\n", thread->tid, code);

	if (thread == thread->proc->main)
	{
		sys_exit(code, thread->proc);

	}
	else
	{
		sched_preempt_disable();

		thread->state = THREAD_STATE_ZOMBIE;

		sched_preempt_enable();
	}

	schedule();
	while(1);
}

void thread_free(struct thread * thread)
{
	ctx_free(&thread->ctx);
	kfree(thread);
}

tid_t kthread_create(void * entry)
{
	struct thread * thread;
	tid_t tid;
	thread = kalloc(sizeof(struct thread));
	thread_init(thread, proc_get_kernel());
	ctx_init(&thread->ctx, entry);

	thread->prio = 5;

	tid = thread_spawn(thread);

	if (tid < 0)
	{
		ctx_free(&thread->ctx);
		kfree(thread);
	}

	return tid;
}
