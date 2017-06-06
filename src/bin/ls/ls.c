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
#include <sys/dirent.h>
#include <sys/stat.h>
#include <sys/syslimits.h>
#include <stdlib.h>
#include <pwd.h>
#include <getopt.h>

#define LS_MODE_SINGLE      0x00
#define LS_MODE_NORMAL      0x01
#define LS_MODE_LONG        0x02

#define LS_FLAGS_ALL        0x01 /* Show dot files */
#define LS_FLAGS_NUMERIC    0x02 /* Don't resolve gid / uid to names */
#define LS_FLAGS_HUMAN      0x04 /* Human readable sizes */
#define LS_FLAGS_INODES     0x08 /* Show inode numbers */
#define LS_FLAGS_COLOR      0x10 /* Koloruj wyniki */

#define LS_COLOR_DIR         "01;34"
#define LS_COLOR_LINK        "01;36"
#define LS_COLOR_EXEC        "01;32"
#define LS_COLOR_OTHER_WRITE "30;42"
#define LS_COLOR_DEVICE      "01;33"

int flags = 0;
int mode = LS_MODE_NORMAL;

struct dircache
{
	char * name;
	struct stat stat;
};

void dircache_free(struct dircache * cache)
{
	int i = 0;
	while(cache[i].name)
	{
		free(cache[i].name);
		i++;
	}
	free(cache);
}

struct dircache * dircache_build(void)
{
	DIR * dp;
	struct dirent * ent;
	struct dircache * cache = malloc(sizeof(struct dircache));
	int i = 0;

	if ((dp = opendir(".")) == NULL)
		return NULL;

	while((ent = readdir(dp)) != NULL)
	{
		if (((flags & LS_FLAGS_ALL) != LS_FLAGS_ALL) && (ent->d_name[0] == '.'))
			continue;
		cache[i].name = strdup(ent->d_name);
		stat(cache[i].name, &cache[i].stat);
		i++;
		cache = realloc(cache, (i + 1) * sizeof(struct dircache));
	}
	closedir(dp);
	cache[i].name = NULL;

	return cache;
}


static inline void put_mode(mode_t mode)
{
	char str[11] = "----------";

	switch(mode & S_IFMT)
	{
		case S_IFSOCK: str[0] = 's'; break;
		case S_IFLNK: str[0] = 'l'; break;
		case S_IFBLK: str[0] = 'b'; break;
		case S_IFDIR: str[0] = 'd'; break;
		case S_IFCHR: str[0] = 'c'; break;
		case S_IFIFO: str[0] = 'p'; break;
	}

	if (mode & S_IRUSR) str[1] = 'r';
	if (mode & S_IWUSR) str[2] = 'w';
	if (mode & S_IXUSR) str[3] = 'x';

	if (mode & S_IRGRP) str[4] = 'r';
	if (mode & S_IWGRP) str[5] = 'w';
	if (mode & S_IXGRP) str[6] = 'x';

	if (mode & S_IROTH) str[7] = 'r';
	if (mode & S_IWOTH) str[8] = 'w';
	if (mode & S_IXOTH) str[9] = 'x';

	printf("%s ", str);
}


void put_uid(uid_t uid)
{
	struct passwd * passwd;

	if (flags & LS_FLAGS_NUMERIC)
		printf("%5d ", uid);
	else
	{
		passwd = getpwuid(uid);
		if (!passwd)
			printf("%8d ", uid);
		else
			printf("%8s ", passwd->pw_name);
	}
}

/*void put_gid(gid_t gid)
{
 printf("%-8s ", "root");
}
*/

void put_name(struct dircache * item)
{
	if (flags & LS_FLAGS_COLOR)
	{
		if (item->stat.st_mode & S_IWOTH)
			printf("\e["LS_COLOR_OTHER_WRITE"m");
		else if (S_ISDIR(item->stat.st_mode))
			printf("\e["LS_COLOR_DIR"m");
		else if ((S_ISBLK(item->stat.st_mode)) || (S_ISCHR(item->stat.st_mode)))
			printf("\e["LS_COLOR_DEVICE"m");
		else if (S_ISLNK(item->stat.st_mode))
			printf("\e["LS_COLOR_LINK"m");
		else if (item->stat.st_mode & (S_IXUSR | S_IXGRP | S_IXOTH))
			printf("\e["LS_COLOR_EXEC"m");
		
		printf("%s\e[0m", item->name);
	}
	else
	{
		printf("%s", item->name);
	}
}

void put_size(uint64_t size)
{
	static char * units[] = { "", "K", "M", "G" };
	int i = 0;

	if (flags & LS_FLAGS_HUMAN)
	{
		
		while ((size >= 1024) && (i < 4))
		{
			size /= 1024;
			i++;
		}
		
		printf("%4llu%s ", size, units[i]);
	}
	else
	{
		printf("%8llu ", size);
	}
}

void put_dir_long(struct dircache * cache)
{
	int i = 0;
	char buf[PATH_MAX];
	
	while(cache[i].name)
	{		
		if (flags & LS_FLAGS_INODES)
			printf("%6d ", cache[i].stat.st_ino);
		put_mode(cache[i].stat.st_mode);
		printf("%2d ", cache[i].stat.st_nlink);
		put_uid(cache[i].stat.st_uid);
		//put_gid(cache[i].stat.st_gid);
		if ((S_ISBLK(cache[i].stat.st_mode)) || (S_ISCHR(cache[i].stat.st_mode)))
			printf("%3d, %3d ", cache[i].stat.st_rdev >> 8, cache[i].stat.st_rdev & 0xFF);
		else
			put_size(cache[i].stat.st_size);
		
		put_name(&cache[i]);
		/*if (S_ISLNK(cache[i].stat.st_mode))
		{
			memset(buf, 0, sizeof(buf));
			readlink(cache[i].name, buf, sizeof(buf) - 1);
			printf(" -> %s", buf);
		}*/
		putchar('\n');
		i++;
	}
}

void put_dir_single(struct dircache * cache)
{
	int i = 0;
	
	while(cache[i].name)
	{
		put_name(&cache[i]);
		putchar('\n');
		i++;
	}
}

int list_directory(char * argv0, char * path)
{
	struct dircache * cache;

	if (path)
	{
		if (chdir(path) < 0)
		{
			fprintf(stderr, "%s: %s: %s\n", argv0, path, strerror(errno));
			return 1;
		}
	}
	
	cache = dircache_build();
	if (!cache)
	{
		fprintf(stderr, "%s: %s: %s\n", argv0, path, strerror(errno));
		return 1;
	}

	put_dir_long(cache);

	dircache_free(cache);
	
	return 0;
}

int main(int argc, char * argv[])
{	
	int option, i;
	
	//if (!isatty(1))
	//	mode = LS_MODE_SINGLE;
	
	while((option = getopt(argc, argv, "1lahinC")) != -1)
	{
		switch(option)
		{
			case 'a': flags |= LS_FLAGS_ALL; break;
			case 'h': flags |= LS_FLAGS_HUMAN; break;
			case 'i': flags |= LS_FLAGS_INODES; break;
			case 'n': flags |= LS_FLAGS_NUMERIC; break;
			case 'C': flags |= LS_FLAGS_COLOR; break;
			
			case 'l': mode = LS_MODE_LONG; break;
			case '1': mode = LS_MODE_SINGLE; break;
			
			default:
			{
				fprintf(stderr, "Usage: %s [-1lahinC]\n", argv[0]);
				return 1;
			}
		}
	}
	
	if (!isatty(1))
		flags &= ~LS_FLAGS_COLOR;
	
	if (optind + 1 < argc)
	{
		for(i = optind; i < argc; i++)
		{
			printf("%s:\n", argv[i]);
			list_directory(argv[0], argv[i]);			
		}
	}
	else
		list_directory(argv[0], optind < argc ? argv[optind] : NULL);
	
	return 0;
}
