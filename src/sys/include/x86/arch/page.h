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
#ifndef __ARCH_PAGE_H
#define __ARCH_PAGE_H

#define PAGE_SIZE           0x1000

/* Mapa pamięci wirtualnej */
#ifdef __X86_64__
#define USERMEM_START       0x0000000000400000 /* Pamięć aplikacji */
#define USERMEM_SIZE        0x0000800000000000
#define LOWMEM_START        0xFFFF800000000000 /* Pamięć fizyczna zamapowana od tych adresów */
#define LOWMEM_SIZE         (KHEAP_START - LOWMEM_START)
#define KHEAP_START         0xFFFFFFFF80000000 /* Sterta kernela */
#define KHEAP_SIZE          0x0000000040000000
#define KMEM_START          0xFFFFFFFFC0000000 /* Dynamiczna pamięć jądra */
#define KMEM_SIZE           0x0000000040000000
#else /* __X86_64__ */
#define USERMEM_START       0x00400000 /* Pamięć aplikacji */
#define USERMEM_SIZE        (LOWMEM_START - USERMEM_START)
#define LOWMEM_START        0xC0000000 /* Pierwsze 512MB pamięci fizycznej */
#define LOWMEM_SIZE         0x20000000
#define KHEAP_START         0xE0000000 /* Sterta kernela */
#define KHEAP_SIZE          0x10000000
#define KMEM_START          0xF0000000 /* Dynamiczna pamięć jądra */
#define KMEM_SIZE           0x10000000
#endif

#define KERNEL_PHYS_ADDR    __CONFIG_KLOAD_ADDR
#define KERNEL_VIRT_ADDR    (LOWMEM_START + KERNEL_PHYS_ADDR)

/* Makra do łatwego przeliczania adresów dla pamięci niskiej */
#define KERNEL_PHYS2VIRT(x) ((x) + LOWMEM_START)
#define KERNEL_VIRT2PHYS(x) ((x) - LOWMEM_START)

/* Makro sprawdza czy adres znajduje się w pamięci wysokiej */
#define IS_IN_HIGHMEM(x)    ((x) >= LOWMEM_SIZE)

/* Makro sprawdza czy adres znajduje sie w pamieci uzytkownika */
#define IS_IN_USERMEM(x)    (((addr_t)(x) >= USERMEM_START) && ((addr_t)(x) < (USERMEM_START + USERMEM_SIZE)))

/* Obszar pamieci zarezerwowany dla DMA (16MB) */
#define DMA_AREA_SIZE       0x01000000

/* Flagi stron */
#define PG_FLAG_PRESENT     (1 << 0) /* Dostępna */
#define PG_FLAG_RW          (1 << 1) /* Zapisywalna */
#define PG_FLAG_USER        (1 << 2) /* Dostępna dla aplikacji */
#define PG_FLAG_NOCACHE     (1 << 4) /* Wylączone cache */
#define PG_FLAG_ACCESSED    (1 << 5) /* Strona była użyta */
#define PG_FLAG_DIRTY       (1 << 6) /* Strona była zapisywana */
#define PG_FLAG_SIZE        (1 << 7) /* Strona 4/2MB */
#define PG_FLAG_GLOBAL      (1 << 8) /* Strona globalna */
/* TODO: Pozostałe flagi */


/* Makra do indeksowania tablic */
#ifndef __X86_64__
#define PG_ADDR_MASK        0xFFFFF000
#ifdef __CONFIG_ENABLE_PAE
#define PGDIR_ENTRY(x)      (((x) >> 21) & 0x1FF)
#define PGTAB_ENTRY(x)      (((x) >> 12) & 0x1FF)
#define PDPTR_ENTRY(x)      ((x) >> 30)
#else /* __CONFIG_ENABLE_PAE */
#define PGDIR_ENTRY(x)      ((x) >> 22)
#define PGTAB_ENTRY(x)      (((x) >> 12) & 0x3FF)
#endif /* __CONFIG_ENABLE_PAE */
#else /* __X86_64__ */
#define PG_ADDR_MASK        0xFFFFFFFFFFFFF000
#define PGML4_ENTRY(x)      ((x >> 39) & 0x1FF)
#define PDPTR_ENTRY(x)      ((x >> 30) & 0x1FF)
#define PGDIR_ENTRY(x)      ((x >> 21) & 0x1FF)
#define PGTAB_ENTRY(x)      ((x >> 12) & 0x1FF)
#endif /* __X86_64__ */

#define PGFAULT_PRESENT     (1 << 0)
#define PGFAULT_WRITE       (1 << 1)
#define PGFAULT_USER        (1 << 2)

#ifndef __ASM__

#include <kernel/types.h>

/* Tablica stron */
struct pg_table
{
	paddr_t pages[PAGE_SIZE / sizeof(paddr_t)]; /* Magia, zawsze mamy 4k na tablice stron ;) */
};

/* Katalog stron */
struct pg_directory
{
	paddr_t tables[PAGE_SIZE / sizeof(paddr_t)]; /* i znów magia */
};

/* Dodatkowe struktury dla 64b */
#ifdef __X86_64__
struct pg_dir_pointers
{
	paddr_t pg_dirs[512];
};

struct pg_map_level4
{
	paddr_t pg_dir_pointers[512];
};
#endif /* __X86_64__ */

void paging_setup(void);
int paging_map_page(addr_t virt, paddr_t phys, uint16_t att, void * dataptr);
int paging_unmap_page(addr_t virt, void * dataptr);
void * paging_create_aspace(void);
void paging_destroy_aspace(void * dataptr);
void paging_switch_aspace(void * dataptr);

#endif /* __ASM__ */

#endif /* __ARCH_PAGE_H */
