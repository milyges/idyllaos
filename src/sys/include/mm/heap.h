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
#ifndef __MM_HEAP_H
#define __MM_HEAP_H

#include <kernel/types.h>
#include <arch/page.h>

void * kalloc(size_t n);
void kfree(void * p);
void * kcalloc(size_t n_elements, size_t element_size);
void * krealloc(void * p, size_t n);
void * kmemalign(size_t alignment, size_t n);
void * kvalloc(size_t n);

#endif /* __MM_HEAP_H */
