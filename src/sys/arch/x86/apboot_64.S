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
#include <arch/multiboot.h>
#include <arch/page.h>
#include <arch/cpu.h>

#ifdef __CONFIG_ENABLE_SMP

.extern __kernel_map_level4
.extern kalloc
.extern cpu_new_id
.extern cpu_init

.code16
.global __ap_boot
/* Code used by APs to boot */
__ap_boot:
	cli

	/* Setup DS */
	mov %cs, %ax
	mov %ax, %ds

	/* Load GDT */
	lgdtl temp_gdtr32 - __ap_boot

	/* Enable PMode */
	mov %cr0, %eax
	or $CPU_CR0_PE, %eax
	mov %eax, %cr0

	ljmpl $0x08,$KERNEL_VIRT2PHYS(1f)

.code32
1: /* PMode */
	/* Setup segment registers */
	mov $0x10, %ax
	mov %ax, %ds
	mov %ax, %es
	mov %ax, %fs
	mov %ax, %gs
	mov %ax, %ss

	/* Load Page Map Level 4 */
        movl $KERNEL_VIRT2PHYS(__kernel_map_level4), %ebx
	movl %ebx, %cr3

	/* set PAE bit in cr4 */
	movl %cr4, %eax
	orl $CPU_CR4_PAE, %eax
	movl %eax, %cr4

	/* Enable long mode */
	mov $CPU_MSR_EFER, %ecx
	rdmsr
	orl $CPU_EFER_LME, %eax
	wrmsr

	/* Activate long-mode */
	mov %cr0, %eax
	or $(CPU_CR0_PG | CPU_CR0_WP), %eax
	mov %eax, %cr0

	/* Load temporary GDT to enable full long mode */
	movl $KERNEL_VIRT2PHYS(temp_gdtr), %eax
	lgdt (%eax)

	ljmp $0x08,$KERNEL_VIRT2PHYS(2f)

.code64
2: /* Long mode */
	movabsq $longmodehi, %rax
	jmp *%rax

longmodehi: /* Higger half */
	/* Wait for stack */
	movabsq $bootstack_lock, %rax
	.ap_lock:
		lock btsl $0x1, (%rax)
	jc .ap_lock

	/* Setup boot stack */
	movabsq $(bootstack + PAGE_SIZE), %rax
	movq %rax, %rsp

	/* Allocate new stack */
	movq $PAGE_SIZE, %rdi
	call kalloc

	movq %rax, %rsp
	add $PAGE_SIZE, %rsp

	pushq %rax

	/* Release stack */
	movabsq $bootstack_lock, %rax
	movl $0, (%rax)

	/* Allocate new CPU id */
	call cpu_new_id

	/* Initialize CPU */
	movq %rax, %rdi
	popq %rsi
	call cpu_init

halt:
	hlt
	jmp halt

temp_gdtr32:
	.word 3*8-1
	.long KERNEL_VIRT2PHYS(temp_gdt32)

.data
temp_gdt32:
	.quad 0x0000000000000000
	.quad 0x00cf9a000000ffff
	.quad 0x00cf92000000ffff

temp_gdtr:
	.word 0x17
	.quad KERNEL_VIRT2PHYS(temp_gdt)

temp_gdt:
	/* Null-descriptor */
	.quad 0x0000000000000000
	.quad 0x0020980000000000 /* Code */
	.quad 0x00A0920000000000 /* Data */

bootstack_lock: .long 0

.bss
bootstack: .fill PAGE_SIZE

#endif /* __CONFIG_ENABLE_SMP */
