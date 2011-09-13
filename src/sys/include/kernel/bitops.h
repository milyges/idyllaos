/*
 * Idylla Operating System
 * Copyright (C) 2009  Idylla Operating System Team
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
#ifndef __KERNEL_BITOPS_H
#define __KERNEL_BITOPS_H

/* Ladujemy operacje zooptymalizowane do danej architektóry */
#include <arch/bitops.h>

#ifndef __HAVE_SET_BIT
static inline void set_bit(unsigned long * base, int idx)
{
	(*base) |= 1 << idx;
}
#endif /* __HAVE_SET_BIT */

#ifndef __HAVE_CLEAR_BIT
static inline void clear_bit(unsigned long * base, int idx)
{
	(*base) &= ~(1 << idx);
}
#endif /* __HAVE_CLEAR_BIT */

#ifndef __HAVE_TOOGLE_BIT
static inline void toogle_bit(unsigned long * base, int idx)
{
	
}
#endif /* __HAVE_TOOGLE_BIT */

#ifndef __HAVE_TEST_BIT
static inline int test_bit(unsigned long base, int idx)
{
	return (base & (1 << idx)) ? 0 : -1;
}
#endif /* __HAVE_TEST_BIT */

/* Find First Zero */
#ifndef __HAVE_FFC_BIT
static inline int ffz_bit(unsigned long base)
{
	int i;
	for(i=0;i<sizeof(unsigned long) * 8;i++)
	{
		if (!(base & (0x1 << i)))
			return i;		
	}
	return -1;
}
#endif /* __HAVE_FFZ_BIT */

/* Find First Set */
#ifndef __HAVE_FFS_BIT
static inline int ffs_bit(unsigned long base)
{
	int i;
	for(i=0;i<sizeof(unsigned long) * 8;i++)
	{
		if (base & (0x1 << i))
			return i;		
	}
	return -1;
}
#endif /* __HAVE_FFS_BIT */

#endif /* __KERNEL_BITOPTS_H */
