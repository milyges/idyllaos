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
#ifndef __ARCH_CPU_H
#define __ARCH_CPU_H

/* cr0 bits */
#define CPU_CR0_PE        (1 << 0)
#define CPU_CR0_MP        (1 << 1)
#define CPU_CR0_EM        (1 << 2)
#define CPU_CR0_TS        (1 << 3)
#define CPU_CR0_ET        (1 << 4)
#define CPU_CR0_NE        (1 << 5)
#define CPU_CR0_WP        (1 << 16)
#define CPU_CR0_AM        (1 << 18)
#define CPU_CR0_NW        (1 << 29)
#define CPU_CR0_CD        (1 << 30)
#define CPU_CR0_PG        (1 << 31)

/* cr4 bits */
#define CPU_CR4_VME       (1 << 0)
#define CPU_CR4_PVI       (1 << 1)
#define CPU_CR4_TSD       (1 << 2)
#define CPU_CR4_DE        (1 << 3)
#define CPU_CR4_PSE       (1 << 4)
#define CPU_CR4_PAE       (1 << 5)
#define CPU_CR4_MCE       (1 << 6)
#define CPU_CR4_PGE       (1 << 7)
#define CPU_CR4_PCE       (1 << 8)
#define CPU_CR4_OSFXSR    (1 << 9)
#define CPU_CR4_OSXMMEXC  (1 << 10)
#define CPU_CR4_VMXE      (1 << 13)

/* flags register */
#define CPU_CF            (1 << 0)
#define CPU_PF            (1 << 2)
#define CPU_AF            (1 << 4)
#define CPU_ZF            (1 << 6)
#define CPU_SF            (1 << 7)
#define CPU_TF            (1 << 8)
#define CPU_IF            (1 << 9)
#define CPU_DF            (1 << 10)
#define CPU_OF            (1 << 11)
#define CPU_IOPL          (1 << 12 | 1 << 13)
#define CPU_NT            (1 << 14)
#define CPU_RF            (1 << 16)
#define CPU_VM            (1 << 17)
#define CPU_AC            (1 << 18)
#define CPU_VIF           (1 << 19)
#define CPU_VIP           (1 << 20)
#define CPU_ID            (1 << 21)

#ifdef __x86_64__

#define CPU_MSR_EFER      0xC0000080

/* extended feature enable register */
#define CPU_EFER_SCE      (1 << 0)
#define CPU_EFER_LME      (1 << 8)
#define CPU_EFER_LMA      (1 << 10)
#define CPU_EFER_NXE      (1 << 11)
#define CPU_EFER_SVME     (1 << 12)
#define CPU_EFER_FFXSR    (1 << 14)

#endif /* __x86_64__ */

#ifndef __ASM__

#ifdef __CONFIG_ENABLE_SMP

#include <arch/apic.h>

#ifdef __CONFIG_CPUS_MAX
#define CPUS_MAX          __CONFIG_CPUS_MAX
#else /* __CONFIG_CPUS_MAX */
#define CPUS_MAX          8
#endif /* __CONFIG_CPUS_MAX */

#define CPU_CURRENT       (__cpus_apic_ids[apic_get(APIC_REG_APICID) >> 24])

#else /* __CONFIG_ENABLE_SMP */
#define CPUS_MAX          1
#define CPU_CURRENT       0
#endif /* __CONFIG_ENABLE_SMP */

#define CPU               (__cpus[CPU_CURRENT])

#include <arch/gdt.h>

struct tss
{
	uint32_t unused0;
	reg_t esp0;
	reg_t ss0;
	#ifdef __X86_64__
	reg_t unused1[20];
	#else /* __X86_64__ */
	reg_t unused1[22];
	#endif /* __x86_64__ */
	uint16_t unused2;
	uint16_t bmoffset;
} PACKED;

struct cpu
{
	uint32_t flags;
	uint32_t bogomips;
	int id;

#ifdef __CONFIG_ENABLE_APIC
	uint8_t apic_id;
	uint32_t apic_timer_init;
#endif /* __CONFIG_ENABLE_APIC */

	void * bootstack;

	struct vm_space * vmspace; /* Aktualna przestrzen adresowa, jesli NULL to kernel */

	/* GDT */
	struct gdt_desc gdt[GDT_DESCRIPTORS] __attribute__ ((aligned (8)));
	struct gdtr gdtr;

	/* TSS */
	struct tss tss __attribute__ ((aligned (8)));;
};

extern struct cpu * __cpus[CPUS_MAX];

#include <arch/mptables.h>
#include <arch/asm.h>

/* Funkcja zwraca ilość procesorów */
static inline int get_cpus(void)
{
	return mp_get_cpus();
}

/* Opóźnienie o x mili sekund */
static inline void delay(uint32_t msec)
{
 uint64_t end = rdtsc() + CPU->bogomips * msec;
 while(rdtsc() < end) ;
}

int cpus_get_live(void);
void execve_user(void * entry, void * stack);

#endif /* __ASM__ */

#endif /* __ARCH_CPU_H */
