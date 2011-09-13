/*
 * Idylla Operating System
 * Copyright (C) 2009-2010  Idylla Operating System Team
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
#ifndef __KERNEL_VC_H
#define __KERNEL_VC_H

#include <kernel/types.h>

#define VC_MAX_SAVES    8 /* Maksymalna ilość zaposanych współrzędnych */
#define VC_MAX_ARGS     4 /* Maksymalna ilość argumentów do sekwencji ANSI */

#define VC_REVERSE      0x01

/* Struktura opisująca położenie kursora */
struct vc_csr
{
	unsigned col;
	unsigned row;
};

/* Wirtualna konsola */
struct vc
{
	uint8_t id; /* Identyfikator */

	struct vc_ops * ops; /* Operacje specyficzne dla konsoli */
	void * dataptr; /* Wskaźnik na dane konsoli */

	/* Rozmiar konsoli */
	unsigned width;
	unsigned height;

	struct vc_csr csr; /* Lokalizacja kursora */

	uint8_t is_active; /* Czy konsola jest aktywna */

	/* Zapisane położenia kursora */
	unsigned saves_count;
	struct vc_csr saves[VC_MAX_SAVES];

	/* Używane przy parsowaniu sekwencji ANSI */
	uint8_t esc;
	unsigned attr;
	unsigned flags;
	unsigned argc;
	unsigned argv[VC_MAX_ARGS];
};

/* Operacje na konsoli */
struct vc_ops
{
	int (*init)(struct vc * vc); /* Inicjalizacja */
	void (*clean)(struct vc * vc); /* Czyszczeine */
	void (*setattr)(struct vc * vc, unsigned attr); /* Ustaw atrybuty */
	void (*movecsr)(struct vc * vc); /* Ustaw kursor */
	int (*putc)(struct vc * vc, char c); /* Wyświetl znak */
	void (*scrollup)(struct vc * vc, int n); /* Przewiń w góre */
	void (*scrolldown)(struct vc * vc, int n); /* Przewiń w dół */
	void (*clearscreen)(struct vc * vc, int mode); /* Wyczyść ekran */
	void (*clearline)(struct vc * vc, int mode); /* Wyczyść linię */
};

int vc_putch(char c, struct vc * vc);

#endif /* __KERNEL_TYPES_H */
