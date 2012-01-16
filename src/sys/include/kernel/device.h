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
#ifndef __KERNEL_DEVICE_H
#define __KERNEL_DEVICE_H

#include <arch/atomic.h>
#include <kernel/types.h>
#include <lib/list.h>

#define DEV_MAJOR(x)          (((uint8_t *)&x)[1])
#define DEV_MINOR(x)          (((uint8_t *)&x)[0])
#define MK_DEV(x,y)           (((x) & 0xFF) << 8 | ((y) & 0xFF))

#define DEV_BLOCK_MAX         255
#define DEV_CHAR_MAX          255

#define DEV_BLOCK             1
#define DEV_CHAR              2

#define DEV_EVENT_REGISTER    1
#define DEV_EVENT_UNREGISTER  2

struct cdev
{
	struct cdev_ops * ops;
	atomic_t refcount;
};

struct bdev
{
	struct bdev_ops * ops;
	atomic_t refcount;	
	void * dataptr;
};

struct cdev_ops
{
	int (*open)(dev_t dev);
	int (*close)(dev_t dev);
	ssize_t (*read)(dev_t dev, void * buf, size_t len);
	ssize_t (*write)(dev_t dev, void * buf, size_t len);
	int (*ioctl)(dev_t dev, int cmd, void * arg);
};

struct bdev_ops
{
	int (*open)(struct bdev * bdev, dev_t dev);
	int (*close)(struct bdev * bdev, dev_t dev);
	ssize_t (*read)(struct bdev * bdev, dev_t dev, void * buf, size_t len, loff_t off);
	ssize_t (*write)(struct bdev * bdev, dev_t dev, void * buf, size_t len, loff_t off);
	int (*ioctl)(struct bdev * bdev, dev_t dev, int cmd, void * arg);
};

/* Tablica przechowywujaca nazwe urzadzenia (np. tty1) i jego numer */
struct devtable
{
	list_t list;
	dev_t id;
	char * name;
};

/* Zdarzenia od urządzeń */
struct dev_event
{
	list_t list;
	uint8_t event_type;
	uint8_t dev_type;
	char name[64];
	dev_t dev_id;
};

dev_t bdev_register(struct bdev_ops * ops, void * dataptr);
int bdev_unregister(dev_t dev);
dev_t cdev_register(struct cdev_ops * ops);
int cdev_unregister(dev_t dev);

int bdev_open(dev_t dev);
int bdev_close(dev_t dev);
ssize_t bdev_read(dev_t dev, void * buf, size_t len, loff_t offset);
ssize_t bdev_write(dev_t dev, void * buf, size_t len, loff_t offset);
int bdev_ioctl(dev_t dev, int cmd, void * arg);

int cdev_open(dev_t dev);
int cdev_close(dev_t dev);
ssize_t cdev_read(dev_t dev, void * buf, size_t len);
ssize_t cdev_write(dev_t dev, void * buf, size_t len);
int cdev_ioctl(dev_t dev, int cmd, void * arg);

int device_register(unsigned type, char * name, dev_t id);
int device_unregister(unsigned type, dev_t id);
dev_t device_lookup(unsigned type, char * name);

int device_get_event(uint8_t * event_type, unsigned * dev_type, char * name, dev_t * devid);

#endif /* __KERNEL_DEVICE_H */
