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

#ifndef __DRIVERS_BUS_SPI_MT8173_H__
#define __DRIVERS_BUS_SPI_MT8173_H__

#include "drivers/bus/spi/spi.h"

/* basic config for spi-controller timing */
enum spi_cpol {
	SPI_CPOL_0,
	SPI_CPOL_1
};

enum spi_cpha {
	SPI_CPHA_0,
	SPI_CPHA_1
};

enum spi_mlsb {
	SPI_LSB,
	SPI_MSB
};

enum spi_endian {
	SPI_LENDIAN,
	SPI_BENDIAN
};

enum spi_fifo {
	FIFO_TX,
	FIFO_RX,
	FIFO_ALL
};

enum spi_transfer_mode {
	FIFO_TRANSFER,          /* RX_FIFO, TX_FIFO */
	DMA_TRANSFER,           /* RX_DMA, TX_DMA */
	OTHER1,                 /* RX_FIFO, TX_DMA */
	OTHER2                  /* RX_DMA, TX_FIFO */
};

enum spi_pause_mode {
	PAUSE_MODE_DISABLE,
	PAUSE_MODE_ENABLE
};
enum spi_finish_intr {
	FINISH_INTR_DIS,
	FINISH_INTR_EN
};

enum spi_deassert_mode {
	DEASSERT_DISABLE,
	DEASSERT_ENABLE
};

enum spi_tckdly {
	TICK_DLY0,
	TICK_DLY1,
	TICK_DLY2,
	TICK_DLY3
};

struct mt8173_spi_cfg {
	uint32_t setuptime;
	uint32_t holdtime;
	uint32_t high_time;
	uint32_t low_time;
	uint32_t cs_idletime;
	uint32_t packet_len;
	uint32_t packet_loop;
	enum spi_cpol cpol;
	enum spi_cpha cpha;
	enum spi_mlsb tx_mlsb;
	enum spi_mlsb rx_mlsb;
	enum spi_endian tx_endian;
	enum spi_endian rx_endian;
	enum spi_pause_mode pause;
	enum spi_finish_intr finish_intr;
	enum spi_deassert_mode deassert;
	enum spi_tckdly tckdly;
} mt8173_spi_cfg;

typedef struct {
	SpiOps ops;
	void *reg_addr;
	enum spi_transfer_mode com_mod;
} MT8173Spi;

MT8173Spi *new_mt8173_spi(uintptr_t reg_addr);
#endif /* __DRIVERS_BUS_SPI_MT8173_H__ */
