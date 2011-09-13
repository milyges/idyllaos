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
#ifndef __ARCH_MULTIBOOT_H
#define __ARCH_MULTIBOOT_H

#define MB_KERNEL_MAGIC    0x1BADB002
#define MB_LOADER_MAGIC    0x2BADB002
#define MB_FLAGS_MODALIGN  (1 << 0)
#define MB_FLAGS_MEMINFO   (1 << 1)
#define MB_FLAGS_KLUDGE    (1 << 16)
#define MB_CHECKSUM(flags) -(MB_KERNEL_MAGIC + (flags))

#ifndef __ASM__

#include <kernel/types.h>

struct multiboot_info
{
	/* Flagi */
	uint32_t flags;

	/* Rozmiar pamięci zwrócony przez BIOS */
	uint32_t mem_lower;
	uint32_t mem_upper;

	/* Urządzenie startowe */
	uint32_t boot_device;

	/* Linia poleceń kernela */
	uint32_t cmdline;

	/* Lista modułów kernela */
	uint32_t mods_count;
	uint32_t mods_addr;

	union
	{
		struct
		{
			/* (a.out) Kernel symbol table info */
			uint32_t tabsize;
			uint32_t strsize;
			uint32_t addr;
			uint32_t pad;
		} a;

		struct
		{
			/* (ELF) Kernel section header table */
			uint32_t num;
			uint32_t size;
			uint32_t addr;
			uint32_t shndx;
		} e;
 	} syms;

	/* Mapa pamięci */
	uint32_t mmap_length;
	uint32_t mmap_addr;
} PACKED;

struct multiboot_module
{
	uint32_t mod_start;
	uint32_t mod_end;
	uint32_t string;
	uint32_t reserved;
} PACKED;


struct multiboot_mmap
{
	uint32_t size;
	uint32_t base_addr_low;
	uint32_t base_addr_high;
	uint32_t length_low;
	uint32_t length_high;
	uint32_t type;
} PACKED;

#endif /* __ASM__ */

#endif /* __ARCH_MULTIBOOT_H */
