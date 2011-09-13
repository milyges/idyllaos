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
#ifndef __ARCH_IOAPIC_H
#define __ARCH_IOAPIC_H

#ifdef __CONFIG_ENABLE_IOAPIC

#include <kernel/types.h>

#define IOAPIC_WINDOW            0x10

#define IOAPIC_REG_ID            0x00
#define IOAPIC_REG_VERSION       0x01
#define IOAPIC_REG_ARBITRATION   0x02
#define IOAPIC_REG_INT0          0x10
#define IOAPIC_REG_INT1          0x12
#define IOAPIC_REG_INT2          0x14
#define IOAPIC_REG_INT3          0x16
#define IOAPIC_REG_INT4          0x18
#define IOAPIC_REG_INT5          0x1A
#define IOAPIC_REG_INT6          0x1C
#define IOAPIC_REG_INT7          0x1E
#define IOAPIC_REG_INT8          0x20
#define IOAPIC_REG_INT9          0x22
#define IOAPIC_REG_INT10         0x24
#define IOAPIC_REG_INT11         0x26
#define IOAPIC_REG_INT12         0x28
#define IOAPIC_REG_INT13         0x2A
#define IOAPIC_REG_INT14         0x2C
#define IOAPIC_REG_INT15         0x2E
#define IOAPIC_REG_INT16         0x30
#define IOAPIC_REG_INT17         0x32
#define IOAPIC_REG_INT18         0x34
#define IOAPIC_REG_INT19         0x36
#define IOAPIC_REG_INT20         0x38
#define IOAPIC_REG_INT21         0x3A
#define IOAPIC_REG_INT22         0x3C
#define IOAPIC_REG_INT23         0x3E

#define IOAPIC_INT_MASK          (1 << 16)
#define IOAPIC_INT_TIGGER_MODE   (1 << 15)
#define IOAPIC_INT_REMOTE_IRR    (1 << 14)
#define IOAPIC_INT_INTPOL        (1 << 13)
#define IOAPIC_INT_DELIVS        (1 << 12)
#define IOAPIC_INT_DESTMOD       (1 << 11)

#define IOAPIC_INT_DELMOD_FIXED  (0 << 8)
#define IOAPIC_INT_DELMOD_LP     (1 << 8)
#define IOAPIC_INT_DELMOD_SMI    (2 << 8)
#define IOAPIC_INT_DELMOD_NMI    (4 << 8)
#define IOAPIC_INT_DELMOD_INIT   (5 << 8)
#define IOAPIC_INT_DELMOD_EXTINT (7 << 8)

struct ioapic
{
 uint8_t id;
 uint32_t phys_base;
 addr_t base;
 int gsi;
};

void ioapic_add(uint8_t id, uint32_t base);
void ioapic_init(void);

#endif /* __CONFIG_ENABLE_IOAPIC */

#endif /* __ARCH_IOAPIC_H */
