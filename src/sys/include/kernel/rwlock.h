/*
 * Idylla Operating System
 * Copyright (C) 2009-2012 Idylla Operating System Team
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
#ifndef __KERNEL_RWLOCK_H
#define __KERNEL_RWLOCK_H

#include <arch/spinlock.h>
#include <kernel/wait.h>

struct rwlock
{
	int active; /* Liczba aktywnych czytelnikow, lub -1 gdy pisarz */
	
	int wr_count; /* ilosc oczekujacych pisarzy */
	int rd_count; /* ilosc oczekujacych czytelnikow */
	spinlock_t lock;
	
	wait_t wr_wait;
	wait_t rd_wait;
};

void rwlock_init(struct rwlock * rw);
void rwlock_lock_shared(struct rwlock * rw);
void rwlock_unlock_shared(struct rwlock * rw);
void rwlock_lock_exclusive(struct rwlock * rw);
void rwlock_unlock_exclusive(struct rwlock * rw);
void rwlock_upgrade(struct rwlock * rw);
void rwlock_downgrade(struct rwlock * rw);

#endif /* __KERNEL_SEM_H */
 
