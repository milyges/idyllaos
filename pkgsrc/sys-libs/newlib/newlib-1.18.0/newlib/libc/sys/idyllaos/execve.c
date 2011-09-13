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
#include <unistd.h>
#include <sys/syscall.h>
#include <errno.h>

int _execve(const char * filename, char * const argv[], char * const envp[])
{
	int ret;
	errno = 0;
	__SYSCALL3(SYS_EXECVE, ret, filename, argv, envp);
	__SYSCALL_EXIT(ret);
}

int execve(const char * filename, char * const argv[], char * const envp[])
{
	return _execve(filename, argv, envp);
}
