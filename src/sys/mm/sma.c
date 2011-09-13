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
#include <lib/string.h>
#include <lib/errno.h>
#include <lib/math.h>
#include <mm/heap.h>
#include <mm/sma.h>
#include <kernel/kprintf.h>
#include <kernel/debug.h>

/* Funkcja dodaje blok do listy (utrzymując listę posortowaną) */
static inline void sma_list_add(list_t * list, struct sma_block * block)
{
	struct sma_block * iter;
	if (LIST_IS_EMPTY(list))
		list_add(list, &block->list);
	else
	{
		LIST_FOREACH(list, iter)
		{
			if (iter->start > block->start)
				break;
		}

		list_add(iter->list.prev, &block->list);
	}
}

struct sma_area * sma_area_create(addr_t start, size_t size, void * do_alloc, void * do_resize, void * do_free)
{
	struct sma_area * area;
	struct sma_block * root_block;

	start = ROUND_DOWN(start, PAGE_SIZE);
	size = ROUND_UP(size, PAGE_SIZE);

	/* Tworzymy nowy obszar */
	area = kalloc(sizeof(struct sma_area));
	mutex_init(&area->mutex);
	list_init(&area->used_list);
	list_init(&area->free_list);
	area->start = start;
	area->size = size;
	area->do_alloc = do_alloc;
	area->do_free = do_free;
	area->do_resize = do_resize;

	/* Tworzymy jeden pusty blok */
	root_block = kalloc(sizeof(struct sma_block));
	memset(root_block, 0, sizeof(struct sma_block));
	list_init(&root_block->list);
	root_block->flags = 0;
	root_block->start = start;
	root_block->size = size;

	/* ...i dodajemy go do listy wolnych bloków */
	list_add(&area->free_list, &root_block->list);

	return area;
}

int sma_area_destroy(struct sma_area * area)
{
	struct sma_block * block;

	mutex_lock(&area->mutex);

	while(!LIST_IS_EMPTY(&area->used_list))
	{
		block = (struct sma_block *)area->used_list.next;
		if (area->do_free)
			area->do_free(block);

		list_remove(&block->list);
		kfree(block);
	}

	while(!LIST_IS_EMPTY(&area->free_list))
	{
		block = (struct sma_block *)area->free_list.next;
		list_remove(&block->list);
		kfree(block);
	}

	mutex_unlock(&area->mutex);

	kfree(area);
	return 0;
}

void sma_area_dump(struct sma_area * area)
{
	struct sma_block * block;
	mutex_lock(&area->mutex);
	kprintf(KERN_DEBUG"Dumping area at 0x%p, start=0x%p, size=0x%p\n", area, area->start, area->size);
	kprintf(KERN_DEBUG"   Start    |    Size    | Flags\n");
	/* Bloki użyte */
	LIST_FOREACH(&area->used_list, block)
		kprintf(KERN_DEBUG" 0x%08p | 0x%08p | 0x%X\n", block->start, block->size, block->flags);

	/* Wolne bloki */
	LIST_FOREACH(&area->free_list, block)
		kprintf(KERN_DEBUG" 0x%08p | 0x%08p | 0x%X\n", block->start, block->size, block->flags);
 	mutex_unlock(&area->mutex);
}

struct sma_block * sma_alloc(struct sma_area * area, addr_t addr, size_t size, unsigned flags)
{
	struct sma_block * block = NULL;
	struct sma_block * newblock;
	struct sma_block * newblock2;


	if (!addr)
		goto nofixed;

	mutex_lock(&area->mutex);

	/* Szukamy wolnego bloku pod podanym adresem */
	LIST_FOREACH(&area->free_list, block)
	{
		/* Sprawdzamy adres początkowy */
		if ((block->start <= addr) && (block->start + block->size > addr))
		{
			/* Sprawdzamy adres końca bloku  */
			if (block->start + block->size < addr + size)
				goto nofixed;
			/* Mamy wolny blok pod podanym adresem */
			goto doalloc;
		}
	}

nofixed:
	/* Jeżeli chcemy mieć blok pod podanym adresem a się nie da to zwracamy NULL */
	if (flags & SMA_ALLOC_FIXED)
	{
		mutex_unlock(&area->mutex);
		return NULL;
	}

	/* Szukamy pierwszego pasującego bloku */
	LIST_FOREACH(&area->free_list, block)
	{
		if (block->size >= size)
			goto doalloc;
	}
	block = NULL;

doalloc:
	/* Nie ma pasującego wolnego bloku */
	if (!block)
	{
		mutex_unlock(&area->mutex);
		return NULL;
	}

	/* Sprawdzamy czy podany adres początkowy odpowiada znalezionemu blokowi */
	if ((block->start > addr) || (block->start + block->size <= addr))
		addr = 0;

	/* Jeżeli początek wolnego bloku i bloku który alokujemy jest taki sam...*/
	if ((block->start == addr) || (!addr))
	{
		/* ... i zgadzają się rozmiary to tylko zaznaczamy go jako zajęty i przenosimy do odpowiedniej listy... */
		if (block->size == size)
		{
			block->flags |= SMA_FLAGS_USED;
			list_remove(&block->list);
			sma_list_add(&area->used_list, block);
		}
		else /* ... w przeciwnym wypadku musimy dodać nowy blok */
		{
			newblock = kalloc(sizeof(struct sma_block));
			memset(newblock, 0, sizeof(struct sma_block));
			list_init(&newblock->list);
			newblock->flags = SMA_FLAGS_USED;
			newblock->start = block->start;
			newblock->size = size;
			sma_list_add(&area->used_list, newblock);
			block->size -= size;
			block->start += size;
			block = newblock;
		}
	}
	else /* Niestety adresy początkowe nie są takie same, więc musimy dodać nowy blok */
	{
		newblock = kalloc(sizeof(struct sma_block));
		memset(newblock, 0, sizeof(struct sma_block));
		list_init(&newblock->list);
		newblock->flags = SMA_FLAGS_USED;
		newblock->start = addr;
		newblock->size = size;
		sma_list_add(&area->used_list, newblock);

		/* Sprawdzamy czy przypadkiem bloki nie kończą się w tym samym miejscu, jeśli nie musimy dodać kolejny blok */
		if ((block->start + block->size) != (addr + size))
		{
			newblock2 = kalloc(sizeof(struct sma_block));
			memset(newblock2, 0, sizeof(struct sma_block));
			list_init(&newblock2->list);
			newblock2->flags = 0;
			newblock2->start = addr + size;
			newblock2->size = (block->start + block->size) - newblock2->start;
			sma_list_add(&area->free_list, newblock2);
		}
		block->size = addr - block->start;
		block = newblock;
	}

	if (area->do_alloc) /* Jeżeli istnieje funkcja do alokacji to ją wywołujemy */
		area->do_alloc(block);

	mutex_unlock(&area->mutex);

	//sma_area_dump(area);
	return block;
}

int sma_free(struct sma_area * area, addr_t start, size_t size)
{
	struct sma_block * block;
	//struct sma_block * block2;
	addr_t addr = start;
	addr_t len;

	/* Przy zwalnianiu musimy rozpatrzyć kilka przypadków,
	    1. start i size pasują idealnie do zajętego bloku,
	    2. start i size określają nie cały blok,
	    3. start i size określają kilka bloków
	   Na początek sprawdźczy czy odnoszą się one rzeczywiście do zajętego obszaru pamięci oraz
	   znajdźmy blok początkowy */

	/* Blokujemy obszar */
	mutex_lock(&area->mutex);

	while(1)
	{
		LIST_FOREACH(&area->used_list, block)
		{
			if ((block->start <= addr) && (block->start + block->size > addr))
				break;
		}

		if ((list_t *)block == &area->used_list)
		{
			TODO("block not found");
			mutex_unlock(&area->mutex);
			while(1);
		}

		len = MIN(block->size, size); /* Bierzemy minimalną wartość */

		/* Sprawdzamy czy początek bloku zgadza się z naszym adresem */
		if (block->start == addr)
		{
			/* Jeśli zgadza się rozmiar, zaznacz blok jako wolny i dodaj do listy czystych bloków */
			if (block->size == size)
			{
				block->flags &= ~SMA_FLAGS_USED;
				if (area->do_free) /* Wywołujemy funkcję dealokującą cały blok */
					area->do_free(block);
				list_remove(&block->list);
				sma_list_add(&area->free_list, block);
				break;
			}
			else
			{
				TODO("free memory");
			}
		}
		else if (block->start + block->size == addr + size) /* Sprawdzamy czy blok i obszar do zwolnienia nie kończą się w tym samym miejscu */
		{
			TODO("free memory");
		}
		else
		{
			TODO("free memory");
		}
		break;
	}

#if 0
	/* Sprawdzamy czy nie ma na liście jakiś bloków które są wolne i ze sobą sąsiadują */
	block = (struct sma_block *)area->free_list.next;
	while(1)
	{
		/* Sprawdzamy czy aby blok nie jest ostanim blokiem */
		if (block->list.next == &area->free_list)
			break;

		block2 = (struct sma_block *)block->list.next;

		if (block->start + block->size == block2->start)
		{
			block->size += block2->size;
			list_remove(&block2->list);
			kfree(block2);
		}
		else
			block = (struct sma_block *)block->list.next;

	}
#endif
	mutex_unlock(&area->mutex);

	return 0;
}


struct sma_block * sma_getblock(struct sma_area * area, addr_t addr)
{
	struct sma_block * block;

	mutex_lock(&area->mutex);
	LIST_FOREACH(&area->used_list, block)
	{
		if ((block->start <= addr) && (block->start + block->size) > addr)
		{
			mutex_unlock(&area->mutex);
			return block;
		}
	}

	LIST_FOREACH(&area->free_list, block)
	{
		if ((block->start <= addr) && (block->start + block->size) > addr)
		{
			mutex_unlock(&area->mutex);
			return block;
		}
	}

	mutex_unlock(&area->mutex);
	return NULL;
}

struct sma_block * sma_get_next_used(struct sma_area * area, struct sma_block * current)
{
	struct sma_block * block;
	mutex_lock(&area->mutex);
	if (!current)
		block = (struct sma_block *)area->used_list.next;
	else
		block = (struct sma_block *)current->list.next;
	mutex_unlock(&area->mutex);

	if ((list_t *)block == &area->used_list)
		return NULL;
	return block;
}
