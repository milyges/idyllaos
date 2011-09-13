/*
 * Idylla Operating System
 * Copyright (C) 2009  Idylla Operating System Team
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
#include <kernel/types.h>
#include <arch/vga.h>
#include <arch/asm.h>
#include <arch/page.h>
#include <kernel/vc.h>
#include <kernel/kprintf.h>
#include <kernel/bitops.h>
#include <lib/string.h>

static uint16_t * _vga_buf = (uint16_t *)(LOWMEM_START + VGA_BUF);

static void vga_setattr(struct vc * vc, unsigned attr)
{
	static const uint16_t ansi_to_vga[] =
	{
		0, 4, 2, 6, 1, 5, 3, 7
	};

	uint16_t new_att = vc->attr;

	if(attr == 0) /* Wyczyść atrybuty */
	{
		new_att = 7;
		vc->flags = 0;
	}
	else if (attr == 1) /* Pogrubienie */
	{
		new_att |= 0x08;
	}
	else if (attr == 5) /* Miganie */
	{
		new_att |= 0x80;
	}
	else if (attr == 7)/* Odwróć */
	{
		if ((vc->flags & VC_REVERSE) != VC_REVERSE)
		{
			new_att = ((new_att & 0x07) << 4) | ((new_att >> 4) & 0x07);
			vc->flags |= VC_REVERSE;
		}
	}
	else if (attr == 27) /* Wylacz odwrocenie */
	{
		if ((vc->flags & VC_REVERSE) == VC_REVERSE)
		{
			new_att = ((new_att & 0x07) << 4) | ((new_att >> 4) & 0x07);
			vc->flags &= ~VC_REVERSE;
		}
	}
	else if ((attr >= 30) && (attr <= 37)) /* Kolory tekstu */
	{
		attr = ansi_to_vga[attr - 30];
		new_att = (new_att & ~0x07) | attr;
	}
	else if ((attr >= 40) && (attr <= 47)) /* Kolory tła */
	{
		attr = ansi_to_vga[attr - 40];
		new_att = (new_att & ~0x70) | attr << 4;
	}

	vc->attr = new_att;
}

static int vga_putc(struct vc * vc, char c)
{
	uint16_t * where = _vga_buf + vc->csr.row * vc->width + vc->csr.col;
	*where = (vc->attr << 8) | c;
	return sizeof(char);
}

static void vga_movecsr(struct vc * vc)
{
	uint16_t temp;
	temp = (vc->csr.row * vc->width + vc->csr.col);
	outportb(VGA_IO + 0, 14);
	outportb(VGA_IO + 1, temp >> 8);
	outportb(VGA_IO + 0, 15);
	outportb(VGA_IO + 1, temp);
}

static void vga_clearscreen(struct vc * vc, int mode)
{
	uint16_t * where;
	uint16_t blank = (vc->attr << 8) | ' ';
	switch(mode) /* Czysci ekran od aktualnej lini do... */
	{
		case 0: /* ...końca ekranu. */
		{
			where = _vga_buf + vc->csr.row * vc->width;
			memsetw(where, blank, (vc->height - vc->csr.row) * vc->width);
			break;
		}
		case 1: /* ...początku. */
		{
			where = _vga_buf;
			memsetw(where, blank, vc->csr.row * vc->width);
			break;
		}
		case 2: /* Cały ekran i przenosi kursor do lewego górnego rogu. */
		{
			where = _vga_buf;
			memsetw(where, blank, vc->height * vc->width);
			vc->csr.col = vc->csr.row = 0;
			break;
		}
	}
}

static void scroll_up(struct vc * vc, int n)
{
	uint16_t * where = _vga_buf;
	uint16_t blank = vc->attr << 8 | ' ';
	memcpy(where, where + n * vc->width, (vc->width - n) * vc->width * 2);
	memsetw(where + (vc->width - n) * vc->width, blank, n * vc->width);
}

static void clearline(struct vc * vc, int mode)
{
	uint16_t * where;
	uint16_t blank = vc->attr << 8 | ' ';
	switch(mode)
	{
		case 2: vc->csr.col = 0;
		case 0:
		{
			where = _vga_buf + (vc->csr.row * vc->width + vc->csr.col);
			memsetw(where, blank, vc->width - vc->csr.col);
			break;
		}
		case 1:
		{
			where = _vga_buf + (vc->csr.row * vc->width);
			memsetw(where, blank, vc->csr.col);
			break;
		}
	}
}

static struct vc_ops _vga_ops =
{
	.init = NULL,
	.clean = NULL,
	.setattr = &vga_setattr,
	.movecsr = &vga_movecsr,
	.putc = &vga_putc,
	.scrollup = &scroll_up,
	.scrolldown = NULL,
	.clearscreen = &vga_clearscreen,
	.clearline = &clearline
};

void vga_init(void)
{
	extern struct vc __vc_boot;
	extern char __release[];

	memset(&__vc_boot, 0x00, sizeof(struct vc));
	__vc_boot.ops = &_vga_ops;
	__vc_boot.width = VGA_WIDTH;
	__vc_boot.height = VGA_HEIGHT;
	__vc_boot.is_active = 1;
	__vc_boot.attr = 0x07;
	__vc_boot.flags = 0;

	kprintf("\e[0m\e[2JBooting IdyllaOS kernel %s...\n", __release);
}
