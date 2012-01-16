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

/* Sygnały */
#define SIGHUP               1 /* hangup */
#define SIGINT               2 /* interrupt */
#define SIGQUIT              3 /* quit */
#define SIGILL               4 /* illegal instruction (not reset when caught) */
#define SIGTRAP              5 /* trace trap (not reset when caught) */
#define SIGIOT               6 /* IOT instruction */
#define SIGABRT              6 /* used by abort, replace SIGIOT in the future */
#define SIGEMT               7 /* EMT instruction */
#define SIGFPE               8 /* floating point exception */
#define SIGKILL              9 /* kill (cannot be caught or ignored) */
#define SIGBUS              10 /* bus error */
#define SIGSEGV             11 /* segmentation violation */
#define SIGSYS              12 /* bad argument to system call */
#define SIGPIPE             13 /* write on a pipe with no one to read it */
#define SIGALRM             14 /* alarm clock */
#define SIGTERM             15 /* software termination signal from kill */
#define SIGURG              16 /* urgent condition on IO channel */
#define SIGSTOP             17 /* sendable stop signal not from tty */
#define SIGTSTP             18 /* stop signal from tty */
#define SIGCONT             19 /* continue a stopped process */
#define SIGCHLD             20 /* to parent on child stop or exit */
#define SIGCLD              20 /* System V name for SIGCHLD */
#define SIGTTIN             21 /* to readers pgrp upon background tty read */
#define SIGTTOU             22 /* like TTIN for output if (tp->t_local&LTOSTOP) */
#define SIGIO               23 /* input/output possible signal */
#define SIGPOLL             SIGIO /* System V name for SIGIO */
#define SIGXCPU             24 /* exceeded CPU time limit */
#define SIGXFSZ             25 /* exceeded file size limit */
#define SIGVTALRM           26 /* virtual time alarm */
#define SIGPROF             27 /* profiling time alarm */
#define SIGWINCH            28 /* window changed */
#define SIGLOST             29 /* resource lost (eg, record-lock lost) */
#define SIGUSR1             30 /* user defined signal 1 */
#define SIGUSR2             31 /* user defined signal 2 */
#define NSIG                32 /* signal 0 implied */

#define SIG_DFL             (0)   /* Default action */
#define SIG_IGN             (1)   /* Ignore action */
#define SIG_ERR             (-1)  /* Error return */

#define SIG_SETMASK         0  /* set mask with sigprocmask() */
#define SIG_BLOCK           1  /* set of signals to block */
#define SIG_UNBLOCK         2  /* set of signals to, well, unblock */

#define SCHED                (__scheds[CPU_CURRENT])

#define WIFEXITED(w)         (((w) & 0xff) == 0)
#define WIFSIGNALED(w)       (((w) & 0x7f) > 0 && (((w) & 0x7f) < 0x7f))
#define WIFSTOPPED(w)        (((w) & 0xff) == 0x7f)
#define WEXITSTATUS(w)       (((w) >> 8) & 0xff)
#define WTERMSIG(w)          ((w) & 0x7f)
#define WSTOPSIG             WEXITSTATUS

#define WNOHANG              0x01

typedef uint32_t sigset_t;

struct sigaction
{
	union 
	{
		void * sa_handler;
		void * sa_sigaction;
	};
	sigset_t sa_mask;
	int sa_flags;
};


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

	pid_t pgrp; /* Grupa procesow */
	
	list_t thread_list; /* Lista wątków */
	atomic_t threads; /* Ilość wątków */
	struct thread * main; /* Główny wątek */

	/* Sygnały */
	sigset_t sigqueue; /* Kolejka oczekujących sygnałów */
	struct sigaction sigaction[NSIG];
	sigset_t sigmask;
	
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
struct proc * get_proc(pid_t pid);
struct proc * proc_get_kernel(void);

void schedule(void);
void sched_init(int cpuid);
void sched_wakeup(struct thread * thread);

int sys_execve(char * path, char * argv[], char * envp[]);

pid_t sys_fork(void * stack);
pid_t sys_vfork(void * stack);

void sys_exit(int code, struct proc * proc);

int sys_waitpid(pid_t pid, int * status, int options);

void do_signals(struct proc * p);
int sys_kill(pid_t pid, int signo);

#endif /* __KERNEL_PROC_H */
