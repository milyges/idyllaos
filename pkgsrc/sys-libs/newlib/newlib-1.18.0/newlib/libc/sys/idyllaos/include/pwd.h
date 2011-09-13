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
#ifndef __PWD_H
#define __PWD_H
#ifdef __cplusplus
extern "C" {
#endif

#include <sys/types.h>

#define	_PATH_PASSWD		"/etc/passwd"
#define	_PASSWORD_LEN		128	/* max length, not counting NULL */

struct passwd
{
	char * pw_name;    /* user name */
	char * pw_passwd;  /* encrypted password */
	uid_t pw_uid;      /* user uid */
	gid_t pw_gid;      /* user gid */
	char * pw_comment; /* comment */
	char * pw_gecos;   /* Honeywell login info */
	char * pw_dir;     /* home directory */
	char * pw_shell;   /* default shell */
};

struct passwd * getpwuid(uid_t);
struct passwd * getpwnam(const char *);
struct passwd * getpwent(void);
void setpwent(void);
void endpwent(void);

#ifdef __cplusplus
}
#endif

#endif
