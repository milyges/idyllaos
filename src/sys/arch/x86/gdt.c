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
#include <arch/gdt.h>
#include <arch/cpu.h>
#include <arch/page.h>
#include <kernel/kprintf.h>

static void set_gdt_dsc(struct gdt_desc * dsc, uint32_t base, uint32_t limit, uint16_t acc)
{
 dsc->base_lo = (uint16_t)(base & 0xFFFF);
 dsc->base_mid = (uint8_t)((base >> 16) & 0xFF);
 dsc->limit_lo = (uint16_t)(limit & 0xFFFF);
 dsc->base_limit_hi = (uint16_t)((limit >> 16) | (acc >> 8) | ((base >> 16) & 0xFF00));
 dsc->acc = acc & 0xFF;
}

void gdt_setup(int id)
{
 /* Null descriptor */
 set_gdt_dsc(&__cpus[id]->gdt[0], 0, 0, 0);
 #ifdef __X86_64__
 set_gdt_dsc(&__cpus[id]->gdt[1], 0, 0, GDT_DSC_P | GDT_DSC_S | GDT_DSC_L | GDT_DSC_CODE | GDT_DPL_0);
 set_gdt_dsc(&__cpus[id]->gdt[2], 0, 0, GDT_DSC_RW | GDT_DSC_P | GDT_DSC_S | GDT_DSC_G | GDT_DSC_L);
 set_gdt_dsc(&__cpus[id]->gdt[3], 0, 0, GDT_DSC_P | GDT_DSC_S | GDT_DSC_L | GDT_DSC_CODE | GDT_DPL_3);
 set_gdt_dsc(&__cpus[id]->gdt[4], 0, 0, GDT_DSC_RW | GDT_DSC_P | GDT_DSC_S | GDT_DSC_G | GDT_DSC_L | GDT_DPL_3);
 #ifdef __CONFIG_ENABLE_X86_SUPPORT
 set_gdt_dsc(&__cpus[id]->gdt[6], 0, 0xFFFFF, GDT_DSC_P | GDT_DSC_S | GDT_DSC_G | GDT_DSC_RW | GDT_DSC_DB | GDT_DSC_CODE | GDT_DPL_3);
 set_gdt_dsc(&__cpus[id]->gdt[7], 0, 0xFFFFF, GDT_DSC_P | GDT_DSC_S | GDT_DSC_G | GDT_DSC_RW | GDT_DSC_DB | GDT_DPL_3);
 #endif /* __CONFIG_ENABLE_X86_SUPPORT */
 #else /* __X86_64__ */
 set_gdt_dsc(&__cpus[id]->gdt[1], 0, 0xFFFFF, GDT_DSC_P | GDT_DSC_S | GDT_DSC_G | GDT_DSC_RW | GDT_DSC_DB | GDT_DSC_CODE | GDT_DPL_0);
 set_gdt_dsc(&__cpus[id]->gdt[2], 0, 0xFFFFF, GDT_DSC_P | GDT_DSC_S | GDT_DSC_G | GDT_DSC_RW | GDT_DSC_DB | GDT_DPL_0);
 set_gdt_dsc(&__cpus[id]->gdt[3], 0, 0xFFFFF, GDT_DSC_P | GDT_DSC_S | GDT_DSC_G | GDT_DSC_RW | GDT_DSC_DB | GDT_DSC_CODE | GDT_DPL_3);
 set_gdt_dsc(&__cpus[id]->gdt[4], 0, 0xFFFFF, GDT_DSC_P | GDT_DSC_S | GDT_DSC_G | GDT_DSC_RW | GDT_DSC_DB | GDT_DPL_3);
 set_gdt_dsc(&__cpus[id]->gdt[5], (uint32_t)&__cpus[id]->tss, sizeof(struct tss), GDT_DSC_P | GDT_DSC_TSS | GDT_DPL_0);
 #endif /* __X86_64__ */

 /* Ustaw gdtr */
 __cpus[id]->gdtr.size = GDT_DESCRIPTORS * 8 - 1;
 __cpus[id]->gdtr.addr = (addr_t)&__cpus[id]->gdt;

 /* Wczytaj nowe GDT */
 __asm__ __volatile__ ("lgdt (%0)"::"r"(&__cpus[id]->gdtr));

 gdt_flush();

 /* Ustaw TSS */
 __cpus[id]->tss.ss0 = KERNEL_DATA_SEL;
 __cpus[id]->tss.bmoffset = sizeof(struct tss) + 1;
 /* Załaduj Task Register */
 __asm__ __volatile__ ("ltr %w0"::"r"(KERNEL_TSS_SEL));
}
