/*
 * Copyright 2015 MediaTek Inc.
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
#include "drivers/bus/spi/mt8173.h"

static int SPI_DEBUG;

enum {
	MT8173_FIFO_SIZE = 32,
	MT8173_PACKET_SIZE = 1024,
	MT8173_TXRX_TIMEOUT_US = 1000 * 1000,
	MT8173_ARBITRARY_VALUE = 0xdeaddead
};

/* SPI peripheral register map. */
typedef struct MT8173SpiRegs {
	uint32_t SPI_CFG0_REG;
	uint32_t SPI_CFG1_REG;
	uint32_t SPI_TX_SRC_REG;
	uint32_t SPI_RX_DST_REG;
	uint32_t SPI_TX_DATA_REG;
	uint32_t SPI_RX_DATA_REG;
	uint32_t SPI_CMD_REG;
	uint32_t SPI_STATUS0_REG;
	uint32_t SPI_STATUS1_REG;
	uint32_t SPI_PAD_MACRO_SEL_REG;
} MT8173SpiRegs;

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
	SPI_CMD_DE_ASSERT_OFFSET = 5,
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
	SPI_CMD_DE_ASSERT_MASK = 0x20,
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

enum {
	SPI_PAD0_MASK = 0x0,
	SPI_PAD1_MASK = 0x1,
	SPI_PAD2_MASK = 0x2,
	SPI_PAD3_MASK = 0x3,

	SPI_PAD_SEL_MASK = 0x3
};

static void mt8173_spi_reset(MT8173SpiRegs *regs)
{
	setbits_le32(&regs->SPI_CMD_REG, 1 << SPI_CMD_RST_OFFSET);
	clrbits_le32(&regs->SPI_CMD_REG, 1 << SPI_CMD_RST_OFFSET);
}

static void mt8173_spi_dump_data(const char *name, const uint8_t *data, int size)
{
	int i;

	printf("%s: 0x ", name);
	for (i = 0; i < size; i++)
		printf("%#x ", data[i]);
	printf("\n");
}

static int mt8173_spi_start(SpiOps *me)
{
	MT8173Spi *bus = container_of(me, MT8173Spi, ops);
	MT8173SpiRegs *regs = bus->reg_addr;

	bus->com_mod = DMA_TRANSFER;
	mt8173_spi_reset(regs);

	/* set pause mode */
	clrsetbits_le32(&regs->SPI_CMD_REG, SPI_CMD_PAUSE_EN_MASK,
	                1 << SPI_CMD_PAUSE_EN_OFFSET);
	return 0;
}

static int mt8173_spi_fifo_transfer_one(SpiOps *me, void *in, const void *out, uint32_t size)
{
	MT8173Spi *bus = container_of(me, MT8173Spi, ops);
	const uint32_t *outb = (const uint32_t *)out;
	uint8_t *inb = (uint8_t *)in;
	MT8173SpiRegs *regs = bus->reg_addr;
	uint32_t packet_len, packet_loop, reg_val, i, cnt, failed = 0;
	uint64_t start;

	if (!size)
		return 0;

	if (size > MT8173_FIFO_SIZE) {
		printf("Error: The FIFO_TRANSFER buffer size is %d bytes limited.\n",
			MT8173_FIFO_SIZE);
		return -1;
	}

	if (size < MT8173_PACKET_SIZE)
		packet_len = size;
	else
		packet_len = MT8173_PACKET_SIZE;

	if (size % packet_len) {
		packet_loop = size / packet_len + 1;
	} else
		packet_loop = size / packet_len;

	clrsetbits_le32(&regs->SPI_CFG1_REG, SPI_CFG1_PACKET_LENGTH_MASK |
			SPI_CFG1_PACKET_LOOP_MASK,
			((packet_len - 1) << SPI_CFG1_PACKET_LENGTH_OFFSET) |
			((packet_loop - 1) << SPI_CFG1_PACKET_LOOP_OFFSET));

	/* Disable the tx&rx DMA */
	clrbits_le32(&regs->SPI_CMD_REG, SPI_CMD_TX_DMA_MASK | SPI_CMD_RX_DMA_MASK);

	if (!failed && inb) {
		for (i = 0; i < size; i++) {
			if (i % 4 == 0) {
				writel(MT8173_ARBITRARY_VALUE, &regs->SPI_TX_DATA_REG);
			}
		}

		setbits_le32(&regs->SPI_CMD_REG, 1 << SPI_CMD_RESUME_OFFSET);

		start = timer_us(0);
		while ((readl(&regs->SPI_STATUS0_REG) & 0x3) == 0) {
			if (timer_us(start) > MT8173_TXRX_TIMEOUT_US) {
				printf("Timeout waiting for inb status.\n");
				failed = 1;
				break;
			}
		}

		for (i = 0; !failed && i < size; i++) {
			if (i % 4 == 0) {
				reg_val = readl(&regs->SPI_RX_DATA_REG);
			}

			*((uint8_t *)(inb + i)) = (reg_val >> ((i % 4) * 8)) & 0xff;
		}
	}

	if (!failed && outb) {
		cnt = (size % 4) ? (size / 4 + 1) : (size / 4);
		for (i = 0; i < cnt; i++) {
			writel(*((u32 *)outb + i), &regs->SPI_TX_DATA_REG);
		}

		setbits_le32(&regs->SPI_CMD_REG, 1 << SPI_CMD_ACT_OFFSET);

		start = timer_us(0);
		while ((readl(&regs->SPI_STATUS1_REG) & 0x1) == 0) {
			if (timer_us(start) > MT8173_TXRX_TIMEOUT_US) {
				printf("Timeout waiting for outb status.\n");
				failed = 1;
				break;
			}
		}

		readl(&regs->SPI_STATUS0_REG);
	}

	if (failed) {
		mt8173_spi_reset(regs);
		return -1;
	} else
		return 0;
}

static int mt8173_spi_fifo_transfer(SpiOps *me, void *in, const void *out, uint32_t size)
{
	uint32_t size_loop, size_length;
	uint8_t *inb = (uint8_t *)in;
	const uint8_t *outb = (const uint8_t *)out;

	size_loop = size / MT8173_FIFO_SIZE;
	size_length = size % MT8173_FIFO_SIZE;

	while (size_loop--) {
		mt8173_spi_fifo_transfer_one(me, inb, outb, MT8173_FIFO_SIZE);

		if (inb)
			inb += MT8173_FIFO_SIZE;
		if (outb)
			outb += MT8173_FIFO_SIZE;
	}

	mt8173_spi_fifo_transfer_one(me, inb, outb, size_length);

	return 0;
}

static int mt8173_spi_dma_transfer(SpiOps *me, void *in, const void *out, uint32_t size)
{
	MT8173Spi *bus = container_of(me, MT8173Spi, ops);
	MT8173SpiRegs *regs = bus->reg_addr;
	uint32_t packet_len, packet_loop, reg_val;
	uint64_t start;
	uint8_t *inb = in;
	const uint8_t *outb = out;

	/* set transfer packet and loop */
	if (size < MT8173_PACKET_SIZE)
		packet_len = size;
	else
		packet_len = MT8173_PACKET_SIZE;

	if (size % packet_len) {
		packet_loop = size / packet_len + 1;
	} else {
		packet_loop = size / packet_len;
	}

	clrsetbits_le32(&regs->SPI_CFG1_REG, SPI_CFG1_PACKET_LENGTH_MASK |
			SPI_CFG1_PACKET_LOOP_MASK,
			((packet_len - 1) << SPI_CFG1_PACKET_LENGTH_OFFSET) |
			((packet_loop - 1) << SPI_CFG1_PACKET_LOOP_OFFSET));

	/* enable the RX/TX DMA */
	setbits_le32(&regs->SPI_CMD_REG, (1 << SPI_CMD_RX_DMA_OFFSET) |
			 (1 << SPI_CMD_TX_DMA_OFFSET));

	if (inb) {
		dcache_invalidate_by_mva(inb, size);
		writel((uint32_t)((uintptr_t)inb), &regs->SPI_RX_DST_REG);
		/* resume the transfer */
		setbits_le32(&regs->SPI_CMD_REG, 1 << SPI_CMD_RESUME_OFFSET);

		start = timer_us(0);
		do {
			reg_val = readl(&regs->SPI_STATUS0_REG) & 0x3;
			if (timer_us(start) > MT8173_TXRX_TIMEOUT_US) {
				return -1;
			}
		} while(reg_val == 0);

		if (SPI_DEBUG)
			mt8173_spi_dump_data("the inb data is", (const uint8_t *)inb, size);
	}
	if (outb) {
		dcache_clean_by_mva(outb, size);
		writel((uint32_t)((uintptr_t)outb), &regs->SPI_TX_SRC_REG);

		if (SPI_DEBUG)
			mt8173_spi_dump_data("the outb data is", (const uint8_t *)outb, size);

		/* start the transfer */
		setbits_le32(&regs->SPI_CMD_REG, 1 << SPI_CMD_ACT_OFFSET);
		start = timer_us(0);
		do {
			reg_val = readl(&regs->SPI_STATUS1_REG) & 0x1;
			if (timer_us(start) > MT8173_TXRX_TIMEOUT_US) {
				return -1;
			}
		} while(reg_val == 0);

		readl(&regs->SPI_STATUS0_REG);
	}

	return 0;
}

static int mt8173_spi_transfer(SpiOps *me, void *in, const void *out, uint32_t size)
{
	MT8173Spi *bus = container_of(me, MT8173Spi, ops);

	if (bus->com_mod == FIFO_TRANSFER)
		mt8173_spi_fifo_transfer(me, in, out, size);
	else
		mt8173_spi_dma_transfer(me, in, out, size);

	return 0;
}

static int mt8173_spi_stop(SpiOps *me)
{
	MT8173Spi *bus = container_of(me, MT8173Spi, ops);
	MT8173SpiRegs *regs = bus->reg_addr;

	/* stop the transfer of the spi controller */
	mt8173_spi_reset(regs);

	/* clear pause mode */
	clrbits_le32(&regs->SPI_CMD_REG, 1 << SPI_CMD_PAUSE_EN_OFFSET);

	return 0;
}

MT8173Spi *new_mt8173_spi(uintptr_t reg_addr)
{
	MT8173Spi *bus = xzalloc(sizeof(*bus));

	bus->ops.start = &mt8173_spi_start;
	bus->ops.transfer = &mt8173_spi_transfer;
	bus->ops.stop = &mt8173_spi_stop;
	bus->reg_addr = (void *)reg_addr;
	return bus;
}
