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
#include <arch/asm.h>
#include <arch/page.h>
#include <kernel/types.h>
#include <kernel/kprintf.h>
#include <kernel/module.h>
#include <kernel/vc.h>
#include <mm/kmem.h>
#include <lib/math.h>
#include <lib/errno.h>
#include <lib/string.h>
#include <modules/drivers/bus/pci.h>

#define VBE_DISPI_INDEX_ID          0x00
#define VBE_DISPI_INDEX_XRES        0x01
#define VBE_DISPI_INDEX_YRES        0x02
#define VBE_DISPI_INDEX_BPP         0x03
#define VBE_DISPI_INDEX_ENABLE      0x04
#define VBE_DISPI_INDEX_BANK        0x05
#define VBE_DISPI_INDEX_VIRT_WIDTH  0x06
#define VBE_DISPI_INDEX_VIRT_HEIGHT 0x07
#define VBE_DISPI_INDEX_X_OFFSET    0x08
#define VBE_DISPI_INDEX_Y_OFFSET    0x09

#define VBE_DISPI_IOPORT_INDEX      0x01CE
#define VBE_DISPI_IOPORT_DATA       0x01CF

#define VBE_DISPI_NOCLEARMEM        0x80
#define VBE_DISPI_LFB_ENABLED       0x40
#define VBE_DISPI_ENABLED           0x01

#define VBE_DISPI_VENDORID          0x1234
#define VBE_DISPI_DEVICEID          0x1111

static uint32_t _vbe_lfb_phys;
static void * _vbe_lfb;
static uint32_t _vbe_lfb_len;
static int _vbe_xres;
static int _vbe_yres;

static void vbe_write_reg(uint16_t idx, uint16_t val)
{
	outportw(VBE_DISPI_IOPORT_INDEX, idx);
	outportw(VBE_DISPI_IOPORT_DATA, val);
}

static uint16_t vbe_read_reg(uint16_t idx)
{
	outportw(VBE_DISPI_IOPORT_INDEX, idx);
	return inportw(VBE_DISPI_IOPORT_DATA);
}


static void vbe_enable_mode(int xres, int yres, int bpp)
{
	addr_t i;	
	_vbe_lfb_len = ROUND_UP(xres * yres * (bpp / 8), PAGE_SIZE);
	_vbe_lfb = kmem_alloc(_vbe_lfb_len);
	_vbe_xres = xres;
	_vbe_yres = yres;
	
	for(i = 0; i < _vbe_lfb_len; i += PAGE_SIZE)
		paging_map_page((addr_t)_vbe_lfb + i, _vbe_lfb_phys + i, PG_FLAG_RW | PG_FLAG_PRESENT, NULL);
	
	vbe_write_reg(VBE_DISPI_INDEX_ENABLE, 0);
	vbe_write_reg(VBE_DISPI_INDEX_XRES, xres);
	vbe_write_reg(VBE_DISPI_INDEX_YRES, yres);
	vbe_write_reg(VBE_DISPI_INDEX_BPP, bpp);
	vbe_write_reg(VBE_DISPI_INDEX_ENABLE, VBE_DISPI_ENABLED | VBE_DISPI_LFB_ENABLED);
}

static void vbe_disable(void)
{
	addr_t i;
	for(i = 0; i < _vbe_lfb_len; i += PAGE_SIZE)
		paging_unmap_page((addr_t)_vbe_lfb + i, NULL);
	kmem_free(_vbe_lfb, _vbe_lfb_len);
	vbe_write_reg(VBE_DISPI_INDEX_ENABLE, 0);
}

static void vbe_add_display(struct pci_device * pcidev)
{
	uint32_t lfb;
	pci_readRegister(pcidev, PCI_REG_BAR0, &lfb);
	lfb &= PCI_BAR_MEMORY_MASK;
	kprintf("vbe: display detected, version 0x%X, lfb at 0x%X\n", vbe_read_reg(VBE_DISPI_INDEX_ID), lfb);
	_vbe_lfb_phys = lfb;
	
	
}

/* TEN KOD POWINIEN BYĆ PRZENIESIONY DO FRAME BUFFERA */
static inline void putpixel24(int x, int y, unsigned color)
{
	uint8_t * screen = _vbe_lfb + x * 3 + y * 3 * _vbe_xres;
	screen[2] = color & 0xFF;
	screen[1] = (color >> 8) & 0xFF;
	screen[0] = (color >> 16) & 0xFF;
}

static inline void put_char(int x, int y, uint8_t * font, char c, int fgcolor, int bgcolor)
{
	/* Czcionka 8x16 */
	int i, j;
	if (c < 32)
		return;
	c -= 31;
	for(i = 0; i < 16; i++)
	{
		for(j = 8; j > 0; j--)
		{
			putpixel24(x + 8 - j, y + i, (font[c * 16 + i] & (1 << j)) ? fgcolor : bgcolor);
		}
	}	
}

static inline void fill_rect24(int x, int y, int width, int height, int color)
{
	uint8_t * where = _vbe_lfb + y * 3 * _vbe_xres + x * 3;
	int i, j;
	for(i = 0; i < width; i++)
	{
		for(j = 0; j < height; j++)
		{
			where[j * 3 + 2] = color & 0xFF;
			where[j * 3 + 1] = (color >> 8) & 0xFF;
			where[j * 3] = (color >> 16) & 0xFF;
		}
		where += _vbe_xres * 3;
	}
}

uint32_t attr2color[] = 
{
	0x000000, // 0
	0x000090, // 1
	0x009000, // 2
	0x009090, // 3
	0x900000, // 4
	0x900090, // 5
	0x909000, // 6
	0x909090, // 7
	
	0x000000, // 8
	0x0000FF, // 9
	0x00FF00, // 10
	0x00FFFF, // 11
	0xFF0000, // 12
	0xFF00FF, // 13
	0xFFFF00, // 14
	0xFFFFFF  // 15
};

static void fb_setattr(struct vc * vc, unsigned attr)
{
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
		new_att = (new_att & ~0x07) | (attr - 30);
	}
	else if ((attr >= 40) && (attr <= 47)) /* Kolory tła */
	{
		new_att = (new_att & ~0x70) | (attr - 40) << 4;
	}

	vc->attr = new_att;
}

static int fb_putc(struct vc * vc, char c)
{
	extern uint8_t __font_bitmap__[];
	put_char(vc->csr.col * 8, vc->csr.row * 16, __font_bitmap__, c, attr2color[vc->attr & 0x0F], attr2color[(vc->attr >> 4) & 7]);
	return sizeof(char);
}

static void fb_movecsr(struct vc * vc)
{
}

static void fb_clearscreen(struct vc * vc, int mode)
{
	switch(mode) /* Czysci ekran od aktualnej lini do... */
	{
		case 0: /* ...końca ekranu. */
		{
			
			break;
		}
		case 1: /* ...początku. */
		{
			
			break;
		}
		case 2: /* Cały ekran i przenosi kursor do lewego górnego rogu. */
		{
			memset(_vbe_lfb, 0, _vbe_lfb_len);
			vc->csr.col = vc->csr.row = 0;
			break;
		}
	}
}

static void scroll_up(struct vc * vc, int n)
{
	memcpy(_vbe_lfb, _vbe_lfb + _vbe_xres * 3 * 16 * n, _vbe_lfb_len - _vbe_xres * 3 * 16 * n);
	memset(_vbe_lfb + _vbe_lfb_len - _vbe_xres * 3 * 16 * n, 0x00, _vbe_xres * 3 * 16 * n);
	//fill_rect24(0, _vbe_yres - _vbe_xres * 16 * n, _vbe_xres, 1, 0x000000);
}

static void clearline(struct vc * vc, int mode)
{
	
}

static struct vc_ops fb_vc_ops = 
{
	.init = NULL,
	.clean = NULL,
	.setattr = &fb_setattr,
	.movecsr = &fb_movecsr,
	.putc = &fb_putc,
	.scrollup = &scroll_up,
	.scrolldown = NULL,
	.clearscreen = &fb_clearscreen,
	.clearline = &clearline
};

int init(int argc, char * argv[])
{
	if (!pci_findByVendor(VBE_DISPI_VENDORID, VBE_DISPI_DEVICEID, &vbe_add_display))
	{
		kprintf("vbe: display not detected\n");
		return -ENODEV;
	}

	vbe_enable_mode(800, 600, 24);
	
	extern struct vc __vc_boot;
	__vc_boot.ops = &fb_vc_ops;
	__vc_boot.width = _vbe_xres / 8;
	__vc_boot.height = _vbe_yres / 16;
	__vc_boot.csr.row = 0;
	__vc_boot.csr.col = 0;	
	return 0;
}

int clean(void)
{
	return 0;
}

MODULE_INFO("vbe", &init, &clean);
