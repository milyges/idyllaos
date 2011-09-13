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
#include <arch/gdt.h>

#define intr(x) \
.global intr_##x ; \
intr_##x: \
	cli ; \
	pushl $0 ; \
	pushl $x ; \
	jmp _intr_handler


#define intr_E(x) \
.global intr_##x ; \
intr_##x: \
	cli ; \
	pushl $x ; \
	jmp _intr_handler

/* Wyjątki */
intr(0x00)
intr(0x01)
intr(0x02)
intr(0x03)
intr(0x04)
intr(0x05)
intr(0x06)
intr(0x07)
intr_E(0x08)
intr_E(0x0A)
intr_E(0x0B)
intr_E(0x0C)
intr_E(0x0D)
intr_E(0x0E)
intr(0x10)
intr_E(0x11)
intr(0x12)
intr(0x13)

/* IRQs */
intr(0x20)
intr(0x21)
intr(0x22)
intr(0x23)
intr(0x24)
intr(0x25)
intr(0x26)
intr(0x27)
intr(0x28)
intr(0x29)
intr(0x2A)
intr(0x2B)
intr(0x2C)
intr(0x2D)
intr(0x2E)
intr(0x2F)
intr(0x30)
intr(0x31)
intr(0x32)
intr(0x33)
intr(0x34)
intr(0x35)
intr(0x36)
intr(0x37)

#ifdef __CONFIG_ENABLE_APIC
intr(0xF0)
intr(0xF1)
intr(0xF2)
intr(0xF3)
intr(0xF4)
intr(0xF5)
#endif /* __CONFIG_ENABLE_APIC */

#ifdef __CONFIG_ENABLE_SMP
intr(0xFF)
#endif /* __CONFIG_ENABLE_SMP */

/* Przerwanie systemowe */
intr(0x80)

.extern intr_dispatcher

_intr_handler:
	/* Clear direction flag */
	cld

	/* Push all registers */
	pushal
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs

	/* Setup segment registers */
	movl $KERNEL_DATA_SEL, %eax
	movl %eax, %ds
	movl %eax, %es
	movl %eax, %fs
	movl %eax, %gs

	/* Save stack pointer */
	pushl %esp
	/* Call C code */
	call intr_dispatcher
	/* Skip saved esp */
	addl $4, %esp

	/* Pop all registers */
	popl %gs
	popl %fs
	popl %es
	popl %ds
	popal

	/* Skip saved error code */
	add $8, %esp

	/* Return */
	iret

