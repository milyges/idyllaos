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
#include <sys/types.h>
#include <sys/mount.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>
#include <string.h>
#include <errno.h>

char * mountoptions;
char * device;
char * mountpoint;
char * filesystem;
int mountflags;

void printusage(char * argv[])
{
	fprintf(stderr, "Usage: %s [-tfilesystem] [-ooption,option,...] device mountpoint\n", argv[0]);
}

void parse_mount_option(char * s)
{
	if (!strcmp(s,"ro"))
		mountflags |= MS_RDONLY;
	else if (!strcmp(s,"nodev"))
		mountflags |= MS_NODEV;
	else if (!strcmp(s,"nosuid"))
		mountflags |= MS_NOSUID;
	else if (!strcmp(s,"noexec"))
		mountflags |= MS_NOEXEC;
}

void parse_mount_options(void)
{
	char * s;
	int isfirst = 1;
	while (1)
	{
		if (isfirst == 1)
		{
			s = strtok(mountoptions, ",");
			isfirst = 0;
		}
		else
		{
			s = strtok(NULL, ",");
		}

		if (!s)
			break;

		parse_mount_option(s);
	}
}

void parse_params(int argc,char * argv[])
{
	int option;
	while ((option = getopt(argc, argv, "o:t:")) != -1)
	{
		switch(option)
		{
			case 't': filesystem = optarg; break;
			case 'o': mountoptions = strdup(optarg); parse_mount_options(); break;
			default:
			{
				printusage(argv);
				exit(1);
			}
		}
	}

	if (optind + 1 >= argc)
	{
		printusage(argv);
		exit(1);
	}

	device = argv[optind];
	mountpoint = argv[optind + 1];

	if (!filesystem)
	{
		fprintf(stderr, "mount: you must specify the filesystem type\n");
		printusage(argv);
		exit(1);
	}
}

int main(int argc, char * argv[])
{
	int err = 0;

	parse_params(argc, argv);
	err = mount(device, mountpoint, filesystem, mountflags, mountoptions);
	if (err != 0)
	{
		printf("%s: unable to mount %s: %s\n", argv[0], mountpoint, strerror(errno));
	}
	return err;
}
