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
#ifndef __ARCH_ASM_H
#define __ARCH_ASM_H

#include <kernel/types.h>

/* Funkcje operujace na portach */
static inline uint8_t inportb(uint16_t port)
{
	uint8_t ret_val;
	__asm__ __volatile__ ("inb %1, %0"
	                      :"=a"(ret_val)
	                      :"d"(port));
	return ret_val;
}

static inline uint16_t inportw(uint16_t port)
{
	uint16_t ret_val;
	__asm__ __volatile__ ("inw %1, %0"
	                      :"=a"(ret_val)
	                      :"d"(port));
	return ret_val;
}

static inline uint32_t inportd(uint16_t port)
{
	uint32_t ret_val;
	__asm__ __volatile__ ("inl %1, %0"
	                      :"=a"(ret_val)
	                      :"d"(port));
	return ret_val;
}

static inline void outportb(uint16_t port, uint8_t value)
{
	__asm__ __volatile__ ("outb %b0, %w1"
	                      :
	                      :"a"(value), "d"(port));
}

static inline void outportd(uint16_t port, uint32_t value)
{
	__asm__ __volatile__ ("out %0, %w1"
	                      :
	                      :"a"(value), "d"(port));
}

static inline void outportw(uint16_t port, uint16_t value)
{
	__asm__ __volatile__ ("outw %w0, %w1"
	                      :
	                      :"a"(value), "d"(port));
}

/* Operacje na rejestrach kontrolnych */
static inline reg_t read_cr0(void)
{
	reg_t ret_val;
	__asm__ __volatile__("mov %%cr0, %0"
	                     : "=r"(ret_val)
	                     :);
	return ret_val;
}

static inline reg_t read_cr1(void)
{
	reg_t ret_val;
	__asm__ __volatile__("mov %%cr1, %0"
	                     : "=r"(ret_val)
	                     :);
	return ret_val;
}

static inline reg_t read_cr2(void)
{
	reg_t ret_val;
	__asm__ __volatile__("mov %%cr2, %0"
	                     : "=r"(ret_val)
	                     :);
	return ret_val;
}

static inline reg_t read_cr3(void)
{
	reg_t ret_val;
	__asm__ __volatile__("mov %%cr3, %0"
	                     : "=r"(ret_val)
	                     :);
	return ret_val;
}

static inline reg_t read_cr4(void)
{
	reg_t ret_val;
	__asm__ __volatile__("mov %%cr4, %0"
	                     : "=r"(ret_val)
	                     :);
	return ret_val;
}

static inline void write_cr0(reg_t value)
{
	__asm__ __volatile__("mov %%eax, %%cr0"
	                     :
	                     :"r"(value));
}

static inline void write_cr1(reg_t value)
{
	__asm__ __volatile__("mov %%eax, %%cr1"
	                     :
	                     :"r"(value));
}


static inline void write_cr3(reg_t value)
{
	__asm__ __volatile__("mov %%eax, %%cr3"
	                     :
	                     :"r"(value));
}

static inline void write_cr4(reg_t value)
{
	__asm__ __volatile__("mov %%eax, %%cr4"
	                     :
	                     :"r"(value));
}

/* Pozostale funkcje */
static inline void cpuid(uint32_t func, uint32_t *a, uint32_t *b, uint32_t *c, uint32_t *d)
{
	__asm__ __volatile__ ("cpuid"
	                      :"=a"(*a), "=b"(*b), "=c"(*c), "=d"(*d)
	                      : "0"(func));
}


static inline void invplg(void * addr)
{
	__asm__ __volatile__("invlpg %0"
	                     :
	                     :"m"(*(char *)addr)
	                    );
}

static inline void idle(void)
{
	__asm__ __volatile__ ("hlt");
}

#ifdef __CONFIG_ENABLE_SMP
#include <arch/apic.h>
#endif /* __CONFIG_ENABLE_SMP */

static inline void halt(void)
{
#ifdef __CONFIG_ENABLE_SMP
	smp_panic();
#endif /* __CONFIG_ENABLE_SMP */
	__asm__ __volatile__ ("cli");
	__asm__ __volatile__ ("hlt");
}




#ifdef __X86_64__
static inline void wrmsr(int msr, uint64_t value)
{
	__asm__ __volatile__("wrmsr"
	                     :
	                     :"a"(value & 0xFFFFFFFF), "c"(msr), "d"(value >> 32));
}

static inline uint64_t rdmsr(int msr)
{
	uint32_t a, d;

	__asm__ __volatile__("rdmsr"
	                     :"=a"(a), "=d"(d)
	                     : "c"(msr));

	return ((uint64_t)d << 32) | a;
}

static inline uint64_t rdtsc(void)
{
	uint32_t a, d;
	__asm__ __volatile__ ("rdtsc"
	                      :"=a"(a), "=d"(d)
	                     );
	return ((uint64_t)d << 32) | a;
}
#else /* __X86_64__ */
static inline uint64_t rdtsc(void)
{
	uint64_t tmp;
	__asm__ __volatile__ (".byte 0x0f, 0x31"
	                      :"=A"(tmp)
	                     );
	return tmp;
}

static inline void wrmsr(int msr, uint64_t value)
{
	__asm__ __volatile__ ("wrmsr"
	                      :
	                      :"A"(value), "c"(msr));
}

static inline uint64_t rdmsr(int msr)
{
	uint64_t ret_val;

	__asm__ __volatile__("rdmsr"
	                     :"=A"(ret_val)
	                     : "c"(msr));
	return ret_val;
}
#endif /* __X86_64__ */

#include <arch/cpu.h>
/* Zarządzanie przerwaniami */
static inline unsigned intr_disable(void)
{
	reg_t flags;
#ifdef __X86_64__
	__asm__ __volatile__ ("pushfq\n"
	                      "popq %%rax\n"
	                      "cli"
	                      :"=a"(flags));
#else /* __X86_64__ */
	__asm__ __volatile__ ("pushfl\n"
	                      "popl %%eax\n"
	                      "cli"
	                      :"=a"(flags));
#endif /* __X86_64__ */
 	return (flags & CPU_IF) == CPU_IF;
}

static inline void intr_enable(void)
{
	__asm__ __volatile__ ("sti");
}

static inline void intr_restore(unsigned irqstate)
{
	if (irqstate)
		intr_enable();
}

#endif /* __ARCH_IO_H */

