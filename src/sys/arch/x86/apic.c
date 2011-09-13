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
#include <arch/apic.h>
#include <arch/cpuid.h>
#include <arch/pit.h>
#include <kernel/types.h>
#include <kernel/kprintf.h>
#include <kernel/debug.h>
#include <kernel/proc.h>
#include <mm/kmem.h>
#include <lib/string.h>

#ifdef __CONFIG_ENABLE_APIC

static paddr_t _apic_phys_base;
addr_t __apic_base;
int __cpus_apic_ids[256];

#ifdef __CONFIG_ENABLE_SMP
static inline void send_ipi(uint64_t ipi)
{
	apic_set(APIC_REG_ICR2, (uint32_t)(ipi >> 32));
	apic_set(APIC_REG_ICR1, (uint32_t)(ipi & 0xFFFFFFFF));
}

void smp_startup(void)
{
	if ((CPU->flags & CPUID_FLAG_APIC) != CPUID_FLAG_APIC)
		return;

	/* Sprawdzamy czy mamy wiecej niz jeden CPU */
	if (get_cpus() < 2)
		return;

	kprintf("SMP: Booting APs...\n");

	/* Kopiujemy kod inicjujący procesory */
	extern char __ap_boot;
	memcpy((void *)KERNEL_PHYS2VIRT(APIC_BOOT_CODE), &__ap_boot, PAGE_SIZE);

	/* Wysyłamy INIT */
	send_ipi((0xFFull << 56) | APIC_IPI_EX_SELF | APIC_IPI_INIT | APIC_IPI_ASSERT | 0x00);

	/* Opóźnienie */
	delay(100);

	/* Wysyłamy STARTUP */
	send_ipi((0xFFull << 56) | APIC_IPI_EX_SELF | APIC_IPI_STARTUP | APIC_IPI_ASSERT | APIC_BOOT_CODE >> 12);

	/* Czekamy aż wszystkie CPU się obudzą i się zainicjują */
	while(get_cpus() != cpus_get_live()) ;
	kprintf("SMP: Brought up %d CPUs\n", cpus_get_live());
}

void smp_panic(void)
{
	/* Wysyłamy IPI do wszystkich CPU poza sobą samym */
	send_ipi((0xFFull << 56) | APIC_IPI_EX_SELF | 0xFF);
}
#endif /* __CONFIG_ENABLE_SMP */

void apic_intr_dispatcher(struct intr_stack * stack)
{
	static char * _apic_interruts[] =
	{
		"Timer",
		"Thermal",
		"Performance Count",
		"LINT0",
		"LINT1",
		"Error"
	};

	switch(stack->intr_no)
	{
		case APIC_INTRNO_TIMER:
		{
			apic_set(APIC_REG_EOI, 0);
			apic_set(APIC_REG_TIC, CPU->apic_timer_init);
			schedule();
			break;
		}
		default:
		{
			kprintf("APIC: %s irq fired\n",  _apic_interruts[stack->intr_no - APIC_INTRNO_TIMER]);
			apic_set(APIC_REG_EOI, 0);
		}
 	}
}

void apic_init(int id)
{
	uint32_t tmp;
	/* Sprawdzamy czy CPU posiada Local APIC */
	if ((__cpus[id]->flags & CPUID_FLAG_APIC) != CPUID_FLAG_APIC)
		return;

	if (!id)
	{
		/* Pobieramy adres fizyczny */
		_apic_phys_base = (paddr_t)rdmsr(APIC_BASE_MSR) & PG_ADDR_MASK;

		/* Mapujemy APIC w pamięci kernela */
		__apic_base = (addr_t)kmem_alloc(PAGE_SIZE);
		paging_map_page(__apic_base, _apic_phys_base, PG_FLAG_PRESENT | PG_FLAG_RW | PG_FLAG_NOCACHE, NULL);
	}
	else
		wrmsr(APIC_BASE_MSR, (_apic_phys_base & PG_ADDR_MASK) | APIC_BASE_MSR_ENABLE); /* Ustawiamy adres fizyczny */

	/* Programowo aktywujemy Local APIC (niektóre biosy go wyłączają) */
	tmp = apic_get(APIC_REG_SIV);
	tmp |= APIC_SINT_ASE;
	apic_set(APIC_REG_SIV, tmp);

	/* Pobieramy Apic ID */
	__cpus[id]->apic_id = apic_get(APIC_REG_APICID) >> 24;
	__cpus_apic_ids[__cpus[id]->apic_id] = id;

	/* Pobieramy wersję APIC */
	tmp = apic_get(APIC_REG_VERSION);
	kprintf(KERN_DEBUG"CPU #%d: Local APIC version %d found\n", id, tmp & 7);

	/* Ustawiamy numery przerwań */
	apic_set(APIC_REG_TILVTE, APIC_INTRNO_TIMER);
	apic_set(APIC_REG_THLVTE, APIC_INTRNO_THERMAL);
	apic_set(APIC_REG_PCLVTE, APIC_INTRNO_PERFCOUNT);
	apic_set(APIC_REG_LI0VTE, APIC_INTRNO_LINT0 | APIC_INT_MT_EXT);
	apic_set(APIC_REG_LI1VTE, APIC_INTRNO_LINT1 | APIC_INT_MT_NMI);
	apic_set(APIC_REG_EVTE, APIC_INTRNO_ERROR);

	/* Kalibrujemy APIC Timer */
	apic_set(APIC_REG_TDC, APIC_TIMER_DIV128);
	apic_set(APIC_REG_TIC, 0xFFFFFFFF); /* Wartość początkowa zegara */
	delay(100); /* Czekaj 100ms */
	tmp = 0xFFFFFFFF - apic_get(APIC_REG_TCC);
	apic_set(APIC_REG_TIC, 0);
	apic_set(APIC_REG_TPR, 0);
	__cpus[id]->apic_timer_init = (tmp * 10) / PIT_HZ;

	if (!id)
	{
		/* Aktywujemy wywoływanie schedule() przez APIC */
		pit_do_schedule(0);
	}

	/* Uruchamiamy APIC Timer */
	apic_set(APIC_REG_TILVTE, APIC_INTRNO_TIMER);
	apic_set(APIC_REG_TIC, __cpus[id]->apic_timer_init);
}

#endif /* __CONFIG_ENABLE_APIC */
