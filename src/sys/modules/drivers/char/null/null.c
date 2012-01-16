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
#include <kernel/types.h>
#include <kernel/kprintf.h>
#include <kernel/module.h>
#include <kernel/device.h>
#include <lib/string.h>
#include <lib/errno.h>

static int null_open(dev_t dev)
{
	return 0;
}

static int null_close(dev_t dev)
{
	return 0;
}

static ssize_t null_read(dev_t dev, void * buf, size_t len)
{
	memset(buf, 0x00, len);
	return len;
}

static ssize_t null_write(dev_t dev, void * buf, size_t len)
{
	return len;
}

static int null_ioctl(dev_t dev, int cmd, void * arg)
{
	return 0;
}

struct cdev_ops _null_ops =
{
	.open = &null_open,
	.close = &null_close,
	.read = &null_read,
	.write = &null_write,
	.ioctl = &null_ioctl
};

int init(int argc, char * argv[])
{
	dev_t id = cdev_register(&_null_ops);
	if (!id)
		return -ENOMEM;

	device_register(DEV_CHAR, "zero", MK_DEV(DEV_MAJOR(id), 0));
	device_register(DEV_CHAR, "null", MK_DEV(DEV_MAJOR(id), 1));
	
	return 0;

}

int clean(void)
{

	return 0;
}

MODULE_INFO("null", &init, &clean);

