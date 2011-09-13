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
#include <kernel/wait.h>
#include <kernel/sem.h>

void sem_init(struct sem * sem, int val)
{
	spinlock_init(&sem->lock);
	wait_init(&sem->wait);
	sem->value = val;	
}

void sem_acquire(struct sem * sem)
{
	spinlock_lock(&sem->lock);
	
	/* Dopóki semafor jest opuszczony */
	while(sem->value <= 0)
		wait_sleep(&sem->wait, &sem->lock); /* Zasnij */
		
 	/* Opuszczamy semafor */
 	sem->value--;
 	spinlock_unlock(&sem->lock);
}

void sem_release(struct sem * sem)
{
	spinlock_lock(&sem->lock);
	/* Podnieś semafor */
	sem->value++;	
	/* Jeżeli wartośc jest więksa od 0, budzimy jeden proces */
	if (sem->value > 0)
		wait_wakeup(&sem->wait);
	spinlock_unlock(&sem->lock);
}
