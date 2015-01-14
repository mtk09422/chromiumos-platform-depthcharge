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

#ifndef __DRIVERS_STORAGE_MTK_MMC_H_
#define __DRIVERS_STORAGE_MTK_MMC_H_

#include <arch/io.h>

#include "drivers/gpio/gpio.h"
#include "drivers/storage/blockdev.h"
#include "drivers/storage/mmc.h"

enum {
	RESP_NONE = 0,
	RESP_R1 = 1,
	RESP_R2 = 2,
	RESP_R3 = 3,
	RESP_R4 = 4,
	RESP_R5 = 1,
	RESP_R6 = 1,
	RESP_R7 = 1,
	RESP_R1B = 7
};

typedef struct {
	uint32_t msdc_cfg;
	uint32_t msdc_iocon;
	uint32_t msdc_ps;
	uint32_t msdc_int;
	uint32_t msdc_inten;
	uint32_t msdc_fifocs;
	uint32_t msdc_txdata;
	uint32_t msdc_rxdata;
	uint8_t reserve0[0x10];
	uint32_t sdc_cfg;
	uint32_t sdc_cmd;
	uint32_t sdc_arg;
	uint32_t sdc_sts;
	uint32_t sdc_resp0;
	uint32_t sdc_resp1;
	uint32_t sdc_resp2;
	uint32_t sdc_resp3;
	uint32_t sdc_blk_num;
	uint32_t reserver1;
	uint32_t sdc_csts;
	uint32_t sdc_csts_en;
	uint32_t sdc_datcrc_sts;
	uint8_t reserve2[0x0c];
	uint32_t emmc_cfg0;
	uint32_t emmc_cfg1;
	uint32_t emmc_sts;
	uint32_t emmc_iocon;
	uint32_t sd_acmd_resp;
	uint32_t sd_acmd19_trg;
	uint32_t sd_acmd19_sts;
	uint32_t reserve3;
	uint32_t dma_sa;
	uint32_t dma_ca;
	uint32_t dma_ctrl;
	uint32_t dma_cfg;
	uint32_t sw_dbg_sel;
	uint32_t sw_dbg_out;
	uint32_t dma_length;
	uint32_t reserve4;
	uint32_t patch_bit0;
	uint32_t patch_bit1;
	uint8_t reserve5[0x34];
	uint32_t pad_tune;
	uint32_t dat_rd_dly0;
	uint32_t dat_rd_dly1;
	uint32_t hw_dbg_sel;
	uint32_t dummy10;
	uint32_t main_ver;
	uint32_t eco_ver;
} MtkMmcReg;

/*--------------------------------------------------------------------------*/
/* Register Mask                                                            */
/*--------------------------------------------------------------------------*/

/* MSDC_CFG mask */
#define MSDC_CFG_MODE		(0x1  << 0)	/* RW */
#define MSDC_CFG_CKPDN		(0x1  << 1)	/* RW */
#define MSDC_CFG_RST		(0x1  << 2)	/* RW */
#define MSDC_CFG_PIO		(0x1  << 3)	/* RW */
#define MSDC_CFG_CKDRVEN	(0x1  << 4)	/* RW */
#define MSDC_CFG_BV18SDT	(0x1  << 5)	/* RW */
#define MSDC_CFG_BV18PSS	(0x1  << 6)	/* R  */
#define MSDC_CFG_CKSTB		(0x1  << 7)	/* R  */
#define MSDC_CFG_CKDIV		(0xff << 8)	/* RW */
#define MSDC_CFG_CKMOD		(0x3  << 16)	/* RW */

/* MSDC_IOCON mask */
#define MSDC_IOCON_SDR104CKS	(0x1  << 0)	/* RW */
#define MSDC_IOCON_RSPL		(0x1  << 1)	/* RW */
#define MSDC_IOCON_DSPL		(0x1  << 2)	/* RW */
#define MSDC_IOCON_DDLSEL	(0x1  << 3)	/* RW */
#define MSDC_IOCON_DDR50CKD	(0x1  << 4)	/* RW */
#define MSDC_IOCON_DSPLSEL	(0x1  << 5)	/* RW */
#define MSDC_IOCON_W_DSPL	(0x1  << 8)	/* RW */
#define MSDC_IOCON_D0SPL	(0x1  << 16)	/* RW */
#define MSDC_IOCON_D1SPL	(0x1  << 17)	/* RW */
#define MSDC_IOCON_D2SPL	(0x1  << 18)	/* RW */
#define MSDC_IOCON_D3SPL	(0x1  << 19)	/* RW */
#define MSDC_IOCON_D4SPL	(0x1  << 20)	/* RW */
#define MSDC_IOCON_D5SPL	(0x1  << 21)	/* RW */
#define MSDC_IOCON_D6SPL	(0x1  << 22)	/* RW */
#define MSDC_IOCON_D7SPL	(0x1  << 23)	/* RW */
#define MSDC_IOCON_RISCSZ	(0x3  << 24)	/* RW */

/* MSDC_PS mask */
#define MSDC_PS_CDEN		(0x1  << 0)	/* RW */
#define MSDC_PS_CDSTS		(0x1  << 1)	/* R  */
#define MSDC_PS_CDDEBOUNCE	(0xf  << 12)	/* RW */
#define MSDC_PS_DAT		(0xff << 16)	/* R  */
#define MSDC_PS_CMD		(0x1  << 24)	/* R  */
#define MSDC_PS_WP		(0x1UL << 31)	/* R  */

/* MSDC_INT mask */
#define MSDC_INT_MMCIRQ		(0x1  << 0)	/* W1C */
#define MSDC_INT_CDSC		(0x1  << 1)	/* W1C */
#define MSDC_INT_ACMDRDY	(0x1  << 3)	/* W1C */
#define MSDC_INT_ACMDTMO	(0x1  << 4)	/* W1C */
#define MSDC_INT_ACMDCRCERR	(0x1  << 5)	/* W1C */
#define MSDC_INT_DMAQ_EMPTY	(0x1  << 6)	/* W1C */
#define MSDC_INT_SDIOIRQ	(0x1  << 7)	/* W1C */
#define MSDC_INT_CMDRDY		(0x1  << 8)	/* W1C */
#define MSDC_INT_CMDTMO		(0x1  << 9)	/* W1C */
#define MSDC_INT_RSPCRCERR	(0x1  << 10)	/* W1C */
#define MSDC_INT_CSTA		(0x1  << 11)	/* R */
#define MSDC_INT_XFER_COMPL	(0x1  << 12)	/* W1C */
#define MSDC_INT_DXFER_DONE	(0x1  << 13)	/* W1C */
#define MSDC_INT_DATTMO		(0x1  << 14)	/* W1C */
#define MSDC_INT_DATCRCERR	(0x1  << 15)	/* W1C */
#define MSDC_INT_ACMD19_DONE	(0x1  << 16)	/* W1C */
#define MSDC_INT_DMA_BDCSERR	(0x1  << 17)	/* W1C */
#define MSDC_INT_DMA_GPDCSERR	(0x1  << 18)	/* W1C */
#define MSDC_INT_DMA_PROTECT	(0x1  << 19)	/* W1C */

/* MSDC_INTEN mask */
#define MSDC_INTEN_MMCIRQ	(0x1  << 0)	/* RW */
#define MSDC_INTEN_CDSC		(0x1  << 1)	/* RW */
#define MSDC_INTEN_ACMDRDY	(0x1  << 3)	/* RW */
#define MSDC_INTEN_ACMDTMO	(0x1  << 4)	/* RW */
#define MSDC_INTEN_ACMDCRCERR	(0x1  << 5)	/* RW */
#define MSDC_INTEN_DMAQ_EMPTY	(0x1  << 6)	/* RW */
#define MSDC_INTEN_SDIOIRQ	(0x1  << 7)	/* RW */
#define MSDC_INTEN_CMDRDY	(0x1  << 8)	/* RW */
#define MSDC_INTEN_CMDTMO	(0x1  << 9)	/* RW */
#define MSDC_INTEN_RSPCRCERR	(0x1  << 10)	/* RW */
#define MSDC_INTEN_CSTA		(0x1  << 11)	/* RW */
#define MSDC_INTEN_XFER_COMPL	(0x1  << 12)	/* RW */
#define MSDC_INTEN_DXFER_DONE	(0x1  << 13)	/* RW */
#define MSDC_INTEN_DATTMO	(0x1  << 14)	/* RW */
#define MSDC_INTEN_DATCRCERR	(0x1  << 15)	/* RW */
#define MSDC_INTEN_ACMD19_DONE	(0x1  << 16)	/* RW */
#define MSDC_INTEN_DMA_BDCSERR	(0x1  << 17)	/* RW */
#define MSDC_INTEN_DMA_GPDCSERR	(0x1  << 18)	/* RW */
#define MSDC_INTEN_DMA_PROTECT	(0x1  << 19)	/* RW */

/* MSDC_FIFOCS mask */
#define MSDC_FIFOCS_RXCNT	(0xff << 0)	/* R */
#define MSDC_FIFOCS_TXCNT	(0xff << 16)	/* R */
#define MSDC_FIFOCS_CLR		(0x1UL << 31)	/* RW */

/* SDC_CFG mask */
#define SDC_CFG_SDIOINTWKUP	(0x1  << 0)	/* RW */
#define SDC_CFG_INSWKUP		(0x1  << 1)	/* RW */
#define SDC_CFG_BUSWIDTH	(0x3  << 16)	/* RW */
#define SDC_CFG_SDIO		(0x1  << 19)	/* RW */
#define SDC_CFG_SDIOIDE		(0x1  << 20)	/* RW */
#define SDC_CFG_INTATGAP	(0x1  << 21)	/* RW */
#define SDC_CFG_DTOC		(0xffUL << 24)	/* RW */

/* SDC_CMD mask */
#define SDC_CMD_OPC		(0x3f << 0)	/* RW */
#define SDC_CMD_BRK		(0x1  << 6)	/* RW */
#define SDC_CMD_RSPTYP		(0x7  << 7)	/* RW */
#define SDC_CMD_DTYP		(0x3  << 11)	/* RW */
#define SDC_CMD_RW		(0x1  << 13)	/* RW */
#define SDC_CMD_STOP		(0x1  << 14)	/* RW */
#define SDC_CMD_GOIRQ		(0x1  << 15)	/* RW */
#define SDC_CMD_BLKLEN		(0xfff << 16)	/* RW */
#define SDC_CMD_AUTOCMD		(0x3  << 28)	/* RW */
#define SDC_CMD_VOLSWTH		(0x1  << 30)	/* RW */

/* SDC_STS mask */
#define SDC_STS_SDCBUSY		(0x1  << 0)	/* RW */
#define SDC_STS_CMDBUSY		(0x1  << 1)	/* RW */
#define SDC_STS_SWR_COMPL	(0x1  << 31)	/* RW */

/* SDC_DCRC_STS mask */
#define SDC_DCRC_STS_NEG	(0xff << 8)	/* RO */
#define SDC_DCRC_STS_POS	(0xff << 0)	/* RO */

/* EMMC_CFG0 mask */
#define EMMC_CFG0_BOOTSTART	(0x1  << 0)	/* W */
#define EMMC_CFG0_BOOTSTOP	(0x1  << 1)	/* W */
#define EMMC_CFG0_BOOTMODE	(0x1  << 2)	/* RW */
#define EMMC_CFG0_BOOTACKDIS	(0x1  << 3)	/* RW */
#define EMMC_CFG0_BOOTWDLY	(0x7  << 12)	/* RW */
#define EMMC_CFG0_BOOTSUPP	(0x1  << 15)	/* RW */

/* EMMC_CFG1 mask */
#define EMMC_CFG1_BOOTDATTMC	(0xfffff << 0)	/* RW */
#define EMMC_CFG1_BOOTACKTMC	(0xfffUL << 20)	/* RW */

/* EMMC_STS mask */
#define EMMC_STS_BOOTCRCERR	(0x1  << 0)	/* W1C */
#define EMMC_STS_BOOTACKERR	(0x1  << 1)	/* W1C */
#define EMMC_STS_BOOTDATTMO	(0x1  << 2)	/* W1C */
#define EMMC_STS_BOOTACKTMO	(0x1  << 3)	/* W1C */
#define EMMC_STS_BOOTUPSTATE	(0x1  << 4)	/* R */
#define EMMC_STS_BOOTACKRCV	(0x1  << 5)	/* W1C */
#define EMMC_STS_BOOTDATRCV	(0x1  << 6)	/* R */

/* EMMC_IOCON mask */
#define EMMC_IOCON_BOOTRST	(0x1  << 0)	/* RW */

/* SDC_ACMD19_TRG mask */
#define SDC_ACMD19_TRG_TUNESEL	(0xf  << 0)	/* RW */

/* MSDC_DMA_CTRL mask */
#define MSDC_DMA_CTRL_START	(0x1  << 0)	/* W */
#define MSDC_DMA_CTRL_STOP	(0x1  << 1)	/* W */
#define MSDC_DMA_CTRL_RESUME	(0x1  << 2)	/* W */
#define MSDC_DMA_CTRL_MODE	(0x1  << 8)	/* RW */
#define MSDC_DMA_CTRL_LASTBUF	(0x1  << 10)	/* RW */
#define MSDC_DMA_CTRL_BRUSTSZ	(0x7  << 12)	/* RW */

/* MSDC_DMA_CFG mask */
#define MSDC_DMA_CFG_STS	(0x1  << 0)	/* R */
#define MSDC_DMA_CFG_DECSEN	(0x1  << 1)	/* RW */
#define MSDC_DMA_CFG_AHBHPROT2	(0x2  << 8)	/* RW */
#define MSDC_DMA_CFG_ACTIVEEN	(0x2  << 12)	/* RW */
#define MSDC_DMA_CFG_CS12B16B	(0x1  << 16)	/* RW */

/* MSDC_PATCH_BIT mask */
#define MSDC_PATCH_BIT_ODDSUPP		(0x1  <<  1)	/* RW */
#define MSDC_INT_DAT_LATCH_CK_SEL	(0x7  <<  7)
#define MSDC_CKGEN_MSDC_DLY_SEL		(0x1F << 10)
#define MSDC_PATCH_BIT_IODSSEL		(0x1  << 16)	/* RW */
#define MSDC_PATCH_BIT_IOINTSEL		(0x1  << 17)	/* RW */
#define MSDC_PATCH_BIT_BUSYDLY		(0xf  << 18)	/* RW */
#define MSDC_PATCH_BIT_WDOD		(0xf  << 22)	/* RW */
#define MSDC_PATCH_BIT_IDRTSEL		(0x1  << 26)	/* RW */
#define MSDC_PATCH_BIT_CMDFSEL		(0x1  << 27)	/* RW */
#define MSDC_PATCH_BIT_INTDLSEL		(0x1  << 28)	/* RW */
#define MSDC_PATCH_BIT_SPCPUSH		(0x1  << 29)	/* RW */
#define MSDC_PATCH_BIT_DECRCTMO		(0x1  << 30)	/* RW */

/* MSDC_PATCH_BIT1 mask */
#define MSDC_PATCH_BIT1_WRDAT_CRCS	(0x7 << 0)
#define MSDC_PATCH_BIT1_CMD_RSP		(0x7 << 3)
#define MSDC_PATCH_BIT1_GET_CRC_MARGIN	(0x01 << 7)	/* RW */

/* MSDC_PAD_CTL0 mask */
#define MSDC_PAD_CTL0_CLKDRVN	(0x7  << 0)	/* RW */
#define MSDC_PAD_CTL0_CLKDRVP	(0x7  << 4)	/* RW */
#define MSDC_PAD_CTL0_CLKSR	(0x1  << 8)	/* RW */
#define MSDC_PAD_CTL0_CLKPD	(0x1  << 16)	/* RW */
#define MSDC_PAD_CTL0_CLKPU	(0x1  << 17)	/* RW */
#define MSDC_PAD_CTL0_CLKSMT	(0x1  << 18)	/* RW */
#define MSDC_PAD_CTL0_CLKIES	(0x1  << 19)	/* RW */
#define MSDC_PAD_CTL0_CLKTDSEL	(0xf  << 20)	/* RW */
#define MSDC_PAD_CTL0_CLKRDSEL	(0xffUL << 24)	/* RW */

/* MSDC_PAD_CTL1 mask */
#define MSDC_PAD_CTL1_CMDDRVN	(0x7  << 0)	/* RW */
#define MSDC_PAD_CTL1_CMDDRVP	(0x7  << 4)	/* RW */
#define MSDC_PAD_CTL1_CMDSR	(0x1  << 8)	/* RW */
#define MSDC_PAD_CTL1_CMDPD	(0x1  << 16)	/* RW */
#define MSDC_PAD_CTL1_CMDPU	(0x1  << 17)	/* RW */
#define MSDC_PAD_CTL1_CMDSMT	(0x1  << 18)	/* RW */
#define MSDC_PAD_CTL1_CMDIES	(0x1  << 19)	/* RW */
#define MSDC_PAD_CTL1_CMDTDSEL	(0xf  << 20)	/* RW */
#define MSDC_PAD_CTL1_CMDRDSEL	(0xffUL << 24)	/* RW */

/* MSDC_PAD_CTL2 mask */
#define MSDC_PAD_CTL2_DATDRVN	(0x7  << 0)	/* RW */
#define MSDC_PAD_CTL2_DATDRVP	(0x7  << 4)	/* RW */
#define MSDC_PAD_CTL2_DATSR	(0x1  << 8)	/* RW */
#define MSDC_PAD_CTL2_DATPD	(0x1  << 16)	/* RW */
#define MSDC_PAD_CTL2_DATPU	(0x1  << 17)	/* RW */
#define MSDC_PAD_CTL2_DATIES	(0x1  << 19)	/* RW */
#define MSDC_PAD_CTL2_DATSMT	(0x1  << 18)	/* RW */
#define MSDC_PAD_CTL2_DATTDSEL	(0xf  << 20)	/* RW */
#define MSDC_PAD_CTL2_DATRDSEL	(0xffUL << 24)	/* RW */

/* MSDC_PAD_TUNE mask */
#define MSDC_PAD_TUNE_DATWRDLY	(0x1F << 0)	/* RW */
#define MSDC_PAD_TUNE_DATRRDLY	(0x1F << 8)	/* RW */
#define MSDC_PAD_TUNE_CMDRDLY	(0x1F << 16)	/* RW */
#define MSDC_PAD_TUNE_CMDRRDLY	(0x1FUL << 22)	/* RW */
#define MSDC_PAD_TUNE_CLKTXDLY	(0x1FUL << 27)	/* RW */

/* MSDC_DAT_RDDLY0/1 mask */
#define MSDC_DAT_RDDLY0_D3	(0x1F << 0)	/* RW */
#define MSDC_DAT_RDDLY0_D2	(0x1F << 8)	/* RW */
#define MSDC_DAT_RDDLY0_D1	(0x1F << 16)	/* RW */
#define MSDC_DAT_RDDLY0_D0	(0x1FUL << 24)	/* RW */

#define MSDC_DAT_RDDLY1_D7	(0x1F << 0)	/* RW */
#define MSDC_DAT_RDDLY1_D6	(0x1F << 8)	/* RW */
#define MSDC_DAT_RDDLY1_D5	(0x1F << 16)	/* RW */
#define MSDC_DAT_RDDLY1_D4	(0x1FUL << 24)	/* RW */

static inline unsigned int uffs(unsigned int x)
{
	unsigned int r = 1;

	if (!x)
		return 0;
	if (!(x & 0xffff)) {
		x >>= 16;
		r += 16;
	}
	if (!(x & 0xff)) {
		x >>= 8;
		r += 8;
	}
	if (!(x & 0xf)) {
		x >>= 4;
		r += 4;
	}
	if (!(x & 3)) {
		x >>= 2;
		r += 2;
	}
	if (!(x & 1)) {
		x >>= 1;
		r += 1;
	}
	return r;
}

#define sdr_read8(reg)		readb(reg)
#define sdr_read16(reg)		readw(reg)
#define sdr_read32(reg)		readl(reg)
#define sdr_write8(reg, val)	writeb(val, reg)
#define sdr_write16(reg, val)	writew(val, reg)
#define sdr_write32(reg, val)	writel(val, reg)
#define sdr_set_bits(reg, bs) \
	do { \
		u32 val = sdr_read32(reg); \
		val |= bs; \
		sdr_write32(reg, val); \
	} while (0)
#define sdr_clr_bits(reg, bs) \
	do { \
		u32 val = sdr_read32(reg); \
		val &= ~((u32)(bs)); \
		sdr_write32(reg, val); \
	} while (0)
#define sdr_set_field(reg, field, val) \
	do { \
		unsigned int tv = sdr_read32(reg); \
		tv &= ~(field); \
		tv |= ((val) << (uffs((unsigned int)field) - 1)); \
		sdr_write32(reg, tv); \
	} while (0)
#define sdr_get_field(reg, field, val) \
	do { \
		unsigned int tv = sdr_read32(reg); \
		val = ((tv & (field)) >> (uffs((unsigned int)field) - 1)); \
	} while (0)
#define sdr_set_field_discrete(reg, field, val) \
	do { \
		unsigned int tv = sdr_read32(reg); \
		tv = (val == 1) ? (tv|(field)) : (tv & ~(field));\
		sdr_write32(reg, tv); \
	} while (0)
#define sdr_get_field_discrete(reg, field, val) \
	do { \
		unsigned int tv = sdr_read32(reg); \
		val = tv & (field); \
		val = (val == field) ? 1 : 0;\
	} while (0)

#define msdc_retry(expr, retry, cnt, id) \
	do { \
		int backup = cnt; \
		while (retry) { \
			if (!(expr)) \
				break; \
			if (cnt-- == 0) { \
				retry--; \
				mdelay(1); \
				cnt = backup; \
			} \
		} \
	} while (0)

#define MSDC_BUS_1BITS		(0)
#define MSDC_BUS_4BITS		(1)
#define MSDC_BUS_8BITS		(2)

#define DEFAULT_DTOC		(3)
#define MSDC_SDMMC		(1)
typedef struct {
	MmcCtrlr mmc;
	MtkMmcReg *reg;
	uint32_t clock;	/* Current clock (MHz) */
	uint32_t src_hz; /* Source clock (hz) */
	int initialized;
	int removable;
} MtkMmcHost;

MtkMmcHost *new_mtk_mmc_host(uintptr_t ioaddr, int bus_width, int removable);
#endif /* __DRIVERS_STORAGE_MTK_MMC_H_ */
