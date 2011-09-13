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
#include <kernel/wait.h>
#include <kernel/proc.h>

void wait_sleep(wait_t * w, spinlock_t * lock)
{
	unsigned intr;

	spinlock_lock(&w->lock);
	sched_preempt_disable();

	SCHED->current->state = THREAD_STATE_WAITING;
	list_add(w->queue.prev, &SCHED->current->list);
	spinlock_unlock(&w->lock);

	if (lock != NULL)
		spinlock_unlock(lock);

	intr = intr_disable();
	sched_preempt_enable();
	schedule();
	intr_restore(intr);

	if (lock != NULL)
		spinlock_lock(lock);
}

void wait_wakeup(wait_t * w)
{
	spinlock_lock(&w->lock);
	if (!LIST_IS_EMPTY(&w->queue))
		sched_wakeup((struct thread *)w->queue.next);
	spinlock_unlock(&w->lock);
}

void wait_wakeup_all(wait_t * w)
{
	spinlock_lock(&w->lock);
	while(!LIST_IS_EMPTY(&w->queue))
		sched_wakeup((struct thread *)w->queue.next);
	spinlock_unlock(&w->lock);
}

void wait_init(wait_t * w)
{
	list_init(&w->queue);
	spinlock_init(&w->lock);
}
