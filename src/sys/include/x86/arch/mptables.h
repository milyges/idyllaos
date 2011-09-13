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
#ifndef __ARCH_MPTABLES_H
#define __ARCH_MPTABLES_H

#include <kernel/types.h>

#define MP_FPS_MAGIC      0x5F504D5F  /* _MP_ */
#define MP_CTH_MAGIC      0x504D4350  /* PCMP */

#define MP_ENTRY_CPU      0
#define MP_ENTRY_BUS      1
#define MP_ENTRY_IOAPIC   2
#define MP_ENTRY_IOINT    3
#define MP_ENTRY_LINT     4

#define MP_CPU_FLAGS_EN   (1 << 0)

/* MP Floating Pointer Structure */
struct mp_fps
{
 uint32_t magic;
 uint32_t ptr;
 uint8_t lenght;
 uint8_t spec_rev;
 uint8_t checksum;
 uint8_t features[5];
} PACKED;

/* MP Config table header */
struct mp_ctable_header
{
 uint32_t magic;
 uint16_t base_length;
 uint8_t spec_rev;
 uint8_t checksum;
 uint8_t oem_id[8];
 uint8_t product_id[12];
 uint32_t oem_table_ptr;
 uint16_t oem_table_size;
 uint16_t entry_count;
 uint32_t lapic_address;
 uint16_t ext_table_len;
 uint8_t ext_table_checksum;
 uint8_t reserved;
} PACKED;

struct mp_entry_cpu
{
 uint8_t type;
 uint8_t cpu_id;
 uint8_t lapic_ver;
 uint8_t flags;
 uint32_t signature;
 uint32_t features;
 uint32_t reserved[2];
} PACKED;

struct mp_entry_bus
{
 uint8_t type;
 uint8_t bus_id;
 uint8_t bus_type[6];
} PACKED;

struct mp_entry_ioapic
{
 uint8_t type;
 uint8_t id;
 uint8_t version;
 uint8_t flags;
 uint32_t address;
} PACKED;

struct mp_entry_int
{
 uint8_t type;
 uint8_t int_type;
 uint16_t flags;
 uint8_t src_bus;
 uint8_t src_irq;
 uint8_t dst_apic;
 uint8_t dst_int;
} PACKED;

void mptables_setup(void);
int mp_get_cpus(void);
void mp_irq_route(void);

#endif /* __ARCH_MPTABLES_H */
