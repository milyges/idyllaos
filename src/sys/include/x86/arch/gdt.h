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
#ifndef __ARCH_GDT_H
#define __ARCH_GDT_H

#define KERNEL_CODE_SEL      0x08
#define KERNEL_DATA_SEL      0x10
#define USER_CODE_SEL        0x1B
#define USER_DATA_SEL        0x23
#define KERNEL_TSS_SEL       0x28

#ifdef __CONFIG_ENABLE_X86_SUPPORT
#define GDT_DESCRIPTORS      8
#define USER32_CODE_SEL      0x33
#define USER32_DATA_SEL      0x3B
#else /* __CONFIG_ENABLE_X86_SUPPORT */
#define GDT_DESCRIPTORS      6
#endif /* __CONFIG_ENABLE_X86_SUPPORT */

#define GDT_DSC_A            0x01 /* Accessed */
#define GDT_DSC_RW           0x02 /* Read (CODE) / Write (DATA) */
#define GDT_DSC_CE           0x04 /* Conforming (CODE) / Expand-down (DATA) */
#define GDT_DSC_CODE         0x08 /* CODE */
#define GDT_DSC_S            0x10 /* Application */
#define GDT_DPL_0            0x00 /* Descriptor Privilege Level = 0 */
#define GDT_DPL_1            0x20 /* Descriptor Privilege Level = 1 */
#define GDT_DPL_2            0x40 /* Descriptor Privilege Level = 2 */
#define GDT_DPL_3            0x60 /* Descriptor Privilege Level = 3 */
#define GDT_DSC_P            0x80 /* Present */

#ifdef __X86_64__
#define GDT_DSC_L            0x2000 /* Long (1) / legacy (0) mode */
#endif /* __X86_64__ */

#define GDT_DSC_DB           0x4000 /* D (CODE) / B (DATA) */
#define GDT_DSC_G            0x8000 /* Granularity */

#define GDT_DSC_LDT          0x02 /* LDT */
#define GDT_DSC_TASK_GATE    0x05 /* Task Gate */
#define GDT_DSC_TSS          0x09 /* Available 32-Bit TSS */
#define GDT_DSC_TSS_BUSY     0x0B /* Busy 32-Bit TSS */
#define GDT_DSC_CALL_GATE    0x0C /* 32-Bit Call Gate */
#define GDT_DSC_INT_GATE     0x0E /* 32-Bit Interrupt Gate */
#define GDT_DSC_TRAP_GATE    0x0F /* 32-Bit Trap Gate */
#define GDT_DSC_DATA_RW      GDT_DSC_S | DSC_RW /* Data Read/Write */
#define GDT_DSC_DATA_R       GDT_DSC_S /* Data Read Only */
#define GDT_DSC_CODE_E       GDT_DSC_S | GDT_DSC_CODE // Code Execute Only */
#define GDT_DSC_CODE_ER      GDT_DSC_S | GDT_DSC_CODE | GDT_DSC_RW /* Code Execute/Read */

#ifndef __ASM__

#include <kernel/types.h>

/* Deskryptor GDT */
struct gdt_desc
{
 uint16_t limit_lo;
 uint16_t base_lo;
 uint8_t base_mid;
 uint8_t acc;
 uint16_t base_limit_hi;
} PACKED;

/* GDT register */
struct gdtr
{
 uint16_t size;
 addr_t addr;
} PACKED;

void gdt_setup(int id);
void gdt_flush(void);

#endif /* __ASM__ */

#endif /* __ARCH_GDT_H */
