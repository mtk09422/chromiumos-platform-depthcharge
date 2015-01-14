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

#include <libpayload.h>
#include <stdint.h>
#include <arch/virtual.h>

#include "base/container_of.h"
#include "drivers/bus/spi/mt8135.h"

#define SPI_DEBUG

enum {
	MT8135_PACKET_SIZE = 1024
};

/* SPI peripheral register map. */
typedef struct MT8135SpiRegs {
	uint32_t SPI_CFG0_REG;
	uint32_t SPI_CFG1_REG;
	uint32_t SPI_TX_SRC_REG;
	uint32_t SPI_RX_DST_REG;
	uint32_t SPI_TX_DATA_REG;
	uint32_t SPI_RX_DATA_REG;
	uint32_t SPI_CMD_REG;
	uint32_t SPI_STATUS0_REG;
	uint32_t SPI_STATUS1_REG;
} MT8135SpiRegs;

/* SPI_CFG0_REG */
enum {
	SPI_CFG0_SCK_HIGH_OFFSET = 0,
	SPI_CFG0_SCK_LOW_OFFSET = 8,
	SPI_CFG0_CS_HOLD_OFFSET = 16,
	SPI_CFG0_CS_SETUP_OFFSET = 24,

	SPI_CFG0_SCK_HIGH_MASK = 0xff,
	SPI_CFG0_SCK_LOW_MASK = 0xff00,
	SPI_CFG0_CS_HOLD_MASK = 0xff0000,
	SPI_CFG0_CS_SETUP_MASK = 0xff000000
};

/* SPI_CFG1_REG */
enum {
	SPI_CFG1_CS_IDLE_OFFSET = 0,
	SPI_CFG1_PACKET_LOOP_OFFSET = 8,
	SPI_CFG1_PACKET_LENGTH_OFFSET = 16,
	SPI_CFG1_GET_TICK_DLY_OFFSET = 30,

	SPI_CFG1_CS_IDLE_MASK = 0xff,
	SPI_CFG1_PACKET_LOOP_MASK = 0xff00,
	SPI_CFG1_PACKET_LENGTH_MASK = 0x3ff0000,
	SPI_CFG1_GET_TICK_DLY_MASK = 0xc0000000
};

enum {
	SPI_CMD_ACT_OFFSET = 0,
	SPI_CMD_RESUME_OFFSET = 1,
	SPI_CMD_RST_OFFSET = 2,
	SPI_CMD_PAUSE_EN_OFFSET = 4,
	SPI_CMD_DEASSERT_OFFSET = 5,
	SPI_CMD_CPHA_OFFSET = 8,
	SPI_CMD_CPOL_OFFSET = 9,
	SPI_CMD_RX_DMA_OFFSET = 10,
	SPI_CMD_TX_DMA_OFFSET = 11,
	SPI_CMD_TXMSBF_OFFSET = 12,
	SPI_CMD_RXMSBF_OFFSET = 13,
	SPI_CMD_RX_ENDIAN_OFFSET = 14,
	SPI_CMD_TX_ENDIAN_OFFSET = 15,
	SPI_CMD_FINISH_IE_OFFSET = 16,
	SPI_CMD_PAUSE_IE_OFFSET = 17,

	SPI_CMD_ACT_MASK = 0x1,
	SPI_CMD_RESUME_MASK = 0x2,
	SPI_CMD_RST_MASK = 0x4,
	SPI_CMD_PAUSE_EN_MASK = 0x10,
	SPI_CMD_DEASSERT_MASK = 0x20,
	SPI_CMD_CPHA_MASK = 0x100,
	SPI_CMD_CPOL_MASK = 0x200,
	SPI_CMD_RX_DMA_MASK = 0x400,
	SPI_CMD_TX_DMA_MASK = 0x800,
	SPI_CMD_TXMSBF_MASK = 0x1000,
	SPI_CMD_RXMSBF_MASK = 0x2000,
	SPI_CMD_RX_ENDIAN_MASK = 0x4000,
	SPI_CMD_TX_ENDIAN_MASK = 0x8000,
	SPI_CMD_FINISH_IE_MASK = 0x10000,
	SPI_CMD_PAUSE_IE_MASK = 0x20000
};

static void mt8135_spi_reset(MT8135SpiRegs *regs)
{
	setbits_le32(&regs->SPI_CMD_REG, 1 << SPI_CMD_RST_OFFSET);
	clrbits_le32(&regs->SPI_CMD_REG, 1 << SPI_CMD_RST_OFFSET);
}

static void mt8135_spi_setup(SpiOps *me)
{
	MT8135Spi *bus = container_of(me, MT8135Spi, ops);
	struct mt8135_spi_cfg *spi_config = bus->spi_cfg;
	MT8135SpiRegs *regs = bus->reg_addr;

	/* set the timing */
	clrsetbits_le32(&regs->SPI_CFG0_REG, SPI_CFG0_SCK_HIGH_MASK |
									SPI_CFG0_SCK_LOW_MASK |
									SPI_CFG0_CS_HOLD_MASK |
									SPI_CFG0_CS_SETUP_MASK,
									((spi_config->high_time-1) << SPI_CFG0_SCK_HIGH_OFFSET) |
									((spi_config->low_time-1) << SPI_CFG0_SCK_LOW_OFFSET) |
									((spi_config->holdtime-1) << SPI_CFG0_CS_HOLD_OFFSET) |
									((spi_config->setuptime-1) << SPI_CFG0_CS_SETUP_OFFSET));
	clrsetbits_le32(&regs->SPI_CFG1_REG, SPI_CFG1_CS_IDLE_MASK | SPI_CFG1_GET_TICK_DLY_MASK,
			((spi_config->cs_idletime-1) << SPI_CFG1_CS_IDLE_OFFSET) |
									((spi_config->tckdly) << SPI_CFG1_GET_TICK_DLY_OFFSET));

	/* set the mlsbx and mlsbtx */
	clrsetbits_le32(&regs->SPI_CMD_REG, (SPI_CMD_TX_ENDIAN_MASK | SPI_CMD_RX_ENDIAN_MASK) |
									(SPI_CMD_TXMSBF_MASK | SPI_CMD_RXMSBF_MASK) |
									(SPI_CMD_CPHA_MASK | SPI_CMD_CPOL_MASK),
									(spi_config->tx_mlsb << SPI_CMD_TXMSBF_OFFSET) |
									(spi_config->rx_mlsb << SPI_CMD_RXMSBF_OFFSET) |
									(spi_config->tx_endian << SPI_CMD_TX_ENDIAN_OFFSET) |
									(spi_config->rx_endian << SPI_CMD_RX_ENDIAN_OFFSET) |
									(spi_config->cpha << SPI_CMD_CPHA_OFFSET) |
									(spi_config->cpol << SPI_CMD_CPOL_OFFSET));

	/* set pause mode */
	clrsetbits_le32(&regs->SPI_CMD_REG, SPI_CMD_PAUSE_EN_MASK, (spi_config->pause << SPI_CMD_PAUSE_EN_OFFSET));

	/* set finish interrupt always enable */
	clrsetbits_le32(&regs->SPI_CMD_REG, SPI_CMD_FINISH_IE_MASK, 1 << SPI_CMD_FINISH_IE_OFFSET);

	/* set pause interrupt always enable */
	clrsetbits_le32(&regs->SPI_CMD_REG, SPI_CMD_PAUSE_IE_MASK, 1 << SPI_CMD_PAUSE_IE_OFFSET);

	/* set the communication of mode */
	clrbits_le32(&regs->SPI_CMD_REG, SPI_CMD_TX_DMA_MASK | SPI_CMD_RX_DMA_MASK);

	/* set deassert mode */
	clrsetbits_le32(&regs->SPI_CMD_REG, SPI_CMD_DEASSERT_MASK, spi_config->deassert << SPI_CMD_DEASSERT_OFFSET);
}

static void mt8135_spi_init(SpiOps *me)
{
	MT8135Spi *bus = container_of(me, MT8135Spi, ops);
	MT8135SpiRegs *regs = bus->reg_addr;
	/* spi controller init */
	mt8135_spi_reset(regs);
	mt8135_spi_setup(me);
}

static int mt8135_spi_start(SpiOps *me)
{
	MT8135Spi *bus = container_of(me, MT8135Spi, ops);

	if (!bus->initialized) {
		mt8135_spi_init(me);
		bus->initialized = 1;
	}

	if (bus->started) {
		printf("%s: Transaction already started.\n", __func__);
		return -1;
	}

	bus->started = 1;

	return 0;
}

static int mt8135_spi_transfer(SpiOps *me, void *in, const void *out, uint32_t size)
{
	MT8135Spi *bus = container_of(me, MT8135Spi, ops);
	MT8135SpiRegs *regs = bus->reg_addr;
	struct mt8135_spi_cfg *spi_config = bus->spi_cfg;
	uint32_t data_length, data_loop;
	uint8_t *inb = in;
	const uint8_t *outb = out;

	/* set transfer packet and loop */
	if (size < MT8135_PACKET_SIZE)
		data_length = size;
	else
		data_length = MT8135_PACKET_SIZE;

	if (size % data_length) {
		data_loop = size / data_length + 1;
		printf("ERR!The lens must be a multiple of %d, your len %u\n",
		       MT8135_PACKET_SIZE, size);
	} else {
		data_loop = size / data_length;
	}

	printf("The packet_len:0x%x packet_loop:0x%x\n",
	       data_length, data_loop);

	spi_config->packet_len = data_length;
	spi_config->packet_loop = data_loop;

	clrsetbits_le32(&regs->SPI_CFG1_REG, SPI_CFG1_PACKET_LENGTH_MASK |
							SPI_CFG1_PACKET_LOOP_MASK,
							((spi_config->packet_len - 1) << SPI_CFG1_PACKET_LENGTH_OFFSET) |
							((spi_config->packet_loop - 1) << SPI_CFG1_PACKET_LOOP_OFFSET));

	/* enable the RX/TX DMA */
	setbits_le32(&regs->SPI_CMD_REG, (1 << SPI_CMD_RX_DMA_OFFSET) | (1 << SPI_CMD_TX_DMA_OFFSET));

	if (inb) {
		/* resume the transfer */
		setbits_le32(&regs->SPI_CMD_REG, 1 << SPI_CMD_RESUME_OFFSET);
		writel((uint32_t)inb, &regs->SPI_RX_DST_REG);
#ifdef SPI_DEBUG
		printf("the received data is 0x");
		for (int i = 0; i < size; ++i)
			printf("%x", (uint8_t)*inb++);
		printf("\n");
#endif
	}
	if (outb) {
		writel((uint32_t)outb, &regs->SPI_TX_SRC_REG);
#ifdef SPI_DEBUG
	printf("the output data is 0x");
		for (int i = 0; i < size; ++i)
			printf("%x", (uint8_t)*outb++);
		printf("\n");
#endif
		/* start the transfer */
		setbits_le32(&regs->SPI_CMD_REG, 1 << SPI_CMD_ACT_OFFSET);
	}

	return 0;
}

static int mt8135_spi_stop(SpiOps *me)
{
	MT8135Spi *bus = container_of(me, MT8135Spi, ops);
	MT8135SpiRegs *regs = bus->reg_addr;

	if (!bus->started) {
		printf("%s: Bus not yet started.\n", __func__);
		return -1;
	}

	/* stop the transfer of the spi controller */
	mt8135_spi_reset(regs);

	bus->started = 0;

	return 0;
}

MT8135Spi *new_mt8135_spi(uintptr_t reg_addr,
			  struct mt8135_spi_cfg *mtspi_config)
{
	MT8135Spi *bus = xzalloc(sizeof(*bus));
	bus->ops.start = &mt8135_spi_start;
	bus->ops.transfer = &mt8135_spi_transfer;
	bus->ops.stop = &mt8135_spi_stop;
	bus->reg_addr = (void *)reg_addr;
	bus->spi_cfg = mtspi_config;
	return bus;
}
