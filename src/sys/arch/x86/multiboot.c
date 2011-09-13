/*
 * Idylla Operating System
 * Copyright (C) 2009-2010  Idylla Operating System Team
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
#include <arch/multiboot.h>
#include <arch/page.h>
#include <kernel/kprintf.h>
#include <kernel/debug.h>
#include <kernel/kld.h>
#include <mm/phys.h>
#include <lib/string.h>
#include <lib/math.h>

static struct multiboot_info * _bootinfo;
int __kernel_argc;
char ** __kernel_argv;

static void _multibot_memory_map(void)
{
	paddr_t start, len;
	struct multiboot_mmap * map = (struct multiboot_mmap *)(LOWMEM_START + _bootinfo->mmap_addr);

	kprintf("Bios memory map:\n");
	while((addr_t)map < (addr_t)(LOWMEM_START + _bootinfo->mmap_addr + _bootinfo->mmap_length))
	{
		kprintf ("  0x%08X%08X size: 0x%08X%08X, %s\n", map->base_addr_high, map->base_addr_low, map->length_high, map->length_low, (map->type == 1) ? "avaiable" : "reserved");
		if (map->type == 1)
		{
#if defined(__X86_64__)
			/* Bez tego gcc robi nam warny... */
			start = map->base_addr_high;
			start <<= 32;
			start |= map->base_addr_low;

			len = map->length_high;
			len <<= 32;
			len |= map->length_low;
			phys_free(start, len / PAGE_SIZE);
#else /* defined(__X86_64__) */
			if (!map->base_addr_high) /* TODO: PAE Support */
			{
				start = map->base_addr_low;
				if (map->length_high > 0)
					len = 0xFFFFFFFF;
				else
					len = map->length_low;
				if (start + len > 0xFFFFFFFF)
					len = 0xFFFFFFFF - start;

				phys_free(start, len / PAGE_SIZE);
			}
#endif /* defined(__X86_64__) */
		}
		map = (struct multiboot_mmap *)((addr_t)map + map->size + sizeof(map->size));
	}
}

void multiboot_init(uint32_t magic, struct multiboot_info * info)
{
	extern int _end;
	int i;
	addr_t bitmap = KERNEL_VIRT2PHYS((addr_t)&_end);
	struct multiboot_module * mods;

	_bootinfo = info;
	kprintf("multiboot: magic: 0x%08X, info at 0x%p\n", magic, info);
	kprintf("Kernel command line: %s\n", (char *)(info->cmdline + LOWMEM_START));

	/* Liczymy gdzie konczy sie pamiec jądra i modułów */
	mods = (struct multiboot_module *)KERNEL_PHYS2VIRT(info->mods_addr);
	for(i=0;i<info->mods_count;i++)
	{
		if (mods[i].mod_end > bitmap)
			bitmap = mods[i].mod_end;
	}

	/* Inicjujemy bitmape pamięci fizycznej */
	phys_init((void *)KERNEL_PHYS2VIRT(bitmap), (_bootinfo->mem_upper + 1024) / 4);

	/* Parsujemy mape pamieci dostarczona przez BIOS */
	_multibot_memory_map();

	/* Oznaczamy pierwszą ramkę jako użytą */
	phys_reserve(0, PAGE_SIZE);

	/* Oznaczamy pamięć kernela i modułów jako użytą */
	phys_reserve(KERNEL_PHYS_ADDR, ROUND_UP(((addr_t)bitmap - KERNEL_PHYS_ADDR), PAGE_SIZE));

	/* Oznaczamy informacje od bootloadera oraz info o modulach jako uzyta pamiec (po zaladowaniu modułów zostanie ona zwolniona) */
	phys_reserve(ROUND_DOWN(KERNEL_VIRT2PHYS((addr_t)_bootinfo), PAGE_SIZE), PAGE_SIZE);
	phys_reserve(ROUND_DOWN(info->mods_addr, PAGE_SIZE), PAGE_SIZE);

	/* Oznaczamy bitmapę jako zajętą */
	phys_reserve_self();

	kprintf("%uMB avaiable memory detected\n", (_bootinfo->mem_upper + 1024) / 1024);

	/* Inicjujemy stronnicowanie */
	paging_setup();

	/* Parsujemy linię poleceń jądra */
	__kernel_argv = str_explode((char *)(info->cmdline + LOWMEM_START), ' ');
	while(__kernel_argv[++__kernel_argc]);
}

void multiboot_load_modules(void)
{
	int i;
	struct multiboot_module * mods;
	char ** argv;

	kprintf("multiboot: loading modules...\n");

	mods = (struct multiboot_module *)KERNEL_PHYS2VIRT(_bootinfo->mods_addr);
	for(i=0;i<_bootinfo->mods_count;i++)
	{
		argv = str_explode((char *)KERNEL_PHYS2VIRT(mods[i].string), ' ');
		kld_load_image((void *)KERNEL_PHYS2VIRT(mods[i].mod_start), argv);
		str_unexplode(argv);
	}

	/* Zwalniamy niepotrzebna pamiec */
	phys_free(ROUND_DOWN(_bootinfo->mods_addr, PAGE_SIZE), 1);
	phys_free(ROUND_DOWN(KERNEL_VIRT2PHYS((addr_t)_bootinfo), PAGE_SIZE), 1);
}
