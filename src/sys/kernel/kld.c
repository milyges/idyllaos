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
#include <arch/spinlock.h>
#include <kernel/types.h>
#include <kernel/kld.h>
#include <kernel/kprintf.h>
#include <kernel/debug.h>
#include <kernel/elf.h>
#include <kernel/vfs.h>
#include <mm/heap.h>
#include <lib/string.h>
#include <lib/errno.h>
#include <lib/list.h>

static LIST_NEW(_modules_list);
static SPINLOCK_NEW(_modules_list_lock);

/* Obsługa listy wykorzystywania modułów przez inne moduły */
static void use_list_add(struct kld_module * parent, struct kld_module * module)
{
	struct kld_use_list * mod_use;

	LIST_FOREACH(&parent->use_list, mod_use)
	{
		if (mod_use->module == module) /* Jest już na liście */
			return;
	}

	mod_use = kalloc(sizeof(struct kld_use_list));
	list_init(&mod_use->list);
	mod_use->module = module;
	list_add(&parent->use_list, &mod_use->list);
	atomic_inc(&module->refs);
}

static void use_list_release(struct kld_module * module)
{
	struct kld_use_list * moduse;

	while(!LIST_IS_EMPTY(&module->use_list))
	{
		moduse = (struct kld_use_list *)module->use_list.next;
		list_remove(&moduse->list);
		atomic_dec(&moduse->module->refs);
		kfree(moduse);
	}
}

void * kld_import(char * name, struct kld_module * module)
{
	int i;
	struct kld_module * iter;
	extern struct kld_symbol __ksymtab[];

	/* Szukemy w symbolach kernela */
	i = 0;
	while(__ksymtab[i].value)
	{
		if (!strcmp(name, __ksymtab[i].name))
			return __ksymtab[i].value;
		i++;
	}

	/* Próbujemy zaimportować z innego modułu */
	spinlock_lock(&_modules_list_lock);
	LIST_FOREACH(&_modules_list, iter)
	{
		for(i=0;i<iter->symtab_items;i++)
		{
			if (!strcmp(iter->symtab[i].name, name))
			{
				use_list_add(module, iter);
				spinlock_unlock(&_modules_list_lock);
				return iter->symtab[i].value;
			}
		}
	}
	spinlock_unlock(&_modules_list_lock);
	return NULL;
}

char * kld_addr2name(void * addr)
{
	int i;
	extern struct kld_symbol __ksymtab[];

	i = 0;
	while(__ksymtab[i].value)
	{
		if (__ksymtab[i].value == addr)
			return __ksymtab[i].name;
		i++;
	}
	
	return NULL;
}

int kld_load_image(void * image, char * argv[])
{
	int err, argc;
	struct kld_module * module;
	struct kld_module * iter;
	module = kalloc(sizeof(struct kld_module));
	memset(module, 0, sizeof(struct kld_module));

	list_init(&module->list);
	atomic_set(&module->refs, 0);
	list_init(&module->use_list);
	module->image = image;

	/* Relokujemy moduł */
	err = elf_module(module);
	if (err != 0)
	{
		use_list_release(module);
		if (module->symtab) kfree(module->symtab);
		if (module->bss) kfree(module->bss);
		kfree(module);
		return err;
	}

	/* Sprawdzamy czy taki moduł nie jest załadowany */
	spinlock_lock(&_modules_list_lock);
	LIST_FOREACH(&_modules_list, iter)
	{
		if (!strcmp(iter->info->name, module->info->name))
		{
			spinlock_unlock(&_modules_list_lock);
			use_list_release(module);
			if (module->symtab) kfree(module->symtab);
			if (module->bss) kfree(module->bss);
			kfree(module);
			return -EEXIST;
		}
	}
	
	spinlock_unlock(&_modules_list_lock);
	/* Obliczamy argc */
	argc = 0;
	while(argv[++argc]);

	/* Odpalamy kod inicjujący w module */
	err = module->info->init(argc, argv);
	if (err != 0)
	{
		use_list_release(module);
		if (module->symtab) kfree(module->symtab);
		if (module->bss) kfree(module->bss);
		kfree(module);
		return err;
	}

	spinlock_lock(&_modules_list_lock);
	list_add(&_modules_list, &module->list);
	spinlock_unlock(&_modules_list_lock);

	return 0;
}

int kld_load(char * path, char * argv[])
{
	void * image;
	struct stat stat;
	int fd, err;
	
	if ((fd = sys_open(path, O_RDONLY, 0, SCHED->current->proc)) < 0)
		return fd;
	
	err = sys_fstat(fd, &stat, SCHED->current->proc);
	if (err < 0)
	{
		sys_close(fd, SCHED->current->proc);
		return err;
	}
	
	image = kalloc(stat.st_size);
	err = sys_read(fd, image, stat.st_size, SCHED->current->proc);
	sys_close(fd, SCHED->current->proc);
	
	if (err < 0)
	{
		kfree(image);
		return err;
	}
	
	err = kld_load_image(image, argv);
	if (err < 0)
		kfree(image);
	
	return err;
}

int kld_unload(char * name)
{
	return -ENOSYS;
}
