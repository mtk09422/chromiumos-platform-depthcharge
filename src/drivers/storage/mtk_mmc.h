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

enum {
	/* MSDC_CFG mask */
	MSDC_CFG_MODE		=(0x1  << 0),	/* RW */
	MSDC_CFG_CKPDN		=(0x1  << 1),	/* RW */
	MSDC_CFG_RST		=(0x1  << 2),	/* RW */
	MSDC_CFG_PIO		=(0x1  << 3),	/* RW */
	MSDC_CFG_CKDRVEN	=(0x1  << 4),	/* RW */
	MSDC_CFG_BV18SDT	=(0x1  << 5),	/* RW */
	MSDC_CFG_BV18PSS	=(0x1  << 6),	/* R  */
	MSDC_CFG_CKSTB		=(0x1  << 7),	/* R  */
	MSDC_CFG_CKDIV		=(0xff << 8),	/* RW */
	MSDC_CFG_CKMOD		=(0x3  << 16),	/* RW */
};

enum {
	/* MSDC_IOCON mask */
	MSDC_IOCON_SDR104CKS	=(0x1  << 0),	/* RW */
	MSDC_IOCON_RSPL		=(0x1  << 1),	/* RW */
	MSDC_IOCON_DSPL		=(0x1  << 2),	/* RW */
	MSDC_IOCON_DDLSEL	=(0x1  << 3),	/* RW */
	MSDC_IOCON_DDR50CKD	=(0x1  << 4),	/* RW */
	MSDC_IOCON_DSPLSEL	=(0x1  << 5),	/* RW */
	MSDC_IOCON_W_DSPL	=(0x1  << 8),	/* RW */
	MSDC_IOCON_D0SPL	=(0x1  << 16),	/* RW */
	MSDC_IOCON_D1SPL	=(0x1  << 17),	/* RW */
	MSDC_IOCON_D2SPL	=(0x1  << 18),	/* RW */
	MSDC_IOCON_D3SPL	=(0x1  << 19),	/* RW */
	MSDC_IOCON_D4SPL	=(0x1  << 20),	/* RW */
	MSDC_IOCON_D5SPL	=(0x1  << 21),	/* RW */
	MSDC_IOCON_D6SPL	=(0x1  << 22),	/* RW */
	MSDC_IOCON_D7SPL	=(0x1  << 23),	/* RW */
	MSDC_IOCON_RISCSZ	=(0x3  << 24),	/* RW */
};

enum {
	/* MSDC_PS mask */
	MSDC_PS_CDEN		=(0x1  << 0),	/* RW */
	MSDC_PS_CDSTS		=(0x1  << 1),	/* R  */
	MSDC_PS_CDDEBOUNCE	=(0xf  << 12),	/* RW */
	MSDC_PS_DAT		=(0xff << 16),	/* R  */
	MSDC_PS_CMD		=(0x1  << 24),	/* R  */
	MSDC_PS_WP		=(0x1UL << 31),	/* R  */
};

enum {
	/* MSDC_INT mask */
	MSDC_INT_MMCIRQ		=(0x1  << 0),	/* W1C */
	MSDC_INT_CDSC		=(0x1  << 1),	/* W1C */
	MSDC_INT_ACMDRDY	=(0x1  << 3),	/* W1C */
	MSDC_INT_ACMDTMO	=(0x1  << 4),	/* W1C */
	MSDC_INT_ACMDCRCERR	=(0x1  << 5),	/* W1C */
	MSDC_INT_DMAQ_EMPTY	=(0x1  << 6),	/* W1C */
	MSDC_INT_SDIOIRQ	=(0x1  << 7),	/* W1C */
	MSDC_INT_CMDRDY		=(0x1  << 8),	/* W1C */
	MSDC_INT_CMDTMO		=(0x1  << 9),	/* W1C */
	MSDC_INT_RSPCRCERR	=(0x1  << 10),	/* W1C */
	MSDC_INT_CSTA		=(0x1  << 11),	/* R */
	MSDC_INT_XFER_COMPL	=(0x1  << 12),	/* W1C */
	MSDC_INT_DXFER_DONE	=(0x1  << 13),	/* W1C */
	MSDC_INT_DATTMO		=(0x1  << 14),	/* W1C */
	MSDC_INT_DATCRCERR	=(0x1  << 15),	/* W1C */
	MSDC_INT_ACMD19_DONE	=(0x1  << 16),	/* W1C */
	MSDC_INT_DMA_BDCSERR	=(0x1  << 17),	/* W1C */
	MSDC_INT_DMA_GPDCSERR	=(0x1  << 18),	/* W1C */
	MSDC_INT_DMA_PROTECT	=(0x1  << 19),	/* W1C */
};

enum {
	/* MSDC_INTEN mask */
	MSDC_INTEN_MMCIRQ	=(0x1  << 0),	/* RW */
	MSDC_INTEN_CDSC		=(0x1  << 1),	/* RW */
	MSDC_INTEN_ACMDRDY	=(0x1  << 3),	/* RW */
	MSDC_INTEN_ACMDTMO	=(0x1  << 4),	/* RW */
	MSDC_INTEN_ACMDCRCERR	=(0x1  << 5),	/* RW */
	MSDC_INTEN_DMAQ_EMPTY	=(0x1  << 6),	/* RW */
	MSDC_INTEN_SDIOIRQ	=(0x1  << 7),	/* RW */
	MSDC_INTEN_CMDRDY	=(0x1  << 8),	/* RW */
	MSDC_INTEN_CMDTMO	=(0x1  << 9),	/* RW */
	MSDC_INTEN_RSPCRCERR	=(0x1  << 10),	/* RW */
	MSDC_INTEN_CSTA		=(0x1  << 11),	/* RW */
	MSDC_INTEN_XFER_COMPL	=(0x1  << 12),	/* RW */
	MSDC_INTEN_DXFER_DONE	=(0x1  << 13),	/* RW */
	MSDC_INTEN_DATTMO	=(0x1  << 14),	/* RW */
	MSDC_INTEN_DATCRCERR	=(0x1  << 15),	/* RW */
	MSDC_INTEN_ACMD19_DONE	=(0x1  << 16),	/* RW */
	MSDC_INTEN_DMA_BDCSERR	=(0x1  << 17),	/* RW */
	MSDC_INTEN_DMA_GPDCSERR	=(0x1  << 18),	/* RW */
	MSDC_INTEN_DMA_PROTECT	=(0x1  << 19),	/* RW */
};

enum {
	/* MSDC_FIFOCS mask */
	MSDC_FIFOCS_RXCNT	=(0xff << 0),	/* R */
	MSDC_FIFOCS_TXCNT	=(0xff << 16),	/* R */
	MSDC_FIFOCS_CLR		=(0x1UL << 31),	/* RW */
};

enum {
	/* SDC_CFG mask */
	SDC_CFG_SDIOINTWKUP	=(0x1  << 0),	/* RW */
	SDC_CFG_INSWKUP		=(0x1  << 1),	/* RW */
	SDC_CFG_BUSWIDTH	=(0x3  << 16),	/* RW */
	SDC_CFG_SDIO		=(0x1  << 19),	/* RW */
	SDC_CFG_SDIOIDE		=(0x1  << 20),	/* RW */
	SDC_CFG_INTATGAP	=(0x1  << 21),	/* RW */
	SDC_CFG_DTOC		=(0xffUL << 24),	/* RW */
};

enum {
	/* SDC_CMD mask */
	SDC_CMD_OPC		=(0x3f << 0),	/* RW */
	SDC_CMD_BRK		=(0x1  << 6),	/* RW */
	SDC_CMD_RSPTYP		=(0x7  << 7),	/* RW */
	SDC_CMD_DTYP		=(0x3  << 11),	/* RW */
	SDC_CMD_RW		=(0x1  << 13),	/* RW */
	SDC_CMD_STOP		=(0x1  << 14),	/* RW */
	SDC_CMD_GOIRQ		=(0x1  << 15),	/* RW */
	SDC_CMD_BLKLEN		=(0xfff << 16),	/* RW */
	SDC_CMD_AUTOCMD		=(0x3  << 28),	/* RW */
	SDC_CMD_VOLSWTH		=(0x1  << 30),	/* RW */
};

enum {
	/* SDC_STS mask */
	SDC_STS_SDCBUSY		=(0x1  << 0),	/* RW */
	SDC_STS_CMDBUSY		=(0x1  << 1),	/* RW */
	SDC_STS_SWR_COMPL	=(0x1  << 31),	/* RW */
};

enum {
	/* SDC_DCRC_STS mask */
	SDC_DCRC_STS_NEG	=(0xff << 8),	/* RO */
	SDC_DCRC_STS_POS	=(0xff << 0),	/* RO */
};

enum {
	/* EMMC_CFG0 mask */
	EMMC_CFG0_BOOTSTART	=(0x1  << 0),	/* W */
	EMMC_CFG0_BOOTSTOP	=(0x1  << 1),	/* W */
	EMMC_CFG0_BOOTMODE	=(0x1  << 2),	/* RW */
	EMMC_CFG0_BOOTACKDIS	=(0x1  << 3),	/* RW */
	EMMC_CFG0_BOOTWDLY	=(0x7  << 12),	/* RW */
	EMMC_CFG0_BOOTSUPP	=(0x1  << 15),	/* RW */
};

enum {
	/* EMMC_CFG1 mask */
	EMMC_CFG1_BOOTDATTMC	=(0xfffff << 0),	/* RW */
	EMMC_CFG1_BOOTACKTMC	=(0xfffUL << 20),	/* RW */
};

enum {
	/* EMMC_STS mask */
	EMMC_STS_BOOTCRCERR	=(0x1  << 0),	/* W1C */
	EMMC_STS_BOOTACKERR	=(0x1  << 1),	/* W1C */
	EMMC_STS_BOOTDATTMO	=(0x1  << 2),	/* W1C */
	EMMC_STS_BOOTACKTMO	=(0x1  << 3),	/* W1C */
	EMMC_STS_BOOTUPSTATE	=(0x1  << 4),	/* R */
	EMMC_STS_BOOTACKRCV	=(0x1  << 5),	/* W1C */
	EMMC_STS_BOOTDATRCV	=(0x1  << 6),	/* R */
};

enum {
	/* EMMC_IOCON mask */
	EMMC_IOCON_BOOTRST	=(0x1  << 0),	/* RW */
};

enum {
	/* SDC_ACMD19_TRG mask */
	SDC_ACMD19_TRG_TUNESEL	=(0xf  << 0),	/* RW */
};

enum {
	/* MSDC_DMA_CTRL mask */
	MSDC_DMA_CTRL_START	=(0x1  << 0),	/* W */
	MSDC_DMA_CTRL_STOP	=(0x1  << 1),	/* W */
	MSDC_DMA_CTRL_RESUME	=(0x1  << 2),	/* W */
	MSDC_DMA_CTRL_MODE	=(0x1  << 8),	/* RW */
	MSDC_DMA_CTRL_LASTBUF	=(0x1  << 10),	/* RW */
	MSDC_DMA_CTRL_BRUSTSZ	=(0x7  << 12),	/* RW */
};

enum {
	/* MSDC_DMA_CFG mask */
	MSDC_DMA_CFG_STS	=(0x1  << 0),	/* R */
	MSDC_DMA_CFG_DECSEN	=(0x1  << 1),	/* RW */
	MSDC_DMA_CFG_AHBHPROT2	=(0x2  << 8),	/* RW */
	MSDC_DMA_CFG_ACTIVEEN	=(0x2  << 12),	/* RW */
	MSDC_DMA_CFG_CS12B16B	=(0x1  << 16),	/* RW */
};

enum {
	/* MSDC_PATCH_BIT mask */
	MSDC_PATCH_BIT_ODDSUPP		=(0x1  <<  1),	/* RW */
	MSDC_INT_DAT_LATCH_CK_SEL	=(0x7  <<  7),
	MSDC_CKGEN_MSDC_DLY_SEL		=(0x1F << 10),
	MSDC_PATCH_BIT_IODSSEL		=(0x1  << 16),	/* RW */
	MSDC_PATCH_BIT_IOINTSEL		=(0x1  << 17),	/* RW */
	MSDC_PATCH_BIT_BUSYDLY		=(0xf  << 18),	/* RW */
	MSDC_PATCH_BIT_WDOD		=(0xf  << 22),	/* RW */
	MSDC_PATCH_BIT_IDRTSEL		=(0x1  << 26),	/* RW */
	MSDC_PATCH_BIT_CMDFSEL		=(0x1  << 27),	/* RW */
	MSDC_PATCH_BIT_INTDLSEL		=(0x1  << 28),	/* RW */
	MSDC_PATCH_BIT_SPCPUSH		=(0x1  << 29),	/* RW */
	MSDC_PATCH_BIT_DECRCTMO		=(0x1  << 30),	/* RW */
};

enum {
	/* MSDC_PATCH_BIT1 mask */
	MSDC_PATCH_BIT1_WRDAT_CRCS	=(0x7 << 0),
	MSDC_PATCH_BIT1_CMD_RSP		=(0x7 << 3),
	MSDC_PATCH_BIT1_GET_CRC_MARGIN	=(0x01 << 7),	/* RW */
};

enum {
	/* MSDC_PAD_CTL0 mask */
	MSDC_PAD_CTL0_CLKDRVN	=(0x7  << 0),	/* RW */
	MSDC_PAD_CTL0_CLKDRVP	=(0x7  << 4),	/* RW */
	MSDC_PAD_CTL0_CLKSR	=(0x1  << 8),	/* RW */
	MSDC_PAD_CTL0_CLKPD	=(0x1  << 16),	/* RW */
	MSDC_PAD_CTL0_CLKPU	=(0x1  << 17),	/* RW */
	MSDC_PAD_CTL0_CLKSMT	=(0x1  << 18),	/* RW */
	MSDC_PAD_CTL0_CLKIES	=(0x1  << 19),	/* RW */
	MSDC_PAD_CTL0_CLKTDSEL	=(0xf  << 20),	/* RW */
	MSDC_PAD_CTL0_CLKRDSEL	=(0xffUL << 24),	/* RW */
};

enum {
	/* MSDC_PAD_CTL1 mask */
	MSDC_PAD_CTL1_CMDDRVN	=(0x7  << 0),	/* RW */
	MSDC_PAD_CTL1_CMDDRVP	=(0x7  << 4),	/* RW */
	MSDC_PAD_CTL1_CMDSR	=(0x1  << 8),	/* RW */
	MSDC_PAD_CTL1_CMDPD	=(0x1  << 16),	/* RW */
	MSDC_PAD_CTL1_CMDPU	=(0x1  << 17),	/* RW */
	MSDC_PAD_CTL1_CMDSMT	=(0x1  << 18),	/* RW */
	MSDC_PAD_CTL1_CMDIES	=(0x1  << 19),	/* RW */
	MSDC_PAD_CTL1_CMDTDSEL	=(0xf  << 20),	/* RW */
	MSDC_PAD_CTL1_CMDRDSEL	=(0xffUL << 24),	/* RW */
};

enum {
	/* MSDC_PAD_CTL2 mask */
	MSDC_PAD_CTL2_DATDRVN	=(0x7  << 0),	/* RW */
	MSDC_PAD_CTL2_DATDRVP	=(0x7  << 4),	/* RW */
	MSDC_PAD_CTL2_DATSR	=(0x1  << 8),	/* RW */
	MSDC_PAD_CTL2_DATPD	=(0x1  << 16),	/* RW */
	MSDC_PAD_CTL2_DATPU	=(0x1  << 17),	/* RW */
	MSDC_PAD_CTL2_DATIES	=(0x1  << 19),	/* RW */
	MSDC_PAD_CTL2_DATSMT	=(0x1  << 18),	/* RW */
	MSDC_PAD_CTL2_DATTDSEL	=(0xf  << 20),	/* RW */
	MSDC_PAD_CTL2_DATRDSEL	=(0xffUL << 24),	/* RW */
};

enum {
	/* MSDC_PAD_TUNE mask */
	MSDC_PAD_TUNE_DATWRDLY	=(0x1F << 0),	/* RW */
	MSDC_PAD_TUNE_DATRRDLY	=(0x1F << 8),	/* RW */
	MSDC_PAD_TUNE_CMDRDLY	=(0x1F << 16),	/* RW */
	MSDC_PAD_TUNE_CMDRRDLY	=(0x1FUL << 22),	/* RW */
	MSDC_PAD_TUNE_CLKTXDLY	=(0x1FUL << 27),	/* RW */
};

enum {
	/* MSDC_DAT_RDDLY0/1 mask */
	MSDC_DAT_RDDLY0_D3	=(0x1F << 0),	/* RW */
	MSDC_DAT_RDDLY0_D2	=(0x1F << 8),	/* RW */
	MSDC_DAT_RDDLY0_D1	=(0x1F << 16),	/* RW */
	MSDC_DAT_RDDLY0_D0	=(0x1FUL << 24),	/* RW */
};

enum {
	MSDC_DAT_RDDLY1_D7	=(0x1F << 0),	/* RW */
	MSDC_DAT_RDDLY1_D6	=(0x1F << 8),	/* RW */
	MSDC_DAT_RDDLY1_D5	=(0x1F << 16),	/* RW */
	MSDC_DAT_RDDLY1_D4	=(0x1FUL << 24),	/* RW */
};

enum {
	MSDC_BUS_1BITS		=0,
	MSDC_BUS_4BITS		=1,
	MSDC_BUS_8BITS		=2
};

enum {
	DEFAULT_DTOC		=3
};

enum {
	MSDC_MS			=0,
	MSDC_SDMMC		=1
};

enum {
	MTK_MMC_TIMEOUT_MS = 1000,
};

typedef struct {
	MmcCtrlr mmc;

	MtkMmcReg *reg;
	uint32_t clock;         /* Current clock (MHz) */
	uint32_t src_hz;        /* Source clock (hz) */

	GpioOps *cd_gpio;	/* Change Detect GPIO */

	int initialized;
	int removable;
} MtkMmcHost;

MtkMmcHost *new_mtk_mmc_host(uintptr_t ioaddr, int bus_width,
                             int removable, GpioOps *card_detect);
#endif // __DRIVERS_STORAGE_MTK_MMC_H_
