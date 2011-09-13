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
#ifndef __LIB_CTYPE_H
#define __LIB_CTYPE_H

static inline int isdigit(int c)
{
	return ((c >= '0') && (c <= '9'));
}

static inline int isspace(int c)
{
	return ((c == ' ') || (c == '\t') || (c == '\n') || (c == '\r'));
}

static inline int isprint(int c)
{
	return ((c >= ' ') && (c < 0x80));
}

static inline int toupper(int c)
{
	if((unsigned int)(c - 'a') < 26u)
		c += 'A' - 'a';
	return c;
}

static inline int tolower(int c)
{
	if((unsigned int)(c - 'A') < 26u)
		c += 'a' - 'A';
	return c;
}

#endif /* __LIB_CTYPE_H */
