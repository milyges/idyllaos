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
#include <arch/spinlock.h>
#include <kernel/types.h>
#include <kernel/kprintf.h>
#include <kernel/module.h>
#include <kernel/device.h>
#include <kernel/vc.h>
#include <mm/heap.h>
#include <lib/list.h>
#include <lib/string.h>
#include <lib/printf.h>
#include <lib/errno.h>
#include <modules/drivers/char/tty.h>

static LIST_NEW(_tty_list);
static SPINLOCK_NEW(_tty_list_lock);
static struct tty * _tty_current;
static dev_t _tty_dev_id;

static inline struct tty * tty_get(int id)
{
	struct tty * tty;

	spinlock_lock(&_tty_list_lock);
	LIST_FOREACH(&_tty_list, tty)
	{
		if (tty->id == id)
		{
			spinlock_unlock(&_tty_list_lock);
			return tty;
		}
	}

	spinlock_unlock(&_tty_list_lock);
	return NULL;
}

static inline int tty_emptyq(struct tty_queue * q)
{
	return (q->in_ptr == q->out_ptr);
}

static inline int tty_inq(struct tty_queue * q, char data)
{
	int temp;
	spinlock_lock(&q->lock);
	temp = q->in_ptr + 1;
	if (temp >= q->size)
		temp = 0;
	/* queue is full */
	if (temp == q->out_ptr)
	{
		spinlock_unlock(&q->lock);
		return -1;
	}
	q->data[q->in_ptr] = data;
	q->in_ptr = temp;
	spinlock_unlock(&q->lock);
	return 0;
}

static inline int tty_deq(struct tty_queue * q, char * data)
{
	spinlock_lock(&q->lock);
	if (q->in_ptr == q->out_ptr)
	{
		spinlock_unlock(&q->lock);
		return -1;
	}
	*data = q->data[q->out_ptr++];
	if (q->out_ptr >= q->size)
		q->out_ptr = 0;
	spinlock_unlock(&q->lock);
	return 0;
}

/* This will remove last element pushed by inq */
static inline int tty_unq(struct tty_queue * q)
{
	spinlock_lock(&q->lock);
	/* Queue is empty */
	if (q->in_ptr == q->out_ptr)
	{
		spinlock_unlock(&q->lock);
		return -1;
	}
	q->in_ptr--;
	if (q->in_ptr < 0)
		q->in_ptr = q->size - 1;

	spinlock_unlock(&q->lock);
	return 0;
}

struct tty * tty_current(void)
{
	return _tty_current;
}

void tty_putc(char c, struct tty * tty)
{
	tty_inq(&tty->input_q, c);
}

struct tty * tty_create(void)
{
	char buf[8];
	struct tty * tty = kalloc(sizeof(struct tty));

	memset(tty, 0, sizeof(struct tty));
	list_init(&tty->list);
	tty->input_q.size = TTY_BUF_LEN;
	tty->sec_q.size = TTY_BUF_LEN;
	tty->id = -1;

	/* Initialize termios struct */
	tty->termios.c_iflag = ICRNL | OPOST | ONLCR;
	tty->termios.c_oflag = OPOST | ONLCR;
	tty->termios.c_cflag = CREAD | CS8 | HUPCL | CLOCAL;
	tty->termios.c_lflag = ICANON | IEXTEN | ISIG | ECHO | ECHOE | ECHONL | ECHOCTL;
	tty->termios.c_line = 0;
	tty->termios.c_cc[VEOF] = CTRL('D');
	tty->termios.c_cc[VEOL] = 0;
	tty->termios.c_cc[VERASE] = 0177;
	tty->termios.c_cc[VINTR] = CTRL('C');
	tty->termios.c_cc[VKILL] = 025;
	tty->termios.c_cc[VMIN] = 01;
	tty->termios.c_cc[VQUIT] = CTRL('\\');
	tty->termios.c_cc[VSTART] = CTRL('S');
	tty->termios.c_cc[VSTOP] = CTRL('Z');
	tty->termios.c_cc[VSUSP] = CTRL('Y');
	tty->termios.c_cc[VTIME] = 0;
	tty->termios.c_cc[VEOL2] = 0;
	tty->termios.c_cc[VWERASE] = CTRL('W');
	tty->termios.c_cc[VREPRINT] = CTRL('R');
	tty->termios.c_cc[VDSUSP] = 0;
	tty->termios.c_cc[VLNEXT] = CTRL('V');
	tty->termios.c_cc[VDISCARD] = CTRL('O');
	tty->termios.c_cc[VSTATUS] = CTRL('T');

	tty->termios.c_ispeed = B0;
	tty->termios.c_ospeed = B9600;

	tty->winsize.ws_row = 25;
	tty->winsize.ws_col = 80;

	wait_init(&tty->wait);
	mutex_init(&tty->write_mutex);

	spinlock_lock(&_tty_list_lock);
	list_add(&_tty_list, &tty->list);
	spinlock_unlock(&_tty_list_lock);

	tty->id = 1; /* TODO: Pierwszy wolny ID */
	device_register(DEV_CHAR, sprintf(buf, "tty%d", tty->id), MK_DEV(DEV_MAJOR(_tty_dev_id), tty->id));
	kprintf(KERN_DEBUG"tty: registering %s\n", buf);
	return tty;
}

void tty_input(struct tty * tty)
{
	char c;

	while(1)
	{
		if (tty_deq(&tty->input_q, &c) != 0)
			break;

		if (c == 13)
		{
			if (tty->termios.c_iflag & ICRNL)
				c = 10;
			else if (tty->termios.c_iflag & IGNCR)
				continue;
		}

		if (tty->termios.c_lflag & ICANON)
		{
			if (c == tty->termios.c_cc[VSTOP])
			{
				continue;
			}
			else if (c == tty->termios.c_cc[VSTART])
			{
				continue;
			}
			else if (c == tty->termios.c_cc[VERASE])
			{
				continue;
			}
			else if ((c == '\n') || (c == tty->termios.c_cc[VEOF]))
			{
				wait_wakeup(&tty->wait);
			}
		}
		else
		{
			wait_wakeup(&tty->wait);
		}

		if ((!(tty->termios.c_lflag & ISIG)) || (tty->termios.c_iflag & IGNBRK))
		{
			if (c == tty->termios.c_cc[VINTR])
			{
				continue;
			}
			else if (c == tty->termios.c_cc[VQUIT])
			{
				continue;
			}
		}

		if (tty->termios.c_lflag & ECHO)
		{
			if (c == '\n')
			{
				tty->write('\r', tty->dataptr);
				tty->write('\n', tty->dataptr);
			}
			else if ((c == '\b') && (tty->termios.c_lflag & ECHOE))
			{
				if (!tty_emptyq(&tty->sec_q))
				{
					tty->write('\b', tty->dataptr);
					tty->write(' ', tty->dataptr);
					tty->write('\b', tty->dataptr);
					tty_unq(&tty->sec_q);
				}
				continue;
			}
			else if (c < ' ')
			{
				if (tty->termios.c_lflag & ECHOCTL)
				{
					tty->write('^', tty->dataptr);
					tty->write(c + 64, tty->dataptr);
				}
			}
			else
				tty->write(c, tty->dataptr);
		}
		tty_inq(&tty->sec_q, c);
	}
}

static int tty_open(dev_t dev)
{
	struct tty * tty = tty_get(DEV_MINOR(dev));
	if (!tty)
		return -ENODEV;
	return 0;
}

static int tty_close(dev_t dev)
{
	struct tty * tty = tty_get(DEV_MINOR(dev));
	if (!tty)
		return -ENODEV;
	return 0;
}

static ssize_t tty_read(dev_t dev, void * buf, size_t len)
{
	struct tty * tty = tty_get(DEV_MINOR(dev));
	unsigned count = 0;
	char * c = buf;

	if (!tty)
		return -ENODEV;
	while(count < len)
	{
		if (tty_emptyq(&tty->sec_q))
		{
			spinlock_lock(&tty->sec_q.lock);
			wait_sleep(&tty->wait, &tty->sec_q.lock);
			spinlock_unlock(&tty->sec_q.lock);
		}

		tty_deq(&tty->sec_q, c);
		if ((tty->termios.c_lflag & ICANON) && (*c == tty->termios.c_cc[VEOF]))
			break;

		count++;
		if ((tty->termios.c_lflag & ICANON) && (*c == '\n'))
			break;
		c++;
	}

	return count;
}

static ssize_t tty_write(dev_t dev, void * buf, size_t len)
{
	struct tty * tty = tty_get(DEV_MINOR(dev));
	char * c = buf;
	size_t count = 0;
	int err;

	if (!tty)
		return -ENODEV;

	mutex_lock(&tty->write_mutex);
	while(count < len)
	{
		err = tty->write(*c, tty->dataptr);
		if (err < 0)
			break;

		count += 1;
		c++;
	}
	mutex_unlock(&tty->write_mutex);
	return count;
}

static int tty_ioctl(dev_t dev, int cmd, void * arg)
{
	struct tty * tty = tty_get(DEV_MINOR(dev));
	if (!tty)
		return -ENODEV;

	switch(cmd)
	{
		case TCGET:
		{
			memcpy(arg, &tty->termios, sizeof(struct termios));
			return 0;
		}
		case TCSANOW:
		case TCSADRAIN:
		{
			memcpy(&tty->termios, arg, sizeof(struct termios));
			return 0;
		}
		case TIOCGWINSZ:
		{
			memcpy(arg, &tty->winsize, sizeof(struct winsize));
			return 0;
		}
		case TIOCSWINSZ:
		{
			return 0;
		}
		default:
		{
			kprintf("tty_ioctl(): unknown cmd=%d\n", cmd);
		}
	}
	return -EINVAL;
}

struct cdev_ops _tty_ops =
{
	.open = &tty_open,
	.close = &tty_close,
	.read = &tty_read,
	.write = &tty_write,
	.ioctl = &tty_ioctl
};

int init(int argc, char * argv[])
{
	struct tty * tty;
	extern struct vc __vc_boot; /* From kernel/vc.c */

	_tty_dev_id = cdev_register(&_tty_ops);
	if (!_tty_dev_id)
		return -ENOMEM;

	tty = tty_create();
	if (!tty)
		return -EFAULT;

	tty->dataptr = &__vc_boot;
	tty->write = (void *)&vc_putch;
	_tty_current = tty;

	kbd_init();

	return 0;

}

int clean(void)
{

	return 0;
}

MODULE_INFO("tty", &init, &clean);

