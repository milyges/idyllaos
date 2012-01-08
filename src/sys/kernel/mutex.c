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
#include <kernel/mutex.h>
#include <kernel/debug.h>

void mutex_init(struct mutex * mutex)
{
	mutex->locked = 0;
	spinlock_init(&mutex->lock);
	wait_init(&mutex->wait);
}

int mutex_lock(struct mutex * mutex)
{
	spinlock_lock(&mutex->lock);
	/* Czekamy na zwolnienie */
	while(mutex->locked)
		wait_sleep(&mutex->wait, &mutex->lock);
	/* Ustawiamy stan zablokowany */
	mutex->locked = 1;
	spinlock_unlock(&mutex->lock);
	return 0;
}

void mutex_unlock(struct mutex * mutex)
{
	spinlock_lock(&mutex->lock);
	mutex->locked = 0;
	wait_wakeup(&mutex->wait);
	spinlock_unlock(&mutex->lock);
}

void mutex_dump(struct mutex * mutex)
{
	spinlock_lock(&mutex->lock);
	kprintf("mutex 0x%X, locked = %d\n", mutex, mutex->locked);
	spinlock_unlock(&mutex->lock);
}
