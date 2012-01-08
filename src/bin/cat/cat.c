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
#include <string.h>
#include <sys/fcntl.h>
#include <sys/stat.h>
#include <stdio.h>
#include <errno.h>

int do_cat(int fd)
{
	char buf[1024];
	ssize_t len;
	struct stat stat;
	fstat(fd, &stat);

	do
	{
		len = read(fd, buf, sizeof(buf));
		if (len <= 0)
			break;
		write(STDOUT_FILENO, buf, len);
	}
	while(len > 0);
	
	return len;
}

int main(int argc, char * argv[])
{
	int i, fd, err;

	if (argc < 2)
	{
		do_cat(STDIN_FILENO);
		return 0;
	}

	for(i=1;i<argc;i++)
	{
		if (!strcmp(argv[i], "-"))
			do_cat(STDIN_FILENO);
		else if ((fd = open(argv[i], O_RDONLY)) >= 0)
		{
			err = do_cat(fd);
			if (err < 0)
				fprintf(stderr, "%s: %s: %s\n", argv[0], argv[i], strerror(errno));
			close(fd);
		}
		else
			fprintf(stderr, "%s: %s: %s\n", argv[0], argv[i], strerror(errno));
	}

	return 0;
}
