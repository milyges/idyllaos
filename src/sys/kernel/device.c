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
#include <mm/heap.h>
#include <lib/errno.h>
#include <lib/string.h>
#include <lib/list.h>

static LIST_NEW(_cdev_list);
static SPINLOCK_NEW(_cdev_list_lock);

static LIST_NEW(_bdev_list);
static SPINLOCK_NEW(_bdev_list_lock);

int device_register(unsigned type, char * name, dev_t id)
{
	//kprintf(KERN_DEBUG"devmgr: registering device %s -> (%d, %d)\n", name, DEV_MAJOR(id), DEV_MINOR(id));

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

