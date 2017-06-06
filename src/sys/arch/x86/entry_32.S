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
	.long MB_FLAGS_MEMINFO | MB_FLAGS_MODALIGN
	.long MB_CHECKSUM(MB_FLAGS_MEMINFO | MB_FLAGS_MODALIGN)

.text

.global _start
.extern _end
.extern __kernel_tables
.extern __kernel_directory
.extern vga_init
.extern multiboot_init
.extern arch_setup
.extern cpu_init
.extern kinit
.extern multiboot_load_modules
.extern start_init

/* Pomocnicze: rozmiar adresu fizycznego */
#ifdef __CONFIG_ENABLE_PAE
#define PG_ITEM_SIZE 8
#else

#endif

_start: /* Bootloader oddaje kontrolę tutaj, baardzo wczesna inicjalizacja */

	/* Ustawiamy stos  */
	movl $KERNEL_VIRT2PHYS(bootstack + __CONFIG_KSTACK_SIZE), %esp

	/* Ustawiamy flagi procesora */
	pushl $0x2
	popfl

	/* Zapisujemy informacje od bootloadera na stosie */
	addl $LOWMEM_START, %ebx
	pushl %ebx
	pushl %eax

	/* Włączamy strony 4MB */
	movl %cr4, %eax
	orl $CPU_CR4_PSE, %eax
	movl %eax, %cr4

	/* Mapujemy kod kernela pod fizyczny = wirtualny (kernel nie powinien przekroczyc 4MiB wiec mapujemy 2 strony) */
	movl $KERNEL_VIRT2PHYS(__kernel_directory + PGDIR_ENTRY(KERNEL_PHYS_ADDR) * PG_ITEM_SIZE), %ebx
	movl $KERNEL_PHYS_ADDR, %eax /* Adres fizyczny kernela */
	andl $0xFC000000, %eax /* Wyrownojemy do 4MiB */
	movl %eax, (%ebx)
	orl $(PG_FLAG_PRESENT | PG_FLAG_RW | PG_FLAG_SIZE), (%ebx)

	addl $PG_ITEM_SIZE, %ebx
	add $(1024 * 1024 * 4), %eax
	movl %eax, (%ebx)
	orl $(PG_FLAG_PRESENT | PG_FLAG_RW | PG_FLAG_SIZE), (%ebx)

	/* Mapujemy pierwsze 512MB ramu */
	movl $0x80, %ecx
	movl $KERNEL_VIRT2PHYS(__kernel_directory + PGDIR_ENTRY(LOWMEM_START) * PG_ITEM_SIZE), %ebx
	xorl %eax, %eax
	.fill_directory:
		movl %eax, (%ebx)
		orl $(PG_FLAG_PRESENT | PG_FLAG_RW | PG_FLAG_SIZE), (%ebx)
		add $(1024 * 1024 * 4), %eax
		add $PG_ITEM_SIZE, %ebx
	loop .fill_directory

	/* Ładujemy katalog stron */
	movl $KERNEL_VIRT2PHYS(__kernel_directory), %eax
	movl %eax, %cr3

	/* Włączamy stronnicowanie */
	mov %cr0, %eax
	or $(CPU_CR0_PG | CPU_CR0_WP), %eax
	mov %eax, %cr0

	/* Skaczemu do kodu pod wysokim adresem */
	ljmp $0x08, $start32

start32:
	/* Przenosimy stos pod wysoki adres */
	addl $LOWMEM_START, %esp

	/* Inicjujemy konsolę VGA */
	call vga_init

	/* Przetwarzamy informacje otrzymane od bootloadera */
	call multiboot_init

	/* Inicjujemy BSP (Boot Strap Processor) */
	pushl $bootstack
	pushl $0
	call cpu_init
	addl $8, %esp

	/* Inicjujemy częsci kernela niezależne od sprzętu */
	call kinit

	/* Ładujemy moduły wczytane do pamięci przez bootloader */
	call multiboot_load_modules

	/* Odmapowywujemy niepotrzebną pamięć */
	movl $(__kernel_directory + PGDIR_ENTRY(KERNEL_PHYS_ADDR) * PG_ITEM_SIZE), %ebx
	movl $0, (%ebx)
	addl $PG_ITEM_SIZE, %ebx
	movl $0, (%ebx)

	/* Tworzymy proces dla programu INIT */
	call start_init

idle:
	hlt
	jmp idle

.global execve_user
/* void execve_user(void * entry, void * stack); */
execve_user:
	mov %ebp, %eax /* Zapisujemy stare ebp w eax */
	movl %esp, %ebp

	/* Przygotowywujemy stos (będziemy emulować przerwanie) */
	pushl $USER_DATA_SEL /* ss */
	pushl 8(%ebp) /* esp */
	pushl $0x202 /* eflags */
	pushl $USER_CODE_SEL /* cs */
	pushl 4(%ebp) /* eip */

	/* Odtwarzamy ebp */
	movl %eax, %ebp

	/* ustawiamy rejestry segmentowe */
	movl $USER_DATA_SEL, %eax
	movl %eax, %ds
	movl %eax, %es
	movl %eax, %fs
	movl %eax, %gs

	/* Zerujemy eax */
	xor %eax, %eax

	/* Przechodzimy w tryb usera poprzez powrót z przerwania */
	iret

/* void __ctx_switch(struct ctx * oldctx, struct ctx * newctx) */
.global __ctx_switch
__ctx_switch: /* Funkcja wykonuje zmianę kontekstu */
	push %ebp
	movl %esp, %ebp

	/* Zapisujemy flagi procesora */
	pushfl

	/* Zapisujemy rejestry */
	pushl %eax
	pushl %ecx
	pushl %edx
	pushl %ebx
	pushl %esi
	pushl %edi

	/* Zapisujemy adresy struktur: old w ebx, new w ecx */
	movl 8(%ebp), %ebx
	movl 12(%ebp), %ecx

	/* Zapisujemy stary stos */
	movl %esp, (%ebx)

	/* Ladujemy nowy stos */
	movl (%ecx), %esp

	/* Odblokowywujemy oldctx i newctx */
	movl $0, 8(%ecx)
	movl $0, 8(%ebx)

	/* Przywracamy rejestry */
	popl %edi
	popl %esi
	popl %ebx
	popl %edx
	popl %ecx
	popl %eax

	/* Przywracamy flagi */
	popfl

	popl %ebp

	/* Zaczynamy wykonywac nowy wątek */
	ret

/* void gdt_flush(void); */
.global gdt_flush
gdt_flush:
	mov $KERNEL_DATA_SEL, %ax
	mov %ax, %ds
	mov %ax, %es
	mov %ax, %fs
	mov %ax, %gs
	mov %ax, %ss
	ljmp $KERNEL_CODE_SEL, $.flush

.flush:
	ret
.bss
bootstack: .fill __CONFIG_KSTACK_SIZE
