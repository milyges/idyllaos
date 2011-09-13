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

#define LS_MODE_SINGLE      0
#define LS_MODE_NORMAL      1
#define LS_MODE_LONG        2

#define LS_FLAGS_ALL        1 /* Show dot files */
#define LS_FLAGS_NUMERIC    2 /* Don't resolve gid / uid to names */
#define LS_FLAGS_HUMAN      4 /* Human readable sizes */
#define LS_FLAGS_INODES     8 /* Show inode numbers */

int flags = 0;
int mode = LS_MODE_LONG | LS_FLAGS_ALL;

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
		/*if (((flags & LS_FLAGS_ALL) != LS_FLAGS_ALL) && (ent->d_name[0] == '.'))
			continue;*/
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
void put_dir_long(struct dircache * cache)
{
	int i = 0;
	char buf[PATH_MAX];

	while(cache[i].name)
	{
		if (flags & LS_FLAGS_INODES)
			printf("%5d ", cache[i].stat.st_ino);
		put_mode(cache[i].stat.st_mode);
		printf("%2d ", cache[i].stat.st_nlink);
		put_uid(cache[i].stat.st_uid);
		//put_gid(cache[i].stat.st_gid);
		if ((S_ISBLK(cache[i].stat.st_mode)) || (S_ISCHR(cache[i].stat.st_mode)))
			printf("%3d, %3d ", cache[i].stat.st_rdev >> 8, cache[i].stat.st_rdev & 0xFF);
		else
			printf("%8llu ", cache[i].stat.st_size);
		printf("%s", cache[i].name);
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

int main(int argc, char * argv[])
{
	struct dircache * cache;

	cache = dircache_build();
	if (!cache)
	{
		fprintf(stderr, "%s: %s\n", argv[0], strerror(errno));
		return 1;
	}

	put_dir_long(cache);

	dircache_free(cache);
	return 0;
}
