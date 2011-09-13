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
#include <arch/fpu.h>
#include <arch/cpu.h>
#include <arch/cpuid.h>
#include <arch/asm.h>
#include <kernel/types.h>
#include <kernel/kprintf.h>

static void fpu_set_cw(uint16_t cw)
{
	if (CPU->flags & CPUID_FLAG_FPU)
	{
		__asm__ __volatile__("fldcw %0"
		                     :
		                     :"m"(cw)
		                    );
	}
}

void fpu_save(void * dst)
{
	if (CPU->flags & CPUID_FLAG_FPU)
	{
		/* TODO: Zapisz kontekst FPU */
	}
}

void fpu_restore(void * src)
{
	if (CPU->flags & CPUID_FLAG_FPU)
	{
		/* TODO: Przywróc kontekst FPU */
	}
}

void fpu_init(void)
{
	reg_t tmp;

	if ((CPU->flags & CPUID_FLAG_FPU) != CPUID_FLAG_FPU)
	{
		kprintf("CPU #%d: FPU not detected\n", CPU->id);
		return;
	}

	kprintf("CPU #%d: Initializing FPU\n", CPU->id);

	/* Włączamy obsługę FXSAVE i FXRSTOR */
	tmp = read_cr4();
	tmp |= CPU_CR4_OSFXSR;
	write_cr4(tmp);

	/* Inicjujemy koprocesor */
	__asm__ __volatile__ ("finit");

	/* Ustawiamy FPU Control Word */
	fpu_set_cw(0x37F);
}
