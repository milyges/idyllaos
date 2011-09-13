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
#ifndef __ARCH_STACKTRACE_H
#define __ARCH_STACKTRACE_H

#include <kernel/types.h>

#define STACKTRACE(top) \
	do { \
		reg_t ebp; \
		__asm__ __volatile__ ("mov %%ebp, %0" \
		                      :"=r"(ebp) \
		                      : \
		                     ); \
		do_stacktrace((addr_t)(top), ebp); \
	} while(0)
	
void do_stacktrace(addr_t stacktop, addr_t bp);

#endif /* __ARCH_CTX_H */
