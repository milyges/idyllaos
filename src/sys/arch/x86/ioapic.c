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
#include <arch/ioapic.h>
#include <arch/page.h>
#include <arch/irq.h>
#include <arch/pic.h>
#include <kernel/types.h>
#include <kernel/kprintf.h>
#include <kernel/debug.h>
#include <mm/heap.h>
#include <mm/kmem.h>

#ifdef __CONFIG_ENABLE_IOAPIC

static int _ioapics_count = 0;
static struct ioapic * _ioapics = NULL;

static inline uint32_t ioapic_read(int num, uint8_t reg)
{
 *((volatile uint32_t *)_ioapics[num].base) = (uint32_t)reg;
 return *((volatile uint32_t *)(_ioapics[num].base + IOAPIC_WINDOW));
}

static inline void ioapic_write(int num, uint8_t reg, uint32_t value)
{
 *((volatile uint32_t *)_ioapics[num].base) = (uint32_t)reg;
 *((volatile uint32_t *)(_ioapics[num].base + IOAPIC_WINDOW)) = value;
}

void ioapic_add(uint8_t id, uint32_t base)
{
 _ioapics = krealloc(_ioapics, sizeof(struct ioapic) * (_ioapics_count + 1));
 
 _ioapics[_ioapics_count].id = id;
 _ioapics[_ioapics_count].phys_base = base;
 _ioapics[_ioapics_count].base = (addr_t)kmem_alloc(PAGE_SIZE);
 paging_map_page(_ioapics[_ioapics_count].base, base, PG_FLAG_PRESENT | PG_FLAG_RW | PG_FLAG_NOCACHE, NULL);
 _ioapics_count++;
}

void ioapic_init(void)
{
	int i, gsi = 0x20;
	
	if (!_ioapics_count)
	{
		kprintf(KERN_DEBUG"IO APIC: No I/O APICs found, using legacy PIC\n");
		return;
	}
	
	//irq_use_pic(0);
	//pic_disable();
	return;
	
	for(i=0;i<_ioapics_count;i++)
	{
		/* Zapisujemy odpowiednie APIC ID */
		ioapic_write(i, IOAPIC_REG_ID, _ioapics[i].id << 24);		
		_ioapics[i].gsi = gsi;
		gsi += (ioapic_read(i, IOAPIC_REG_VERSION) & 0xFF0000) >> 16;
		
		kprintf(KERN_DEBUG"IO APIC: I/O APIC detected at 0x%lX->0x%p, id: %d, gsi: 0x%02X\n", _ioapics[_ioapics_count].phys_base, _ioapics[_ioapics_count].base, _ioapics[i].id, _ioapics[i].gsi);
	}

}

#endif /* __CONFIG_ENABLE_IOAPIC */
