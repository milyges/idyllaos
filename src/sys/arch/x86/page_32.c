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
#include <arch/page.h>
#include <arch/asm.h>
#include <arch/spinlock.h>
#include <kernel/types.h>
#include <kernel/debug.h>
#include <mm/phys.h>
#include <lib/string.h>

/* Katalog stron jądra */
struct pg_directory __kernel_directory __attribute__ ((aligned (PAGE_SIZE)));

/* Podpięcie tablic stron do katalogu stron jądra */
void paging_setup(void)
{
	int i;
	paddr_t table_phys;
	for(i=PGDIR_ENTRY(KHEAP_START);i<1024;i++)
	{
		table_phys = phys_alloc(1, PHYS_KERNEL);
		memset((void *)KERNEL_PHYS2VIRT(table_phys), 0x00, sizeof(struct pg_table));
		__kernel_directory.tables[i] = table_phys | PG_FLAG_PRESENT | PG_FLAG_RW;
	}
}

int paging_map_page(addr_t virt, paddr_t phys, uint16_t att, void * dataptr)
{
	struct pg_directory * dir = dataptr;
	struct pg_table * table = NULL;
	paddr_t table_phys;

	if (!dir)
		dir = &__kernel_directory;

	/* Sprawdzamy czy istnieje odpowiednia tablica stron */
	if ((dir->tables[PGDIR_ENTRY(virt)] & PG_FLAG_PRESENT) != PG_FLAG_PRESENT)
	{
		/* Zaalokuj stronę na tablicę z pamięci niskiej */
		table_phys = phys_alloc(1, PHYS_KERNEL);
		if (!table_phys)
		{
			kprintf("paging_map_page(): failed\n");
			return -1;
		}

		memset((void *)KERNEL_PHYS2VIRT(table_phys), 0x00, sizeof(struct pg_table));
		dir->tables[PGDIR_ENTRY(virt)] = table_phys | PG_FLAG_USER | PG_FLAG_RW | PG_FLAG_PRESENT;
	}

	table = (struct pg_table *)KERNEL_PHYS2VIRT((addr_t)dir->tables[PGDIR_ENTRY(virt)] & PG_ADDR_MASK);
	table->pages[PGTAB_ENTRY(virt)] = phys | att;

	/* Czyscimy cache od tej strony */
	invplg((void *)virt);

	return 0;
}

int paging_unmap_page(addr_t virt, void * dataptr)
{
	struct pg_directory * dir = dataptr;
	struct pg_table * table = NULL;

	if (!dir)
		dir = &__kernel_directory;

	if ((dir->tables[PGDIR_ENTRY(virt)] & PG_FLAG_PRESENT) != PG_FLAG_PRESENT)
		return -1;

	//if (virt >= 0xF0000000) kprintf("paging_unmap_page(%x)\n", virt);
	table = (struct pg_table *)KERNEL_PHYS2VIRT((addr_t)dir->tables[PGDIR_ENTRY(virt)] & PG_ADDR_MASK);
	table->pages[PGTAB_ENTRY(virt)] = 0;

	/* Czyscimy cache */
	invplg((void *)virt);

	return 0;
}

void * paging_create_aspace(void)
{
	struct pg_directory * dir;
	paddr_t dir_phys;

	dir_phys = phys_alloc(1, PHYS_KERNEL);
	if (!dir_phys)
		return NULL;

	dir = (struct pg_directory *)KERNEL_PHYS2VIRT(dir_phys);

	/* Kopiujemy zawartość katalogu kernela */
	memcpy(dir, &__kernel_directory, sizeof(struct pg_directory));

	return dir;
}

void paging_destroy_aspace(void * dataptr)
{
	struct pg_directory * dir = dataptr;
	int i;

	for(i=0;i<PGDIR_ENTRY(LOWMEM_START);i++)
	{
		if (dir->tables[i] & PG_FLAG_PRESENT)
			phys_free(dir->tables[i] & PG_ADDR_MASK, 1);
	}
}

void paging_switch_aspace(void * dataptr)
{
	paddr_t dir;

	if (!dataptr)
		dir = KERNEL_VIRT2PHYS((addr_t)&__kernel_directory);
	else
		dir = KERNEL_VIRT2PHYS((addr_t)dataptr);

	write_cr3(dir);
}
