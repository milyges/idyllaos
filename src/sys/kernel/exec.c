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
#include <kernel/types.h>
#include <kernel/proc.h>
#include <kernel/kprintf.h>
#include <kernel/debug.h>
#include <kernel/vfs.h>
#include <kernel/elf.h>
#include <mm/heap.h>
#include <mm/vmm.h>
#include <lib/string.h>
#include <lib/errno.h>
#include <lib/math.h>

int sys_execve(char * path, char * argv[], char * envp[])
{
	int err, fd;
	struct stat stat;
	void * entry;
	struct vm_space * vmspace;
	struct vm_space * oldvmspace;
	void * ustack;
	void * ustack_ptr;
	size_t ustack_size;
	int i, uargc = 0, uenvc = 0, uargv_size = 0, uenv_size = 0;
	char ** uargv;
	char ** uenvp;
	char ** kargv;
	char ** kenvp;
	
	/* Sprawdzamy prawa dostepu do pliku */
	err = sys_access(path, 1, SCHED->current->proc);
	if (err != 0)
		return err;

	fd = sys_open(path, O_RDONLY, 0, SCHED->current->proc);
	if (fd < 0)
		return fd;

	sys_fstat(fd, &stat, SCHED->current->proc);
	if (!S_ISREG(stat.st_mode))
	{
		sys_close(fd, SCHED->current->proc);
		if (S_ISDIR(stat.st_mode))
			return -EISDIR;
		else
			return -EACCES;
	}

	/* TODO: Sprawdzamy czy plik jest skryptem */	
	
	/* Tworzymy przestrzen adresowa */
	vmspace = vm_space_create();
	if(!vmspace)
	{
		sys_close(fd, SCHED->current->proc);
		return -ENOMEM;
	}

	err = elf_execve(fd, vmspace, &entry);
	if (err != 0)
	{
		vm_space_destroy(vmspace);
		sys_close(fd, SCHED->current->proc);
		return err;
	}

	/* Obliczamy rozmiar argumentów i zmiennych środowiskowych */
	while(argv[uargc])
	{
		uargv_size += strlen(argv[uargc]) + 1;
		uargc++;
	}

	while(envp[uenvc])
	{
		uenv_size += strlen(envp[uenvc]) + 1;
		uenvc++;
	}

	/* Tworzymy stos użytkownika */
	ustack_size = ROUND_UP(8 * PAGE_SIZE + uargv_size + uenv_size, PAGE_SIZE);
	ustack = (void *)(USERMEM_START + USERMEM_SIZE - ustack_size);

	vm_mmap(ustack, ustack_size, PROT_READ | PROT_WRITE | MAP_FIXED | MAP_ANONYMOUS, -1, 0, vmspace, &ustack);
	ustack_ptr = (void *)(ustack + ustack_size);

	/* TODO: Zabij wszystkie wątki poza wywołującym */

	/* Kopiujemy argv i env do przestrzeni jądra */
	kargv = kalloc(sizeof(char *) * (uargc + 1));
	for(i=0;i<uargc;i++)
		kargv[i] = strdup(argv[i]);
	kargv[i] = NULL;

	kenvp = kalloc(sizeof(char *) * (uenvc + 1));
	for(i=0;i<uenvc;i++)
		kenvp[i] = strdup(envp[i]);
	kenvp[i] = NULL;

	/* Przełączamy się na nową przestrzen adresowa */
	sched_preempt_disable();
	oldvmspace = SCHED->current->proc->vmspace;
	SCHED->current->proc->vmspace = vmspace;
	vm_space_switch(SCHED->current->proc->vmspace);
	sched_preempt_enable();

	if (oldvmspace)
		vm_space_destroy(oldvmspace);

	/* Jeżeli rodzic jest zablokowany to go budzimy */
	if (SCHED->current->proc->parent->main->state == THREAD_STATE_BLOCKED)
		sched_wakeup(SCHED->current->proc->parent->main);

	/* Zamykamy pliki z flagą close-on-exec */
	for(fd=0;fd<OPEN_MAX;fd++)
	{
		if ((SCHED->current->proc->filedes[fd]) && (SCHED->current->proc->filedes[fd]->flags & FD_CLOEXEC))
			sys_close(fd, SCHED->current->proc);
	}

	/* Uwzględniamy bity set uid i set gid */
	if ((stat.st_mode & S_ISUID) == S_ISUID)
		SCHED->current->proc->euid = stat.st_uid;
	if ((stat.st_mode & S_ISGID) == S_ISGID)
		SCHED->current->proc->egid = stat.st_gid;

	/* Kopiujemy argumenty i zmienne srodowiskowe */
	ustack_ptr -= sizeof(char *) * (uargc + 1);
	uargv = ustack_ptr;
	for(i=0;i<uargc;i++)
	{
		ustack_ptr -= strlen(kargv[i]) + 1;
		strcpy(ustack_ptr, kargv[i]);
		uargv[i] = ustack_ptr;
		kfree(kargv[i]);
	}
	kfree(kargv);
	uargv[i] = NULL;

	ustack_ptr -= sizeof(char *) * (uenvc + 1);
	uenvp = ustack_ptr;
	for(i=0;i<uenvc;i++)
	{
		ustack_ptr -= strlen(kenvp[i]) + 1;
		strcpy(ustack_ptr, kenvp[i]);
		uenvp[i] = ustack_ptr;
		kfree(kenvp[i]);
	}
	kfree(kenvp);
	uenvp[i] = NULL;

	ustack_ptr -= sizeof(char **); /* envp */
	*(char ***)ustack_ptr = uenvp;
	ustack_ptr -= sizeof(char **); /* argc */
	*(char ***)ustack_ptr = uargv;
	ustack_ptr -= sizeof(int); /* argc */
	*(int *)ustack_ptr = uargc;

	/* Przechodzimy w tryb uzytkownika */
	execve_user(entry, ustack_ptr);

	panic("execve: we shouldn't be here!");
	return 0;
}
