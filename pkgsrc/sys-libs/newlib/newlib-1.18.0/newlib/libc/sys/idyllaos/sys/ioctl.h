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
#ifndef __SYS_IOCTL_H
#define __SYS_IOCTL_H

/* Urządzenie sieciowe */
#define NETDEV_IOCTL_SETIPV4      0x01
#define NETDEV_IOCTL_SETMASK      0x02
#define NETDEV_IOCTL_SETBROADCAST 0x03
#define NETDEV_IOCTL_GETIPV4      0x04
#define NETDEV_IOCTL_GETMASK      0x05
#define NETDEV_IOCTL_GETBROADCAST 0x06
#define NETDEV_IOCTL_GETFLAGS     0x07
#define NETDEV_IOCTL_SETFLAGS     0x08
#define NETDEV_IOCTL_GETHWTYPE    0x09
#define NETDEV_IOCTL_GETHWADDR    0x0A
#define NETDEV_IOCTL_GETTXBYTES   0x0B
#define NETDEV_IOCTL_GETRXBYTES   0x0C
#define NETDEV_IOCTL_GETMTU       0x0D
#define NETDEV_IOCTL_SETMTU       0x0E

int ioctl(int fd, int request, ...);

#endif /* __SYS_IOCTL_H */
