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

/* TODO: Potoki nazwane */
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
			
			/* Budzimy pisarza */
			kprintf("pipe_read(): sleep, done=%d\n", done);
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

static ssize_t pipe_write(struct vnode * vnode, void * buf, size_t len, loff_t offset)
{
	struct pipe * pipe = vnode->data;
	int done = 0, tmp;
	uint8_t * ptr = buf;
	
	kprintf("pipe_write()\n");
	
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
