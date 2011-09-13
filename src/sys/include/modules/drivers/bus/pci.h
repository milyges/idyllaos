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
#ifndef __MODULES_DRIVERS_BUS_PCI_H
#define __MODULES_DRIVERS_BUS_PCI_H

#include <kernel/types.h>
#include <lib/list.h>

#define PCI_CONFIG_ADDRESS       0xCF8
#define PCI_CONFIG_DATA          0xCFC

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
#define PCI_REG_SUBSYSID         0x2E
#define PCI_REG_EXPROM           0x30
#define PCI_REG_INTLINE          0x3C
#define PCI_REG_INTPIN           0x3D
#define PCI_REG_MINGRANT         0x3E
#define PCI_REG_MAXLATENCY       0x3F

/* Dla PCI-to-PCI mamy nieco inne niektóre rejestry*/
#define PCI_REG_PRIMARYBUS       0x18
#define PCI_REG_SECONDARYBUS     0x19
#define PCI_REG_SUBORDINATEBUS   0x1A
#define PCI_REG_SECLATENCYTIMER  0x1B
#define PCI_REG_IOBASE           0x1C
#define PCI_REG_IOLIMIT          0x1D
#define PCI_REG_SECONDARYSTATUS  0x1E
/* TODO: Pozostałe rejestry (jak beda potrzebne) */

#define PCI_HEADER_PCIBRIDGE     0x01
#define PCI_HEADER_CARDBUSBRIDGE 0x02
#define PCI_HEADER_MULTIFUNC     0x80

#define PCI_BAR_IS_MEMORY(x)     (((x) & 1) == 0)
#define PCI_BAR_IS_IO(x)         (((x) & 1) == 1)

#define PCI_BAR_MEMORY_MASK      0xFFFFFFF0
#define PCI_BAR_IO_MASK          0xFFFFFFFC

struct pci_device
{
	list_t list;

	/* Lokalizacja urządzenia */
	int bus;
	int device;
	int function;

	/* Informacje o typie */
	uint8_t classcode;
	uint8_t subclass;

	/* Informacje o producencie */
	uint16_t vendor_id;
	uint16_t device_id;
};

int pci_findByType(int classcode, int subclass, void (*callback)(struct pci_device * dev));
int pci_findByVendor(uint16_t vendor_id, uint16_t device_id, void (*callback)(struct pci_device * dev));
int pci_readRegister(struct pci_device * dev, int reg, uint32_t * value);
int pci_writeRegister(struct pci_device * dev, int reg, uint32_t value);

/* z direct.c */
int pci_directRead(int bus, int dev, int func, int offset, int size, uint32_t * value);
int pci_directWrite(int bus, int dev, int func, int offset, int size, uint32_t value);
int pci_directProbe(void);

#endif /* __MODULES_DRIVERS_BUS_PCI_H */
