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
#ifndef __GRP_H
#define __GRP_H

#include <sys/types.h>
#define	_PATH_GROUP		"/etc/group"

struct group
{
 char * gr_name;    /* group name */
 char * gr_passwd;  /* group password */
 gid_t gr_gid;      /* group id */
 char ** gr_mem;    /* group members */
};

#ifdef __cplusplus
extern "C" {
#endif

struct group * getgrgid(gid_t);
struct group * getgrnam(const char *);
struct group * getgrent(void);
void setgrent(void);
void endgrent(void);

#ifdef __cplusplus
}
#endif

#endif
