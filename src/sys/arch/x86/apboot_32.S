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

.extern __kernel_directory
.extern kalloc
.extern cpu_new_id
.extern cpu_init

.text
.code16
.global __ap_boot
/* Code used by APs to boot */
__ap_boot:
	cli

	/* Ustawiamy segment danych */
	mov %cs, %ax
	mov %ax, %ds

	/* ładujemy GDT */
	lgdtl temp_gdtr - __ap_boot

	/* Włączamy tryb chroniony */
	mov %cr0, %eax
	or $CPU_CR0_PE, %eax
	mov %eax, %cr0

	ljmpl $0x08,$KERNEL_VIRT2PHYS(1f)

.code32
1: /* PMode */

	/* Ustawiamy rejestry segmentowe */
	mov $0x10, %ax
	mov %ax, %ds
	mov %ax, %es
	mov %ax, %fs
	mov %ax, %gs
	mov %ax, %ss

	mov $KERNEL_VIRT2PHYS(__kernel_directory), %eax
	mov %eax, %cr3

	/* Włączamy strony 4MB */
	movl %cr4, %eax
	orl $CPU_CR4_PSE, %eax
	movl %eax, %cr4

	/* Włączamy stronnicowanie */
	mov %cr0, %eax
	or $(CPU_CR0_PG | CPU_CR0_WP), %eax
	mov %eax, %cr0

	ljmp $0x08, $2f
2: /* Higger Half */

	/* Czekamy na tymczasowy stos */
	.ap_lock:
		lock btsl $0x1, bootstack_lock
	jc .ap_lock

	/* Ustawiamy esp */
	movl $(bootstack + PAGE_SIZE), %esp

	/* Alogujemy nowy stos */
	pushl $PAGE_SIZE
	call kalloc

	/* Przechodzimy na nowy stos */
	movl %eax, %esp
	addl $PAGE_SIZE, %esp
	pushl %eax

	/* Zwalniamy tymczasowy stos */
	movl $0, bootstack_lock

	/* Przydzielamy ID dla nowego procesora */
	call cpu_new_id

	/* Inicjujemy procesor */
	pushl %eax
	call cpu_init

	addl $8, %esp
	/* Czekamy na zadania od schedulera... */

halt:
	hlt
	jmp halt

temp_gdtr:
	.word 3*8-1
	.long KERNEL_VIRT2PHYS(temp_gdt)

.data
temp_gdt:
	.quad 0x0000000000000000
	.quad 0x00cf9a000000ffff
	.quad 0x00cf92000000ffff

bootstack_lock: .long 0

.bss
bootstack: .fill PAGE_SIZE

#endif /* __CONFIG_ENABLE_SMP */
