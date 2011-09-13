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
#ifndef __ARCH_ATOMIC_H
#define __ARCH_ATOMIC_H 

#ifdef __CONFIG_ENABLE_SMP
#define LOCK_PREFIX "lock; "
#else /* __CONFIG_ENABLE_SMP */
#define LOCK_PREFIX ""
#endif /* __CONFIG_ENABLE_SMP */

typedef struct
{
 volatile int counter;
} atomic_t;

#define ATOMIC_INIT(x)  { (x) }

#define atomic_get(v)   ((int)(v)->counter)
#define atomic_set(v,x) ((v)->counter = x)

static inline void atomic_add(int i, atomic_t * v)
{
 __asm__ __volatile__(LOCK_PREFIX "addl %1,%0"
                      :"=m" (v->counter)
                      :"ir" (i), "m" (v->counter)
                     );
}

static inline void atomic_sub(int i, atomic_t * v)
{
 __asm__ __volatile__(LOCK_PREFIX "subl %1,%0"
                      :"=m" (v->counter)
                      :"ir" (i), "m" (v->counter)
                     );
}

static inline void atomic_inc(atomic_t * v)
{
 __asm__ __volatile__(LOCK_PREFIX "incl %0"
                      :"=m" (v->counter)
                      :"m" (v->counter)
                     );
}

static inline void atomic_dec(atomic_t * v)
{
 __asm__ __volatile__(LOCK_PREFIX "decl %0"
                      :"=m" (v->counter)
                      :"m" (v->counter)
                     );
}

static inline int atomic_dec_and_test(atomic_t * v)
{
 unsigned char c;

 __asm__ __volatile__ (LOCK_PREFIX "decl %0; sete %1"
                       :"=m" (v->counter), "=qm" (c)
                       :"m" (v->counter) : "memory"
		      );
 return c != 0;
}

static inline int atomic_test_and_set(atomic_t * v)
{
 volatile int old = 1;

 __asm__ __volatile__ (LOCK_PREFIX "btsl $1, %1\n"
                       "sbbl %0, %0"
                       :"=r"(old), "=m"(v->counter)
                       :
		       :"memory"
                      );
 return old;
}

static inline void atomic_clear(atomic_t * v)
{
 __asm__ __volatile__ ("movl $0, %0"
                       :"=m"(v->counter)
		      );
}

#endif /* __ARCH_ATOMIC_H */