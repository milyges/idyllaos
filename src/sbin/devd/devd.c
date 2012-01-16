/*
 * Idylla Operating System
 * Copyright (C) 2009-2012 Idylla Operating System Team
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
#include <errno.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/mount.h>
#include <sys/kctl.h>
#include <sys/fcntl.h>

/* 
 * devd jest to daemon odpowiedzialny za tworzenie / usuwanie plików urządzeń z /dev
 * pobiera on bezpośrednio z jądra nazwy oraz numery urządzeń i na tej podstawie tworzy ich pliki.
 * Ponadto przy starcie, montuje /dev jako ramfs i przenosi poprzednie dwa pliki (/dev/root i /dev/console)
 * na nowo zamontowany system plików. Daemon ten powinien być uruchomiony jako pierwszy.
*/

#define PID_FILE    "/dev/.devd_pid"
#define CONSOLE_DEV  "/dev/console"
#define ROOT_DEV     "/dev/root"

int load_module(char * path, char * argv[])
{
	static char * nullargv[] = { NULL };
	struct kctl_kld_load_arg arg;
	
	arg.path = path;
	arg.argv = (argv != NULL) ? argv : nullargv;
	return kctl(KCTL_KLD_LOAD_MODULE, &arg);	
}

int prepare(void)
{
	struct stat statbuf;
	dev_t rootdev, consoledev;
	
	/* Zapisujemy dane o /dev/console i /dev/root */
	if (stat(CONSOLE_DEV, &statbuf) < 0)
	{
		perror("devd: unable to stat "CONSOLE_DEV);
		return 1;
	}
	
	consoledev = statbuf.st_rdev;
	
	if (stat(ROOT_DEV, &statbuf) < 0)
	{
		perror("devd: unable to stat "ROOT_DEV);
		return 1;
	}
	
	rootdev = statbuf.st_rdev;
	
	/* Próbujemy zamontować /dev */
	if (mount("none", "/dev", "ramfs", 0, NULL) < 0)
	{
		if (errno == ENOTSUP) /* Moduł ramfs nie wczytany */
		{
			if (load_module("/boot/idylla/modules/ramfs.ko", NULL))
			{
				perror("devd: unable to load ramfs module");
				return 1;
			}
			
			if (mount("none", "/dev", "ramfs", 0, NULL) < 0)
			{
				perror("devd: unable to mount /dev");
			}
		}
	}
	
	/* Odtwarzamy pliki root i console */
	if (mknod(CONSOLE_DEV, S_IFCHR | 0600, consoledev) < 0)
		perror("devd: warn: unable to create "CONSOLE_DEV);
	
	if (mknod(ROOT_DEV, S_IFBLK | 0600, consoledev) < 0)
		perror("devd: warn: unable to create "ROOT_DEV);
	
	return 0;
}

void register_event(struct kctl_kdev_get_event_arg * event)
{
	char path[255];
	mode_t mode = 0600;
	if (event->dev_type == 1)
		mode |= S_IFBLK;
	else if (event->dev_type == 2)
		mode |= S_IFCHR;
	else
		return;
	
	sprintf(path, "/dev/%s", event->name);
	mknod(path, mode, event->dev_id);
}

void daemon_main(int argc, char * argv[])
{
	struct kctl_kdev_get_event_arg event;
	
	while(1)
	{
		if (kctl(KCTL_KDEV_GET_DEVICE_EVENT, &event) < 0)
		{
			perror("devd: get event");
			break;
		}
		
		//printf("devd: event %d, name=%s, id=0x%X, type=%d\n", event.event_type, event.name, event.dev_id, event.dev_type);
		
		switch(event.event_type)
		{
			case 1: register_event(&event); break;
		}
		
	}
}

int main(int argc, char * argv[])
{
	struct stat stbuf;
	pid_t pid;
	int fd;
	char buf[16];
	
	/* Sprawdzamy czy daemon nie był już uruchomiony */
	if (stat(PID_FILE, &stbuf) == 0)
		return 0;
	
	/* Przygotowywujemy /dev */
	if (prepare() < 0)
		return 1;
	
	/* Forkujemy i tworzymy plik pid */
	pid = fork();
	if (pid < 0)
		perror("devd: fork()");
	else if (!pid)
		daemon_main(argc,argv);
	
	if ((fd = creat(PID_FILE, 0644)) < 0)
	{
		perror("devd: can not create pid file");
		return 1;
	}
	
	sprintf(buf, "%d\n", pid);
	write(fd, buf, strlen(buf));
	close(fd);
	return 0;
}
