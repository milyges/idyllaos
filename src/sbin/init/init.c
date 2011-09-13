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
#include <stdlib.h>
#include <stdio.h>
#include <sys/wait.h>
#include <sys/mount.h>

int main(int argc, char * argv[])
{
	pid_t pid;

	if (getpid() != 1)
		return 1;

	printf("IdyllaOS INIT, version 0.2-dev\n");

	printf("INIT: Starting shell...\n");
	setenv("HOME", "/root", 1);
	chdir("/root");

	while(1)
	{
		pid = fork();
		switch(pid)
		{
			case -1: perror("init: fork()"); exit(1);
			case 0: execl("/bin/bash", "/bin/bash", "--login", NULL); perror("init: execve()"); _exit(1);
		}
		while(wait(NULL) != pid);
		printf("INIT: Restarting shell...\n");
	}
	return 0;
}
