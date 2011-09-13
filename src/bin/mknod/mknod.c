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
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

/* TODO: Wywalic to do libc */
#define makedev(x,y)       (((x) & 0xFF) << 8 | ((y) & 0xFF))

int main(int argc, char * argv[])
{
	mode_t mode = 0777;
	dev_t dev = 0;
	int err;

	if (argc < 3)
	{
		fprintf(stderr, "Usage: %s name type [major minor]\n", argv[0]);
		return 1;
	}


	if (!strcmp(argv[2], "c"))
		mode |= S_IFCHR;
	else if (!strcmp(argv[2], "b"))
		mode |= S_IFBLK;
	else if (!strcmp(argv[2], "p"))
		mode |= S_IFIFO;

	if ((S_ISCHR(mode)) || (S_ISBLK(mode)))
	{
		if (argc < 5)
		{
			fprintf(stderr, "%s: major and minor not defined\n"
			                "Usage: %s name type [major minor]\n", argv[0], argv[0]);
			return 2;
		}
		dev = makedev(atoi(argv[3]), atoi(argv[4]));
	}

	err = mknod(argv[1], mode, dev);
	if (err != 0)
	{
		perror(argv[0]);
		return 4;
	}

	return 0;
}
