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
#include <arch/asm.h>
#include <kernel/types.h>
#include <kernel/panic.h>
#include <kernel/vc.h>
#include <lib/stdarg.h>
#include <lib/printf.h>

static int do_panic(unsigned c, void ** ptr)
{
	extern struct vc __vc_boot;
	return vc_putch(c, &__vc_boot);
}

static void panic_puts(char * s)
{
	extern struct vc __vc_boot;
	while(*s)
		vc_putch(*s++, &__vc_boot);
}

void panic(char * fmt, ...)
{
	va_list args;
	panic_puts("\e[37;1mKernel panic: ");
	va_start(args, fmt);
	doprintf(fmt, args, &do_panic, NULL);
	va_end(args);
	panic_puts("\n");
	halt();
	while(1); /* To i tak się nie powinno wykonać ale... */
}
