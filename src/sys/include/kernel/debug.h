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
#ifndef __KERNEL_DEBUG_H
#define __KERNEL_DEBUG_H

#include <kernel/kprintf.h>

#define ASSERT(x) \
	do { \
		if (!(x)) \
			kprintf(KERN_CRIT "Assertion failed in " __FILE__ " on line %d\n",  __LINE__); \
	} while(0)

#ifdef __CONFIG_DEBUG_TODO
#define TODO(x...) \
	do { \
		kprintf(KERN_INFO __FILE__ ": %s(): TODO: ", __func__); \
		kprintf(KERN_INFO x); \
		kprintf(KERN_INFO "\n"); \
	} while(0)
#else /* __CONFIG_DEBUG_TODO */
#define TODO(x...)
#endif /* __CONFIG_DEBUG_TODO */

#endif /* __KERNEL_DEBUG_H */