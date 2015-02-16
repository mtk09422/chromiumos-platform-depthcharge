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
#include "drivers/flash/mtk_emmc_flash.h"

/*
 * [TODO] This driver act as the flash wrapper for emmc device. Once MTK SPI
 * NOR is ready, we should remove this driver if no other concern.
 * We added some interfaces in mmc driver to support emmc flash wrapper. When
 * we decide to remove this driver, we probably need to check the mmc driver
 * as well.
 */

/*
 * Firmware are stored across BOOT0 and BOOT1 partition. mtk_emmc_flash_read
 * should take care of the partition switch if necessary.
 */
static void *mtk_emmc_flash_read(FlashOps *me, uint32_t offset, uint32_t size)
{
	/* number of blocks in each boot partition */
	static lba_t boot_block_num;

	MtkEmmcFlash *flash = container_of(me, MtkEmmcFlash, ops);
	uint8_t *data = NULL;
	MmcCtrlr *mmc = &flash->mmc_host->mmc;
	BlockDevCtrlr *bdev_ctrlr = &flash->mmc_host->mmc.ctrlr;
	uint32_t block_size = 0;
	lba_t start_block = 0;
	lba_t end_block = 0;
	lba_t block_cnt = 0;
	lba_t remain_block_cnt = 0;
	int retcode = 0;

	if ((offset > flash->rom_size) ||
	    ((offset + size) > flash->rom_size) ||
	    (0 == size)) {
		mmc_debug("Bad flash read offset or size.\n");
		return NULL;
	}

	if (bdev_ctrlr->need_update && bdev_ctrlr->ops.update) {
		retcode = bdev_ctrlr->ops.update(&bdev_ctrlr->ops);
		if (retcode) {
			mmc_debug("BlockDevCtrlrOps update failed.\n");
			goto exit;
		}

		retcode = mmc_get_boot_part_blocks(mmc, &boot_block_num);
		if (retcode) {
			mmc_debug("Get boot1 block numbers failed.\n");
			goto exit;
		}
	}

	MmcMedia *media = mmc->media;
	BlockDevOps *bdev_ops = &media->dev.ops;

	/* convert offset to block numbers and switch to specified partition */
	block_size = media->dev.block_size;
	start_block = offset / block_size;
	end_block = (offset + size - 1) / block_size;
	block_cnt = end_block - start_block + 1;

	if (start_block < boot_block_num) {
		/* switch to first boot region */
		retcode = mmc_switch_part(mmc, EXT_CSD_PART_CONF_ACC_BOOT0);
		if (retcode) {
			mmc_debug("mmc_switch_part boot1 failed.\n");
			goto exit;
		}

		if (end_block >= boot_block_num) {
			lba_t tmp = block_cnt;

			block_cnt = boot_block_num - start_block;
			remain_block_cnt = tmp - block_cnt;
		} else {
			remain_block_cnt = 0;
		}
	} else {
		/* switch to 2nd boot region */
		retcode = mmc_switch_part(mmc, EXT_CSD_PART_CONF_ACC_BOOT1);
		if (retcode) {
			mmc_debug("mmc_switch_part boot2 failed.\n");
			goto exit;
		}
		remain_block_cnt = 0;
	}

	if (!bdev_ops || !bdev_ops->read) {
		mmc_debug("bdev_ops or bdev_ops->read is null.\n");
		retcode = -1;
		goto exit;
	}

	data = flash->buffer + start_block * block_size;
	if (bdev_ops->read(bdev_ops, start_block, block_cnt, data) !=
	    block_cnt) {
		mmc_debug("Read block failed.\n");
		retcode = -1;
		goto exit;
	}

	if (remain_block_cnt) {
		/* switch to 2nd boot region */
		retcode = mmc_switch_part(mmc, EXT_CSD_PART_CONF_ACC_BOOT1);
		if (retcode) {
			mmc_debug("mmc_switch_part boot2 failed.\n");
			goto exit;
		}

		uint8_t *remain_data = data + (block_cnt * block_size);

		if (bdev_ops->read(bdev_ops, boot_block_num, remain_block_cnt,
				   remain_data) != remain_block_cnt) {
			mmc_debug("Read 2nd boot partition failed.\n");
			retcode = -2;
			goto exit;
		}
	}

exit:
	/* switch back to default partition before leaving */
	if (mmc_switch_part(mmc, EXT_CSD_PART_CONF_ACC_DEFAULT)) {
		mmc_debug("mmc_switch_part USER failed.\n");
		retcode = -3;
	}

	if (retcode)
		printf("mtk_emmc_flash_read failed!\n");

	return retcode ? NULL : flash->buffer + offset;
}

MtkEmmcFlash *new_mtk_emmc_flash(MtkMmcHost *mmc_host, uint32_t rom_size)
{
	MtkEmmcFlash *flash = xzalloc(sizeof(*flash));

	flash->ops.read = &mtk_emmc_flash_read;
	flash->mmc_host = mmc_host;
	flash->rom_size = rom_size;

	ALLOC_CACHE_ALIGN_BUFFER(uint8_t, buffer, rom_size);
	flash->buffer = buffer;

	return flash;
}
