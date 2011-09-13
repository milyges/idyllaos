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
#ifndef __ARCH_APIC_H
#define __ARCH_APIC_H

#ifdef __CONFIG_ENABLE_APIC

#define APIC_BASE_MSR         0x1B
#define APIC_BASE_MSR_ENABLE  0x800

#define APIC_REG_APICID       0x020
#define APIC_REG_VERSION      0x030
#define APIC_REG_TPR          0x080 /* Task priority register */
#define APIC_REG_PPR          0x090 /* Processor priority register */
#define APIC_REG_EOI          0x0B0 /* End of interrupt register */
#define APIC_REG_RRR          0x0C0 /* Remote read register */
#define APIC_REG_LDR          0x0D0 /* Logical Destination Register */
#define APIC_REG_DFR          0x0E0 /* Destination Format Register */
#define APIC_REG_SIV          0x0F0 /* Spurious Interrupt Vector Register */
#define APIC_REG_ISR          0x100 /* In-service register */
#define APIC_REG_TMR          0x180 /* Trigger mode register */
#define APIC_REG_IRR          0x200 /* Interrupt request register */
#define APIC_REG_ESR          0x280 /* Error status register */
#define APIC_REG_ICR1         0x300 /* Interrupt command register Low */
#define APIC_REG_ICR2         0x310 /* Interrupt command register High */
#define APIC_REG_TILVTE       0x320 /* Timer Local Vector Table Entry */
#define APIC_REG_THLVTE       0x330 /* Thermal Local Cector Table Entry */
#define APIC_REG_PCLVTE       0x340 /* Performance Counter Local Vector Table Entry */
#define APIC_REG_LI0VTE       0x350 /* Local Interrupt 0 Vector Table Entry */
#define APIC_REG_LI1VTE       0x360 /* Local Interrupt 1 Vector Table Entry */
#define APIC_REG_EVTE         0x370 /* Error Vector Table Entry */
#define APIC_REG_TIC          0x380 /* Timer Initial Count Register */
#define APIC_REG_TCC          0x390 /* Timer Current Count Register */
#define APIC_REG_TDC          0x3E0 /* Timer Divide Configuration Register */
#define APIC_REG_EAF          0x400 /* Extended APIC Feature Register */
#define APIC_REG_EAC          0x410 /* Extended APIC Control Register */
#define APIC_REG_SEOI         0x420 /* Specific End of Interrupt Register */
#define APIC_REG_IER          0x480 /* Interrupt Enable Registers */
#define APIC_REG_EILVT        0x500 /* Extended Interrupt Local Vector Table Registers */

/* Message types */
#define APIC_INT_MT_FIXED     (0 << 8)
#define APIC_INT_MT_SMI       (2 << 8)
#define APIC_INT_MT_NMI       (4 << 8)
#define APIC_INT_MT_EXT       (7 << 8)

#define APIC_INT_DS           (1 << 12) /* Delivery status */
#define APIC_INT_RIR          (1 << 14) /* Remote IRR */
#define APIC_INT_TGM          (1 << 15) /* Trigger mode */
#define APIC_INT_MASK         (1 << 16) /* Mask */
#define APIC_INT_TMM          (1 << 17) /* Timer mode */

/* Spurious Interrupts */
#define APIC_SINT_GCC         (1 << 9) /* Force CPU Core Checking */
#define APIC_SINT_ASE         (1 << 8) /* APIC Software Enable */

/* Timer Divides */
#define APIC_TIMER_DIV1       0xB
#define APIC_TIMER_DIV16      0x3
#define APIC_TIMER_DIV128     0xA

#define APIC_IPI_SELF         (1 << 18)
#define APIC_IPI_ALL          (2 << 18)
#define APIC_IPI_EX_SELF      (3 << 18)

#define APIC_IPI_ASSERT       (1 << 14)
#define APIC_IPI_LP           (1 << 8) /* Lowest Priority */
#define APIC_IPI_SMP          (2 << 8)
#define APIC_IPI_NMI          (4 << 8)
#define APIC_IPI_INIT         (5 << 8)
#define APIC_IPI_STARTUP      (6 << 8)

#define APIC_BOOT_CODE        0x7C000

#define APIC_INTRNO_TIMER     0xF0
#define APIC_INTRNO_THERMAL   0xF1
#define APIC_INTRNO_PERFCOUNT 0xF2
#define APIC_INTRNO_LINT0     0xF3
#define APIC_INTRNO_LINT1     0xF4
#define APIC_INTRNO_ERROR     0xF5

#ifndef __ASM__

#include <arch/types.h>
#include <arch/intr.h>

static inline void apic_set(unsigned reg, uint32_t val)
{
 extern addr_t __apic_base;
 if (!__apic_base)
    return;
 *((volatile uint32_t *)(__apic_base + reg)) = val;
}

static inline uint32_t apic_get(unsigned reg)
{
 extern addr_t __apic_base;
 if (!__apic_base)
    return 0;
 return *((volatile uint32_t *)(__apic_base + reg));
}

extern int __cpus_apic_ids[256];

void apic_init(int id);
void apic_intr_dispatcher(struct intr_stack * stack);

#ifdef __CONFIG_ENABLE_SMP
void smp_startup(void);
void smp_panic(void);
#endif /* __CONFIG_ENABLE_SMP */

#endif /* __ASM__ */

#endif /* __CONFIG_ENABLE_APIC */

#endif /* __ARCH_APIC_H */
