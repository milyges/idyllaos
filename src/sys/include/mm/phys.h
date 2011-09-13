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
#ifndef __MM_PHYS_H
#define __MM_PHYS_H

#include <kernel/types.h>

#define PHYS_ZONE_NORMAL 0x01
#define PHYS_ZONE_DMA    0x02
#define PHYS_ZONE_HIGH   0x04

#define PHYS_ALLOW_WAIT  0x08
#define PHYS_ALLOW_IO    0x10

#define PHYS_ATOMIC      (PHYS_ZONE_NORMAL)
#define PHYS_KERNEL      (PHYS_ZONE_NORMAL | PHYS_ALLOW_WAIT | PHYS_ALLOW_IO)

void phys_init(void * ptr, size_t frames);
void phys_free(paddr_t start, size_t len);
void phys_reserve(paddr_t start, size_t len);
void phys_reserve_self(void);
paddr_t phys_alloc(size_t len, unsigned flags);

#endif /* __MM_PHYS_H */
