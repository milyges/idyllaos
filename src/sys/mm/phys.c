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
#include <arch/spinlock.h>
#include <kernel/types.h>
#include <kernel/bitops.h>
#include <kernel/kprintf.h>
#include <kernel/proc.h>
#include <mm/phys.h>
#include <lib/string.h>
#include <lib/math.h>

static unsigned long * _bitmap;
static size_t _bitmap_size;
static unsigned long _bitmap_items;
static SPINLOCK_NEW(_bitmap_lock);

#define ADDR2IDX(a)    ((a) / (8 * sizeof(unsigned long)))
#define ADDR2OFF(a)    ((a) % (8 * sizeof(unsigned long)))

#define SET_FRAME(x)   set_bit(&_bitmap[ADDR2IDX(x)], ADDR2OFF(x))
#define CLEAR_FRAME(x) clear_bit(&_bitmap[ADDR2IDX(x)], ADDR2OFF(x))
#define TEST_FRAME(x)  test_bit(_bitmap[ADDR2IDX(x)], ADDR2OFF(x))

void phys_init(void * ptr, size_t frames)
{
	_bitmap = ptr;
	_bitmap_items = ROUND_UP(frames, sizeof(unsigned long) * 8);
	_bitmap_size = _bitmap_items / 8 / sizeof(unsigned long);

	/* Poczatkowo cala pamiec zostaje oznaczona jako zajeta, zostanie wyzerowana dopiero przez arch_setup() */
	memset(_bitmap, 0xFF, _bitmap_size * sizeof(unsigned long));

	kprintf("phys: memory bitmap at 0x%p, %lu items (%d bytes)\n", _bitmap, _bitmap_items, _bitmap_size * sizeof(unsigned long));
}

void phys_free(paddr_t start, size_t len)
{
	unsigned long i, frame = start / PAGE_SIZE;

 	if (frame >= _bitmap_items)
    		return;
 	if (frame + len >= _bitmap_items)
    		len = _bitmap_items - frame;

	spinlock_lock(&_bitmap_lock);
	for(i=frame;i<frame+len;i++)
		CLEAR_FRAME(i);
	spinlock_unlock(&_bitmap_lock);
}

void phys_reserve(paddr_t start, size_t len)
{
	unsigned long i, frame = start / PAGE_SIZE;
	unsigned long pages = len / PAGE_SIZE;

 	if (frame >= _bitmap_items)
    		return;
 	if (frame + pages >= _bitmap_items)
    		pages = _bitmap_items - frame;

	kprintf("phys: reserve memory from 0x%p, length: 0x%p\n", start, len);
	spinlock_lock(&_bitmap_lock);
	for(i = frame; i < frame + pages; i++)
		SET_FRAME(i);
	spinlock_unlock(&_bitmap_lock);
}

/* Funkcja rezerwuje pamięć dla bitmapy */
void phys_reserve_self(void)
{
	phys_reserve(ROUND_DOWN((addr_t)_bitmap, PAGE_SIZE), ROUND_UP(_bitmap_size, PAGE_SIZE));
}

/* Funkcja zwraca pierwszą wolną stronę w bitmapie począwszy od start */
static unsigned long inline _find_first_zero(unsigned long start)
{
	unsigned long i;
 	for (i=ADDR2IDX(start);i<_bitmap_size;i++)
	{
		if (_bitmap[i] != ~0UL) /* mamy conajmniej jeden pusty bit tutaj */
			 return i * sizeof(unsigned long) * 8 + ffz_bit(_bitmap[i]);
  	}

  	return 0;
}

static paddr_t _alloc_frames(paddr_t start, size_t len)
{
	unsigned i, c;
 	paddr_t frame;
 	i = start / PAGE_SIZE;
	spinlock_lock(&_bitmap_lock);

	while(1)
	{
		frame = _find_first_zero(i);
		if (!frame)
		{
			spinlock_unlock(&_bitmap_lock);
			return 0;
  		}

		c = len; i = frame;

		while(c)
		{
			if (TEST_FRAME(i))
				break;
			c--; i++;
		}

		if (!c)
		{
			for(i=0;i<len;i++)
				SET_FRAME(i + frame);
   			spinlock_unlock(&_bitmap_lock);
   			return (paddr_t)(frame * PAGE_SIZE);
  		}
 	}

 	spinlock_unlock(&_bitmap_lock);
 	return (paddr_t)0;
}

paddr_t phys_alloc(size_t len, unsigned flags)
{
	paddr_t page = 0;

#ifndef __X86_64__
	/* Na początek próbujemy zaalokować stronę z pamięci wysokiej */
	if ((flags & PHYS_ZONE_HIGH) == PHYS_ZONE_HIGH)
	{
		page = _alloc_frames(LOWMEM_SIZE, len);
		if (page)
			return page;
	}
#endif /* __X86_64__ */

	/* Następnie z pamięci normalnej */
	if ((flags & PHYS_ZONE_NORMAL) == PHYS_ZONE_NORMAL)
	{
		page = _alloc_frames(DMA_AREA_SIZE, len);
		if (page)
			return page;
	}


	/* Jeżeli brakuje pamięci normalnej, spróbujmy z obszaru DMA */
	page = _alloc_frames(0, len);
	if (page)
		return page;

	/* Jezeli mozemy, probojemy zapisać bufory na dysk */
	if (flags & PHYS_ALLOW_IO)
	{
		//TODO: Zapisz bufory na dysk
		page = _alloc_frames(0, len);
		if (page)
			return page;
	}

	/* Nadal brakuje pamieci, jesli mozemy, probojemy uruchomic swappera */
	if (flags & PHYS_ALLOW_WAIT)
	{
		//TODO: Obudz demona
		//schedule();

		page = _alloc_frames(0, len);
		if (page)
			return page;
	}

	/* Niestety nie mamy pamieci :( */
	kprintf("phys_alloc(): out of memory\n");
	return 0;
}
