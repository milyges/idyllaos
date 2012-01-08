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
#ifndef __KERNEL_MUTEX_H
#define __KERNEL_MUTEX_H

#include <arch/spinlock.h>
#include <kernel/wait.h>

struct mutex
{
 int locked;
 spinlock_t lock;
 wait_t wait;
};

#define MUTEX_INIT(name) { 0, SPINLOCK_INIT(), WAIT_INIT(name.wait) }
#define MUTEX_NEW(name)  struct mutex name = MUTEX_INIT(name)

void mutex_init(struct mutex * mutex);
int mutex_lock(struct mutex * mutex);
void mutex_unlock(struct mutex * mutex);
void mutex_dump(struct mutex * mutex);

#endif /* __KERNEL_MUTEX_H */
