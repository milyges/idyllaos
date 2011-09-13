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
#include <arch/asm.h>
#include <kernel/types.h>
#include <kernel/kprintf.h>
#include <modules/drivers/bus/pci.h>
#include <lib/errno.h>

int pci_directRead(int bus, int dev, int func, int offset, int size, uint32_t * value)
{
	outportd(PCI_CONFIG_ADDRESS,  0x80000000 | (bus << 16) | (dev << 11) | (func << 8) | (offset & ~3));

	switch (size)
	{
		case 1: *value = inportb(PCI_CONFIG_DATA + (offset & 3)); break;
		case 2: *value = inportw(PCI_CONFIG_DATA + (offset & 2)); break;
		case 4: *value = inportd(PCI_CONFIG_DATA); break;
		default: return -1;
	}
	return 0;
}

int pci_directWrite(int bus, int dev, int func, int offset, int size, uint32_t value)
{
	outportd(PCI_CONFIG_ADDRESS,  0x80000000 | (bus << 16) | (dev << 11) | (func << 8) | (offset & ~3));
	switch (size)
	{
		case 1: outportb(PCI_CONFIG_DATA + (offset & 3),  value); break;
		case 2: outportw(PCI_CONFIG_DATA + (offset & 2), value); break;
		case 4: outportd(PCI_CONFIG_DATA,  value); break;
		default: return -1;
	}
	return 0;
}

int pci_directProbe(void)
{
	uint32_t temp;
	/* Sprawdzamy czy dostęp bezpośredni działa */
	outportb(PCI_CONFIG_ADDRESS + 3, 0x01);
	temp = inportd(PCI_CONFIG_ADDRESS);
	outportd(PCI_CONFIG_ADDRESS, 0x80000000);
	if (inportd(PCI_CONFIG_ADDRESS) == 0x80000000)
	{
		outportd(PCI_CONFIG_ADDRESS, temp);
		kprintf(KERN_INFO"pci: using direct mode\n");
		return 0;
	}
	return -ENOTSUP;
}
