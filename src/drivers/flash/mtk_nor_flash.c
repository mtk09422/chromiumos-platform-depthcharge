/*
 * Copyright 2013 Google Inc.
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but without any warranty; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <libpayload.h>

#include "base/container_of.h"
#include <stdint.h>
#include "drivers/flash/flash.h"

#include "mtk_nor_flash.h"
#include "mtk_nor_if.h"
#include "typedefs.h"
#include "mtk_serialflash_hw.h"

#include <assert.h>
/*
 * [TODO] This driver act as the flash wrapper for nor device. Once MTK SPI
 * NOR is ready, we should remove this driver if no other concern.
 * We added some interfaces in nor driver to support nor flash wrapper. When
 * we decide to remove this driver, we probably need to check the nor driver
 * as well.
 */

/*
 * Firmware are stored across BOOT0 and BOOT1 partition. mtk_emmc_flash_read
 * should take care of the partition switch if necessary.
 */



static void *mtk_nor_flash_read(FlashOps *me, uint32_t offset, uint32_t size)
{
	MtkNorFlash *flash = container_of(me, MtkNorFlash, ops);
	uint8_t *data = NULL;
	int ret;

	if ((offset > flash->rom_size) ||
	    ((offset + size) > flash->rom_size) ||
	    (0 == size)) {
		///printk("Bad flash read offset or size.\n");
		return NULL;
	}

	NOR_Init();

	data = flash->buffer + offset;

	if ((ret = NOR_Read((uint64_t)offset, (uint32_t)((uintptr_t)data), size)) != 0 ) {
		///printk("partitial Read  fail !!! \n");
	}

	return data;
}

static int mtk_nor_flash_write(FlashOps *me, const void *buffer,
			       uint32_t offset, uint32_t size)
{
	MtkNorFlash *flash = container_of(me, MtkNorFlash, ops);

	uint32_t ret;

	assert(offset + size <= flash->rom_size);

	/* Write in chunks guaranteed not to cross page boundaries, */
	if ((ret = NOR_Write((uint64_t)offset, (uint32_t)((uintptr_t)buffer), size)) != 0) {
		///printk("partitial write test fail !!! \n");
	}
	return ret;
}

static int mtk_nor_flash_erase(FlashOps *me, uint32_t start, uint32_t size)
{
	MtkNorFlash *flash = container_of(me, MtkNorFlash, ops);
	uint32_t sector_size = 4096;

	if ((start % sector_size) || (size % sector_size)) {
		///printk("%s: Erase not %u aligned, start=%u size=%u\n",
		///      __func__, sector_size, start, size);
		return -1;
	}
	assert(start + size <= flash->rom_size);
	int offset;
	for (offset = 0; offset < size; offset += sector_size) {
		SFLASH_EraseAddr(start + offset, sector_size);
	}
	return offset;
}
#define DMA_MINALIGN (64)
#define ROUND(a, b) (((a) + (b) - 1) & ~((b) - 1))
#define ALLOC_CACHE_ALIGN_BUFFER(type, name, size)		     \
	char __ ## name[ROUND(size * sizeof(type), DMA_MINALIGN) +     \
			DMA_MINALIGN - 1];			       \
	type *name = (type*)ALIGN((uintptr_t)__ ## name, DMA_MINALIGN)


MtkNorFlash *new_mtk_nor_flash(uint32_t rom_size)
{
	MtkNorFlash *flash = xmalloc(sizeof(*flash));

	memset(flash, 0, sizeof(*flash));
	flash->ops.read = mtk_nor_flash_read;
	flash->ops.write = mtk_nor_flash_write;
	flash->ops.erase = mtk_nor_flash_erase;
	flash->ops.sector_size = 4096;
	flash->rom_size = rom_size;
	//flash->erase_cmd = erase_cmd;

	ALLOC_CACHE_ALIGN_BUFFER(uint8_t, buffer, rom_size);
	flash->buffer = buffer;
	return flash;
}
#if 0

static int mtk_nor_update(BlockDevCtrlrOps *me)
{
	MtkNorHost *host = container_of(me, MtkNorHost, mmc.ctrlr.ops);

	if (!host->initialized && NOR_Init())
		return -1;
	host->initialized = 1;

	//if (mmc_setup_media(&host->mmc))
	//	return -1;
	//host->mmc.media->dev.name = "mtk_mmc";
	//host->mmc.media->dev.removable = 0;
	//host->mmc.media->dev.ops.read = &block_mmc_read;
	//host->mmc.media->dev.ops.write = &block_mmc_write;
	//host->mmc.media->dev.ops.new_stream = &new_simple_stream;
	//list_insert_after(&host->mmc.media->dev.list_node,
	//		&fixed_block_devices);
	host->mmc.ctrlr.need_update = 0;

	return 0;
}

MtkNorHost *new_mtk_nor_host(uintptr_t ioaddr, int bus_width, int removable)
{
	MtkNorHost *ctrlr = xzalloc(sizeof(*ctrlr));

	ctrlr->nor.ctrlr.ops.update = &mtk_nor_update;
	ctrlr->nor.ctrlr.need_update = 1;


	ctrlr->reg = (MtkNorReg*)ioaddr;
	ctrlr->removable = removable;

	return ctrlr;
}
#endif