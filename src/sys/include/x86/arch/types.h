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
#ifndef __ARCH_TYPES_H
#define __ARCH_TYPES_H

#ifndef __ASM__

/* Standardowe typy liczbowe */
typedef signed char int8_t;
typedef signed short int16_t;
typedef signed int int32_t;

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;

#ifdef __X86_64__
typedef signed long int64_t;
typedef unsigned long uint64_t;
#else /* __X86_64__ */
typedef signed long long int64_t;
typedef unsigned long long uint64_t;
#endif /* __X86_64__ */

typedef unsigned long addr_t; /* Adres wirtualny */
typedef unsigned long reg_t; /* Rejestr procesora */

/* Adres fizyczny */
#if defined(__CONFIG_ENABLE_PAE) && !defined(__X86_64__)
typedef uint64_t paddr_t;
#else /* defined(__CONFIG_ENABLE_PAE) && !defined(__X86_64__) */
typedef unsigned long paddr_t;
#endif /* defined(__CONFIG_ENABLE_PAE) && !defined(__X86_64__) */

#endif /* __ASM__ */

#endif /* __ARCH_TYPES_H */
