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
#include <mm/vmm.h>
#include <lib/string.h>
#include <lib/list.h>

struct scheduler * __scheds[CPUS_MAX];
static LIST_NEW(_ready_queue);
static SPINLOCK_NEW(_ready_queue_lock);
static LIST_NEW(_sleep_queue);
static SPINLOCK_NEW(_sleep_queue_lock);

/* Proces kernela (rodzic wątków IDLE, nie widoczny normalnie) */
static struct proc _kernel_proc =
{
	.pid = 0,
	.uid = 0,
	.gid = 0,
	.euid = 0,
	.egid = 0,
	.thread_list = LIST_INIT(_kernel_proc.thread_list),
	.threads = ATOMIC_INIT(0),
	.main = NULL,
	.parent = &_kernel_proc,
	.lock = SPINLOCK_INIT()
};

struct proc * proc_get_kernel(void)
{
	return &_kernel_proc;
}

void sched_wakeup(struct thread * thread)
{
	list_remove(&thread->list);
	thread->state = THREAD_STATE_READY;
	sched_preempt_disable();
	spinlock_lock(&_ready_queue_lock);
	list_add(&_ready_queue, &thread->list);
	spinlock_unlock(&_ready_queue_lock);
	sched_preempt_enable();
}

void schedule(void)
{
	struct thread * old;

	ASSERT(intr_disable() == 0);

	if (atomic_get(&SCHED->preempt_disabled) > 0)
		return;

	/* Jeżeli aktualne zadanie jest uruchomione */
	if (SCHED->current->state == THREAD_STATE_RUNNING)
	{
		if (SCHED->current->timeout-- > 0) /* Zmniejszamy i sprawdzamy czas do wysyłaszczenia */
     			return;

		/* Zmieniamy stan na gotowe */
		SCHED->current->state = THREAD_STATE_READY;
	}


	old = SCHED->current;

	spinlock_lock(&_ready_queue_lock);

	/* Jeżeli aktualne zadanie nie jest wątkiem IDLE oraz ma stan "Gotowe do wykonania", dodajemy do kolejki oczekujących */
	if ((old->tid > 0) && (old->state == THREAD_STATE_READY))
		list_add(_ready_queue.prev, &old->list);

	/* Jeżeli lista oczekujących na wykonanie jest pusta, uruchamiamy wątek bezczynności (IDLE) */
	if (LIST_IS_EMPTY(&_ready_queue))
	{
		SCHED->current = SCHED->idle;
		SCHED->current->timeout = 0;

	}
	else /* W przeciwnym wypadku wybieramy pierwszy wątek z kolejki */
	{
		SCHED->current = (struct thread *)_ready_queue.next;
		list_remove(&SCHED->current->list);
		SCHED->current->timeout = SCHED->current->prio;
	}

	/* Zmieniamy stan na uruchomione */
	SCHED->current->state = THREAD_STATE_RUNNING;

	spinlock_unlock(&_ready_queue_lock);

	/* Jeżeli musimy, zmieniamy proces */
	if (old != SCHED->current)
	{
		SCHED->ctxsw++; /* Aktualizacja statystyk */

		/* Przestrzeń adresową zmieniamy tylko przy zmianie procesu */
		if(old->proc->vmspace != SCHED->current->proc->vmspace)
			vm_space_switch(SCHED->current->proc->vmspace);

		/* Zmieniamy kontekst */
		ctx_switch(&old->ctx, &SCHED->current->ctx);
	}
	
	do_signals(SCHED->current->proc);
}

void sched_init(int cpuid)
{
	struct scheduler * sched;
	struct thread * thread;

	thread = kalloc(sizeof(struct thread));
	memset(thread, 0, sizeof(struct thread));
	list_init(&thread->list);
	list_init(&thread->t_list);
	list_init(&thread->s_list);
	
	thread->proc = &_kernel_proc;
	thread->state = THREAD_STATE_RUNNING;
	thread->timeout = 1;

	list_add(&_kernel_proc.thread_list, &thread->t_list);
	atomic_inc(&_kernel_proc.threads);

	sched = kalloc(sizeof(struct scheduler));
	memset(sched, 0, sizeof(struct scheduler));

	sched->cpuid = cpuid;
	sched->idle = thread;
	sched->current = thread;

	/* Wywyłaszczenie domyślnie wyłączone */
	atomic_set(&sched->preempt_disabled, 1);

	__scheds[cpuid] = sched;
}
