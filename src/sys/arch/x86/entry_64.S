/*
 * Idylla Operating System
 * Copyright (C) 2009-2010  Idylla Operating System Team
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
#include <arch/gdt.h>

/* Nagłówek multiboot */
.section .header
.align 4
mbheader:
        .long MB_KERNEL_MAGIC
	.long MB_FLAGS_MEMINFO | MB_FLAGS_MODALIGN | MB_FLAGS_KLUDGE
	.long MB_CHECKSUM(MB_FLAGS_MEMINFO | MB_FLAGS_MODALIGN | MB_FLAGS_KLUDGE)
	.long KERNEL_VIRT2PHYS(mbheader)
	.long KERNEL_VIRT2PHYS(_text)
	.long KERNEL_VIRT2PHYS(_edata)
	.long KERNEL_VIRT2PHYS(_end)
	.long KERNEL_VIRT2PHYS(_start)

.text

.code32
.global _start
.extern __kernel_tables
.extern __kernel_directory
.extern __kernel_dir_pointers
.extern __kernel_map_level4
.extern multiboot_init
.extern kinit
.extern multiboot_load_modules
.extern start_init

_start: /* Bootloader oddaje kontrolę tutaj */

	/* Ustawiamy stos  */
	movl $KERNEL_VIRT2PHYS(bootstack + __CONFIG_KSTACK_SIZE), %esp

	/* Ustawiamy flagi procesora */
	pushl $0x2
	popfl

	/* Zapisujemy informacje od bootloadera na stosie */
	pushl $0
	pushl %ebx
	pushl $0
	pushl %eax


	/* Inicjujemy wstępnie stronnicowanie. Mapujemy pierwsze 2GiB pamięci fizycznej */
	xorl %eax, %eax
	movl $KERNEL_VIRT2PHYS(__kernel_directory), %ebx
	movl $0x200, %ecx
	.fill_directory:
		movl %eax, (%ebx)
		orl $(PG_FLAG_PRESENT | PG_FLAG_RW | PG_FLAG_SIZE), (%ebx)
		add $0x08, %ebx
		add $(1024 * 1024 * 2), %eax
	loop .fill_directory

	/* Ustawiamy wskazniki na katalog stron */
	movl $KERNEL_VIRT2PHYS(__kernel_dir_pointers), %ebx
	movl $KERNEL_VIRT2PHYS(__kernel_directory), (%ebx)
	orl $(PG_FLAG_PRESENT | PG_FLAG_RW), (%ebx)

	/* Ladujemy rejestr cr3 */
        movl $KERNEL_VIRT2PHYS(__kernel_map_level4), %ebx
	movl %ebx, %cr3

	/* Ustawiamy "Page Map Level 4" */
	movl $KERNEL_VIRT2PHYS(__kernel_dir_pointers), (%ebx)
	orl $(PG_FLAG_PRESENT | PG_FLAG_RW), (%ebx)

	movl $KERNEL_VIRT2PHYS(__kernel_map_level4 + (PGML4_ENTRY(LOWMEM_START) * 8)), %ebx
	movl $KERNEL_VIRT2PHYS(__kernel_dir_pointers), (%ebx)
	orl $(PG_FLAG_PRESENT | PG_FLAG_RW), (%ebx)

	/* Włączamy PAE */
	movl %cr4, %eax
	orl $CPU_CR4_PAE, %eax
	movl %eax, %cr4

	/* Włączamy long-mode */
	mov $CPU_MSR_EFER, %ecx
	rdmsr
	orl $CPU_EFER_LME, %eax
	wrmsr

	/* Aktywujemy long-mode */
	mov %cr0, %eax
	or $(CPU_CR0_PG | CPU_CR0_WP), %eax
	mov %eax, %cr0

	/* Wczytujemy tymczasowe GDT aby w pełni włączyć tryb 64b */
	movl $KERNEL_VIRT2PHYS(tempgdtr), %eax
	lgdt (%eax)

 	ljmp $0x08, $KERNEL_VIRT2PHYS(_start64_low)
halt32:
	hlt
	jmp halt32

.code64
_start64_low:
	/* Procesor jest w trybie 64b, skaczemy pod wirtualny adres kernela */
	movabsq $_start64, %rax
	jmp *%rax

_start64:
	/* Przeładowywujemy rejestry segmentowe */
	mov $0x10, %ax
	mov %ax, %ds
	mov %ax, %fs
	mov %ax, %gs
	mov %ax, %es
	mov %ax, %ss

	/* Przenosimy stos pod wysoki adres */
	mov $LOWMEM_START, %rax
	add %rax, %rsp

	/* Inicjujemy konsolę VGA */
	call vga_init

	/* Przetwarzamy informacje otrzymane od bootloader */
	popq %rdi
	popq %rsi
	call multiboot_init

	/* Inicjujemy BSP (Boot Strap Processor) */
	xor %rdi, %rdi
	movabsq $bootstack, %rsi
	call cpu_init

	/* Inicjujemy częsci kernela niezależne od sprzętu */
	call kinit

	/* Ładujemy moduły wczytane do pamięci przez bootloader */
	call multiboot_load_modules

	/* Odmapowywujemy niepotrzebną pamięć */

	/* Tworzymy proces dla programu INIT */
	call start_init

halt64:
	hlt
	jmp halt64


.global __ctx_switch
__ctx_switch: /* Funkcja wykonuje zmianę kontekstu */
	pushq %rbp /* Dla zgodności z 32b */

	/* Zapisujemy flagi procesora */
	pushfq

	/* Zapisujemy rejestry */
	pushq %rax
	pushq %rcx
	pushq %rdx
	pushq %rbx
	pushq %rsi
	pushq %rdi

	pushq %r8
	pushq %r9
	pushq %r10
	pushq %r11
	pushq %r12
	pushq %r13
	pushq %r14
	pushq %r15

	/* Zapisujemy stary stos */
	movq %rsp, (%rdi)

	/* Ladujemy nowy stos */
	movq (%rsi), %rsp

	/* Odblokowywujemy oldctx i newctx */
	movl $0, 16(%rdi)
	movl $0, 16(%rsi)

	/* Przywracamy rejestry */
	popq %r15
	popq %r14
	popq %r13
	popq %r12
	popq %r11
	popq %r10
	popq %r9
	popq %r8

	popq %rdi
	popq %rsi
	popq %rbx
	popq %rdx
	popq %rcx
	popq %rax

	/* Przywracamy flagi */
	popfq

	popq %rbp

	ret

.global gdt_flush
gdt_flush:
	mov $KERNEL_DATA_SEL, %ax
	mov %ax, %ds
	mov %ax, %es
	mov %ax, %fs
	mov %ax, %gs
	mov %ax, %ss

//	movabsq $.flush, %rax

//	pushq $KERNEL_CODE_SEL
//	pushq %rax
//	retf

.flush:
	ret

.data
tempgdtr:
	.word 0x17
	.quad KERNEL_VIRT2PHYS(tempgdt)

tempgdt:
	/* Null-descriptor */
	.quad 0x0000000000000000
	.quad 0x0020980000000000 /* Code */
	.quad 0x00A0920000000000 /* Data */

.bss
bootstack: .fill __CONFIG_KSTACK_SIZE
