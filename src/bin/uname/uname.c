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
#include <stdio.h>
#include <sys/utsname.h>
#include <errno.h>
#include <getopt.h>

#define UNAME_SYSNAME       0x01
#define UNAME_NODENAME      0x02
#define UNAME_RELEASE       0x04
#define UNAME_VERSION       0x08
#define UNAME_MACHINE       0x10
#define UNAME_ALL           (UNAME_MACHINE | UNAME_VERSION | UNAME_RELEASE | UNAME_NODENAME | UNAME_SYSNAME)

int main(int argc, char * argv[])
{
	int flags = 0, option;
	struct utsname utsname;

	while((option = getopt(argc, argv, "asnrvmh")) != -1)
	{
		switch (option)
		{
			case 'a': flags |= UNAME_ALL; break;
			case 's': flags |= UNAME_SYSNAME; break;
			case 'n': flags |= UNAME_NODENAME; break;
			case 'r': flags |= UNAME_RELEASE; break;
			case 'v': flags |= UNAME_VERSION; break;
			case 'm': flags |= UNAME_MACHINE; break;
			default:
			{
				fprintf(stderr, "Usage: %s [-a] [-s] [-n] [-r] [-v] [-m]\n", argv[0]);
				return 1;
			}
		}
	}

	if (!flags)
		flags |= UNAME_SYSNAME;

	if (uname(&utsname) != 0)
	{
		perror(argv[0]);
		return 1;
	}

	if (flags & UNAME_SYSNAME)
		printf("%s ", utsname.sysname);
	if (flags & UNAME_NODENAME)
		printf("%s ", utsname.nodename);
	if (flags & UNAME_RELEASE)
		printf("%s ", utsname.release);
	if (flags & UNAME_VERSION)
		printf("%s ", utsname.version);
	if (flags & UNAME_MACHINE)
		printf("%s ", utsname.machine);

	putchar('\n');
	return 0;
}
