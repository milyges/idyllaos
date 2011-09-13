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
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

int main(int argc, char * argv[])
{
	unsigned int mode;
	if (argc < 3)
	{
		fprintf(stderr, "Usage: %s mode path\n", argv[0]);
		return 1;
	}
	sscanf(argv[1], "%o", (unsigned int *)&mode);
	if (chmod(argv[2], mode) != 0)
	{
		fprintf(stderr, "%s: %s: %s\n", argv[0], argv[2], strerror(errno));
		return 1;
	}
	return 0;
}
