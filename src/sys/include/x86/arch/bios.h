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
#ifndef __ARCH_BIOS_H
#define __ARCH_BIOS_H

#include <arch/page.h>

static inline addr_t bios_get_ebda(void)
{
 addr_t addr = *(uint16_t *)KERNEL_PHYS2VIRT(0x40E);
 addr <<= 4;
 return addr;
}

#endif /* __ARCH_BIOS_H */
