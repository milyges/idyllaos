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
#ifndef __KERNEL_WAIT_H
#define __KERNEL_WAIT_H

#include <lib/list.h>
#include <arch/spinlock.h>

typedef struct
{
	list_t queue;
	spinlock_t lock;
} wait_t;

#define WAIT_INIT(name) { LIST_INIT(name.queue), SPINLOCK_INIT() }
#define WAIT_NEW(name)  wait_t name = WAIT_INIT(name)

void wait_sleep(wait_t * w, spinlock_t * lock);
void wait_wakeup(wait_t * w);
void wait_wakeup_all(wait_t * w);
void wait_init(wait_t * w);

#endif /* __KERNEL_WAIT_H */
