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

enum {
	EXT_CSD_PART_CFG_DEFT_PART = 0,		/* default user partition */
	EXT_CSD_PART_CFG_BOOT_PART_1 = 1,	/* 1st boot partition */
	EXT_CSD_PART_CFG_BOOT_PART_2 = 2	/* 2nd boot partition */
};

/* EXT_CSD field */
enum { EXT_CSD_BOOT_SIZE_MULT = 226 };

static int __emmc_flash_debug;
static int __emmc_flash_trace;

#define emmc_flash_debug(format...) \
	while (__emmc_flash_debug) { printf("emmc flash: " format); break; }
#define emmc_flash_trace(format...) \
	while (__emmc_flash_trace) { printf(format); break; }
#define emmc_flash_error(format...) printf("emmc flash: ERROR: " format)

/* functions from drivers/storage/mmc.c */
static int mmc_send_cmd(MmcCtrlr *ctrlr, MmcCommand *cmd, MmcData *data)
{
	int ret = -1;
	int retries = 2;

	emmc_flash_trace("CMD_SEND:%d %p\n", cmd->cmdidx, ctrlr);
	emmc_flash_trace("\tARG\t\t\t %#8.8x\n", cmd->cmdarg);
	emmc_flash_trace("\tFLAG\t\t\t %d\n", cmd->flags);
	if (data) {
		emmc_flash_trace("\t%s %d block(s) of %d bytes (%p)\n",
				 data->flags == MMC_DATA_READ ? "READ" : "WRITE",
			data->blocks, data->blocksize, data->dest);
	}

	while (retries--) {
		ret = ctrlr->send_cmd(ctrlr, cmd, data);

		switch (cmd->resp_type) {
		case MMC_RSP_NONE:
			emmc_flash_trace("\tMMC_RSP_NONE\n");
			break;

		case MMC_RSP_R1:
			emmc_flash_trace("\tMMC_RSP_R1,5,6,7\t %#8.8x\n",
					 cmd->response[0]);
			break;

		case MMC_RSP_R1b:
			emmc_flash_trace("\tMMC_RSP_R1b\t\t %#8.8x\n",
					 cmd->response[0]);
			break;

		case MMC_RSP_R2:
			emmc_flash_trace("\tMMC_RSP_R2\t\t %#8.8x\n",
					 cmd->response[0]);
			emmc_flash_trace("\t          \t\t %#8.8x\n",
					 cmd->response[1]);
			emmc_flash_trace("\t          \t\t %#8.8x\n",
					 cmd->response[2]);
			emmc_flash_trace("\t          \t\t %#8.8x\n",
					 cmd->response[3]);
			break;

		case MMC_RSP_R3:
			emmc_flash_trace("\tMMC_RSP_R3,4\t\t %#8.8x\n",
					 cmd->response[0]);
			break;

		default:
			emmc_flash_trace("\tERROR MMC rsp not supported\n");
			break;
		}
		emmc_flash_trace("\trv:\t\t\t %d\n", ret);

		/* Retry failed data commands, bail out otherwise. */
		if (!data || !ret)
			break;
	}

	return ret;
}

static int mmc_send_ext_csd(MmcCtrlr *ctrlr, uint8_t *ext_csd)
{
	int rv;
	MmcCommand cmd;
	MmcData data;

	cmd.cmdidx = MMC_CMD_SEND_EXT_CSD;
	cmd.resp_type = MMC_RSP_R1;
	cmd.cmdarg = 0;
	cmd.flags = 0;

	data.dest = (char *)ext_csd;
	data.blocks = 1;
	data.blocksize = 512;
	data.flags = MMC_DATA_READ;

	rv = mmc_send_cmd(ctrlr, &cmd, &data);
	if (!rv && __emmc_flash_trace) {
		int i, size;

		size = data.blocks * data.blocksize;
		emmc_flash_trace("\t%p ext_csd:", ctrlr);
		for (i = 0; i < size; i++) {
			if (!(i % 32))
				printf("\n");
			printf(" %2.2x", ext_csd[i]);
		}
		printf("\n");
	}
	return rv;
}

static int mmc_send_status(MmcMedia *media, int tries)
{
	MmcCommand cmd;

	cmd.cmdidx = MMC_CMD_SEND_STATUS;
	cmd.resp_type = MMC_RSP_R1;
	cmd.cmdarg = media->rca << 16;
	cmd.flags = 0;

	while (tries--) {
		int err = mmc_send_cmd(media->ctrlr, &cmd, NULL);
		if (err) {
			return err;
		} else if (cmd.response[0] & MMC_STATUS_RDY_FOR_DATA) {
			break;
		} else if (cmd.response[0] & MMC_STATUS_MASK) {
			emmc_flash_error("Status Error: %#8.8x\n",
					 cmd.response[0]);
			return MMC_COMM_ERR;
		}

		udelay(100);
	}

	emmc_flash_trace("CURR STATE:%d\n",
			 (cmd.response[0] & MMC_STATUS_CURR_STATE) >> 9);

	if (tries < 0) {
		emmc_flash_error("Timeout waiting card ready\n");
		return MMC_TIMEOUT;
	}
	return 0;
}

static int mmc_switch(MmcMedia *media, uint8_t set, uint8_t index,
		      uint8_t value)
{
	MmcCommand cmd;

	cmd.cmdidx = MMC_CMD_SWITCH;
	cmd.resp_type = MMC_RSP_R1b;
	cmd.cmdarg = ((MMC_SWITCH_MODE_WRITE_BYTE << 24) |
		      (index << 16) |
		      (value << 8));
	cmd.flags = 0;

	int ret = mmc_send_cmd(media->ctrlr, &cmd, NULL);

	/* Waiting for the ready status */
	mmc_send_status(media, MMC_IO_RETRIES);

	return ret;
}
/* end of functions from drivers/storage/mmc.c  */

static int mmc_switch_part(MmcCtrlr *ctrlr, uint32_t new_part)
{
	int err;
	uint8_t config;

	ALLOC_CACHE_ALIGN_BUFFER(unsigned char, ext_csd, 512);

	err = mmc_send_ext_csd(ctrlr, ext_csd);
	if (err)
		return -1;

	if (IS_SD(ctrlr->media))
		return -2;

	if (ext_csd[EXT_CSD_REV] >= 3) {
		config = ext_csd[EXT_CSD_PART_CONF];

		/* already set to specified partition */
		if ((config & 0x7) == new_part)
			return 0;

		config = (config & ~0x7) | new_part;

		err = mmc_switch(ctrlr->media, EXT_CSD_CMD_SET_NORMAL,
				 EXT_CSD_PART_CONF, config);
		if (err) {
			err = mmc_send_ext_csd(ctrlr, ext_csd);
			if (err && (ext_csd[EXT_CSD_PART_CONF] != config))
				return -3;
		}
	} else {
		return -4;
	}

	return 0;
}

static lba_t mmc_get_boot_part_blocks(MmcCtrlr *ctrlr, lba_t *blocks_cnt)
{
	int err;
	ALLOC_CACHE_ALIGN_BUFFER(unsigned char, ext_csd, 512);

	if (IS_SD(ctrlr->media))
		return -1;

	err = mmc_send_ext_csd(ctrlr, ext_csd);
	if (err)
		return err;

	if (ext_csd[EXT_CSD_REV] < 3)
		return -1;

	*blocks_cnt = ext_csd[EXT_CSD_BOOT_SIZE_MULT] * 128 * 1024 / 512;
	return 0;
}

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
		emmc_flash_debug("Bad flash read offset or size.\n");
		return NULL;
	}

	if (bdev_ctrlr->need_update && bdev_ctrlr->ops.update) {
		retcode = bdev_ctrlr->ops.update(&bdev_ctrlr->ops);
		if (retcode) {
			emmc_flash_debug("BlockDevCtrlrOps update failed.\n");
			goto exit;
		}

		retcode = mmc_get_boot_part_blocks(mmc, &boot_block_num);
		if (retcode) {
			emmc_flash_debug("Get boot1 block numbers failed.\n");
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
		retcode = mmc_switch_part(mmc, EXT_CSD_PART_CFG_BOOT_PART_1);
		if (retcode) {
			emmc_flash_debug("mmc_switch_part boot1 failed.\n");
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
		retcode = mmc_switch_part(mmc, EXT_CSD_PART_CFG_BOOT_PART_2);
		if (retcode) {
			emmc_flash_debug("mmc_switch_part boot2 failed.\n");
			goto exit;
		}
		remain_block_cnt = 0;
	}

	if (!bdev_ops || !bdev_ops->read) {
		emmc_flash_debug("bdev_ops or bdev_ops->read is null.\n");
		retcode = -1;
		goto exit;
	}

	data = flash->buffer + start_block * block_size;
	if (bdev_ops->read(bdev_ops, start_block, block_cnt, data) !=
			block_cnt) {
		emmc_flash_debug("Read block failed.\n");
		retcode = -1;
		goto exit;
	}

	if (remain_block_cnt) {
		/* switch to 2nd boot region */
		retcode = mmc_switch_part(mmc, EXT_CSD_PART_CFG_BOOT_PART_2);
		if (retcode) {
			emmc_flash_debug("mmc_switch_part boot2 failed.\n");
			goto exit;
		}

		uint8_t *remain_data = data + (block_cnt * block_size);
		if (bdev_ops->read(bdev_ops, boot_block_num, remain_block_cnt,
				   remain_data) != remain_block_cnt) {
			emmc_flash_debug("Read 2nd boot partition failed.\n");
			retcode = -2;
			goto exit;
		}
	}

exit:
	/* switch back to default partition before leaving */
	if (mmc_switch_part(mmc, EXT_CSD_PART_CFG_DEFT_PART)) {
		emmc_flash_debug("mmc_switch_part USER failed.\n");
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
