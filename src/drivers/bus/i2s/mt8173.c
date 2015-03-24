/*
 * Copyright 2015 Mediatek Inc.
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
#include "drivers/bus/i2s/mt8173.h"
#include "drivers/bus/i2s/i2s.h"

#ifdef MT8173_I2S_DEBUG
#define I2S_LOG(args...)	printf(args)

static void mtk_i2s_dump(MtkI2s *bus)
{
	Mt8173I2sRegs *regs = bus->regs;
	Mt8173ApMixedRegs *apmixed_regs = bus->apmixed_regs;
	Mt8173TopCkRegs *topck_regs = bus->topck_regs;

	I2S_LOG("AFE register:\n"
		"TOP_CON0 %x\nDAC_CON0 %x\nDAC_CON1 %x\nI2S_CON1 %x\nCONN1 %x\n"
		"CONN2 %x\nCONN_24BIT %x\nDL1_BASE %x\nDL1_CUR %x\nDL1_END %x\n"
		"APLL1_TUNER_CFG %x\nAPLL2_TUNER_CFG %x\nDCM_CFG %x\n"
		"CLK_CFG_4 %x\nCLK_CFG_6 %x\nCLK_CFG_7 %x\nCLK_DIV_0 %x\n"
		"CLK_DIV_1 %x\nCLK_DIV_2 %x\nCLK_DIV_3 %x\nAP_PLL_CON5 %x\n"
		"APLL1_CON0 %x\nAPLL1_CON1 %x\nAPLL1_CON2 %x\nAPLL1_P_CON0 %x\n"
		"APLL2_CON0 %x\nAPLL2_CON1 %x\nAPLL2_CON2 %x\nAPLL2_P_CON0 %x\n",
		readl(&regs->top_con0),
		readl(&regs->dac_con0),
		readl(&regs->dac_con1),
		readl(&regs->i2s_con1),
		readl(&regs->conn1),
		readl(&regs->conn2),
		readl(&regs->conn_24bit),
		readl(&regs->dl1_base),
		readl(&regs->dl1_cur),
		readl(&regs->dl1_end),
		readl(&regs->apll1_cfg),
		readl(&regs->apll2_cfg),
		readl(&topck_regs->dcm_cfg),
		readl(&topck_regs->clk_cfg4),
		readl(&topck_regs->clk_cfg6),
		readl(&topck_regs->clk_cfg7),
		readl(&topck_regs->clk_auddiv0),
		readl(&topck_regs->clk_auddiv1),
		readl(&topck_regs->clk_auddiv2),
		readl(&topck_regs->clk_auddiv3),
		readl(&apmixed_regs->ap_pll_con5),
		readl(&apmixed_regs->apll1_con0),
		readl(&apmixed_regs->apll1_con1),
		readl(&apmixed_regs->apll1_con2),
		readl(&apmixed_regs->apll1_pwr_con0),
		readl(&apmixed_regs->apll2_con0),
		readl(&apmixed_regs->apll2_con1),
		readl(&apmixed_regs->apll2_con2),
		readl(&apmixed_regs->apll2_pwr_con0));
}
#else
#define I2S_LOG(args...)
static void mtk_i2s_dump(MtkI2s *bus)	{}
#endif

static uint32_t mtk_i2s_rate(uint32_t rate)
{
	switch (rate) {
	case 8000: return 0;
	case 11025: return 1;
	case 12000: return 2;
	case 16000: return 4;
	case 22050: return 5;
	case 24000: return 6;
	case 32000: return 8;
	case 44100: return 9;
	case 48000: return 10;
	case 88000: return 11;
	case 96000: return 12;
	case 174000: return 13;
	case 192000: return 14;
	default:
		return 10;
	}
}

static int mtk_i2s_init(MtkI2s *bus)
{
	Mt8173I2sRegs *regs = bus->regs;

	/* set O3/O4 16bits */
	clrbits_le32(&regs->conn_24bit, (1 << 3) | (1 << 4));

	/* I05/I06 -> O03/O04 connection */
	setbits_le32(&regs->conn1, 1 << 21);
	setbits_le32(&regs->conn2, 1 << 6);

	return 0;
}

enum {
	BUFFER_PADDING_LENGTH = 2400,    /* 50ms@48KHz stereo */
};

static int mtk_i2s_send(I2sOps *me, uint32_t *data, unsigned int length)
{
	MtkI2s *bus = container_of(me, MtkI2s, ops);
	Mt8173I2sRegs *regs = bus->regs;
	Mt8173ApMixedRegs *apmixed_regs = bus->apmixed_regs;
	Mt8173TopCkRegs *topck_regs = bus->topck_regs;
	uint32_t *buffer;
	uintptr_t buf_size, buf_base, data_end;
	uint32_t val, mclk_div;
	uint32_t apll_clock = MTK_AFE_APLL2_CLK_FREQ;
	uint32_t rate = mtk_i2s_rate(bus->rate);
	int apll1 = 0;

	if (!bus->initialized) {
		if (mtk_i2s_init(bus))
			return -1;
		else
			bus->initialized = 1;
	}

	buf_size = (length + BUFFER_PADDING_LENGTH) * sizeof(uint32_t);
	buffer = xzalloc(buf_size + 16);
	/* TODO: 4GB mode */
	if ((uintptr_t)buffer >= 0x100000000) {
		printf("not support AFE buffer at 0x%p\n", buffer);
		return -1;
	}
	buf_base = (uintptr_t)buffer;
	buf_base = ((buf_base + 15) >> 4) << 4; /* make it 16 bytes align */
	data_end = buf_base + length * sizeof(uint32_t) - 1;

	I2S_LOG("len = %d buf = %p base = %lx end = %lx data = %lx\n",
		length, buffer, buf_base, buf_base + buf_size - 1, data_end);

	memcpy((void *)buf_base, data, length * sizeof(uint32_t));
	/* flush data to memory for dma read */
	dcache_clean_by_mva((void *)buf_base, length * sizeof(uint32_t));

	writel(buf_base, &regs->dl1_base);
	writel(buf_base + buf_size - 1, &regs->dl1_end);

	/* enable APLL tuner */
	clrbits_le32(&regs->top_con0, (1 << 18) | (1 << 19));
	val = AFE_APLL_TUNER_UPPER_BOUND |
	      AFE_APLL_TUNER_DIV |
	      AFE_APLL_TUNER_EN;
	writel(val | AFE_APLL_TUNER_PLL1_SEL, &regs->apll1_cfg);
	writel(val | AFE_APLL_TUNER_PLL2_SEL, &regs->apll2_cfg);
	setbits_le32(&apmixed_regs->ap_pll_con5, 0x3);

	if (bus->rate % 11025 == 0) {
		/* use APLL1 instead */
		apll1 = 1;
		apll_clock = MTK_AFE_APLL1_CLK_FREQ;
	}
	/* I2S1 clock */
	mclk_div = (apll_clock / 256 / bus->rate) - 1;
	if (apll1) {
		/* mclk */
		clrbits_le32(&topck_regs->clk_auddiv0, 1 << 5);
		clrsetbits_le32(&topck_regs->clk_auddiv1,
				0xff00, mclk_div << 8);
		/* bclk */
		clrsetbits_le32(&topck_regs->clk_auddiv0,
				0x0f000000, 7 << 24);
	} else {
		/* mclk */
		setbits_le32(&topck_regs->clk_auddiv0, 1 << 5);
		clrsetbits_le32(&topck_regs->clk_auddiv2,
				0xff00, mclk_div << 8);
		/* bclk */
		clrsetbits_le32(&topck_regs->clk_auddiv0,
				0xf0000000, 7 << 28);
	}

	/* set channel */
	if (bus->channels == 1)
		setbits_le32(&regs->dac_con1, 1 << 21);
	else
		clrbits_le32(&regs->dac_con1, 1 << 21);

	/* set rate */
	clrsetbits_le32(&regs->dac_con1, 0xf, rate);

	/* set I2S output */
	val = AFE_I2S_CON1_LOW_JITTER_CLOCK |
	      ((rate & AFE_I2S_RATE_MASK) << AFE_I2S_RATE_SHIFT) |
	      AFE_I2S_CON1_FORMAT_I2S |
	      AFE_I2S_CON1_ENABLE;
	writel(val, &regs->i2s_con1);

	/* enable AFE */
	setbits_le32(&regs->dac_con0, 1 << 0);

	/* enable DL1 */
	setbits_le32(&regs->dac_con0, 1 << 1);
	mtk_i2s_dump(bus);

	while (readl(&regs->dl1_cur) <= data_end)
		;  /* wait until HW read pointer pass the end of data */

	/* stop DL1 */
	clrbits_le32(&regs->dac_con0, 1 << 1);

	/* stop I2S */
	clrbits_le32(&regs->i2s_con1, 1 << 0);

	/* disable APLL tuner */
	setbits_le32(&regs->top_con0, (1 << 18) | (1 << 19));
	clrbits_le32(&regs->apll1_cfg, 1 << 0);
	clrbits_le32(&regs->apll2_cfg, 1 << 0);
	clrbits_le32(&apmixed_regs->ap_pll_con5, 0x3);

	/* turn off AFE */
	clrbits_le32(&regs->dac_con0, 1 << 0);

	free(buffer);
	return 0;
}

MtkI2s *new_mtk_i2s(uintptr_t base, uint32_t bits_per_sample, uint32_t channels,
		    int rate, uintptr_t apmixed_base, uintptr_t topck_base)
{
	MtkI2s *bus = xzalloc(sizeof(*bus));

	bus->ops.send = &mtk_i2s_send;
	bus->regs = (void *)base;
	bus->bits_per_sample = bits_per_sample;
	bus->channels = channels;
	bus->rate = rate;
	/* clks */
	bus->apmixed_regs = (void *)apmixed_base;
	bus->topck_regs = (void *)topck_base;
	return bus;
}
