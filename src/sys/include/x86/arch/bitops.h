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
#ifndef __ARCH_BITOPS_H
#define __ARCH_BITOPS_H 

#ifndef __KERNEL_BITOPS_H
#error "You can't use arch/bitops.h! Use kernel/bitops.h instead."
#endif /* __KERNEL_BITOPS_H */

#define __HAVE_SET_BIT
static inline void set_bit(unsigned long * base, int idx)
{
	__asm__ __volatile__ ("bts %1,%0"
	                      :"=m"(*base)
	                      :"Ir"(idx)
	                      :"memory");
}

#define __HAVE_CLEAR_BIT
static inline void clear_bit(unsigned long * base, int idx)
{
	__asm__ __volatile__ ("btr %1,%0"
	                      :"=m"(*base)
	                      :"Ir"(idx));
}

#define __HAVE_TOOGLE_BIT
static inline void toogle_bit(unsigned long * base, int idx)
{
	__asm__ __volatile__ ("btc %1,%0"
	                      :"=m"(*base)
	                      :"Ir"(idx));
}

#define __HAVE_TEST_BIT
static inline int test_bit(unsigned long base, int idx)
{
	int oldbit;
	__asm__ __volatile__ ("bt %2,%1\n"
	                      "sbb %0,%0"
	                      :"=r"(oldbit)
	                      :"m"(base), "Ir"(idx));
	return oldbit;
}

#define __HAVE_FFC_BIT
static inline unsigned long ffz_bit(unsigned long base)
{
	__asm__ __volatile__ ("bsf %1,%0"
	                      :"=r"(base)
	                      :"r"(~base));
	return base;
}

#define __HAVE_FFS_BIT
static inline int ffs_bit(unsigned long base)
{
	__asm__ __volatile__ ("bsf %1,%0"
	                      :"=r"(base)
	                      :"r"(base));
	return base;
}

#endif /* __ARCH_BITOPS_H */