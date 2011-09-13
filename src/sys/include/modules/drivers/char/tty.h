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
#ifndef __MODULES_DRIVERS_CHAR_TTY_H
#define __MODULES_DRIVERS_CHAR_TTY_H

#include <arch/spinlock.h>
#include <kernel/types.h>
#include <kernel/wait.h>
#include <kernel/mutex.h>
#include <lib/list.h>

typedef unsigned int tcflag_t;
typedef unsigned char cc_t;
typedef unsigned char speed_t;

#define NCCS            18

/* c_cc names */
#define VEOF            0
#define VEOL            1
#define VERASE          2
#define VINTR           3
#define VKILL           4
#define VMIN            5
#define VQUIT           6
#define VSTART          7
#define VSTOP           8
#define VSUSP           9
#define VTIME           10
#define	VEOL2           11
#define VWERASE         12
#define	VREPRINT        13
#define VDSUSP          14
#define	VLNEXT          15
#define	VDISCARD        16
#define VSTATUS         17

/* c_iflag */
#define BRKINT          0x00000001
#define ICRNL           0x00000002
#define IGNBRK          0x00000004
#define IGNCR           0x00000008
#define IGNPAR          0x00000010
#define INLCR           0x00000020
#define INPCK           0x00000040
#define ISTRIP          0x00000080
#define IUCLC           0x00000100
#define IXANY           0x00000200
#define IXOFF           0x00000400
#define IXON            0x00000800
#define PARMRK          0x00001000

/* c_oflag */
#define OPOST           0x00000001
#define OLCUC           0x00000002
#define ONLCR           0x00000004
#define OCRNL           0x00000008
#define ONOCR           0x00000010
#define ONLRET          0x00000020
#define OFILL           0x00000040

/* c_cflag */
#define CSIZE           0x00000300
#define CS5             0x00000000
#define CS6             0x00000100
#define CS7             0x00000200
#define CS8             0x00000300
#define CSTOPB          0x00000400
#define CREAD           0x00000800
#define PARENB          0x00001000
#define PARODD          0x00002000
#define HUPCL           0x00004000
#define CLOCAL          0x00008000

/* c_lflag */
#define ECHO            0x00000001
#define ECHOE           0x00000002
#define ECHOK           0x00000004
#define ECHONL          0x00000008
#define ICANON          0x00000010
#define IEXTEN          0x00000020
#define ISIG            0x00000040
#define NOFLSH          0x00000080
#define TOSTOP          0x00000100
#define XCASE           0x00000200
#define ECHOKE          0x00000400
#define ECHOCTL         0x00000800

#define B0              0
#define B50             1
#define B75             2
#define B110            3
#define B134            4
#define B150            5
#define B200            6
#define B300            7
#define B600            8
#define B1200           9
#define B1800           10
#define B2400           11
#define B4800           12
#define B9600           13
#define B19200          14
#define B38400          15

#define KBD_IRQ         1

#define TTY_BUF_LEN     256

#define CTRL(c)         (c & 0xBF)

/* Polecenia dla ioctl() */
#define TCGET           1
#define TCSAFLUSH       2
#define TCSADRAIN       3
#define TCSANOW         4
#define TCGET_SID       5
#define TCSEND_BRK      6
#define TCIFLUSH        7
#define TCOFLUSH        8
#define TCIOFLUSH       9
#define TCOON           10
#define TCOOFF          11
#define TCION           12
#define TCIOFF          13
#define TCDRAIN         14
#define TIOCGWINSZ      15
#define TIOCSWINSZ      16
#define TIOCGPGRP       17
#define TIOCSPGRP       18

#define TCGET_PGRP      17
#define TCSET_PGRP      18


struct termios
{
	tcflag_t c_iflag;
	tcflag_t c_oflag;
	tcflag_t c_cflag;
	tcflag_t c_lflag;
	cc_t c_line;
	cc_t c_cc[NCCS];
	speed_t c_ispeed;
	speed_t c_ospeed;
};

struct winsize
{
        unsigned short ws_row;
        unsigned short ws_col;
        unsigned short ws_xpixel;
        unsigned short ws_ypixel;
};

struct tty_queue
{
	size_t size;
	size_t in_ptr, out_ptr;
	spinlock_t lock;
	char data[TTY_BUF_LEN];
};

struct tty
{
	list_t list;
	int id;

	struct tty_queue input_q; /* Input queue */
	struct tty_queue sec_q;  /* Secondary queue */

	struct termios termios;
	struct winsize winsize;

	void * dataptr;

	int (*write)(char c, void * ptr);
	struct mutex write_mutex;

	wait_t wait; /* Wait queue */
};

struct kbd_map
{
	char * name;
	uint8_t normal[128];
	uint8_t shift[128];
};

struct tty * tty_current(void);
void tty_putc(char c, struct tty * tty);
void tty_input(struct tty * tty);

void kbd_init(void);

#endif /* __MODULES_DRIVERS_CHAR_TTY_H */
