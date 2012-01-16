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
#include <arch/atomic.h>
#include <arch/cpu.h>
#include <arch/spinlock.h>
#include <arch/ctx.h>
#include <kernel/types.h>
#include <kernel/proc.h>
#include <kernel/kprintf.h>
#include <kernel/debug.h>
#include <mm/heap.h>
#include <lib/string.h>
#include <lib/list.h>
#include <lib/errno.h>

void do_signals(struct proc * p)
{
	int signo = 0;
	int tmp = p->sigqueue & ~p->sigmask;

	if (!tmp) 
		return;

	while(!(tmp & 1))
	{
		tmp >>= 1; 
		signo++;
	}
	p->sigqueue &= ~(1 << signo);
	
	kprintf("do_signals()\n");

	if (p->sigaction[signo].sa_handler == (void *)SIG_IGN) 
		return;
	else if (p->sigaction[signo].sa_handler == (void *)SIG_DFL)
	{
		switch(signo + 1)
		{
			case SIGHUP:
			case SIGINT:
			case SIGKILL:
			case SIGUSR1:
			case SIGUSR2:
			case SIGPIPE:
			case SIGALRM:
			case SIGTERM:
			case SIGVTALRM:
			case SIGPROF:
			case SIGPOLL:
			{
				/* TODO: Zakończ proces */
				TODO("exit process\n");
				break;
			}
			case SIGQUIT:
			case SIGILL:
			case SIGTRAP:
			case SIGABRT:
			case SIGBUS:
			case SIGFPE:
			case SIGSEGV:
			case SIGXCPU:
			case SIGXFSZ:
			{
				TODO("core dump");
    
				break;
			}
			case SIGCONT:
			{
				break;
			}
			case SIGSTOP:
			case SIGTSTP:
			case SIGTTIN:
			case SIGTTOU:
			{
				TODO("stop process");
				break;
			}
		}
		
	}
	else
	{
		TODO("custom handler");
	}
}

int sys_kill(pid_t pid, int signo)
{
	if ((signo < 1) || (signo > NSIG))
		return -EINVAL;
	
	return -ENOSYS;
}
