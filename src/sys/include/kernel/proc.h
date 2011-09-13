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
#ifndef __KERNEL_PROC_H
#define __KERNEL_PROC_H

#include <arch/atomic.h>
#include <arch/cpu.h>
#include <arch/ctx.h>
#include <kernel/types.h>
#include <kernel/panic.h>
#include <lib/list.h>

/* Stany wątków */
#define THREAD_STATE_RUNNING 0
#define THREAD_STATE_READY   1
#define THREAD_STATE_WAITING 2
#define THREAD_STATE_NEW     3
#define THREAD_STATE_BLOCKED 4
#define THREAD_STATE_ZOMBIE  5

/* Limity systemowe */
#define PROC_MAX             1024
#define OPEN_MAX             64

#define SCHED                (__scheds[CPU_CURRENT])

#define WIFEXITED(w)         (((w) & 0xff) == 0)
#define WIFSIGNALED(w)       (((w) & 0x7f) > 0 && (((w) & 0x7f) < 0x7f))
#define WIFSTOPPED(w)        (((w) & 0xff) == 0x7f)
#define WEXITSTATUS(w)       (((w) >> 8) & 0xff)
#define WTERMSIG(w)          ((w) & 0x7f)
#define WSTOPSIG             WEXITSTATUS

#define WNOHANG              0x01

struct thread
{
	list_t list; /* Lista wątków (dla schedulera) */
	list_t t_list; /* Lista wątków w danym procesie */

	tid_t tid; /* ID wątku */

	struct proc * proc; /* Proces macierzysty */

	int state; /* Stan wątku */
	int timeout; /* Czas do końca wykonania/czekania */
	int prio; /* Priorytet */

	int waitpid;
	int exit_code; /* Kod zakończenia */

	struct ctx ctx; /* Kontekst procesu */
};

struct proc
{
	list_t list;
	list_t list_siblings;

	pid_t pid; /* ID procesu */

	uid_t uid; /* Identyfikatory użytkowników i grup */
	gid_t gid;

	uid_t euid;
	gid_t egid;

	list_t thread_list; /* Lista wątków */
	atomic_t threads; /* Ilość wątków */
	struct thread * main; /* Główny wątek */

	/* Lista procesów potomnych */
	list_t childs_list;

	struct proc * parent;

	/* Dane VFS */
	struct vnode * vnode_root;
	struct vnode * vnode_current;
	struct filedes * filedes[OPEN_MAX];
	mode_t umask;

	struct vm_space * vmspace;

	spinlock_t lock;
};

struct scheduler
{
	int cpuid;
	atomic_t preempt_disabled; /* wywłaszczanie wyłączone */
	struct thread * current; /* Aktualny wątek */
	struct thread * idle; /* Wątek bezczonności */
	uint64_t ctxsw; /* Ilość przełączeń kontekstu */
};

extern struct scheduler * __scheds[CPUS_MAX];

static inline void sched_preempt_disable(void)
{
	atomic_inc(&SCHED->preempt_disabled);
}

static inline void sched_preempt_enable(void)
{
	if (!atomic_get(&SCHED->preempt_disabled))
		panic("Preemption already enabled");
	atomic_dec(&SCHED->preempt_disabled);
}

tid_t kthread_create(void * entry);
void thread_init(struct thread * thread, struct proc * parent);
tid_t thread_spawn(struct thread * thread);
void thread_exit(struct thread * thread, int code);
void thread_free(struct thread * thread);

void proc_init(struct proc * proc, struct proc * parent);
pid_t proc_spawn(struct proc * proc);

struct proc * proc_get_kernel(void);

void schedule(void);
void sched_init(int cpuid);
void sched_wakeup(struct thread * thread);

int sys_execve(char * path, char * argv[], char * envp[]);

pid_t sys_fork(void * stack);
pid_t sys_vfork(void * stack);

void sys_exit(int code, struct proc * proc);

int sys_waitpid(pid_t pid, int * status, int options);

#endif /* __KERNEL_PROC_H */
