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
#include <arch/asm.h>
#include <arch/irq.h>
#include <kernel/types.h>
#include <kernel/kprintf.h>
#include <kernel/module.h>
#include <kernel/device.h>
#include <kernel/vc.h>
#include <mm/heap.h>
#include <lib/ctype.h>
#include <lib/list.h>
#include <lib/string.h>
#include <lib/printf.h>
#include <lib/errno.h>
#include <modules/drivers/char/tty.h>

#define KBD_HOME             71
#define KBD_UP               72
#define KBD_PGUP             73
#define KBD_LEFT             75
#define KBD_RIGHT            77
#define KBD_END              79
#define KBD_DOWN             80
#define KBD_PGDN             81
#define KBD_INS              82
#define KBD_DEL              83
#define KBD_F1               59
#define KBD_F2               60
#define KBD_F3               61
#define KBD_F4               62
#define KBD_F5               63
#define KBD_F6               64
#define KBD_F7               65
#define KBD_F8               66
#define KBD_F9               67
#define KBD_F10              68
#define KBD_F11              87
#define KBD_F12              88

#define KBD_LOCK_NUMLOCK     0x001
#define KBD_LOCK_CAPSLOCK    0x002
#define KBD_LOCK_SCROLLLOCK  0x004

#define KBD_MOD_LALT         0x008
#define KBD_MOD_RALT         0x010
#define KBD_MOD_LCTRL        0x020
#define KBD_MOD_RCTRL        0x040
#define KBD_MOD_SHIFT        0x080

#define KBD_STATE_ESCAPED    0x100

static uint16_t _state;
/* Domyślna mapa klawiatury */
static struct kbd_map _generic_map =
{
	"generic",
	{
		0, 27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
		'\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\r',
		0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', 0,
		'\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0, '*',
		0, ' ', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, '-',
		0, '5', 0, '+', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	},
	{
		0,  27, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b',
		'\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',
		0, 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '\"', '~', 0,
		'|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0, '*',
		0, ' ', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, '-',
		0, '5', 0, '+', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	}
};

static struct kbd_map * _kbdmap = &_generic_map;

static int kbd_read(void)
{
	int timeout;
	unsigned stat, data;

	for(timeout=500000L;timeout!=0;timeout--)
	{
		stat = inportb(0x64);

		if(stat & 0x01)
		{
			data = inportb(0x60);

			if((stat & 0xC0) == 0)
				return data;
		}
	}
	return -1;
}

static void kbd_write(unsigned adr, unsigned data)
{
	int timeout;
	unsigned stat;
	for(timeout=500000L;timeout!= 0;timeout--)
	{
		stat = inportb(0x64);
		if((stat & 0x02) == 0)
			break;
	}
	if(timeout == 0)
	{
		return;
	}
	outportb(adr, data);
}

static void kbd_puts(char * s)
{
	struct tty * tty = tty_current();
	while(*s)
	{
		tty_putc(*s, tty);
		s++;
	}
}

static void kbd_irq(int num)
{
	char data;
	struct tty * tty = tty_current();
	int scancode = kbd_read();

	if (scancode < 0)
		return;

	/* Check for escape scancode */
	if (scancode == 0xE0)
	{
		_state |= KBD_STATE_ESCAPED;
		return;
	}

	/* Detect scancode type (make/break) */
	if (scancode & 0x80) /* Break scancode */
	{
		scancode &= 0x7F;
		/* We will check it only for mod keys */
		if (scancode == 0x38)  /* Alt */
		{
			if (_state & KBD_STATE_ESCAPED)
				_state &= ~KBD_MOD_RALT;
			else
				_state &= ~KBD_MOD_LALT;
		}
		else if (scancode == 0x1D) /* Control */
		{
			if (_state & KBD_STATE_ESCAPED)
				_state &= ~KBD_MOD_RCTRL;
			else
				_state &= ~KBD_MOD_LCTRL;
		}
		else if ((scancode == 0x2A) || (scancode == 0x36)) /* Shift */
		{
			_state &= ~KBD_MOD_SHIFT;
		}
		goto end;
	}

	/* Check for mod keys */
	if (scancode == 0x38)  /* Alt */
	{
		if (_state & KBD_STATE_ESCAPED)
			_state |= KBD_MOD_RALT;
		else
			_state |= KBD_MOD_LALT;
		goto end;
	}
	else if (scancode == 0x1D) /* Control */
	{
		if (_state & KBD_STATE_ESCAPED)
			_state |= KBD_MOD_RCTRL;
		else
			_state |= KBD_MOD_LCTRL;
		goto end;
	}
	else if ((scancode == 0x2A) || (scancode == 0x36)) /* Shift */
	{
		_state |= KBD_MOD_SHIFT;
		goto end;
	}
	else if (scancode == 0x45) /* Numlock */
	{
		if (_state & KBD_LOCK_NUMLOCK)
			_state &= ~KBD_LOCK_NUMLOCK;
		else
			_state |= KBD_LOCK_NUMLOCK;
		goto end;
	}
	else if (scancode == 0x3A) /* Caps lock */
	{
		if (_state & KBD_LOCK_CAPSLOCK)
			_state &= ~KBD_LOCK_CAPSLOCK;
		else
			_state |= KBD_LOCK_CAPSLOCK;
		goto end;
	}
	else if (scancode == 0x46) /* Scroll lock */
	{
		if (_state & KBD_LOCK_SCROLLLOCK)
			_state &= ~KBD_LOCK_SCROLLLOCK;
		else
			_state |= KBD_LOCK_SCROLLLOCK;
		goto end;
	}

	/* Handle Shift/Capslock keys */
	if ((_state & KBD_MOD_SHIFT) || (_state & KBD_MOD_RCTRL) || (_state & KBD_MOD_LCTRL))
		data = _kbdmap->shift[scancode];
	else
		data = _kbdmap->normal[scancode];

	if (_state & KBD_LOCK_CAPSLOCK)
	{
		if (_state & KBD_MOD_SHIFT)
			data = tolower(data);
		else
			data = toupper(data);
	}

	if ((_state & KBD_MOD_RCTRL) || (_state & KBD_MOD_LCTRL))
		data = CTRL(data);

	/* If data != 0, we got normal key */
	if (data != 0)
	{
		tty_putc(data, tty);
		goto end;
	}

	/* Check for special keys */
	switch(scancode)
	{
		case KBD_HOME: /* Home or Keypad 7 */
		{
			if (((_state & KBD_STATE_ESCAPED) != KBD_STATE_ESCAPED) && (_state & KBD_LOCK_NUMLOCK))
				tty_putc('7', tty);
			else
				kbd_puts("\e[H");
			break;
		}
		case KBD_UP: /* Up or Keypad 8 */
		{
			if (((_state & KBD_STATE_ESCAPED) != KBD_STATE_ESCAPED) && (_state & KBD_LOCK_NUMLOCK))
				tty_putc('8', tty);
			else
				kbd_puts("\e[A");
			break;
		}
		case KBD_PGUP: /* Up or Keypad 9 */
		{
			if (((_state & KBD_STATE_ESCAPED) != KBD_STATE_ESCAPED) && (_state & KBD_LOCK_NUMLOCK))
				tty_putc('9', tty);
			else
				kbd_puts("\e[5~");
			break;
		}
		case KBD_LEFT: /* Left or Keypad 4 */
		{
			if (((_state & KBD_STATE_ESCAPED) != KBD_STATE_ESCAPED) && (_state & KBD_LOCK_NUMLOCK))
				tty_putc('4', tty);
			else
				kbd_puts("\e[D");
			break;
		}
		case KBD_RIGHT: /* Right or Keypad 6 */
		{
			if (((_state & KBD_STATE_ESCAPED) != KBD_STATE_ESCAPED) && (_state & KBD_LOCK_NUMLOCK))
				tty_putc('6', tty);
			else
				kbd_puts("\e[C");
			break;
		}
		case KBD_END: /* End or Keypad 1 */
		{
			if (((_state & KBD_STATE_ESCAPED) != KBD_STATE_ESCAPED) && (_state & KBD_LOCK_NUMLOCK))
				tty_putc('1', tty);
			else
				kbd_puts("\e[F");
			break;
		}
		case KBD_DOWN: /* Down or Keypad 2 */
		{
			if (((_state & KBD_STATE_ESCAPED) != KBD_STATE_ESCAPED) && (_state & KBD_LOCK_NUMLOCK))
				tty_putc('2', tty);
			else
				kbd_puts("\e[B");
			break;
		}
		case KBD_PGDN: /* Page Down or Keypad 3 */
		{
			if (((_state & KBD_STATE_ESCAPED) != KBD_STATE_ESCAPED) && (_state & KBD_LOCK_NUMLOCK))
				tty_putc('3', tty);
			else
				kbd_puts("\e[6~");
			break;
		}
		case KBD_INS: /* Insert or Keypad 0 */
		{
			if (((_state & KBD_STATE_ESCAPED) != KBD_STATE_ESCAPED) && (_state & KBD_LOCK_NUMLOCK))
				tty_putc('0', tty);
			else
				kbd_puts("\e[2~");
			break;
		}
		case KBD_DEL: /* Delete or Keypad . */
		{
			if (((_state & KBD_STATE_ESCAPED) != KBD_STATE_ESCAPED) && (_state & KBD_LOCK_NUMLOCK))
				tty_putc('.', tty);
			else
				kbd_puts("\e[3~");
			break;
		}
		/* F1 - F12 */
		case KBD_F1: kbd_puts("\eOP"); break;
		case KBD_F2: kbd_puts("\eOQ"); break;
		case KBD_F3: kbd_puts("\eOR"); break;
		case KBD_F4: kbd_puts("\eOS"); break;
		case KBD_F5: kbd_puts("\e[15~"); break;
		case KBD_F6: kbd_puts("\e[17~"); break;
		case KBD_F7: kbd_puts("\e[18~"); break;
		case KBD_F8: kbd_puts("\e[19~"); break;
		case KBD_F9: kbd_puts("\e[20~"); break;
		case KBD_F10: kbd_puts("\e[21~"); break;
		case KBD_F11: kbd_puts("\e[22~"); break;
		case KBD_F12: /*kbd_puts("\e[23~");*/proctab_dump(); break;
	}

end:
	tty_input(tty);
	_state &= ~KBD_STATE_ESCAPED;
}

void kbd_init(void)
{
	/* Register Keyboard IRQ */
	irq_register(KBD_IRQ, &kbd_irq);
}
