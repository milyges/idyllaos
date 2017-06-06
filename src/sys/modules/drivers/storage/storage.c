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
#include <mm/heap.h>
#include <modules/drivers/storage/storage.h>
#include <lib/errno.h>
#include <lib/string.h>
#include <lib/printf.h>

static LIST_NEW(_storage_list);
static SPINLOCK_NEW(_storage_list_lock);
static atomic_t _next_id[4];
char * __storage_names[4] = { "unknown", "disk", "optical", "floppy" };

static int storage_open(struct bdev * bdev, dev_t dev)
{
	return 0;
}

static int storage_close(struct bdev * bdev, dev_t dev)
{
	return 0;
}

static ssize_t storage_read(struct bdev * bdev, dev_t dev, void * buf, size_t len, loff_t off)
{
	struct storage * storage = bdev->dataptr;
	int p = DEV_MINOR(dev);
	
	if ((storage->type == STORAGE_TYPE_DISK) && (p > 0))
	{
		if (!storage->parts[p - 1].type)
			return -ENODEV;
		
		if (off >= storage->parts[p - 1].size)
			return 0;
		if (off + len > storage->parts[p - 1].size)
			len = storage->parts[p - 1].size - off;
			
		off += storage->parts[p - 1].start;
	}
	
	return storage_buffer_rw(storage, buf, len, off, STORAGE_RW_READ);
	//return storage->ops->read(storage, buf, len, off);
}

static ssize_t storage_write(struct bdev * bdev, dev_t dev, void * buf, size_t len, loff_t off)
{
	struct storage * storage = bdev->dataptr;
	int p = DEV_MINOR(dev);
	
	if ((storage->type == STORAGE_TYPE_DISK) && (p > 0))
	{
		if (!storage->parts[p - 1].type)
			return -ENODEV;
		
		if (off >= storage->parts[p - 1].size)
			return 0;
		if (off + len > storage->parts[p - 1].size)
			len = storage->parts[p - 1].size - off;
			
		off += storage->parts[p - 1].start;
	}
	
	return storage_buffer_rw(storage, buf, len, off, STORAGE_RW_WRITE);
	//return storage->ops->write(storage, buf, len, off);
}

static int storage_ioctl(struct bdev * bdev, dev_t dev, int cmd, void * arg)
{
	return -ENOSYS;
}

static struct bdev_ops _storage_ops =
{
	.open = &storage_open,
	.close = &storage_close,
	.read = &storage_read,
	.write = &storage_write,
	.ioctl = &storage_ioctl
};

int storage_register(uint8_t type, struct storage_ops * ops, void * dataptr, dev_t * id)
{
	struct storage * storage;
	char buf[32];
	dev_t tmp;
	int i;
	
	storage = kalloc(sizeof(struct storage));
	memset(storage, 0x00, sizeof(struct storage));
	list_init(&storage->list);
	list_init(&storage->buffer_list);
	mutex_init(&storage->mutex);
	storage->type = type;
	storage->ops = ops;
	storage->dataptr = dataptr;
	storage->devid = bdev_register(&_storage_ops, storage);	
	*id = storage->devid;
	tmp = storage->devid;
	
	spinlock_lock(&_storage_list_lock);
	list_add(&_storage_list, &storage->list);
	storage->id = atomic_get(&_next_id[type]);
	atomic_inc(&_next_id[type]);
	spinlock_unlock(&_storage_list_lock);
	
	sprintf(buf, "%s%d", __storage_names[type], storage->id);
	kprintf("storage: new device %s\n", buf);
	device_register(DEV_BLOCK, buf, storage->devid);
		
	if (type == STORAGE_TYPE_DISK)
	{
		partirion_msdos_detect(storage);
		for(i = 0; i < STORAGE_MAX_PARTS; i++)
		{
			if (!storage->parts[i].type)
				continue;
			
			DEV_MINOR(tmp) = i + 1;
			sprintf(buf, "%s%dp%d", __storage_names[type], storage->id, i + 1);
			device_register(DEV_BLOCK, buf, tmp);
		}
	}
	return 0;
}

int storage_unregister(dev_t id)
{
	return -ENOSYS;
}

int init(int argc, char * argv[])
{
	return 0;
}

int clean(void)
{
	return 0;
}

MODULE_EXPORT(storage_register);
MODULE_EXPORT(storage_unregister);
MODULE_INFO("storage", &init, &clean);
