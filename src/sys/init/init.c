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
#include <arch/cpu.h>
#include <arch/stacktrace.h>
#include <kernel/types.h>
#include <kernel/kprintf.h>
#include <kernel/proc.h>
#include <kernel/device.h>
#include <kernel/init.h>
#include <kernel/vfs.h>
#include <mm/heap.h>
#include <lib/string.h>

struct proc * __init_proc;

void init_exec(char * path)
{
	char * argv[] = { path, NULL };
	char * envp[] = { "TERM=vt100", "PATH=/bin:/sbin:/usr/bin:/usr/sbin", NULL };

	kprintf(KERN_DEBUG"init: starting %s\n", path);
	sys_execve(path, argv, envp);
}

void init(void)
{
	char * initpath;
	/* Montujemy główny system plików */
	vfs_mount_root();

	/* Otwieramy konsole inita */
	vfs_setup_console();

	/* Odpalamy aplikacje inita */
	initpath = kargv_lookup("init=");
	if (initpath)
	{
		initpath += strlen("init=");
		init_exec(initpath);
	}
	init_exec("/sbin/init");
	init_exec("/bin/sh");

	panic("Unable to start init!");
}

void start_init(void)
{
	struct thread * thread;
	pid_t pid;

	/* Tworzymy nowy proces */
	__init_proc = kalloc(sizeof(struct proc));
	proc_init(__init_proc, __init_proc);

	thread = kalloc(sizeof(struct thread));
	thread_init(thread, __init_proc);
	ctx_init(&thread->ctx, &init);
	thread->prio = 5;
	__init_proc->main = thread;

	pid = proc_spawn(__init_proc);
	if (pid < 0)
	{
		panic("Unable to spawn init process");
	}

	/* Włączamy wywłaszcenie kernela */
	sched_preempt_enable();
}
