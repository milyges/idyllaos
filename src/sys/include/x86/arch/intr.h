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
#ifndef __ARCH_IDT_H
#define __ARCH_IDT_H

#define IDT_PRESENT     0x80 /* Present */
#define IDT_SIZE32      0x08 /* 32 bit gate */
#define IDT_DPL0        0x00 /* Descriptor Privilege Level = 0 */
#define IDT_DPL1        0x20 /* Descriptor Privilege Level = 1 */
#define IDT_DPL2        0x40 /* Descriptor Privilege Level = 2 */
#define IDT_DPL3        0x60 /* Descriptor Privilege Level = 3 */
#define IDT_TYPE_INTR   0x06 /* Interrupt gate */
#define IDT_TYPE_TRAP   0x07 /* Trap gate */
#define IDT_TYPE_TASK   0x05 /* Task gate */

#define IDT_DESCRIPTORS 256

#ifndef __ASM__

#include <kernel/types.h>

/* Deskryptor IDT */
struct idt_desc
{
 uint16_t offset_lo;
 uint16_t sel;
 uint8_t reserved;
 uint8_t acc;
 uint16_t offset_hi;
 #ifdef __X86_64__
 uint32_t offset_ext;
 uint32_t reserved2;
 #endif /* __X86_64__ */
} PACKED;


/* IDT Register */
struct idtr
{
 uint16_t size;
 addr_t addr;
} PACKED;

struct intr_stack
{
 #ifdef __X86_64__
 /* Rejestry kładzione tylko w long-mode */
 reg_t r8;
 reg_t r9;
 reg_t r10;
 reg_t r11;
 reg_t r12;
 reg_t r13;
 reg_t r14;
 reg_t r15;
 #endif

 /* Rejestry segmentowe */
 reg_t gs;
 reg_t fs;
 reg_t es;
 reg_t ds;

 /* Kładzione przez pusha */
 reg_t di;
 reg_t si;
 reg_t bp;
 reg_t old_sp;
 reg_t bx;
 reg_t dx;
 reg_t cx;
 reg_t ax;

 reg_t intr_no;
 reg_t err_code;

 /* Odkładane przez CPU */
 reg_t ip;
 reg_t cs;
 reg_t flags;

 /* Dodatkowe, tylko przy zmianie DPL */
 reg_t sp;
 reg_t ss;

 /* Dodatkowe, tylko dla monitora v86 */
 reg_t v86_es;
 reg_t v86_ds;
 reg_t v86_fs;
 reg_t v86_gs;
} PACKED;

void idt_setup(void);
void idt_load(void);

#endif /* __ASM__ */

#endif /* __ARCH_IDT_H */
