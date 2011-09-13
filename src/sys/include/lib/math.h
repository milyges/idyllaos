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
#ifndef __LIB_MATH_H
#define __LIB_MATH_H

#define ROUND_UP(v,n)    (((v) % (n)) ? ((v) + ((n) - ((v) % (n)))) : (v))
#define ROUND_DOWN(v,n)  ((v) - ((v) % (n)))
#define MAX(a,b)         ((a > b) ? (a) : (b))
#define MIN(a,b)         ((a < b) ? (a) : (b))

#endif /* __LIB_MATH_H */
