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
#include <kernel/types.h>
#include <lib/string.h>
#include <mm/heap.h>

#define WORD_SIZE      sizeof(uint_t)  /* Rozmiar słowa dla architektury */
#define UNALIGNED(x)   ((uint_t)(x) & (WORD_SIZE - 1))

void * memcpy(void * dst, void * src, size_t len)
{
	char * dbuf = dst;
	char * sbuf = src;
	uint_t * aligned_dbuf;
	uint_t * aligned_sbuf;

	/* Wyronójemy do pełnego słowa */
	while(UNALIGNED(sbuf))
	{
		if(len--)
			*dbuf++ = *sbuf++;
		else
			return dst;
	}

	/* Jeżeli lokalizacja docelowa nie jest wyrównana, kopiujemy po bajcie... */
	if ((len >= WORD_SIZE) && (!UNALIGNED(dbuf)))
	{
		aligned_dbuf = (uint_t *)dbuf;
		aligned_sbuf = (uint_t *)sbuf;

		while(len >= WORD_SIZE)
		{
			*aligned_dbuf++ = *aligned_sbuf++;
			len -= WORD_SIZE;
		}

		sbuf = (char *)aligned_sbuf;
		dbuf = (char *)aligned_dbuf;
	}

	while(len--)
		*dbuf++ = *sbuf++;

	return dst;
}


void * memset(void * dst, int c, size_t len)
{
	uint8_t * buf = dst;
	uint_t * aligned_buf;
	uint_t tmp = 0;
	int i;

	/* Aż nie osiągniemy wyrównania do pełnego słowa kopiujemy po bajcie */
	while(UNALIGNED(buf))
	{
		if (len--)
			*buf++ = (uint8_t)c;
		else
			return dst;
	}

	/* Jeżeli to co zostało jest równe lub większe od 4 słów */
	if (len >= WORD_SIZE * 4)
	{
		/* Generujemy pełne słowo wzorcem */
		tmp = (uint8_t)c;
		for(i = 0; i < WORD_SIZE - 1; i++)
			tmp = (tmp << 8) | (uint8_t)c;

		aligned_buf = (uint_t *)buf;

		while(len >= WORD_SIZE)
		{
			*aligned_buf++ = tmp;
			len -= WORD_SIZE;
		}

		buf = (uint8_t *)aligned_buf;
	}

	while(len--)
		*buf++ = (char)c;

	return dst;
}


void * memsetw(void * dst, uint16_t c, int len)
{
	uint16_t * buf = dst;

	if ((sizeof(uint_t) % sizeof(uint16_t)) == 0)
	{

	}

	while(len--)
		*buf++ = c;

	return dst;
}

int memcmp(void * p1, void * p2, int len)
{
	char * c1 = p1;
	char * c2 = p2;
	int i = 0;
	
	while((i < len) && (*c1 == *c2))
	{
		c1++;
		c2++;
		i++;
	}
	
	return (i == len) ? 0 : (int)(*c1 - *c2);
}

int strcmp(const char * s1, const char * s2)
{
	while((*s1) && (*s1 == *s2))
 	{
		s1++;
		s2++;
	}
	return (int)(*s1 - *s2);
}


char * strcpy(char * dst, const char * src)
{
	char * tmp = (char *)dst;

	while(*src)
		*tmp++ = *src++;

	*tmp = '\0';
	return dst;
}

size_t strlen(const char * s)
{
	char * buf = (char *)s;
	size_t len = 0;

	while(*buf++)
		len++;

	return len;
}

int strncmp(const char * s1, const char * s2, size_t n)
{
	do
	{
		if(*s1 == *s2)
		{
			s1++;
			s2++;
			n--;
			continue;
		}
		else
			return (int)(*s1 - *s2);

	} while((n) && (*s1));
	return 0;
}

char * strncpy(char * dst, const char * src, size_t n)
{
	int i = 0;
	while((src[i]) && (i < n))
	{
		dst[i] = src[i];
		i++;
	}
	dst[i] = '\0';
	return dst;
}

char * strdup(const char * s)
{
	char * buf = kalloc(strlen(s) + 1);
	strcpy(buf, s);
	return buf;
}


void str_unexplode(char ** explode)
{
	int i = 0;
	while(explode[i])
		kfree(explode[i++]);
	kfree(explode);
}

char ** str_explode(char * str, char c)
{
	int depth = 0;
	int i = 0, old;
	char ** exploded = NULL;

	exploded = (char **)kalloc(sizeof(char *));

	while(str[i] == c)
		i++;

	if (str[i] == '\0')
	{
		exploded[0] = NULL;
		return exploded;
	}

	old = i;
	for(;i<strlen(str)+1;i++)
	{
		if ((str[i] == c) || (str[i] == '\0'))
		{
			depth++;
			exploded = krealloc(exploded, (depth + 1) * sizeof(char *));
			exploded[depth - 1] = kalloc((i - old + 1) * sizeof(char));
			strncpy(exploded[depth - 1], &str[old], (i - old * sizeof(char)));
			while(str[i] == c)
				i++;
			old = i;
		}
	}

	exploded[depth] = NULL;
	return exploded;
}
