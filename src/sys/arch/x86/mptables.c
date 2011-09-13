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
#include <arch/mptables.h>
#include <arch/cpu.h>
#include <arch/bios.h>
#include <arch/ioapic.h>
#include <kernel/kprintf.h>
#include <kernel/debug.h>
#include <lib/string.h>

static struct mp_fps * _mp_fps;
static struct mp_ctable_header * _mp_ctable_header;
static int _mp_cpus;

int mp_get_cpus(void)
{
	return _mp_cpus;
}

static uint8_t mp_checksum(uint8_t * ptr, int len)
{
	int checksum = 0;	
	while(len--)
		checksum += *ptr++;
	
	return (checksum & 0xFF);
}

#define MPC_NEXT_ENTRY(ptr) \
        do { \
        	if (*ptr == MP_ENTRY_CPU) ptr += sizeof(struct mp_entry_cpu); \
		else ptr += 8; \
	} while(0)

void mptables_setup(void)
{
	addr_t ebda;
	uint8_t * ptr;
	struct mp_entry_cpu * cpu_entry;
	struct mp_entry_bus * bus_entry;
	struct mp_entry_ioapic * ioapic_entry;
	
	int i;
	char tmp[16];
	
	ebda = bios_get_ebda();
	if (ebda > 0)
	{
		kprintf("BIOS: Found EBDA at 0x%p\n", ebda);
		
		/* Szukamy tablic w EBDA */
		ebda = KERNEL_PHYS2VIRT(ebda);
		_mp_fps = (struct mp_fps *)ebda;
		while((addr_t)_mp_fps < ebda + 1024)
		{
			if ((_mp_fps->magic == MP_FPS_MAGIC) && (_mp_fps->lenght == 1) && 
			    (!mp_checksum((uint8_t *)_mp_fps, sizeof(struct mp_fps))) && 
			    ((_mp_fps->spec_rev == 1) || (_mp_fps->spec_rev == 4)))
				goto found;
			_mp_fps++;
		}
	}
	
	/* Szukamy w pamięci tylko do odczytu biosu */
	_mp_fps = (struct mp_fps *)KERNEL_PHYS2VIRT(0xE0000);
	while((addr_t)_mp_fps < KERNEL_PHYS2VIRT(0xFFFFF))
	{
		if ((_mp_fps->magic == MP_FPS_MAGIC) && (_mp_fps->lenght == 1) && 
		    (!mp_checksum((uint8_t *)_mp_fps, sizeof(struct mp_fps))) && 
		    ((_mp_fps->spec_rev == 1) || (_mp_fps->spec_rev == 4)))
			goto found;
		_mp_fps++;
	}

	_mp_fps = NULL;
	_mp_ctable_header = NULL;
	kprintf(KERN_WARN"MP: Floating Pointer Structure not found\n");
	return;
	
found:
	kprintf("MP: Intel MultiProcessor Specification v1.%d\n", _mp_fps->spec_rev);
	kprintf(KERN_DEBUG"MP: Floating Pointer Structure at 0x%p\n", KERNEL_VIRT2PHYS((addr_t)_mp_fps));
	
	_mp_ctable_header = (struct mp_ctable_header *)KERNEL_PHYS2VIRT(_mp_fps->ptr);
	
	/* Sprawdzamy poprawność tablicy */
	if ((!_mp_ctable_header) || (_mp_ctable_header->magic != MP_CTH_MAGIC) ||
	    ((_mp_ctable_header->spec_rev != 1) && (_mp_ctable_header->spec_rev != 4)) ||
	    (mp_checksum((uint8_t *)_mp_ctable_header, _mp_ctable_header->base_length)))
	{
		_mp_ctable_header = NULL;
		return;
	}
	
	kprintf(KERN_DEBUG"MP: Config table header at 0x%p\n", _mp_fps->ptr);
	
	ptr = (uint8_t *)((void *)_mp_ctable_header + sizeof(struct mp_ctable_header));
	
	/* Przeglądamy tablice konfiguracyjną */
	for(i=1;i<_mp_ctable_header->entry_count;i++)
	{
		switch(*ptr)
		{
			case MP_ENTRY_CPU:
			{
				cpu_entry = (struct mp_entry_cpu *)ptr;
				if ((cpu_entry->flags & MP_CPU_FLAGS_EN) != MP_CPU_FLAGS_EN)
					break;
				_mp_cpus++;
				break;
			}
			case MP_ENTRY_BUS:
			{
				bus_entry = (struct mp_entry_bus *)ptr;				
				strncpy(tmp, (char *)bus_entry->bus_type, 6);
				kprintf("MP: BUS id=%d, type=%s\n", bus_entry->bus_id, tmp);
				break;
			}
#ifdef __CONFIG_ENABLE_IOAPIC			
			case MP_ENTRY_IOAPIC:
			{

				ioapic_entry = (struct mp_entry_ioapic *)ptr;
				ioapic_add(ioapic_entry->id, ioapic_entry->address);
				break;
			}
#endif /* __CONFIG_ENABLE_IOAPIC */

		}
		MPC_NEXT_ENTRY(ptr);
	}
	kprintf(KERN_DEBUG"MP: %d CPUs detected (max %d supported)\n", _mp_cpus, CPUS_MAX);
}

#ifdef __CONFIG_ENABLE_IOAPIC
void mp_irq_route(void)
{
	int i;
	uint8_t * ptr = (uint8_t *)((void *)_mp_ctable_header + sizeof(struct mp_ctable_header));
	struct mp_entry_int * entry;
	/* Przeglądamy tablice konfiguracyjną */
	for(i=1;i<_mp_ctable_header->entry_count;i++)
	{
		entry = (struct mp_entry_int *)ptr;
		if (entry->type == MP_ENTRY_IOINT)
		{
			kprintf("MP: IRQ Link [ IRQ %d -> apic=%d, int=%d, type 0x%d, flags: 0x%x ]\n", entry->src_irq, entry->dst_apic, entry->dst_int, entry->int_type, entry->flags);			
		}
		
		MPC_NEXT_ENTRY(ptr);
	}
}
#endif /* __CONFIG_ENABLE_IOAPIC */
