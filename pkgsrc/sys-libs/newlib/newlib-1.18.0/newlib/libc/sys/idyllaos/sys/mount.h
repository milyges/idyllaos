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
#ifndef _SYS_MOUNT_H
#define _SYS_MOUNT_H

#ifdef __cplusplus
extern "C" {
#endif

/* Mount flags */
#define MS_RDONLY          0x01 /* Mount read only */
#define MS_NOATIME         0x02 /* Dont update access time */
#define MS_NODEV           0x04 /* Ignore deivce fles */
#define MS_NOEXEC          0x08 /* Do not allow programs to be executed from this file system */
#define MS_NOSUID          0x10 /* Ignore Set UID and Set GID bits */
#define MS_REMOUNT         0x20 /* Remount existing fs */

int mount(const char * src, const char * target, const char * fs, unsigned long flags, const void *data);
int umount(const char * target);
int umount2(const char * target, int flags);

#ifdef __cplusplus
};
#endif

#endif /* _SYS_UTIME_H */
