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
#include <stdio.h>
#include <errno.h>
#include <string.h>

int main(int argc, char * argv[])
{
	char buf[256];
	int err;
	if (argc < 2)
	{
		err = gethostname(buf, sizeof(buf));
		if (err < 0)
			perror(argv[0]);
		else
			printf("%s\n", buf);
	}
	else
	{
		err = sethostname(argv[1], strlen(argv[1]));
		if (err < 0)
			perror(argv[0]);
	}
	return err;
}
 