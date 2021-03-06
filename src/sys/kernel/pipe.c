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
#include <kernel/vfs.h>
#include <kernel/kprintf.h>
#include <kernel/init.h>
#include <kernel/panic.h>
#include <kernel/debug.h>
#include <kernel/device.h>
#include <mm/heap.h>
#include <lib/errno.h>
#include <lib/string.h>
#include <lib/math.h>

#if 0
/* TODO: Przepisać kod! */
struct pipe
{
	/* Kolejka danych FIFO */
	int in_ptr;
	int out_ptr;
	uint8_t buf[PIPE_SIZE];
	spinlock_t lock;
	/* Lista śpiących procesów czekających na dane */
	wait_t rd_wait;
	wait_t wr_wait;
};

static int pipe_open(struct vnode * vnode)
{
	struct pipe * pipe;
	pipe = kalloc(sizeof(struct pipe));
	memset(pipe, 0, sizeof(struct pipe));
	spinlock_init(&pipe->lock);
	wait_init(&pipe->rd_wait);
	wait_init(&pipe->wr_wait);
	vnode->data = pipe;

	//kprintf("pipe_open(%x)\n", vnode);
	//kprintf("pipe=%x\n", pipe);
	
	vnode->mode = S_IFIFO | 0666;
	vnode->nlink = 1;
	vnode->size = PIPE_SIZE;
	vnode->uid = 0;
	vnode->gid = 0;
	vnode->dev = 0xFF;
	vnode->blocks = PIPE_SIZE / 512;
	vnode->block_size = 512;
	vnode->rdev = 0;	
	vnode->data = pipe;
	return 0;
}

static int pipe_close(struct vnode * vnode)
{
	struct pipe * pipe = vnode->data;	
	
	//kprintf("pipe_close(%x)\n", vnode);
	kfree(pipe);
	
	return 0;
}

static ssize_t pipe_read(struct vnode * vnode, void * buf, size_t len, loff_t offset)
{
	struct pipe * pipe = vnode->data;
	int done = 0;
	uint8_t * ptr = buf;

	spinlock_lock(&pipe->lock);
	while(done < len)
	{
		/* Sprawdzamy czy rurka nie jest pusta */
		if (pipe->in_ptr == pipe->out_ptr)
		{
			/* Sprawdz czy zyje pisarz */
			if ((atomic_get(&vnode->refcount) < 2) || (done > 0))
				break;
			
			spinlock_unlock(&pipe->lock);
			//schedule();
			spinlock_lock(&pipe->lock);
			/* Budzimy pisarza */
			//kprintf("pipe_read(): sleep, done=%d\n", done);
			//wait_wakeup(&pipe->wr_wait);
			//wait_sleep(&pipe->rd_wait, &pipe->lock);
			continue;
		}
		*ptr = pipe->buf[pipe->out_ptr];
		pipe->out_ptr = (pipe->out_ptr + 1) % PIPE_SIZE;
		done++;
		ptr++;
	}
	spinlock_unlock(&pipe->lock);
	return done;
}

static ssize_t pipe_write(struct vnode * vnode, void * buf, size_t len, loff_t offset)
{
	struct pipe * pipe = vnode->data;
	int done = 0, tmp;
	uint8_t * ptr = buf;
	
	//kprintf("pipe_write()\n");
	
	spinlock_lock(&pipe->lock);
	while(done < len)
	{
		tmp = (pipe->in_ptr + 1) % PIPE_SIZE;

		/* Kolejka pełna */
		if (tmp == pipe->out_ptr)
		{
			/* Sprawdz czy istnieje jaki kolwiek czytajacy */
			if (atomic_get(&vnode->refcount) < 2)
				break;
			
			spinlock_unlock(&pipe->lock);
			//schedule();
			spinlock_lock(&pipe->lock);
			
			/* Budzimy czytacza */
			//wait_wakeup(&pipe->rd_wait);
		
			/* Usypiamy */
			//wait_sleep(&pipe->wr_wait, &pipe->lock);
			continue;
		}

		pipe->buf[pipe->in_ptr] = *ptr;
		pipe->in_ptr = tmp;
		done++;
		ptr++;
	}

	/* Budzimy czytacza */
	wait_wakeup(&pipe->rd_wait);
	spinlock_unlock(&pipe->lock);
	
	return done;
}

static struct vnode_ops _pipefs_vnode_ops =
{
	.open = &pipe_open,
	.close = &pipe_close,
	.read = &pipe_read,
	.write = &pipe_write
};

/* System plikow zawierajacy wszystkie nienazwane potoki */
static struct filesystem _pipefs =
{
	.name = "pipefs",
	.fsid = 0xFF,
	.flags = FS_NODEV,
	.fs_ops = NULL,
	.vnode_ops = &_pipefs_vnode_ops
};

struct mountpoint __pipefs_mount =
{
	.list = LIST_INIT(__pipefs_mount.list),
	.device = 0,
	.devname = "/dev/null",
	.fs = &_pipefs,
	.data = NULL,
	.root_vnode = NULL,
	.parent_vnode = NULL,
	.vnode_list = LIST_INIT(__pipefs_mount.vnode_list),
	.mutex = MUTEX_INIT(__pipefs_mount.mutex)
};

#endif

struct pipe
{
	/* Kolejka danych FIFO */
	int in_ptr;
	int out_ptr;
	uint8_t buf[PIPE_SIZE];
	spinlock_t lock;
	atomic_t refs;
	/* Lista śpiących procesów czekających na dane */
	wait_t rd_wait;
	wait_t wr_wait;	
};

int pipe_stat(struct file * file, struct stat * stat)
{
	stat->st_mode = S_IFIFO | 0600;
	stat->st_size = PIPE_SIZE;
	stat->st_nlink = 1;
	stat->st_blksize = 1;
	stat->st_blocks = PIPE_SIZE;
	return 0;
}

ssize_t pipe_read(struct file * file, void * buf, size_t bufsz)
{
	struct pipe * pipe = file->dataptr;
	int done = 0;
	uint8_t * ptr = buf;

	spinlock_lock(&pipe->lock);
	while(done < bufsz)
	{
		/* Sprawdzamy czy rurka nie jest pusta */
		if (pipe->in_ptr == pipe->out_ptr)
		{
			/* Sprawdz czy zyje pisarz */
			if ((atomic_get(&pipe->refs) < 2) || (done > 0))
				break;
			
			
			/* Budzimy pisarza */
			//kprintf("pipe_read(): sleep, done=%d\n", done);
			wait_wakeup(&pipe->wr_wait);
			wait_sleep(&pipe->rd_wait, &pipe->lock);
			continue;
		}
		*ptr = pipe->buf[pipe->out_ptr];
		pipe->out_ptr = (pipe->out_ptr + 1) % PIPE_SIZE;
		done++;
		ptr++;
	}
	spinlock_unlock(&pipe->lock);
	return done;
}

ssize_t pipe_write(struct file * file, void * buf, size_t bufsz)
{
	struct pipe * pipe = file->dataptr;
	int done = 0, tmp;
	uint8_t * ptr = buf;
	
	//kprintf("pipe_write()\n");
	
	spinlock_lock(&pipe->lock);
	while(done < bufsz)
	{
		tmp = (pipe->in_ptr + 1) % PIPE_SIZE;

		/* Kolejka pełna */
		if (tmp == pipe->out_ptr)
		{
			/* Sprawdz czy istnieje jaki kolwiek czytajacy */
			if (atomic_get(&pipe->refs) < 2)
				break;
			
			//spinlock_unlock(&pipe->lock);
			//schedule();
			//spinlock_lock(&pipe->lock);
			
			/* Budzimy czytacza */
			wait_wakeup(&pipe->rd_wait);
		
			/* Usypiamy */
			wait_sleep(&pipe->wr_wait, &pipe->lock);
			continue;
		}

		pipe->buf[pipe->in_ptr] = *ptr;
		pipe->in_ptr = tmp;
		done++;
		ptr++;
	}

	/* Budzimy czytacza */
	wait_wakeup(&pipe->rd_wait);
	spinlock_unlock(&pipe->lock);
	
	//kprintf("pipe_write(): %d\n", done);
	return done;
}

int pipe_free(struct file * file)
{
	struct pipe * pipe = file->dataptr;
	
	atomic_dec(&pipe->refs);
	
	if (!atomic_get(&pipe->refs))
	{
		
	}
	
	return 0;
}

int sys_pipe(int fds[2], struct proc * proc)
{
	struct filedes * fdw;
	struct filedes * fdr;
	struct pipe * pipe;
	int i;
	
	pipe = kalloc(sizeof(struct pipe));
	pipe->in_ptr = 0;
	pipe->out_ptr = 0;
	memset(pipe->buf, 0, PIPE_SIZE);
	spinlock_init(&pipe->lock);
	atomic_set(&pipe->refs, 2);
	wait_init(&pipe->rd_wait);
	wait_init(&pipe->wr_wait);
	
	/* Tworzymy pliki i deskryptory plikow */
	fdr = kalloc(sizeof(struct filedes));
	fdr->flags = 0;
	fdr->file = kalloc(sizeof(struct file));
	mutex_init(&fdr->file->mutex);
	atomic_set(&fdr->file->refs, 1);	
	fdr->file->vnode = NULL;
	fdr->file->dataptr = pipe;
 	fdr->file->flags = 0;
	fdr->file->type = S_IFIFO;
	fdr->file->mode = O_RDONLY;
	fdr->file->pos = 0;

	fdw = kalloc(sizeof(struct filedes));
	fdw->flags = 0;
	fdw->file = kalloc(sizeof(struct file));
	mutex_init(&fdw->file->mutex);
	atomic_set(&fdw->file->refs, 1);
	fdw->file->vnode = NULL;
	fdw->file->dataptr = pipe;
 	fdw->file->flags = 0;
	fdw->file->type = S_IFIFO;
	fdw->file->mode = O_WRONLY;
	fdw->file->pos = 0;

	/* Dodajemy do listy otwartych plikÃ³w */
	for(i = 0; i < OPEN_MAX; i++)
	{
		if (!proc->filedes[i])
		{
			proc->filedes[i] = fdr;
			fds[0] = i;
			break;
		}
	}

	if (i == OPEN_MAX)
		goto err;

	for( ; i < OPEN_MAX; i++)
	{
		if (!proc->filedes[i])
		{
			proc->filedes[i] = fdw;
			fds[1] = i;
			break;
		}
	}

	if (i < OPEN_MAX)
		return 0;

	proc->filedes[fds[0]] = NULL;

err:
	/* Zwalniamy pamiec */
	kfree(pipe);
	kfree(fdr->file);
	kfree(fdw->file);
	kfree(fdr);
	kfree(fdw);

	return -EMFILE;
}
