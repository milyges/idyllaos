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
#include <kernel/types.h>
#include <lib/string.h>
#include <lib/errno.h>

char __hostname[64] = "(none)";

int sys_gethostname(char * buf, size_t bufsz)
{
	if (bufsz < strlen(__hostname))
		return -EINVAL;
	
	strcpy(buf, __hostname);
	return 0;
}

int sys_sethostname(char * buf)
{
	if (strlen(buf) >= sizeof(__hostname))
		return -EINVAL;
	
	strcpy(__hostname, buf);
	return 0;
}
