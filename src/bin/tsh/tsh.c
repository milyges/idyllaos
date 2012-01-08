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
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/wait.h>

#define MAX_CMDLINE     80
#define MAX_PARAMS      32

extern char ** environ;

void show_help(void)
{
	fprintf(stderr, "IdyllaOS Tiny SHell, version 0.1\n"
	         "Built-in commands:\n"
	         " cd [directory]     change working directory to [directory]\n"
	         " echo [message]     display message\n"
	         " help               show this message\n"
	         " exit [code]        exit [with error code]\n"
	         " exec [cmdline]     replace shell with the given [cmdline]\n"
	         " env                show show environment variables\n"
	         " set [name] [value] set environment variable to [value]\n");
}

void do_test(void)
{
	int fds[2];
	char buf[10];
	int err;
	
	err = pipe(fds);
	printf("pipe() = %d\n", err);
	
	err = write(fds[1], "TEST", 5);
	printf("write() = %d (%s)\n", err, err < 0 ? strerror(errno) : "OK");
	
	err = read(fds[0], buf, 5);
	printf("read() = %d, buf = %s\n", err, buf);
}

void exec_cmdline(char * cmd)
{
	char * argv[MAX_PARAMS];
	int argc, i, err;
	pid_t pid;

	while (*cmd == ' ') cmd++;

	argv[0] = cmd;
	argc = 1;
	while(*cmd)
	{
		if (*cmd == ' ')
		{
			*cmd = '\0';
			cmd++;
			while (*cmd == ' ') cmd++;
			if (*cmd == '\0')
				break;
			argv[argc++] = cmd;
		}
		cmd++;
	}

	argv[argc] = NULL;
	if (!strcmp(argv[0], "exit"))
	{
		if (argc > 1)
			exit(atoi(argv[1]));
		else
			exit(0);
	}
	else if (!strcmp(argv[0], "help"))
	{
		show_help();
	}
	else if (!strcmp(argv[0], "echo"))
	{
		for(i=1;i<argc;i++)
			printf("%s ", argv[i]);
		putchar('\n');
	}
	else if (!strcmp(argv[0], "cd"))
	{
		if (argc < 2)
		{
			argv[1] = getenv("HOME");
			if (!argv[1])
				argv[1] = "/";
		}
		if (chdir(argv[1]) < 0)
			fprintf(stderr, "cd: %s: %s\n", argv[1], strerror(errno));
	}
	else if (!strcmp(argv[0], "exec"))
	{
		if (argc < 2)
			fprintf(stderr, "tsh: usage exec [cmdline]\n");
		else
		{
			execvp(argv[1], &argv[1]);
			fprintf(stderr, "tsh: %s: %s\n", argv[1], strerror(errno));
		}
	}
	else if (!strcmp(argv[0], "env"))
	{
		i = 0;
		while(environ[i] != NULL)
			printf("%s\n", environ[i++]);
	}
	else if (!strcmp(argv[0], "set"))
	{
		if (argc != 3)
			fprintf(stderr, "tsh: usage: set [name] [value]\n");
		else
			setenv(argv[1], argv[2], 1);
	}
	else if (!strcmp(argv[0], "dotest"))
	{
		do_test();
	}
	else
	{
		pid = fork();
		switch(pid)
		{
			case -1:
			{
				fprintf(stderr, "tsh: fork(): %s\n", strerror(errno));
				break;
			}
			case 0:
			{
				execvp(argv[0], argv);
				fprintf(stderr, "tsh: %s: %s\n", argv[0], strerror(errno));
				_exit(errno);
				break;
			}
			default:
			{
				waitpid(pid, &err, 0);
			}
		}
	}
}

int main(int argc, char * argv[], char * envp[])
{
	char line[MAX_CMDLINE + 1];
	int len;
	FILE * infp = stdin;

	setvbuf(stdout, NULL, _IONBF, 0);

	if (argc > 1)
	{
		infp = fopen(argv[1], "r");
		if (!infp)
		{
			fprintf(stderr, "%s: %s: %s\n", argv[0], argv[1], strerror(errno));
			return 1;
		}
	}

	while(1)
	{
		memset(line, 0, sizeof(line));
		if (infp == stdin)
			printf("# ");
		fgets(line, sizeof(line) - 1, infp);
		len = strlen(line);
		if (!len)
			break;

		line[len - 1] = '\0';
		if (!line[0])
			continue;
		exec_cmdline(line);
	}

	return 0;
}
