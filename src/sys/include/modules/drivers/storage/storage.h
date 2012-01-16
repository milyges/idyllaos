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
#ifndef __MODULES_DRIVERS_STORAGE_H
#define __MODULES_DRIVERS_STORAGE_H

#include <kernel/types.h>
#include <kernel/device.h>
#include <kernel/mutex.h>
#include <kernel/rwlock.h>
#include <lib/list.h>

#define STORAGE_TYPE_UNKNOWN   0x00
#define STORAGE_TYPE_DISK      0x01
#define STORAGE_TYPE_OPTICAL   0x02
#define STORAGE_TYPE_FLOPPY    0x03

#define STORAGE_MAX_PARTS      0x10

#define STORAGE_BUFER_SIZE     0x10000  /* 64K */

#define STORAGE_RW_READ        0x00
#define STORAGE_RW_WRITE       0x01

struct storage_buffer
{
	list_t list;
	
	void * data;
	loff_t start;
	struct rwlock lock;
	
	uint64_t last_used;
	int used;
	uint8_t is_dirty;
	
	struct storage * storage;
};

struct storage_partition
{
	uint64_t start;
	uint64_t size;
	uint8_t type;
};

struct storage
{
	list_t list;
	unsigned id;
	
	uint8_t type;
	dev_t devid;
	
	list_t buffer_list;
	struct mutex mutex;
	
	struct storage_ops * ops;	
	struct storage_partition parts[STORAGE_MAX_PARTS];
	
	void * dataptr;
};

struct storage_ops
{
	int (*open)(struct storage * storage);
	int (*close)(struct storage * storage);
	ssize_t (*read)(struct storage * storage, void * buf, size_t size, loff_t off);
	ssize_t (*write)(struct storage * storage, void * buf, size_t size, loff_t off);
	int (*ioctl)(struct storage * storage, int cmd, void * arg);
};

ssize_t storage_buffer_rw(struct storage * storage, void * buf, size_t len, loff_t off, int rw);

void storage_buffer_sync(void);
	
int storage_register(uint8_t type, struct storage_ops * ops, void * dataptr, dev_t * id);
int storage_unregister(dev_t id);

void partirion_msdos_detect(struct storage * storage);

extern char * __storage_names[4];

#endif /* __MODULES_DRIVERS_STORAGE_ATA_H */
 
