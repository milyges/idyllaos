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
#ifndef __KERNEL_SEM_H
#define __KERNEL_SEM_H

#include <arch/spinlock.h>
#include <kernel/wait.h>

struct sem
{
	int value;
	wait_t wait;
	spinlock_t lock;
};

void sem_init(struct sem * sem, int val);
void sem_acquire(struct sem * sem);
void sem_release(struct sem * sem);

#endif /* __KERNEL_SEM_H */
 
