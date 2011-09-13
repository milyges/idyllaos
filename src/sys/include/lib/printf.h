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
#ifndef __LIB_PRINTF_H
#define __LIB_PRINTF_H

#include <kernel/types.h>
#include <lib/stdarg.h>

int doprintf(char * fmt, va_list args, int (*func)(unsigned c, void ** ptr), void * ptr);
char * sprintf(char * buf, char * fmt, ...);
char * snprintf(char * buf, size_t len, char * fmt, ...);

#endif
