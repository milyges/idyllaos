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
#include <arch/pit.h>
#include <arch/intr.h>
#include <arch/asm.h>
#include <arch/irq.h>
#include <kernel/types.h>
#include <kernel/kprintf.h>
#include <kernel/proc.h>

volatile uint64_t __ticks;
static unsigned _do_schedule = 1;
extern time_t __systime;

void pit_do_schedule(unsigned enabled)
{
	_do_schedule = enabled;
}

void pit_delay(int ms)
{
	volatile uint64_t end;
	ms = ms / (1000 / PIT_HZ);
	end = __ticks + ms;
	while(end > __ticks) ;
}

static void pit_handler(int irq)
{
	__ticks++;
	if (!(__ticks % PIT_HZ))
		__systime++;

	if (_do_schedule)
		schedule();
}

int sys_uptime(time_t * uptime)
{
	*uptime = __ticks / PIT_HZ;
}

void pit_init(void)
{
	uint32_t val = 1193180 / PIT_HZ;
	__ticks = 0;
	outportb(PIT_COMMAND, 0x36);
	outportb(PIT_CHANNEL0, val & 0xFF);
	outportb(PIT_CHANNEL0, (val >> 8) & 0xFF);
	irq_register(PIT_IRQ, pit_handler);
	kprintf(KERN_INFO"PIT: Timer initialized (%uHz)\n", PIT_HZ);
}
