/*
 * Idylla Operating System
 * Copyright (C) 2009-2012 Idylla Operating System Team
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
#include <arch/spinlock.h>
#include <kernel/types.h>
#include <kernel/kprintf.h>
#include <kernel/module.h>
#include <kernel/device.h>
#include <kernel/debug.h>
#include <kernel/time.h>
#include <kernel/proc.h>
#include <mm/heap.h>
#include <modules/drivers/storage/storage.h>
#include <lib/errno.h>
#include <lib/string.h>
#include <lib/printf.h> 
#include <lib/math.h> 

/*
 * TODO: 
 *  Utrzymywanie buforów na poziomie MAX 1/2 całego RAMu
 *  Wątek sprzątający bufory i zapisujący je na dysk (opóźniony zapis)
 *  Zapis buforowany
*/

static atomic_t _total_buffers = ATOMIC_INIT(0);
extern uint64_t __ticks;

/* NOTE: Przed wywołaniem tych funkcji, storage->mutex powinien być zablokowany */
static struct storage_buffer * buffer_read(struct storage * storage, loff_t start)
{
	struct storage_buffer * new_buffer;
	int err;
	
	new_buffer = kalloc(sizeof(struct storage_buffer));
	memset(new_buffer, 0, sizeof(struct storage_buffer));
	list_init(&new_buffer->list);
	new_buffer->start = start;
	new_buffer->data = kalloc(STORAGE_BUFER_SIZE);
	new_buffer->last_used = __ticks;
	new_buffer->used++;
	err = storage->ops->read(storage, new_buffer->data, STORAGE_BUFER_SIZE / 512, new_buffer->start);
	if (err < 0)
	{
		kfree(new_buffer->data);
		kfree(new_buffer);
		return NULL;
	}
	
	atomic_inc(&_total_buffers);
	
	return new_buffer;
}

static int buffer_write(struct storage * storage, struct storage_buffer * buffer)
{
	int err;
	
	err = storage->ops->write(storage, buffer->data, STORAGE_BUFER_SIZE / 512, buffer->start);
	if (err < 0)
		return err;
	
	buffer->is_dirty = 0;
	
	return 0;
}

static int buffer_free(struct storage * storage, struct storage_buffer * buffer)
{
	int err;
	
	if (buffer->is_dirty)
	{
		err = buffer_write(storage, buffer);
		if (err < 0)
			return err;
	}
	
	list_remove(&buffer->list);
	kfree(buffer->data);
	kfree(buffer);
	
	return 0;
}

void storage_buffer_sync(void)
{
	while(1)
	{
	}
}

ssize_t storage_buffer_rw(struct storage * storage, void * buf, size_t len, loff_t off, int rw)
{
	struct storage_buffer * buffer;
	struct storage_buffer * buffer2;
	size_t left = len;
	size_t sects;

	mutex_lock(&storage->mutex);	
	
	/* Sprawdzamy czy możemy coś odczytać/zapisać z buforów */
	LIST_FOREACH(&storage->buffer_list, buffer)
	{
		//kprintf("buffer %llx-%llx, off=%llx, left=%x\n", buffer->start, buffer->start + STORAGE_BUFER_SIZE / 512, off, left);
		buffer2 = (buffer->list.next == &storage->buffer_list) ? NULL : (struct storage_buffer *)buffer->list.next;
		
		if (buffer->start <= off)
		{
			if ((buffer->start + STORAGE_BUFER_SIZE / 512 < off))
			{
				if (!buffer2)
					break;
				
				if (buffer2->start <= off)
					continue;
				
				buffer2 = buffer_read(storage, ROUND_DOWN(off, STORAGE_BUFER_SIZE / 512));
				if (!buffer2)
				{
					mutex_unlock(&storage->mutex);
					return -EIO;
				}
				
				list_add(&buffer->list, &buffer2->list);
				buffer = buffer2;
			}
			
			buffer->last_used = __ticks;
			buffer->used++;
			sects = MIN(left, (STORAGE_BUFER_SIZE / 512 - (off - buffer->start)));
			if (rw == STORAGE_RW_READ)
			{
				memcpy(buf, (void *)(buffer->data + (off - buffer->start) * 512), sects * 512);
			}
			else
			{
				memcpy((void *)(buffer->data + (off - buffer->start) * 512), buf, sects * 512);
				buffer->is_dirty = 1;
			}
			left -= sects;
			buf += sects * 512;
			off += sects;
		}
	}

	while (left > 0)
	{
		buffer = buffer_read(storage, ROUND_DOWN(off, STORAGE_BUFER_SIZE / 512));
		if (!buffer)
		{
			mutex_unlock(&storage->mutex);
			return -EIO;
		}
		
		list_add(storage->buffer_list.prev, &buffer->list);
		
		sects = MIN(left, (STORAGE_BUFER_SIZE / 512 - (off - buffer->start)));
		//memcpy(buf, (void *)(buffer->data + (off - buffer->start) * 512), sects * 512);
		if (rw == STORAGE_RW_READ)
		{
			memcpy(buf, (void *)(buffer->data + (off - buffer->start) * 512), sects * 512);
		}
		else
		{
			memcpy((void *)(buffer->data + (off - buffer->start) * 512), buf, sects * 512);
			buffer->is_dirty = 1;			
		}
		left -= sects;
		buf += sects * 512;
		off += sects;
	}
	
	mutex_unlock(&storage->mutex);
	
	return len - left;
}
