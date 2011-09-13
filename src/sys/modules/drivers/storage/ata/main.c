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
#include <arch/irq.h>
#include <kernel/types.h>
#include <kernel/kprintf.h>
#include <kernel/module.h>
#include <kernel/device.h>
#include <mm/heap.h>
#include <lib/errno.h>
#include <lib/string.h>
#include <lib/printf.h>
#include <modules/drivers/storage/ata.h>
#include <modules/drivers/bus/pci.h>

static LIST_NEW(_controllers_list);
static int _controllers_count;

static struct ata_controller * ata_decode_devid(dev_t id, int * chan, int * device, int * part)
{
	struct ata_controller * ctrl;
	int tmp = DEV_MINOR(id);

	*part = tmp & 0xF;
	tmp >>= 4;
	*device = tmp % ATA_DEVICES;
	tmp /= ATA_DEVICES;
	*chan = tmp % ATA_CHANNELS;
	tmp /= ATA_CHANNELS;

	LIST_FOREACH(&_controllers_list, ctrl)
	{
		if(ctrl->id == tmp)
			return ctrl;
	}

	return NULL;
}

static void ata_irq(unsigned irq)
{
	struct ata_controller * ctrl;
	int i;

	LIST_FOREACH(&_controllers_list, ctrl)
	{
		for(i=0;i<ATA_CHANNELS;i++)
		{
			if (ctrl->channels[i].irq == irq)
			{
				//kprintf("ata%d: IRQ!\n", ctrl->channels[i].id);
				atomic_inc(&ctrl->channels[i].irq_counter);
				return;
			}
		}
	}
}


static int ata_open(dev_t devid)
{
	struct ata_controller * ctrl;
	int chan, device, part, err;
	ctrl = ata_decode_devid(devid, &chan, &device, &part);

	if ((!ctrl) || (ctrl->channels[chan].devices[device].type == ATA_TYPE_NONE))
		return -ENODEV;
	else if (ctrl->channels[chan].devices[device].type == ATA_TYPE_PATAPI)
	{
		/* Tylko przy otwieraniu urzadzen ATAPI musimy podjac dodatkowe akcje */
		mutex_lock(&ctrl->channels[chan].mutex);
		err = patapi_load(&ctrl->channels[chan], device);
		if (err != 0)
		{
			mutex_lock(&ctrl->channels[chan].mutex);
			return err;
		}

		//err = patapi_lock(&ctrl->channels[chan], device);
		mutex_unlock(&ctrl->channels[chan].mutex);
	}

	return 0;
}

static int ata_close(dev_t devid)
{
	struct ata_controller * ctrl;
	int chan, device, part, err;
	ctrl = ata_decode_devid(devid, &chan, &device, &part);

	if ((!ctrl) || (ctrl->channels[chan].devices[device].type == ATA_TYPE_NONE))
		return -ENODEV;
	else if (ctrl->channels[chan].devices[device].type == ATA_TYPE_PATAPI)
	{
		/* Tylko przy zamykaniu urzadzen ATAPI musimy podjac dodatkowe akcje */
		mutex_lock(&ctrl->channels[chan].mutex);
		mutex_unlock(&ctrl->channels[chan].mutex);
	}

	return 0;
}

static ssize_t ata_read(dev_t devid, void * buf, size_t len, loff_t off)
{
	struct ata_controller * ctrl;
	int chan, device, part;
	ctrl = ata_decode_devid(devid, &chan, &device, &part);
	ssize_t ret = -ENOTSUP;

	if ((!ctrl) || (ctrl->channels[chan].devices[device].type == ATA_TYPE_NONE))
		return -ENODEV;

	mutex_lock(&ctrl->channels[chan].mutex);
	if (ctrl->channels[chan].devices[device].type == ATA_TYPE_PATA)
	{
		if (!part)
			ret = pata_read(&ctrl->channels[chan], device, buf, len, off);
		else
			ret = part_read(&ctrl->channels[chan], device, part - 1, buf, len, off);

	}
	else if (ctrl->channels[chan].devices[device].type == ATA_TYPE_PATAPI)
	{
		ret = -ENOSYS;
		//return patapi_read(&ctrl->channels[chan], device, buf, len, off);
	}

	mutex_unlock(&ctrl->channels[chan].mutex);
	return ret;
}

static ssize_t ata_write(dev_t devid, void * buf, size_t len, loff_t off)
{
	struct ata_controller * ctrl;
	int chan, device, part;
	ctrl = ata_decode_devid(devid, &chan, &device, &part);
	ssize_t ret = -ENOTSUP;

	if ((!ctrl) || (ctrl->channels[chan].devices[device].type == ATA_TYPE_NONE))
		return -ENODEV;

	mutex_lock(&ctrl->channels[chan].mutex);
	if (ctrl->channels[chan].devices[device].type == ATA_TYPE_PATA)
	{
		if (!part)
			ret = pata_write(&ctrl->channels[chan], device, buf, len, off);
		else
			ret = part_write(&ctrl->channels[chan], device, part - 1, buf, len, off);

	}
	else if (ctrl->channels[chan].devices[device].type == ATA_TYPE_PATAPI)
	{
		ret = -ENOTSUP;
	}

	mutex_unlock(&ctrl->channels[chan].mutex);
	return ret;
}

static int ata_ioctl(dev_t devid, int cmd, void * arg)
{
	return -ENOSYS;
}

static struct bdev_ops _ata_ops =
{
	.open = &ata_open,
	.close = &ata_close,
	.read = &ata_read,
	.write = &ata_write,
	.ioctl = &ata_ioctl
};

static void controller_add(struct pci_device * pcidev)
{
	struct ata_controller * ctrl;
	uint32_t tmp;
	int i, j, k, err;
	char buf[32];

	ctrl = kalloc(sizeof(struct ata_controller));
	memset(ctrl, 0, sizeof(struct ata_controller));
	list_init(&ctrl->list);
	ctrl->id = _controllers_count++;
	dev_t devid = bdev_register(&_ata_ops);

	/* Odczytujemy wartości porów IO z PCI */
	pci_readRegister(pcidev, PCI_REG_BAR0, &tmp);
	ctrl->channels[0].base = (tmp & PCI_BAR_IO_MASK) ? tmp & PCI_BAR_IO_MASK : 0x1F0;
	pci_readRegister(pcidev, PCI_REG_BAR1, &tmp);
	ctrl->channels[0].ctrl = (tmp & PCI_BAR_IO_MASK) ? tmp & PCI_BAR_IO_MASK : 0x3F6;
	pci_readRegister(pcidev, PCI_REG_BAR2, &tmp);
	ctrl->channels[1].base = (tmp & PCI_BAR_IO_MASK) ? tmp & PCI_BAR_IO_MASK : 0x170;
	pci_readRegister(pcidev, PCI_REG_BAR3, &tmp);
	ctrl->channels[1].ctrl = (tmp & PCI_BAR_IO_MASK) ? tmp & PCI_BAR_IO_MASK : 0x376;
	pci_readRegister(pcidev, PCI_REG_BAR4, &tmp);
	ctrl->channels[0].bdma = (tmp & PCI_BAR_IO_MASK);
	ctrl->channels[1].bdma = (tmp & PCI_BAR_IO_MASK) + 8;
	pci_readRegister(pcidev, PCI_REG_INTLINE, &tmp);
	ctrl->channels[0].irq = tmp ? tmp : 14;
	ctrl->channels[1].irq = tmp ? tmp : 15;

	list_add(&_controllers_list, &ctrl->list);

	for(i=0;i<ATA_CHANNELS;i++)
	{
		ctrl->channels[i].id = i + ctrl->id * ATA_CHANNELS;
		kprintf("ata%d: base: 0x%X, control: 0x%X, bdma: 0x%X, IRQ: %d\n", ctrl->channels[i].id,
		ctrl->channels[i].base, ctrl->channels[i].ctrl, ctrl->channels[i].bdma, ctrl->channels[i].irq);

		mutex_init(&ctrl->channels[i].mutex);

		/* Resetujemy kanał */
		err = ata_channel_reset(&ctrl->channels[i]);
		if (err != 0)
			continue;

		/* Rejestrujemy IRQ */
		irq_register(ctrl->channels[i].irq, &ata_irq);

		/* Wykrywamy urządzenia */
		ata_channel_probe(&ctrl->channels[i]);

		/* Rejestrujemy urzadzenia */
		for(j=0;j<ATA_DEVICES;j++)
		{
			if (ctrl->channels[i].devices[j].type == ATA_TYPE_NONE)
				continue;
			DEV_MINOR(devid) = (ctrl->channels[i].id * ATA_DEVICES + j) << 4;
			device_register(DEV_BLOCK, sprintf(buf, "ata%d.%d", ctrl->channels[i].id, j), devid);

			if (ctrl->channels[i].devices[j].type == ATA_TYPE_PATA)
			{
				for(k=0;k<14;k++)
				{
					if (ctrl->channels[i].devices[j].parts[k].type != 0)
					{
						DEV_MINOR(devid) = ((ctrl->channels[i].id * ATA_DEVICES + j) << 4) + k + 1;
						device_register(DEV_BLOCK, sprintf(buf, "ata%d.%d.%d", ctrl->channels[i].id, j, k), devid);
					}
				}
			}
		}
	}
}

int init(int argc, char * argv[])
{
	_controllers_count = 0;
	if (!pci_findByType(PCI_CLASS_STORAGE, PCI_SUBCLASS_IDE, &controller_add))
		return -ENODEV;

	return 0;
}

int clean(void)
{
	return 0;
}

MODULE_INFO("ata", &init, &clean);

