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
#ifndef __ARCH_SPINLOCK_H
#define __ARCH_SPINLOCK_H 

#ifndef __ASM__

#include <arch/atomic.h>

typedef struct spinlock
{
 atomic_t locked;
} spinlock_t;

#define SPINLOCK_INIT()     { ATOMIC_INIT(0) }
#define SPINLOCK_NEW(name)  spinlock_t name = SPINLOCK_INIT()

static inline void spinlock_init(spinlock_t * slock)
{
 atomic_set(&slock->locked, 0);
}

static inline int spinlock_trylock(spinlock_t * slock)
{
 if (atomic_test_and_set(&slock->locked) != 0)
    return -1;
 else
    return 0;
}

static inline void spinlock_lock(spinlock_t * slock)
{
 while(atomic_test_and_set(&slock->locked) != 0)
 {
  while(atomic_get(&slock->locked) != 0);
 }
}

static inline void spinlock_unlock(spinlock_t * slock)
{
 atomic_clear(&slock->locked);
}

#endif /* __ASM__ */

#endif /* __ARCH_SPINLOCK_H */