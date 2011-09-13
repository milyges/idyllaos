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

static struct cdev * _cdev_table[DEV_CHAR_MAX];
static SPINLOCK_NEW(_cdev_table_lock);

dev_t cdev_register(struct cdev_ops * ops)
{
	int i;
	spinlock_lock(&_cdev_table_lock);

	for(i=0;i<DEV_BLOCK_MAX;i++)
	{
		if (!_cdev_table[i])
		{
			_cdev_table[i] = kalloc(sizeof(struct cdev));
			_cdev_table[i]->ops = ops;
			atomic_set(&_cdev_table[i]->refcount, 0);
			spinlock_unlock(&_cdev_table_lock);
			return MK_DEV(i + 1, 0);
		}
	}

	spinlock_unlock(&_cdev_table_lock);
	return 0;
}

int cdev_unregister(dev_t dev)
{
	return -ENOSYS;
}

int cdev_open(dev_t dev)
{
	struct cdev * cdev;
	int err;

	spinlock_lock(&_cdev_table_lock);
	cdev = _cdev_table[DEV_MAJOR(dev) - 1];
	if (cdev != NULL)
		atomic_inc(&cdev->refcount);
	spinlock_unlock(&_cdev_table_lock);

	if (!cdev)
		return -ENODEV;

	err = cdev->ops->open(dev);
	if (err != 0)
		atomic_dec(&cdev->refcount);
	return err;
}

int cdev_close(dev_t dev)
{
	return -ENOSYS;
}

ssize_t cdev_read(dev_t dev, void * buf, size_t len)
{
	if (!_cdev_table[DEV_MAJOR(dev) - 1])
		return -ENODEV;

	if (!_cdev_table[DEV_MAJOR(dev) - 1]->ops->read)
		return -ENOTSUP;

	return _cdev_table[DEV_MAJOR(dev) - 1]->ops->read(dev, buf, len);
}

ssize_t cdev_write(dev_t dev, void * buf, size_t len)
{
	if (!_cdev_table[DEV_MAJOR(dev) - 1])
		return -ENODEV;
	if (!_cdev_table[DEV_MAJOR(dev) - 1]->ops->write)
		return -ENOTSUP;
	return _cdev_table[DEV_MAJOR(dev) - 1]->ops->write(dev, buf, len);
}

int cdev_ioctl(dev_t dev, int cmd, void * arg)
{
	if (!_cdev_table[DEV_MAJOR(dev) - 1])
		return -ENODEV;
	if (!_cdev_table[DEV_MAJOR(dev) - 1]->ops->ioctl)
		return -ENOTSUP;
	return _cdev_table[DEV_MAJOR(dev) - 1]->ops->ioctl(dev, cmd, arg);
}
