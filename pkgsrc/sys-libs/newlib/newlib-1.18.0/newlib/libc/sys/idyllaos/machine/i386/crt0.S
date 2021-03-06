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
.text
.global _start
.extern main
.extern tzset
.extern environ
.extern _init

_start:
	movl 8(%esp), %eax
	movl %eax, environ

	/* Sekcja .init */
	call _init

	/* Inicjujemy informacje o strefie czasowej */
	call tzset

	/* Wywolujemy glowna funkcje */
	call main

	/* Wywolujemy sys_exit() */
	pushl %eax
	call _exit

.halt:
	jmp .halt

