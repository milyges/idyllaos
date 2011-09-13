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
#include <arch/spinlock.h>
//#include <arch/cpu.h>
#include <kernel/kprintf.h>
#include <kernel/vc.h>
#include <kernel/proc.h>
#include <lib/stdarg.h>
#include <lib/printf.h>

static SPINLOCK_NEW(_kprintf_lock);

static int do_kprintf(unsigned c, void ** ptr)
{
	extern struct vc __vc_boot;
	return vc_putch(c, &__vc_boot);
}

void kprintf(char * fmt, ...)
{
	va_list args;

	if ((*fmt > 0) && (*fmt < 8))
		fmt++;

	sched_preempt_disable();
	spinlock_lock(&_kprintf_lock);
	va_start(args, fmt);
	doprintf(fmt, args, &do_kprintf, NULL);
	va_end(args);
	spinlock_unlock(&_kprintf_lock);
	sched_preempt_enable();
}

