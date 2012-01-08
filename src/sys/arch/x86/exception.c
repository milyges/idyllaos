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
#include <arch/cpu.h>
#include <arch/exception.h>
#include <arch/cpu.h>
#include <arch/asm.h>
#include <arch/page.h>
#include <arch/stacktrace.h>
#include <kernel/panic.h>
#include <kernel/kprintf.h>
#include <kernel/proc.h>
#include <mm/vmm.h>

static char * _exc_table[] = {
	"Divide by zero",
	"Debug",
	"Non Maskable Interrupt",
	"Breakpoint",
	"Overflow",
	"Bound Rande",
	"Invalid Opcode",
	"Device Not Avaiable",
	"Double Fault",
	"Coprocessor Segment Overrun",
	"Invalid TSS",
	"Segment Not Present",
	"Stack",
	"General Protection",
	"Page Fault",
	"Reserved",
	"x87 FPEP",
	"Aligment Check",
	"Machine Check",
	"SIMF Floating Point"
};

void dump_registers(struct intr_stack * regs)
{
#ifdef __x86_64__
#define RP "r"
#else /* __X86_64__ */
#define RP "e"
#endif /* __X86_64__ */

	kprintf("cs:  0x%04p\t"RP"ip: 0x%p\t"RP"flags: 0x%p\n", regs->cs, regs->ip, regs->flags);
	kprintf("ds:  0x%04p\tes:  0x%04p\tfs:  0x%04p\tgs:  0x%04p\n", regs->ds, regs->es, regs->fs, regs->gs);
	if (regs->cs & 0x03)
		kprintf("ss:  0x%04p\t"RP"sp: 0x%04p\n", regs->ss, regs->sp);
	kprintf(RP"ax: 0x%p\t"RP"bx: 0x%p\n", regs->ax, regs->bx);
	kprintf(RP"cx: 0x%p\t"RP"dx: 0x%p\n", regs->cx, regs->dx);
	kprintf(RP"si: 0x%p\t"RP"di: 0x%p\n", regs->si, regs->di);
	kprintf("cr0: 0x%08p\tcr2: 0x%p\tcr3: 0x%p\n", read_cr0(), read_cr2(), read_cr3());
	kprintf("Error code: 0x%p\n", regs->err_code);
}

void do_exception(struct intr_stack * regs)
{
	int err;

	if (regs->intr_no == 14)
	{
		intr_enable();
		//kprintf("eip=%x\n", regs->ip);
		err = vm_pagefault(read_cr2(), regs->err_code & PGFAULT_WRITE);
		intr_disable();

		if (!err)
			return;
	}

	if (SCHED->current->ctx.stack)
		STACKTRACE(SCHED->current->ctx.stack);

	dump_registers(regs);

	if (SCHED->current->tid > 0)
	{
		kprintf("%s exception in process %d, thread %d (0x%X:0x%p)\n",  _exc_table[regs->intr_no], SCHED->current->proc->pid, SCHED->current->tid, regs->cs, regs->ip);
		//thread_exit(SCHED->current, 1);
	}
	panic("CPU #%d: Unhandled exception #%d (%s)", CPU_CURRENT, regs->intr_no, _exc_table[regs->intr_no]);
}
