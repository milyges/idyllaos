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
#ifndef __KERNEL_KLD_H
#define __KERNEL_KLD_H

#include <arch/atomic.h>
#include <kernel/module.h>
#include <lib/list.h>

struct kld_symbol
{
	char * name;
	void * value;
};

#ifndef __SYMTAB_C
struct kld_use_list
{
	list_t list;
	struct kld_module * module;
};

struct kld_module
{
	list_t list;

	atomic_t refs;
	unsigned flags;

	struct __module_info * info;

	void * image; /* Położenie obrazu i sekcji .bss w pamięci */
	void * bss;

	int symtab_items; /* Ilośc i tablica udostępnianych symboli */
	struct kld_symbol * symtab;

	list_t use_list; /* Lista używanych modułów */
};

void * kld_import(char * name, struct kld_module * module);
int kld_load_image(void * image, char * argv[]);
int kld_load_file(char * path, char * argv[]);
int kld_unload(char * name);
char * kld_addr2name(void * addr);

#endif /* __SYMTAB_C */

#endif /* __KERNEL_KLD_H */
