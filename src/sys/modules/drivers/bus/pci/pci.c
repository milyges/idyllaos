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
#include <kernel/module.h>
#include <modules/drivers/bus/pci.h>
#include <mm/heap.h>
#include <lib/list.h>
#include <lib/errno.h>

static LIST_NEW(_pci_devices);
static int (*pci_read)(int bus, int dev, int func, int offset, int size, uint32_t * value);
static int (*pci_write)(int bus, int dev, int func, int offset, int size, uint32_t value);

static uint8_t pci_read8(int bus, int device, int function, int offset)
{
 uint32_t res = 0;
 pci_read(bus, device, function, offset, 1, &res);
 return (uint8_t)res;
}

static uint16_t pci_read16(int bus, int device, int function, int offset)
{
 uint32_t res = 0;
 pci_read(bus, device, function, offset, 2, &res);
 return (uint16_t)res;
}

static uint32_t pci_read32(int bus, int device, int function, int offset)
{
 uint32_t res = 0;
 pci_read(bus, device, function, offset, 4, &res);
 return res;
}

static void pci_scan(int bus)
{
	int func, dev;
	uint16_t tmp;
	uint8_t header = 0;
	struct pci_device * pcidev;

	for(dev=0;dev<32;dev++)
	{
		for(func=0;func<8;func++)
		{
			tmp = pci_read16(bus, dev, func, PCI_REG_VENDORID);
			if ((tmp == 0xFFFF) || (tmp == 0x0000))
				continue; /* Nic tutaj nie ma */

			if (!func)
				header = pci_read8(bus, dev, func, PCI_REG_HEADERTYPE);
			else if ((header & PCI_HEADER_MULTIFUNC) != PCI_HEADER_MULTIFUNC)
				continue;

			pcidev = kalloc(sizeof(struct pci_device));
			list_init(&pcidev->list);
			pcidev->bus = bus;
			pcidev->device = dev;
			pcidev->function = func;

			pcidev->vendor_id = tmp;
			pcidev->device_id = pci_read16(bus, dev, func, PCI_REG_DEVICEID);

			pcidev->classcode = pci_read8(bus, dev, func, PCI_REG_CLASSCODE);
			pcidev->subclass = pci_read8(bus, dev, func, PCI_REG_SUBCLASS);

			//kprintf("pci: %d.%d.%d type: %02x.%02x [%04x:%04x]\n", bus, dev, func, pcidev->classcode, pcidev->subclass, pcidev->vendor_id, pcidev->device_id);

			list_add(&_pci_devices, &pcidev->list);

			if ((header & PCI_HEADER_PCIBRIDGE) == PCI_HEADER_PCIBRIDGE)
			{
				//tmp = pci_read8(bus, dev, func, PCI_REG_SECONDARYBUS);
				//kprintf("pci: secondary bus=%d\n", tmp);
				//pci_scan(tmp);
			}
		}
	}

}

int pci_findByType(int classcode, int subclass, void (*callback)(struct pci_device * dev))
{
	struct pci_device * pcidev;
	int found = 0;

	LIST_FOREACH(&_pci_devices, pcidev)
	{
		if ((pcidev->classcode == classcode) && (pcidev->subclass == subclass))
		{
			found++;
			callback(pcidev);
		}
	}

	return found;
}

int pci_findByVendor(uint16_t vendor_id, uint16_t device_id, void (*callback)(struct pci_device * dev))
{
	struct pci_device * pcidev;
	int found = 0;

	LIST_FOREACH(&_pci_devices, pcidev)
	{
		if ((pcidev->vendor_id == vendor_id) && (pcidev->device_id == device_id))
		{
			found++;
			callback(pcidev);
		}
	}

	return found;
}

/*
#define PCI_REG_VENDORID         0x00
#define PCI_REG_DEVICEID         0x02
#define PCI_REG_COMMAND          0x04
#define PCI_REG_STATUS           0x06
#define PCI_REG_REVID            0x08
#define PCI_REG_PROGIF           0x09
#define PCI_REG_SUBCLASS         0x0A
#define PCI_REG_CLASSCODE        0x0B
#define PCI_REG_CACHELINESIZE    0x0C
#define PCI_REG_LATENCYTIMER     0x0D
#define PCI_REG_HEADERTYPE       0x0E
#define PCI_REG_BIST             0x0F
#define PCI_REG_BAR0             0x10
#define PCI_REG_BAR1             0x14
#define PCI_REG_BAR2             0x18
#define PCI_REG_BAR3             0x1C
#define PCI_REG_BAR4             0x20
#define PCI_REG_BAR5             0x24
#define PCI_REG_CARDBUSCISPTR    0x28
#define PCI_REG_SUBSYSVENDOR     0x2C
#define PCI_REG_SYBSYSID         0x2E
#define PCI_REG_EXPROM           0x30
#define PCI_REG_INTLINE          0x3C
#define PCI_REG_INTPIN           0x3D
#define PCI_REG_MINGRANT         0x3E
#define PCI_REG_MAXLATENCY       0x3F*/

int pci_readRegister(struct pci_device * dev, int reg, uint32_t * value)
{
	switch(reg)
	{
		case PCI_REG_EXPROM:
		case PCI_REG_CARDBUSCISPTR:
		case PCI_REG_BAR5:
		case PCI_REG_BAR4:
		case PCI_REG_BAR3:
		case PCI_REG_BAR2:
		case PCI_REG_BAR1:
		case PCI_REG_BAR0:
		{
			*value = pci_read32(dev->bus, dev->device, dev->function, reg);
			break;
		}
		case PCI_REG_DEVICEID:
		case PCI_REG_VENDORID:
		case PCI_REG_STATUS:
		case PCI_REG_COMMAND:
		case PCI_REG_SUBSYSID:
		case PCI_REG_SUBSYSVENDOR:
		{
			*value = pci_read16(dev->bus, dev->device, dev->function, reg);
			break;
		}
		case PCI_REG_CLASSCODE:
		case PCI_REG_SUBCLASS:
		case PCI_REG_PROGIF:
		case PCI_REG_REVID:
		case PCI_REG_BIST:
		case PCI_REG_HEADERTYPE:
		case PCI_REG_LATENCYTIMER:
		case PCI_REG_CACHELINESIZE:
		case PCI_REG_MAXLATENCY:
		case PCI_REG_MINGRANT:
		case PCI_REG_INTPIN:
		case PCI_REG_INTLINE:
		{
			*value = pci_read8(dev->bus, dev->device, dev->function, reg);
			break;
		}
		default: return -EFAULT;
	}

	return 0;
}

int pci_writeRegister(struct pci_device * dev, int reg, uint32_t value)
{
	return -ENOSYS;
}

MODULE_EXPORT(pci_findByType);
MODULE_EXPORT(pci_findByVendor);
MODULE_EXPORT(pci_readRegister);
MODULE_EXPORT(pci_writeRegister);

int init(int argc, char * argv[])
{
	if (!pci_directProbe())
	{
		pci_read = &pci_directRead;
		pci_write = &pci_directWrite;
	}
	else
	{
		return -ENODEV;
	}

	pci_scan(0);
	return 0;
}

int clean(void)
{
	return 0;
}

MODULE_INFO("pci", &init, &clean);
