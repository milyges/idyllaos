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
#ifndef __SYS_UTSNAME_H
#define __SYS_UTSNAME_H

#ifdef __cplusplus
extern "C" {
#endif

struct utsname
{
	char sysname[16];
	char nodename[32];
	char release[32];
	char version[64];
	char machine[16];
};

int uname(struct utsname * buf);

#ifdef __cplusplus
}
#endif

#endif /* __SYS_UTSNAME_H */
