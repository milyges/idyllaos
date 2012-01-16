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
#include <kernel/kctl.h>
#include <lib/errno.h>

int sys_kctl(int cmd, void * arg)
{
	switch(cmd)
	{
		case KCTL_KLD_LOAD_MODULE:
			return kld_load(((struct kctl_kld_load_arg *)arg)->path, ((struct kctl_kld_load_arg *)arg)->argv);
		case KCTL_KDEV_GET_DEVICE_EVENT:
			return device_get_event(&((struct kctl_kdev_get_event_arg *)arg)->event_type, &((struct kctl_kdev_get_event_arg *)arg)->dev_type, ((struct kctl_kdev_get_event_arg *)arg)->name, &((struct kctl_kdev_get_event_arg *)arg)->dev_id);
		default:
			return -ENOTSUP;
	}
}
