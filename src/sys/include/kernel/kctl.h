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
#ifndef __KERNEL_KCTL_H
#define __KERNEL_KCTL_H

#include <kernel/types.h>
#include <kernel/device.h>
#include <kernel/kld.h>

/* Linker jądra */
#define KCTL_KLD_LOAD_MODULE       0x10
#define KCTL_KLD_UNLOAD_MODULE     0x11
#define KCTL_KLD_STAT_MODULE       0x12
#define KCTL_KLD_NEXT_MODULE       0x13

/* Interfejs urządzeń */
#define KCTL_KDEV_STAT_DEVICE      0x20
#define KCTL_KDEV_NEXT_DEVICE      0x21
#define KCTL_KDEV_GET_DEVICE_EVENT 0x22

/* Lista procesów */
#define KCTL_PROC_STAT             0x30
#define KCTL_PROC_NEXT             0x31

/* Manager pamięci */
#define KCTL_MEM_GET_PHYS          0x40
#define KCTL_MEM_GET_USED          0x41
#define KCTL_MEM_GET_KHEAP         0x42

struct kctl_kld_load_arg
{
	char * path;
	char ** argv;
};

struct kctl_kdev_get_event_arg
{
	uint8_t event_type;
	unsigned dev_type;
	char name[64];
	dev_t dev_id;
};

int sys_kctl(int cmd, void * arg);

#endif /* __KERNEL_KLD_H */
 
