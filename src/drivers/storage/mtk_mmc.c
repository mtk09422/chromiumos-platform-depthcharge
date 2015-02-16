/*
 * Copyright 2014 MediaTek Inc.
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

#include <assert.h>
#include <libpayload.h>
#include <stddef.h>
#include <stdint.h>

#include "drivers/storage/mtk_mmc.h"

#define MSDC_CMD_INTS	(MSDC_INTEN_CMDRDY | MSDC_INTEN_RSPCRCERR | \
			MSDC_INTEN_CMDTMO)

#define MSDC_DAT_INTS	(MSDC_INTEN_XFER_COMPL | MSDC_INTEN_DATTMO | \
			MSDC_INTEN_DATCRCERR | MSDC_INTEN_DMA_BDCSERR | \
			MSDC_INTEN_DMA_GPDCSERR | MSDC_INTEN_DMA_PROTECT)

enum {
	/* For card identification, and also the highest low-speed SDOI card */
	/* frequency (actually 400Khz). */
	MtkMmcMinFreq = 375000,

	/* Highest HS eMMC clock as per the SD/MMC spec (actually 52MHz). */
	MtkMmcMaxFreq = 50000000,

	/* Source clock configured by loader in previous stage. */
	MtkMmcSourceClock = 200000000,

	MtkMmcVoltages = (MMC_VDD_32_33 | MMC_VDD_33_34),
};

static void msdc_set_buswidth(MtkMmcHost *host, u32 width)
{
	MtkMmcReg *reg = host->reg;
	u32 val = readl(&reg->sdc_cfg);

	val &= ~SDC_CFG_BUSWIDTH;

	switch (width) {
	default:
	case 1:
		val |= (MSDC_BUS_1BITS << 16);
		break;
	case 4:
		val |= (MSDC_BUS_4BITS << 16);
		break;
	case 8:
		val |= (MSDC_BUS_8BITS << 16);
		break;
	}

	writel(val, &reg->sdc_cfg);
}

static void mtk_mmc_change_clock(MtkMmcHost *host, uint32_t clock)
{
	u32 mode;
	u32 div;
	u32 sclk;
	u32 hclk = MtkMmcSourceClock;
	MtkMmcReg *reg = host->reg;

	/* Only support SDR mode */
	if (clock >= hclk) {
		mode = 0x1;	/* no divisor */
		div = 0;
		sclk = hclk;
	} else {
		mode = 0x0;	/* use divisor */
		if (clock >= (hclk >> 1)) {
			div = 0;	/* mean div = 1/2 */
			sclk = hclk >> 1;	/* sclk = clk / 2 */
		} else {
			div = (hclk + ((clock << 2) - 1)) / (clock << 2);
			sclk = (hclk >> 2) / div;
		}
	}

	sdr_set_field(&reg->msdc_cfg, MSDC_CFG_CKMOD | MSDC_CFG_CKDIV,
	              (mode << 8) | ((div + 1) % 0xff));
	while (!(readl(&reg->msdc_cfg) & MSDC_CFG_CKSTB))
		mdelay(1);
	mmc_debug("sclk: %d\n", sclk);
}

static inline u32 msdc_cmd_find_resp(MmcCommand *cmd)
{
	u32 resp;

	switch (cmd->resp_type) {
		/* Actually, R1, R5, R6, R7 are the same */
	case MMC_RSP_R1:
		resp = RESP_R1;
		break;
	case MMC_RSP_R1b:
		resp = RESP_R1B;
		break;
	case MMC_RSP_R2:
		resp = RESP_R2;
		break;
	case MMC_RSP_R3:
		resp = RESP_R3;
		break;
	case MMC_RSP_NONE:
	default:
		resp = RESP_NONE;
		break;
	}

	return resp;
}

static void msdc_dma_on(MtkMmcHost *host)
{
	MtkMmcReg *reg = host->reg;
	sdr_clr_bits(&reg->msdc_cfg, MSDC_CFG_PIO);
}

static void msdc_dma_start(MtkMmcHost *host)
{
	MtkMmcReg *reg = host->reg;
	sdr_set_field(&reg->dma_ctrl, MSDC_DMA_CTRL_START, 1);
}

static void msdc_dma_stop(MtkMmcHost *host)
{
	MtkMmcReg *reg = host->reg;
	sdr_set_field(&reg->dma_ctrl, MSDC_DMA_CTRL_STOP, 1);
	while (readl(&reg->dma_cfg) & MSDC_DMA_CFG_STS) ;
}

static inline u32 msdc_cmd_prepare_raw_cmd(MtkMmcHost *host,
        MmcCommand *cmd, MmcData *data)
{
	MtkMmcReg *reg = host->reg;
	/* rawcmd :
	 * vol_swt << 30 | auto_cmd << 28 | blklen << 16 | go_irq << 15 |
	 * stop << 14 | rw << 13 | dtype << 11 | rsptyp << 7 | brk << 6 | opcode
	 */
	u32 opcode = cmd->cmdidx;
	u32 resp = msdc_cmd_find_resp(cmd);
	u32 rawcmd = (opcode & 0x3F) | ((resp & 7) << 7);

	if (opcode == MMC_CMD_STOP_TRANSMISSION)
		rawcmd |= (1 << 14);

	if ((opcode == SD_CMD_APP_SEND_SCR) ||
	    (opcode == SD_CMD_SWITCH_FUNC &&
	     cmd->resp_type == MMC_RSP_R1 /* for Sd card */) ||
	    (opcode == MMC_CMD_SEND_EXT_CSD && data != NULL))
		rawcmd |= (1 << 11);

	if (data) {
		rawcmd |= ((data->blocksize & 0xFFF) << 16);
		if (data->flags & MMC_DATA_WRITE)
			rawcmd |= (1 << 13);
		if (data->blocks > 1)
			rawcmd |= (2 << 11);
		else
			rawcmd |= (1 << 11);

		msdc_dma_on(host);	/* always use basic DMA */

		writel(data->blocks, &reg->sdc_blk_num);
	}
	return rawcmd;
}

static inline u32 msdc_cmd_prepare_raw_arg(MtkMmcHost *host, MmcCommand *cmd)
{
	u32 rawarg = cmd->cmdarg;
	return rawarg;
}

static inline u32 msdc_get_irq_status(MtkMmcHost *host, u32 mask)
{
	u32 status = readl(&host->reg->msdc_int);
	if (status)
		writel(status, &host->reg->msdc_int);
	return status & mask;
}

/* returns true if command is fully handled; returns false otherwise */
static int msdc_cmd_done(MtkMmcHost *host, int events, MmcCommand *cmd)
{
	MtkMmcReg *reg = host->reg;

	if (events & (MSDC_INT_CMDRDY | MSDC_INT_RSPCRCERR | MSDC_INT_CMDTMO)) {
		if (!(events & MSDC_INT_CMDRDY)) {
			if (events & MSDC_INT_CMDTMO) {
				mmc_debug("cmd: %d timeout!\n", cmd->cmdidx);
				return MMC_TIMEOUT;
			}
			mmc_error("cmd: %d crc error!\n", cmd->cmdidx);
			return MMC_COMM_ERR;
		}
		switch (cmd->resp_type) {
		case MMC_RSP_NONE:
			break;
		case MMC_RSP_R2:
			cmd->response[0] = readl(&reg->sdc_resp3);
			cmd->response[1] = readl(&reg->sdc_resp2);
			cmd->response[2] = readl(&reg->sdc_resp1);
			cmd->response[3] = readl(&reg->sdc_resp0);
			break;
		default:	/* Response types 1, 3, 4, 5, 6, 7(1b) */
			cmd->response[0] = readl(&reg->sdc_resp0);
			break;

		}
		return 0;
	}

	return -1;		/* unexpected interrupts */
}

static inline int msdc_cmd_do_poll(MtkMmcHost *host, MmcCommand *cmd)
{
	int ret;
	int intsts;
	u32 done = 0;

	for (; !done;) {
		intsts = msdc_get_irq_status(host, MSDC_CMD_INTS);
		done = intsts;
	}
	ret = msdc_cmd_done(host, intsts, cmd);
	if (ret < 0) {
		return ret;
	}

	return 0;
}

static void mtk_mmc_prepare_data(MtkMmcHost *host, MmcData *data,
                                 struct bounce_buffer *bbstate)
{
	MtkMmcReg *reg = host->reg;

	writel((uintptr_t) bbstate->bounce_buffer, &reg->dma_sa);
	writel(data->blocks * data->blocksize, &reg->dma_length);
	sdr_set_field(&reg->dma_ctrl, MSDC_DMA_CTRL_LASTBUF, 1);
	sdr_set_field(&reg->dma_ctrl, MSDC_DMA_CTRL_MODE, 0);
}

static int sdc_is_busy(MtkMmcHost *host, unsigned int timeout,
                       MmcCommand *cmd, MmcData *data)
{
	MtkMmcReg *reg = host->reg;
	while (readl(&reg->sdc_sts) & SDC_STS_CMDBUSY) {
		if (timeout == 0) {
			mmc_error("%s: timeout error\n", __func__);
			return -1;
		}
		timeout--;
		udelay(1000);
	}

	if (cmd->resp_type == MMC_RSP_R1b || data) {
		/* should wait SDCBUSY goto high */
		while (readl(&reg->sdc_sts) & SDC_STS_SDCBUSY)
			udelay(1000);
	}

	return 0;
}

static int mtk_mmc_send_cmd_bounced(MmcCtrlr *ctrlr, MmcCommand *cmd,
                                    MmcData *data,
                                    struct bounce_buffer *bbstate)
{
	MtkMmcHost *host = container_of(ctrlr, MtkMmcHost, mmc);
	MtkMmcReg *reg = host->reg;
	u32 done = 0;
	int ret = 0;
	int result;
	u32 rawcmd;
	u32 rawarg;
	u32 ints;
	mmc_debug("%s called\n", __func__);

	result = sdc_is_busy(host, 10, cmd, data);

	if (result < 0)
		return result;

	rawcmd = msdc_cmd_prepare_raw_cmd(host, cmd, data);
	rawarg = msdc_cmd_prepare_raw_arg(host, cmd);

	writel(rawarg, &reg->sdc_arg);
	writel(rawcmd, &reg->sdc_cmd);

	ret = msdc_cmd_do_poll(host, cmd);
	if (ret < 0) {
		return ret;
	}

	if (data) {
		mtk_mmc_prepare_data(host, data, bbstate);
		msdc_dma_start(host);
		for (; !done;) {
			ints = msdc_get_irq_status(host, MSDC_DAT_INTS);
			done = ints;
		}
		msdc_dma_stop(host);

		if (ints & MSDC_INT_XFER_COMPL) {
			return 0;
		} else {
			if (ints & MSDC_INT_DATTMO)
				mmc_error("Data timeout, ints: %x\n", ints);
			else if (ints & MSDC_INT_DATCRCERR)
				mmc_error("Data crc error, ints: %x\n", ints);
			return -1;
		}
	}

	mmc_debug("cmd->arg: %08x\n", cmd->cmdarg);

	return 0;
}

static int mtk_mmc_send_cmd(MmcCtrlr *ctrlr, MmcCommand *cmd, MmcData *data)
{
	void *buf;
	unsigned int bbflags;
	size_t len;
	struct bounce_buffer bbstate;
	int ret;

	if (data) {
		if (data->flags & MMC_DATA_READ) {
			buf = data->dest;
			bbflags = GEN_BB_WRITE;
		} else {
			buf = (void *)data->src;
			bbflags = GEN_BB_READ;
		}
		len = data->blocks * data->blocksize;

		bounce_buffer_start(&bbstate, buf, len, bbflags);
	}

	ret = mtk_mmc_send_cmd_bounced(ctrlr, cmd, data, &bbstate);

	if (data)
		bounce_buffer_stop(&bbstate);

	return ret;
}

static void mtk_mmc_set_ios(MmcCtrlr *ctrlr)
{
	MtkMmcHost *host = container_of(ctrlr, MtkMmcHost, mmc);

	mmc_debug("%s: called, bus_width: %x, clock: %d -> %d\n", __func__,
	          ctrlr->bus_width, host->clock, ctrlr->bus_hz);

	/* Change clock first */
	mtk_mmc_change_clock(host, ctrlr->bus_hz);
	msdc_set_buswidth(host, ctrlr->bus_width);
}

static void msdc_reset_hw(MtkMmcHost *host)
{
	MtkMmcReg *reg = host->reg;

	u32 val;
	sdr_set_bits(&(reg)->msdc_cfg, MSDC_CFG_RST);
	while (readl(&(reg)->msdc_cfg) & MSDC_CFG_RST)
		mdelay(1);
	sdr_set_bits(&(reg)->msdc_fifocs, MSDC_FIFOCS_CLR);
	while (readl(&(reg)->msdc_fifocs) & MSDC_FIFOCS_CLR)
		mdelay(1);
	val = readl(&(reg)->msdc_int);
	writel(val, &(reg)->msdc_int);
}

static int mtk_mmc_init(BlockDevCtrlrOps *me)
{
	MtkMmcHost *host = container_of(me, MtkMmcHost, mmc.ctrlr.ops);
	MtkMmcReg *reg = host->reg;
	mmc_debug("%s called\n", __func__);

	/* Configure to MMC/SD mode */
	sdr_set_field(&reg->msdc_cfg, MSDC_CFG_MODE, MSDC_SDMMC);
	/* Reset */
	msdc_reset_hw(host);
	/* Disable card detection */
	sdr_clr_bits(&reg->msdc_ps, MSDC_PS_CDEN);
	/* Disable and clear all interrupts */
	sdr_clr_bits(&reg->msdc_inten, readl(&reg->msdc_inten));
	writel(readl(&reg->msdc_int), &reg->msdc_int);
	/* Configure to default data timeout */
	sdr_set_field(&reg->sdc_cfg, SDC_CFG_DTOC, DEFAULT_DTOC);
	msdc_set_buswidth(host, 1);

	return 0;
}

static int mtk_mmc_update(BlockDevCtrlrOps *me)
{
	MtkMmcHost *host = container_of(me, MtkMmcHost, mmc.ctrlr.ops);
	if (!host->initialized && mtk_mmc_init(me))
		return -1;
	host->initialized = 1;

	if (host->removable) {
		int present = !host->cd_gpio->get(host->cd_gpio);
		mmc_debug("SD card present: %d\n", present);
		if (present && !host->mmc.media) {
			/* A card is present and not set up yet. Get it ready. */
			if (mmc_setup_media(&host->mmc))
				return -1;
			host->mmc.media->dev.name = "removable mtk_mmc";
			host->mmc.media->dev.removable = 1;
			host->mmc.media->dev.ops.read = &block_mmc_read;
			host->mmc.media->dev.ops.write = &block_mmc_write;
			list_insert_after(&host->mmc.media->dev.list_node,
			                  &removable_block_devices);
		} else if (!present && host->mmc.media) {
			/* A card was present but isn't any more. Get rid of it. */
			list_remove(&host->mmc.media->dev.list_node);
			free(host->mmc.media);
			host->mmc.media = NULL;
		}
	} else {
		if (mmc_setup_media(&host->mmc))
			return -1;
		host->mmc.media->dev.name = "mtk_mmc";
		host->mmc.media->dev.removable = 0;
		host->mmc.media->dev.ops.read = &block_mmc_read;
		host->mmc.media->dev.ops.write = &block_mmc_write;
		host->mmc.media->dev.ops.new_stream = &new_simple_stream;
		list_insert_after(&host->mmc.media->dev.list_node,
		                  &fixed_block_devices);
		host->mmc.ctrlr.need_update = 0;
	}
	return 0;
}

MtkMmcHost *new_mtk_mmc_host(uintptr_t ioaddr, int bus_width, int removable,
                             GpioOps *card_detect)
{
	MtkMmcHost *ctrlr = xzalloc(sizeof(*ctrlr));

	ctrlr->mmc.ctrlr.ops.update = &mtk_mmc_update;
	ctrlr->mmc.ctrlr.need_update = 1;

	ctrlr->mmc.voltages = MtkMmcVoltages;
	ctrlr->mmc.f_min = MtkMmcMinFreq;
	ctrlr->mmc.f_max = MtkMmcMaxFreq;

	ctrlr->mmc.bus_width = bus_width;
	ctrlr->mmc.bus_hz = ctrlr->mmc.f_min;
	ctrlr->mmc.b_max = 65535;	/* Some controllers use 16-bit regs. */

	if (bus_width == 8) {
		ctrlr->mmc.caps |= MMC_MODE_8BIT;
		ctrlr->mmc.caps &= ~MMC_MODE_4BIT;
	} else {
		ctrlr->mmc.caps |= MMC_MODE_4BIT;
		ctrlr->mmc.caps &= ~MMC_MODE_8BIT;
	}
	ctrlr->mmc.caps |= MMC_MODE_HS | MMC_MODE_HS_52MHz | MMC_MODE_HC;
	ctrlr->mmc.send_cmd = &mtk_mmc_send_cmd;
	ctrlr->mmc.set_ios = &mtk_mmc_set_ios;

	ctrlr->src_hz = MtkMmcSourceClock;
	ctrlr->reg = (MtkMmcReg *)ioaddr;
	ctrlr->removable = removable;
	ctrlr->cd_gpio = card_detect;

	return ctrlr;
}
