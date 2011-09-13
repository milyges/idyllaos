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
#include <arch/atomic.h>
#include <arch/intr.h>
#include <arch/cpu.h>
#include <arch/page.h>
#include <arch/apic.h>
#include <arch/cpuid.h>
#include <arch/pit.h>
#include <arch/spinlock.h>
#include <arch/pic.h>
#include <arch/ioapic.h>
#include <arch/fpu.h>
#include <arch/v86.h>
#include <kernel/types.h>
#include <kernel/kprintf.h>
#include <kernel/init.h>
#include <kernel/debug.h>
#include <kernel/proc.h>
#include <lib/string.h>
#include <mm/heap.h>
#include <mm/kmem.h>

static SPINLOCK_NEW(_cpu_next_id_lock);
static int _cpu_next_id = 1;

static atomic_t _cpus_live = ATOMIC_INIT(0);
struct cpu * __cpus[CPUS_MAX];

static inline uint32_t calculate_bogomips(void)
{
	uint64_t start, end, delta;
	pit_delay(10); /* Czekamy na przerwanie aby uzyskac większą dokładność */
	start = rdtsc();
	pit_delay(100); /* Czekamy 100ms */
	end = rdtsc();

	/* Obliczamy bogomips */
	delta = end - start;
	delta = delta / 100ull;
	return (uint32_t)delta;
}

int cpu_new_id(void)
{
	int id;

	spinlock_lock(&_cpu_next_id_lock);
	id = _cpu_next_id++;
	spinlock_unlock(&_cpu_next_id_lock);

	return id;
}

/* Funkcja zwraca ilość uruchomionych CPU */
int cpus_get_live(void)
{
	return atomic_get(&_cpus_live);
}

void cpu_init(uint8_t id, void * stack)
{
	uint32_t tmp[3];

	if (!id) /* Procesor BSD musi wykonać nieco więcej inicjalizacji */
	{
		/* Inicjujemy alokator dynamicznej pamięci jądra */
		kmem_init();

		/* Ustawiamy IDT (jeden, wspólny dla wszystkich CPU */
		idt_setup();

		/* Parsujemy tablice MP */
		if (!kargv_lookup("nomp"))
			mptables_setup();

		/* Inicjujemy PIC */
		pic_init();

#ifdef __CONFIG_ENABLE_IOAPIC
		/* Inicjujemy I/O APIC */
		ioapic_init();
#endif /* __CONFIG_ENABLE_IOAPIC */

#ifdef __CONFIG_ENABLE_V86
		/* Inicjujemy monitor v86 */
		v86_init();
#endif /* __CONFIG_ENABLE_V86 */

	}

	kprintf(KERN_INFO"Initializing CPU #%d (stack at 0x%p)\n", id, stack);
	atomic_inc(&_cpus_live);

	__cpus[id] = kalloc(sizeof(struct cpu));
	memset(__cpus[id], 0x00, sizeof(struct cpu));

	__cpus[id]->bootstack = stack;
	__cpus[id]->id = id;

	/* Pobierz flagi z CPUID */
	cpuid(CPUID_GETFEATURES, &tmp[0], &tmp[1], &tmp[2], &__cpus[id]->flags);

	/* Ustaw i wczytaj GDT */
	gdt_setup(id);

	/* Zaladuj IDT */
	idt_load();

	/* Zainicjuj scheduler */
	sched_init(id);

	/* Włącz przerwania */
	intr_enable();

#ifdef __CONFIG_ENABLE_APIC
	/* Zainicjuj Local APIC */
	if (!kargv_lookup("nolapic"))
		apic_init(id);
#endif /* __CONFIG_ENABLE_APIC */

	/* Wykalibruj TSC */
	if (!id)
	{
#ifdef __CONFIG_ENABLE_IOAPIC
		/* Ustawiamy przekierowania przerwań */
		mp_irq_route();
#endif /* __CONFIG_ENABLE_IOAPIC */

		/* Inicjujemy PIT */
		pit_init();

		/* Kalibrujemy funkcję delay() */
		kprintf(KERN_INFO"CPU #%d: TSC calibration using PIT\n", id);
		__cpus[id]->bogomips = calculate_bogomips();
	}
	else
	{
		kprintf(KERN_INFO"CPU #%d: Skipping TSC calibration\n", id);
		__cpus[id]->bogomips = __cpus[0]->bogomips;
	}

	kprintf("CPU #%d: %u.%uMHz, bogomips: %u\n", id, __cpus[id]->bogomips / 1000, __cpus[id]->bogomips % 1000, __cpus[id]->bogomips);

	/* Inicjujemy FPU */
	fpu_init();

#ifdef __CONFIG_ENABLE_SMP
	if ((!kargv_lookup("nosmp")) && (!id))
		smp_startup();
#endif /* __CONFIG_ENABLE_SMP */

	if (id > 0) /* Procesory AP mogą już zacząć wykonywać wątki */
		sched_preempt_enable();
}

