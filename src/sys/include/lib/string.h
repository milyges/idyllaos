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
#ifndef __LIB_STRING_H
#define __LIB_STRING_H

#include <kernel/types.h>

void * memcpy(void * dst, void * src, size_t n);
void * memset(void * dst, int c, size_t len);
void * memsetw(void * s, uint16_t c, int n);

int strcmp(const char * s1, const char * s2);
char * strcpy(char * dst, const char * src);
size_t strlen(const char * s);
int strncmp(const char * s1, const char * s2, size_t n);
char * strncpy(char * s1, const char * s2, size_t n);
char * strdup(const char * s);
char ** str_explode(char * str, char c);
void str_unexplode(char ** explode);

#endif /* __LIB_STRING_H */
