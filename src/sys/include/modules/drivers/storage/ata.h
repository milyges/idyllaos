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
#ifndef __MODULES_DRIVERS_STORAGE_ATA_H
#define __MODULES_DRIVERS_STORAGE_ATA_H

#include <arch/atomic.h>
#include <kernel/types.h>
#include <kernel/mutex.h>
#include <kernel/device.h>

#define PCI_CLASS_STORAGE      0x01
#define PCI_SUBCLASS_IDE       0x01

#define ATA_CHANNELS           0x02 /* Ilość kanałów na kontroler */
#define ATA_DEVICES            0x02 /* Ilość urządzeń na kanał */

/* Typy urządzeń */
#define ATA_TYPE_NONE          0x00 /* Brak urządzenia */
#define ATA_TYPE_PATA          0x01
#define ATA_TYPE_PATAPI        0x02
#define ATA_TYPE_SATA          0x03
#define ATA_TYPE_SATAPI        0x04

/* Opóźnienie 400ns */
#define DELAY400NS()                { inportb(8); inportb(8); inportb(8); inportb(8); }

/* Rejestry */
#define ATA_REG_DATA                0x00
#define ATA_REG_ERROR               0x01
#define ATA_REG_SECCOUNT            0x02
#define ATA_REG_SEC                 0x03
#define ATA_REG_CYLLO               0x04
#define ATA_REG_CYLHI               0x05
#define ATA_REG_HEAD                0x06
#define ATA_REG_CMD                 0x07
#define ATA_REG_STAT                0x07

#define ATA_REG_CTRL                0
#define ATA_REG_ALTSTAT             0
#define ATA_REG_ADDRESS             1

#define ATA_REG_BDMA_COMMAND        0
#define ATA_REG_BDMA_STATUS         2
#define ATA_REG_BDMA_PRDT           4

/* Bity rejestru kontrolnego */
#define ATA_CTRL_HOB                0x80
#define ATA_CTRL_RESET              0x04
#define ATA_CTRL_NOIRQ              0x02

/* Bity rejestru stanu */
#define ATA_STAT_BSY                0x80
#define ATA_STAT_RDY                0x40
#define ATA_STAT_DF                 0x20
#define ATA_STAT_SRV                0x10
#define ATA_STAT_DRQ                0x08
#define ATA_STAT_ERR                0x01

/* Bity rejestru stanu dla BDMA */
#define ATA_BDMA_STAT_IRQ           0x04
#define ATA_BDMA_STAT_ERROR         0x02
#define ATA_BDMA_STAT_DMAMODE       0x01

/* Polecenia dla BDMA */
#define ATA_BDMA_CMD_START          0x01
#define ATA_BDMA_CMD_READ           0x08

/* Polecenia */
#define ATA_CMD_NOP                 0x00
#define ATA_CMD_PATAPIRESET         0x08
#define ATA_CMD_READ_PIO            0x20
#define ATA_CMD_READ_PIO_EXT        0x24
#define ATA_CMD_READ_DMA_EXT        0x25
#define ATA_CMD_WRITE_PIO           0x30
#define ATA_CMD_WRITE_PIO_EXT       0x34
#define ATA_CMD_WRITE_DMA_EXT       0x35
#define ATA_CMD_PATAPIPACKET        0xA0
#define ATA_CMD_PATAPIIDENTIFY      0xA1
#define ATA_CMD_READ_DMA            0xC8
#define ATA_CMD_WRITE_DMA           0xCA
#define ATA_CMD_IDENTIFY            0xEC
#define ATA_CMD_PATAPISETFEAT       0xEF /* Set features */

/* Polecenia PATAPI (SCSI) */
#define ATA_PATAPI_START_STOP       0x1B

/* Pakiety PATAPI */
#define ATA_PATAPI_PKT_LOAD         ((uint8_t[]){ ATA_PATAPI_START_STOP, 0, 0, 0, 3, 0, 0, 0, 0, 0, 0, 0 })
#define ATA_PATAPI_PKT_EJECT        ((uint8_t[]){ ATA_PATAPI_START_STOP, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0 })
#define ATA_PATAPI_PKT_START        ((uint8_t[]){ ATA_PATAPI_START_STOP, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0 })
#define ATA_PATAPI_PKT_STOP         ((uint8_t[]){ ATA_PATAPI_START_STOP, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 })

/* Tryby LBA */
#define ATA_NO_LBA                  0x00
#define ATA_LBA28                   0x01
#define ATA_LBA48                   0x02

/* R/W */
#define ATA_RW_READ                 0
#define ATA_RW_WRITE                1

/* Przydatne makra */
#define ATA_READ_BASE(c,r)          inportb((c)->base + (r))
#define ATA_READ_CTRL(c,r)          inportb((c)->ctrl + (r))
#define ATA_WRITE_BASE(c,r,v)       outportb((c)->base + (r), v)
#define ATA_WRITE_CTRL(c,r,v)       outportb((c)->ctrl + (r), v)
#define ATA_WRITE_BDMA(c,r,v)       outportb((c)->bdma + (r), v)
#define ATA_READ_BDMA(c,r)          inportb((c)->bdma + (r))

/* I tak nigdy nie będziemy czytać jednocześnie */
#define ATA_DMA_BUF_SIZE            0x10000

/* Dane zwracane przez polecenie IDENTIFY */
struct ata_identify_info
{
 uint16_t configuration;
 uint16_t cylinders;
 uint16_t reserved01;
 uint16_t heads;
 uint16_t track_bytes;

 uint16_t sector_bytes;
 uint16_t sectors;
 uint16_t reserved03[3];

 uint8_t serial_number[20];
 uint16_t buf_type;
 uint16_t buf_size;
 uint16_t ecc_bytes;
 uint8_t revision[8];

 uint8_t model_id[40];
 uint8_t sectors_per_rw_long;
 uint8_t reserved05;
 uint16_t pio32;

 uint16_t capabilities;

 uint16_t reserved08;
 uint8_t reserved09;
 uint8_t pio_cycle_time;
 uint8_t reserved10;

 uint8_t dma;
 uint16_t valid;
 uint16_t current_cylinders;
 uint16_t current_heads;
 uint16_t current_sectors;
 uint16_t current_capacity0;
 uint16_t current_capacity1;
 uint8_t sectors_per_rw_irq;
 uint8_t sectors_per_rw_irq_valid;
 uint32_t lba_sectors;
 uint16_t single_word_dma_info;
 uint16_t multi_word_dma_info;
 uint16_t eide_pio_modes;
 uint16_t eide_dma_min;
 uint16_t eide_dma_time;

 uint16_t eide_pio;
 uint16_t eide_pio_iordy;
 uint16_t reserved13[2];
 uint16_t reserved14[4];
 uint16_t command_queue_depth;
 uint16_t reserved15[4];
 uint16_t major;
 uint16_t minor;
 uint16_t command_set1;
 uint16_t command_set2;

 uint16_t command_set_features_extensions;
 uint16_t command_set_features_enable1;
 uint16_t command_set_features_enable2;
 uint16_t command_set_features_default;

 uint16_t ultra_dma_modes;

 uint16_t reserved16[2];
 uint16_t advanced_power_mangement;

 uint16_t reserved17;
 uint16_t hardware_config;

 uint16_t acoustic;
 uint16_t reserved25[5];

 uint32_t lba_capacity48;

 uint16_t reserved18[ 22 ];
 uint16_t last_lun;
 uint16_t reserved19;
 uint16_t device_lock_functions;
 uint16_t current_set_features_options;

 uint16_t reserved20[26];
 uint16_t reserved21;
 uint16_t reserved22[3];
 uint16_t reserved23[95];
} PACKED;

struct ata_dma_prdtable
{
	uint32_t addr;
	uint16_t size;
	uint16_t last;
} PACKED;

/* Partycja */
struct ata_partition
{
	uint64_t start;
	uint64_t size;
	uint8_t type;
};

/* Urządzenie ATA */
struct ata_device
{
	dev_t id;
	uint8_t type;

	/* Czy urządzenie wspiera DMA */
	uint8_t dma;
};

/* Kanał ATA */
struct ata_channel
{
	int id;

	/* Porty I/O */
	uint16_t base;
	uint16_t ctrl;
	uint16_t bdma;
	
	paddr_t dma_buf_phys;
	paddr_t prdtable_phys;
	void * dma_buf;
	struct ata_dma_prdtable * prd;
	uint8_t dma_xfer;
	
	uint8_t irq; /* Przypisane IRQ */
	atomic_t irq_counter; /* Licznik wywołań IRQ */

	uint8_t selected; /* Aktualnie wybrane urządzenie */
	struct ata_device devices[ATA_DEVICES]; /* Urządzenia */

	struct mutex mutex;
};

/* Kontroler ATA */
struct ata_controller
{
	list_t list; /* Lista kontrolerów */

	int id;
	struct ata_channel channels[ATA_CHANNELS];
};

static inline int ata_id2dev(struct ata_channel * chan, dev_t id)
{
	int i;
	for(i = 0; i < ATA_DEVICES; i++)
	{
		if (DEV_MAJOR(chan->devices[i].id) == DEV_MAJOR(id))
			return i;
	}
	
	return -1;
}

void ata_device_select(struct ata_channel * chan, uint8_t dev);
int ata_channel_reset(struct ata_channel * chan);
int ata_channel_wait(struct ata_channel * chan);
int ata_channel_poll(struct ata_channel * chan, unsigned status, unsigned enabled);
void ata_channel_probe(struct ata_channel * chan);

void pio_xfer_in(struct ata_channel * chan, void * buf, unsigned len);
void pio_xfer_out(struct ata_channel * chan, void * buf, unsigned len);

int dma_xfer_prepare(struct ata_channel * chan, uint8_t device, unsigned rw, uint32_t bytes);
int dma_xfer_start(struct ata_channel * chan);
int dma_xfer_finish(struct ata_channel * chan);

int patapi_reset(struct ata_channel * chan, uint8_t device);
int patapi_load(struct ata_channel * chan, uint8_t device);
int patapi_eject(struct ata_channel * chan, uint8_t device);
ssize_t patapi_read(struct ata_channel * chan, uint8_t device, void * dest, size_t len, loff_t off);


#endif /* __MODULES_DRIVERS_STORAGE_ATA_H */
