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
#include <arch/intr.h>
#include <arch/cpu.h>
#include <arch/page.h>
#include <arch/exception.h>
#include <arch/irq.h>
#include <arch/apic.h>
#include <kernel/types.h>
#include <kernel/kprintf.h>

static struct idt_desc _idt[IDT_DESCRIPTORS] __attribute__ ((aligned (0x10)));
static struct idtr _idtr;

static void set_idt_dsc(struct idt_desc * dsc, uint16_t sel, addr_t offset, uint8_t acc)
{
	dsc->offset_lo = offset & 0xFFFF;
	dsc->sel = sel;
	dsc->reserved = 0;
	dsc->acc = acc;
	dsc->offset_hi = (offset >> 16) & 0xFFFF;
	#ifdef __X86_64__
	dsc->offset_ext = (offset >> 32) & 0xFFFFFFFF;
	dsc->reserved2 = 0;
	#endif /* __X86_64__ */
}

void idt_load(void)
{
	__asm__ __volatile__ ("lidt (%0)"::"r"(&_idtr));
}


#define INTR_SETUP(x) \
        extern void intr_##x(void); \
        set_idt_dsc(&_idt[x], KERNEL_CODE_SEL, (addr_t)&intr_##x, IDT_PRESENT | IDT_SIZE32 | IDT_TYPE_INTR | IDT_DPL0)

void idt_setup(void)
{
	_idtr.size = IDT_DESCRIPTORS * sizeof(struct idt_desc) - 1;
	_idtr.addr = (addr_t)&_idt;


	/* Wyjątki */
	INTR_SETUP(0x00);
	INTR_SETUP(0x01);
	INTR_SETUP(0x02);
	INTR_SETUP(0x03);
	INTR_SETUP(0x04);
	INTR_SETUP(0x05);
	INTR_SETUP(0x06);
	INTR_SETUP(0x07);
	INTR_SETUP(0x08);
	INTR_SETUP(0x0A);
	INTR_SETUP(0x0B);
	INTR_SETUP(0x0C);
	INTR_SETUP(0x0D);
	INTR_SETUP(0x0E);
	INTR_SETUP(0x10);
	INTR_SETUP(0x11);
	INTR_SETUP(0x12);
	INTR_SETUP(0x13);

	/* IRQs */
	INTR_SETUP(0x20);
	INTR_SETUP(0x21);
	INTR_SETUP(0x22);
	INTR_SETUP(0x23);
	INTR_SETUP(0x24);
	INTR_SETUP(0x25);
	INTR_SETUP(0x26);
	INTR_SETUP(0x27);
	INTR_SETUP(0x28);
	INTR_SETUP(0x29);
	INTR_SETUP(0x2A);
	INTR_SETUP(0x2B);
	INTR_SETUP(0x2C);
	INTR_SETUP(0x2D);
	INTR_SETUP(0x2E);
	INTR_SETUP(0x2F);
	INTR_SETUP(0x30);
	INTR_SETUP(0x31);
	INTR_SETUP(0x32);
	INTR_SETUP(0x33);
	INTR_SETUP(0x34);
	INTR_SETUP(0x35);
	INTR_SETUP(0x36);
	INTR_SETUP(0x37);

#ifdef __CONFIG_ENABLE_APIC
	INTR_SETUP(0xF0);
	INTR_SETUP(0xF1);
	INTR_SETUP(0xF2);
	INTR_SETUP(0xF3);
	INTR_SETUP(0xF4);
	INTR_SETUP(0xF5);
#endif /* __CONFIG_ENABLE_APIC */

#ifdef __CONFIG_ENABLE_SMP
	INTR_SETUP(0xFF);
#endif /* __CONFIG_ENABLE_SMP */

#ifndef __X86_64__
	extern void intr_0x80(void);
	set_idt_dsc(&_idt[0x80], KERNEL_CODE_SEL, (addr_t)&intr_0x80, IDT_PRESENT | IDT_SIZE32 | IDT_TYPE_INTR | IDT_DPL3);
#endif /* __X86_64__ */
}

void intr_dispatcher(struct intr_stack * stack)
{
	extern void syscall32(struct intr_stack * stack);

	if(stack->intr_no < 20)
		do_exception(stack);
	else if ((stack->intr_no >= 0x20) && (stack->intr_no < 0x38))
		irq_dispatcher(stack);
#ifdef __CONFIG_ENABLE_APIC
	else if ((stack->intr_no >= 0xF0) && (stack->intr_no < 0xF6))
		apic_intr_dispatcher(stack);
#endif /* __CONFIG_ENABLE_APIC */
#ifndef __X86_64__
	else if (stack->intr_no == 0x80)
		syscall32(stack);
#endif /* __X86_64__ */
#ifdef __CONFIG_ENABLE_SMP
	else if (stack->intr_no == 0xFF)
		halt();
#endif /* __CONFIG_ENABLE_SMP */
 	else
		kprintf("CPU #%d: Interrupt %d\n", CPU_CURRENT, stack->intr_no);
}

