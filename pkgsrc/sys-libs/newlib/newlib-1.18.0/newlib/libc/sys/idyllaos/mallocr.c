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
#include <_ansi.h>
#include <reent.h>
#include <stdlib.h>
#include <malloc.h>

void * _malloc_r (struct _reent * ptr, size_t size)
{
	return malloc(size);
}

void _free_r (struct _reent * ptr, void * addr)
{
	free(addr);
}

void * _calloc_r(struct _reent * ptr, size_t size, size_t len)
{
	return calloc(size, len);
}

void * _realloc_r(struct _reent * ptr, void * old, size_t newlen)
{
	return realloc(old, newlen);
}
