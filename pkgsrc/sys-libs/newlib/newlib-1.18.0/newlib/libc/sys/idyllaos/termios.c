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
#include <sys/syscall.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <stdarg.h>
#include <termios.h>

int tcgetattr(int fd, struct termios * t)
{
	return ioctl(fd, TCGET, t);
}

int tcsetattr(int fd, int optional_actions, const struct termios * t)
{
	return ioctl(fd, optional_actions, t);
}

int tcsendbreak(int fd, int duration)
{
	return ioctl(fd, TCSEND_BRK, duration);
}

int tcdrain(int fd)
{
	return ioctl(fd, TCDRAIN);
}

int tcflush(int fd, int queue_selector)
{
	return ioctl(fd, queue_selector);
}

int tcflow(int fd, int action)
{
	return ioctl(fd, action);
}

void cfmakeraw(struct termios * t)
{

}

speed_t cfgetispeed(const struct termios *t)
{
	return t->c_ispeed;
}

speed_t cfgetospeed(const struct termios *t)
{
	return t->c_ospeed;
}

int cfsetispeed(struct termios *t, speed_t speed)
{
	t->c_ispeed = speed;
	return 0;
}

int cfsetospeed(struct termios *t, speed_t speed)
{
	t->c_ospeed = speed;
	return 0;
}

int cfsetspeed(struct termios * t, speed_t speed)
{
	errno = ENOTSUP;
	return -1;
}

int tcsetpgrp(int fildes, pid_t pgid)
{
	if (!isatty(fildes))
	{
		errno = ENOTTY;
		return -1;
	}
	return ioctl(fildes, TCSET_PGRP, pgid);
}

pid_t tcgetpgrp(int fildes)
{
	if (!isatty(fildes))
	{
		errno = ENOTTY;
		return -1;
	}
	return ioctl(fildes, TCGET_PGRP);
}
