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
#include <arch/page.h>
#include <kernel/types.h>
#include <kernel/panic.h>
#include <kernel/debug.h>
#include <mm/phys.h>
#include <lib/string.h>

struct pg_directory __kernel_directory __attribute__ ((aligned (PAGE_SIZE))); /* Katalog stron jądra */
struct pg_dir_pointers __kernel_dir_pointers __attribute__ ((aligned (PAGE_SIZE))); /* Wskazniki na katalogi stron */
struct pg_map_level4 __kernel_map_level4 __attribute__ ((aligned (PAGE_SIZE))); /* "Page Map Level 4" */

void paging_setup(void)
{
	paddr_t phys;
	
	/* Tworzymy nowy Page Dir Pointers dla sterty i pamieci dynamicznej */
	
	phys = phys_alloc(1, PHYS_KERNEL);
	if (!phys)
		panic("Unable to init paging (out of memory)");
		
	memset((void *)KERNEL_PHYS2VIRT(phys), 0x00, sizeof(struct pg_table));
	__kernel_map_level4.pg_dir_pointers[PGML4_ENTRY(KHEAP_START)] = phys | PG_FLAG_PRESENT | PG_FLAG_RW;
	
	/* TODO: Jeżeli system posiada więcej niż 2GiB pamięci, zamapuj resztę */
}

int paging_map_page(addr_t virt, paddr_t phys, uint16_t att, void * dataptr)
{
	struct pg_map_level4 * pml4 = dataptr;
	struct pg_dir_pointers * pdp;
	struct pg_directory * dir;
	struct pg_table * table;
	paddr_t tmp;
	
	if (!pml4)
		pml4 = &__kernel_map_level4;
	
	/* Po kolei sprawdzamy czy wymagane struktury istnieja, jesli nie to je tworzymy */
	if ((pml4->pg_dir_pointers[PGML4_ENTRY(virt)] & PG_FLAG_PRESENT) != PG_FLAG_PRESENT)
	{
		tmp = phys_alloc(1, PHYS_KERNEL);
		if (!tmp)
			return -1;
		
		memset((void *)KERNEL_PHYS2VIRT(tmp), 0x00, sizeof(struct pg_dir_pointers));
		pml4->pg_dir_pointers[PGML4_ENTRY(virt)] = tmp | PG_FLAG_PRESENT | PG_FLAG_RW | PG_FLAG_USER;
	}
	
	pdp = (struct pg_dir_pointers *)KERNEL_PHYS2VIRT(pml4->pg_dir_pointers[PGML4_ENTRY(virt)] & PG_ADDR_MASK);
	if ((pdp->pg_dirs[PDPTR_ENTRY(virt)] & PG_FLAG_PRESENT) != PG_FLAG_PRESENT)
	{
		tmp = phys_alloc(1, PHYS_KERNEL);
		if (!tmp)
			return -1;
		
		memset((void *)KERNEL_PHYS2VIRT(tmp), 0x00, sizeof(struct pg_directory));
		pdp->pg_dirs[PDPTR_ENTRY(virt)] = tmp | PG_FLAG_USER | PG_FLAG_RW | PG_FLAG_PRESENT;
	}
	
	dir = (struct pg_directory *)KERNEL_PHYS2VIRT(pdp->pg_dirs[PDPTR_ENTRY(virt)] & PG_ADDR_MASK);
	
	if ((dir->tables[PGDIR_ENTRY(virt)] & PG_FLAG_PRESENT) != PG_FLAG_PRESENT)
	{
		tmp = phys_alloc(1, PHYS_KERNEL);
		if (!tmp)
			return -1;
		
		memset((void *)KERNEL_PHYS2VIRT(tmp), 0x00, sizeof(struct pg_table));
		dir->tables[PGDIR_ENTRY(virt)] = tmp | PG_FLAG_PRESENT | PG_FLAG_RW | PG_FLAG_USER;		
	}
	
	table = (struct pg_table *)KERNEL_PHYS2VIRT(dir->tables[PGDIR_ENTRY(virt)] & PG_ADDR_MASK);
	
	table->pages[PGTAB_ENTRY(virt)] = phys | att;
	
	return 0;
}

int paging_unmap_page(addr_t virt, void * dataptr)
{
	struct pg_map_level4 * pml4 = dataptr;
	struct pg_dir_pointers * pdp;
	struct pg_directory * dir;
	struct pg_table * table;

	if (!dataptr)
		pml4 = &__kernel_map_level4;
	
	if ((pml4->pg_dir_pointers[PGML4_ENTRY(virt)] & PG_FLAG_PRESENT) != PG_FLAG_PRESENT)
		return -1;
	
	pdp = (struct pg_dir_pointers *)KERNEL_PHYS2VIRT(pml4->pg_dir_pointers[PGML4_ENTRY(virt)] & PG_ADDR_MASK);
	if ((pdp->pg_dirs[PDPTR_ENTRY(virt)] &  PG_FLAG_PRESENT) != PG_FLAG_PRESENT)
		return -1;
	
	dir = (struct pg_directory *)KERNEL_PHYS2VIRT(pdp->pg_dirs[PDPTR_ENTRY(virt)] & PG_ADDR_MASK);
	if ((dir->tables[PGDIR_ENTRY(virt)] & PG_FLAG_PRESENT) != PG_FLAG_PRESENT)
		return -1;
	
	table = (struct pg_table *)KERNEL_PHYS2VIRT(dir->tables[PGDIR_ENTRY(virt)] & PG_ADDR_MASK);
	
	table->pages[PGTAB_ENTRY(virt)] = 0;
	
	return 0;
}

void * paging_create_aspace(void)
{
	TODO("create new address space");
	return NULL;
}

void paging_destroy_aspace(void * dataptr)
{
	TODO("destroy address space");
}
