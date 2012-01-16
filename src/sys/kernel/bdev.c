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

static struct bdev * _bdev_table[DEV_BLOCK_MAX];
static SPINLOCK_NEW(_bdev_table_lock);

dev_t bdev_register(struct bdev_ops * ops, void * dataptr)
{
	int i;
	spinlock_lock(&_bdev_table_lock);

	for(i=0;i<DEV_BLOCK_MAX;i++)
	{
		if (!_bdev_table[i])
		{
			_bdev_table[i] = kalloc(sizeof(struct bdev));
			_bdev_table[i]->ops = ops;
			_bdev_table[i]->dataptr = dataptr;
			atomic_set(&_bdev_table[i]->refcount, 0);
			spinlock_unlock(&_bdev_table_lock);
			return MK_DEV(i + 1, 0);
		}
	}

	spinlock_unlock(&_bdev_table_lock);
	return 0;
}

int bdev_unregister(dev_t dev)
{
	return -ENOSYS;
}

int bdev_open(dev_t dev)
{
	struct bdev * bdev;
	int err;

	spinlock_lock(&_bdev_table_lock);
	bdev = _bdev_table[DEV_MAJOR(dev) - 1];
	if (bdev != NULL)
		atomic_inc(&bdev->refcount);
	spinlock_unlock(&_bdev_table_lock);

	if (!bdev)
		return -ENODEV;

	err = bdev->ops->open(bdev, dev);
	if (err != 0)
		atomic_dec(&bdev->refcount);
	return err;
}

int bdev_close(dev_t dev)
{
	return -ENOSYS;
}

ssize_t bdev_read(dev_t dev, void * buf, size_t len, loff_t offset)
{
	if (!_bdev_table[DEV_MAJOR(dev) - 1])
		return -ENODEV;
	if (!_bdev_table[DEV_MAJOR(dev) - 1]->ops->read)
		return -ENOTSUP;
	return _bdev_table[DEV_MAJOR(dev) - 1]->ops->read(_bdev_table[DEV_MAJOR(dev) - 1], dev, buf, len, offset);
}

ssize_t bdev_write(dev_t dev, void * buf, size_t len, loff_t offset)
{
	if (!_bdev_table[DEV_MAJOR(dev) - 1])
		return -ENODEV;
	if (!_bdev_table[DEV_MAJOR(dev) - 1]->ops->write)
		return -ENOTSUP;
	return _bdev_table[DEV_MAJOR(dev) - 1]->ops->write(_bdev_table[DEV_MAJOR(dev) - 1], dev, buf, len, offset);
}

int bdev_ioctl(dev_t dev, int cmd, void * arg)
{
	if (!_bdev_table[DEV_MAJOR(dev) - 1])
		return -ENODEV;
	if (!_bdev_table[DEV_MAJOR(dev) - 1]->ops->ioctl)
		return -ENOTSUP;
	return _bdev_table[DEV_MAJOR(dev) - 1]->ops->ioctl(_bdev_table[DEV_MAJOR(dev) - 1], dev, cmd, arg);
}
