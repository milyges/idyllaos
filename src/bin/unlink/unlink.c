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
#include <errno.h>
#include <stdio.h>
#include <string.h>

int main(int argc, char * argv[])
{
	int i;
	if (argc < 2)
	{
		fprintf(stderr, "Usage: %s file1 file2 ...\n", argv[0]);
		return 1;
	}

	for(i = 1; i < argc; i++)
	{
		if (unlink(argv[i]) < 0)
		{
			fprintf(stderr, "%s: %s: %s\n", argv[0], argv[i], strerror(errno));
			return 2;
		}
	}
	return 0;
}
