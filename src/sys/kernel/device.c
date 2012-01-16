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
#include <kernel/device.h>
#include <kernel/kprintf.h>
#include <kernel/wait.h>
#include <mm/heap.h>
#include <lib/errno.h>
#include <lib/string.h>
#include <lib/list.h>

static LIST_NEW(_cdev_list);
static SPINLOCK_NEW(_cdev_list_lock);

static LIST_NEW(_bdev_list);
static SPINLOCK_NEW(_bdev_list_lock);

static LIST_NEW(_event_list);
static SPINLOCK_NEW(_event_list_lock);
static WAIT_NEW(_event_wait);

static void add_event(uint8_t type, uint8_t dev_type, struct devtable * dev)
{
	struct dev_event * ev;
	ev = kalloc(sizeof(struct dev_event));
	list_init(&ev->list);
	ev->event_type = type;
	ev->dev_type = dev_type;
	strcpy(ev->name, dev->name);
	ev->dev_id = dev->id;
	
	spinlock_lock(&_event_list_lock);
	list_add(&_event_list, &ev->list);
	spinlock_unlock(&_event_list_lock);
	wait_wakeup(&_event_wait);
}

int device_get_event(uint8_t * event_type, unsigned * dev_type, char * name, dev_t * devid)
{
	struct dev_event * event;
	
	spinlock_lock(&_event_list_lock);
	
	while(1)
	{		
		if (!LIST_IS_EMPTY(&_event_list))
		{
			event = (struct dev_event *)_event_list.prev;
			list_remove(&event->list);
			break;
		}
		
		wait_sleep(&_event_wait, &_event_list_lock);
	}
	
	spinlock_unlock(&_event_list_lock);
	
	*event_type = event->event_type;
	*dev_type = event->dev_type;
	strcpy(name, event->name);
	*devid = event->dev_id;
	
	kfree(event);
	
	return 0;
}

int device_register(unsigned type, char * name, dev_t id)
{
	struct devtable * dev = kalloc(sizeof(struct devtable));
	list_init(&dev->list);
	dev->id = id;
	dev->name = strdup(name);

	if (type == DEV_BLOCK)
	{
		spinlock_lock(&_bdev_list_lock);
		list_add(&_bdev_list, &dev->list);
		spinlock_unlock(&_bdev_list_lock);
	}
	else if (type == DEV_CHAR)
	{
		spinlock_lock(&_cdev_list_lock);
		list_add(&_cdev_list, &dev->list);
		spinlock_unlock(&_cdev_list_lock);
	}

	add_event(DEV_EVENT_REGISTER, type, dev);
	return 0;
}

int device_unregister(unsigned type, dev_t id)
{
	return -ENOSYS;
}

dev_t device_lookup(unsigned type, char * name)
{
	list_t * list = NULL;
	spinlock_t * slock = NULL;
	struct devtable * dev;

	if (type == DEV_BLOCK)
	{
		list = &_bdev_list;
		slock = &_bdev_list_lock;
	}
	else if (type == DEV_CHAR)
	{
		list = &_cdev_list;
		slock = &_cdev_list_lock;
	}

	spinlock_lock(slock);
	LIST_FOREACH(list, dev)
	{
		if (!strcmp(name, dev->name))
		{
			spinlock_unlock(slock);
			return dev->id;
		}
	}
	spinlock_unlock(slock);
	return 0;
}

