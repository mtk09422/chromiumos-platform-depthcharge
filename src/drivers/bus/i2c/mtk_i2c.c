/*
 * Copyright (c) 2012, Code Aurora Forum. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *  * Neither the name of Google, Inc. nor the names of its contributors
 *    may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "base/container_of.h"
#include "drivers/bus/i2c/mtk_i2c.h"

uint32_t mtk_i2c_set_speed(uint8_t channel, uint32_t clock,
			       I2C_SPD_MODE mode, uint32_t khz)
{
	uint32_t ret_code = I2C_OK;
	uint32_t i2c_base = I2C0_BASE + channel * 0x1000;

	uint16_t sample_cnt_div;
	uint16_t step_cnt_div;
	uint16_t max_step_cnt_div = (mode == HS_MODE) ?
				MAX_HS_STEP_CNT_DIV : MAX_STEP_CNT_DIV;
	uint32_t tmp;
	uint32_t sclk;

	uint32_t diff;
	uint32_t min_diff = clock;
	uint16_t sample_div = MAX_SAMPLE_CNT_DIV;
	uint16_t step_div = max_step_cnt_div;
	for (sample_cnt_div = 1; sample_cnt_div <= MAX_SAMPLE_CNT_DIV;
		sample_cnt_div++) {
		for (step_cnt_div = 1; step_cnt_div <= max_step_cnt_div;
			step_cnt_div++) {
			sclk = (clock >> 1) /
				(sample_cnt_div * step_cnt_div);
			if (sclk > khz)
				continue;
			diff = khz - sclk;

			if (diff < min_diff) {
				min_diff = diff;
				sample_div = sample_cnt_div;
				step_div = step_cnt_div;
			}
		}
	}

	sample_cnt_div = sample_div;
	step_cnt_div = step_div;

	sclk = clock / (2 * sample_cnt_div * step_cnt_div);
	if (sclk > khz) {
		ret_code = I2C_SET_SPEED_FAIL_OVER_SPEED;
		return ret_code;
	}

	step_cnt_div--;
	sample_cnt_div--;

	if (mode == HS_MODE) {
		tmp = readw((volatile void *)(uintptr_t)(i2c_base + MTK_I2C_HS))
				& ((0x7 << 12) | (0x7 << 8));
		tmp = (sample_cnt_div & 0x7) << 12 | (step_cnt_div & 0x7) << 8 |
		      tmp;
		writew(tmp, (volatile void *)(uintptr_t)(i2c_base + MTK_I2C_HS));
		I2C_SET_HS_MODE(i2c_base, 1);
	} else {
		tmp = readw((volatile void *)(uintptr_t)(i2c_base + MTK_I2C_TIMING))
				& ~((0x7 << 8) | (0x3f << 0));
		tmp = (sample_cnt_div & 0x7) << 8 | (step_cnt_div & 0x3f) << 0
				| tmp;
		writew(tmp, (volatile void *)(uintptr_t)(i2c_base + MTK_I2C_TIMING));
		I2C_SET_HS_MODE(i2c_base, 0);
	}

	return ret_code;
}

static inline void mtk_i2c_dump_info(struct mtk_i2c_t *i2c)
{
	uint32_t i2c_base;
	i2c_base = I2C0_BASE + i2c->id * 0x1000;
	printf("I2C register:\nSLAVE_ADDR %x\nINTR_MASK %x\nINTR_STAT %x\n"
	       "CONTROL %x\nTRANSFER_LEN %x\nTRANSAC_LEN %x\nDELAY_LEN %x\n"
	       "TIMING %x\nSTART %x\nFIFO_STAT %x\nIO_CONFIG %x\nHS %x\n"
	       "DEBUGSTAT %x\nEXT_CONF %x\nPATH_DIR %x\n",
		(readw((volatile void *)(uintptr_t)(i2c_base + MTK_I2C_SLAVE_ADDR))),
		(readw((volatile void *)(uintptr_t)(i2c_base + MTK_I2C_INTR_MASK))),
		(readw((volatile void *)(uintptr_t)(i2c_base + MTK_I2C_INTR_STAT))),
		(readw((volatile void *)(uintptr_t)(i2c_base + MTK_I2C_CONTROL))),
		(readw((volatile void *)(uintptr_t)(i2c_base + MTK_I2C_TRANSFER_LEN))),
		(readw((volatile void *)(uintptr_t)(i2c_base + MTK_I2C_TRANSAC_LEN))),
		(readw((volatile void *)(uintptr_t)(i2c_base + MTK_I2C_DELAY_LEN))),
		(readw((volatile void *)(uintptr_t)(i2c_base + MTK_I2C_TIMING))),
		(readw((volatile void *)(uintptr_t)(i2c_base + MTK_I2C_START))),
		(readw((volatile void *)(uintptr_t)(i2c_base + MTK_I2C_FIFO_STAT))),
		(readw((volatile void *)(uintptr_t)(i2c_base + MTK_I2C_IO_CONFIG))),
		(readw((volatile void *)(uintptr_t)(i2c_base + MTK_I2C_HS))),
		(readw((volatile void *)(uintptr_t)(i2c_base + MTK_I2C_DEBUGSTAT))),
		(readw((volatile void *)(uintptr_t)(i2c_base + MTK_I2C_EXT_CONF))),
		(readw((volatile void *)(uintptr_t)(i2c_base + MTK_I2C_PATH_DIR))));

	/* 8135 side and PMIC side clock */
	printf("Clock %s\n", (((readw((volatile void *)0x10003018) >> 26) |
			     (readw((volatile void *)0x1000301c) & 0x1 << 6)) &
			     (1 << i2c->id)) ?  "disable" : "enable");
	printf("base address %x\n", (uint32_t)i2c_base);
	return;
}

uint32_t mtk_i2c_read(struct mtk_i2c_t *i2c, uint8_t *buffer,
			      uint32_t len)
{
	uint32_t ret_code = I2C_OK;
	uint32_t i2c_clk;
	uint8_t *ptr = buffer;
	uint16_t status;
	int ret = len;
	uint32_t time_out_val = 0;
	uint32_t count = 0;
	uint32_t temp_len = 0;
	uint32_t total_count = 0;
	uint8_t tpm_data_fifo_v = 0x5;
	uint32_t i2c_base;

	i2c_base = I2C0_BASE + i2c->id * 0x1000;

	if ((len == 0) || (len > 255)) {
		printf("[i2c_read] I2C doesn't support len = %d.\n", len);
		return I2C_READ_FAIL_ZERO_LENGTH;
	}

	if (i2c->dir == 0) {
		I2C_PATH_NORMAL(i2c_base);
		i2c_clk = I2C_CLK_RATE;
	} else {
		I2C_PATH_PMIC(i2c_base);
		i2c_clk = I2C_CLK_RATE_PMIC;
	}

	count = len / 8;
	if (len % 8 != 0)
		count += 1;

	if (count > 1)
		total_count = count;

	while (count) {
		/* For TPM. */
		if (count < total_count && count > 0 && i2c->id == 6)
			mtk_i2c_write(i2c, &tpm_data_fifo_v, 1);

		if (count == 1) {
			temp_len = len % 8;
			if (temp_len == 0)
				temp_len = 8;
			I2C_SET_TRANS_LEN(i2c_base, temp_len);
		}
		else{
			I2C_SET_TRANS_LEN(i2c_base, 8);
			temp_len = 8;
		}

		I2C_CLR_INTR_STATUS(i2c_base, I2C_TRANSAC_COMP | I2C_ACKERR
					| I2C_HS_NACKERR);
		/* setting speed */
		mtk_i2c_set_speed(i2c->id, i2c_clk, i2c->mode, i2c->speed);
		if (i2c->speed <= 100)
			writew(0x8001, (volatile void *)(uintptr_t)(i2c_base + MTK_I2C_EXT_CONF));

		/* control registers */
		I2C_SET_SLAVE_ADDR(i2c_base, ((i2c->addr<<1) | 0x1));

		I2C_SET_TRANSAC_LEN(i2c_base, 1);
		I2C_SET_INTR_MASK(i2c_base, I2C_HS_NACKERR | I2C_ACKERR
					| I2C_TRANSAC_COMP);
		I2C_FIFO_CLR_ADDR(i2c_base);
		I2C_SET_TRANS_CTRL(i2c_base, ACK_ERR_DET_EN
				| (i2c->is_clk_ext_disable ?  0 : CLK_EXT)
				| (i2c->is_rs_enable
				? REPEATED_START_FLAG : STOP_FLAG));

		/* start trnasfer transaction */
		I2C_START_TRANSAC(i2c_base);

		/* polling mode : see if transaction complete */
		while (1) {
			status = I2C_INTR_STATUS(i2c_base);

			if (status & I2C_TRANSAC_COMP &&
			    (!I2C_FIFO_IS_EMPTY(i2c_base))) {
				ret = 0;
				ret_code = I2C_OK;
				break;
			} else if (status & I2C_HS_NACKERR) {
				ret = 1;
				ret_code = I2C_READ_FAIL_HS_NACKERR;
				printf("[i2c%d read] transaction NACK error (%x)\n",
				       i2c->id, status);
				mtk_i2c_dump_info(i2c);
				break;
			} else if (status & I2C_ACKERR) {
				ret = 2;
				ret_code = I2C_READ_FAIL_ACKERR;
				printf("[i2c%d read] transaction ACK error (%x)\n",
				       i2c->id, status);
				mtk_i2c_dump_info(i2c);
				break;
			} else if (time_out_val > 100000) {
				ret = 3;
				ret_code = I2C_READ_FAIL_TIMEOUT;
				I2C_SOFTRESET(i2c_base);
				printf("[i2c%d read] transaction timeout:%d\n",
				       i2c->id, time_out_val);
				mtk_i2c_dump_info(i2c);
				break;
			}
			time_out_val++;
		}

		I2C_CLR_INTR_STATUS(i2c_base, I2C_TRANSAC_COMP | I2C_ACKERR
					| I2C_HS_NACKERR);

		if (!ret) {
			while (temp_len > 0) {
				I2C_READ_BYTE(i2c_base, ptr);
				ptr++;
				temp_len--;
			}
		}

		/* clear bit mask */
		I2C_CLR_INTR_MASK(i2c_base, I2C_HS_NACKERR | I2C_ACKERR
					| I2C_TRANSAC_COMP);

		I2C_SOFTRESET(i2c_base);
		count--;
	}

	return ret_code;
}

uint32_t mtk_i2c_write(struct mtk_i2c_t *i2c, uint8_t *buffer,
				uint32_t len)
{
	uint32_t ret_code = I2C_OK;
	uint32_t i2c_clk;
	uint8_t *ptr = buffer;
	uint16_t status;
	uint32_t time_out_val = 0;

	uint32_t count = 0;
	uint32_t temp_len = 0;
	uint32_t total_count = 0;
	uint8_t tpm_data_fifo_v = 0x5;
	uint32_t i2c_base;

	i2c_base = I2C0_BASE + i2c->id * 0x1000;

	/* CHECKME. mt65xx doesn't support len = 0. */
	if ((len == 0) || (len > 255)) {
		printf("[i2c_write] I2C doesn't support len = %d.\n", len);
		return I2C_WRITE_FAIL_ZERO_LENGTH;
	}

	/*setting path direction and clock first all */
	if (i2c->dir == 0) {
		I2C_PATH_NORMAL(i2c_base);
		i2c_clk = I2C_CLK_RATE;
	} else {
		I2C_PATH_PMIC(i2c_base);
		i2c_clk = I2C_CLK_RATE_PMIC;
	}

	count = len / 8;
	if (len % 8 != 0)
		count += 1;

	if (count > 1)
		total_count = count;

	while (count) {
		/* FOR TPM */
		if (count < total_count && count > 0 && i2c->id == 6)
			mtk_i2c_write(i2c, &tpm_data_fifo_v, 1);

		if (count == 1) {
			temp_len = len % 8;
			if (temp_len == 0)
				temp_len = 8;
			I2C_SET_TRANS_LEN(i2c_base, temp_len);
		}
		else{
			I2C_SET_TRANS_LEN(i2c_base, 8);
			temp_len = 8;
		}


		I2C_CLR_INTR_STATUS(i2c_base, I2C_TRANSAC_COMP | I2C_ACKERR
					| I2C_HS_NACKERR);
		/* setting speed */
		mtk_i2c_set_speed(i2c->id, i2c_clk, i2c->mode, i2c->speed);
		if (i2c->speed <= 100)
			writew(0x8001, (volatile void *)(uintptr_t)(i2c_base + MTK_I2C_EXT_CONF));

		/* control registers */
		I2C_SET_SLAVE_ADDR(i2c_base, i2c->addr<<1);

		I2C_SET_TRANSAC_LEN(i2c_base, 1);
		I2C_SET_INTR_MASK(i2c_base, I2C_HS_NACKERR | I2C_ACKERR
					| I2C_TRANSAC_COMP);

		I2C_FIFO_CLR_ADDR(i2c_base);
		I2C_SET_TRANS_CTRL(i2c_base, ACK_ERR_DET_EN |
				   (i2c->is_clk_ext_disable ? 0 : CLK_EXT) |
				   (i2c->is_rs_enable ?
					REPEATED_START_FLAG : STOP_FLAG));

		/* start to write data */
		while (temp_len > 0)
		{
			I2C_WRITE_BYTE(i2c_base, *ptr);
			ptr++;
			temp_len--;
		}

		/* start trnasfer transaction */
		I2C_START_TRANSAC(i2c_base);

		/* polling mode : see if transaction complete */
		while (1) {
			status = I2C_INTR_STATUS(i2c_base);
			if (status & I2C_HS_NACKERR) {
				ret_code = I2C_WRITE_FAIL_HS_NACKERR;
				printf("[i2c%d write] transaction NACK error\n",
				       i2c->id);
				mtk_i2c_dump_info(i2c);
				break;
			} else if (status & I2C_ACKERR) {
				ret_code = I2C_WRITE_FAIL_ACKERR;
				printf("[i2c%d write] transaction ACK error\n",
				       i2c->id);
				mtk_i2c_dump_info(i2c);
				break;
			} else if (status & I2C_TRANSAC_COMP) {
				ret_code = I2C_OK;
				break;
			} else if (time_out_val > 100000) {
				ret_code = I2C_WRITE_FAIL_TIMEOUT;
				printf("[i2c%d write] transaction timeout:%d\n",
				       i2c->id, time_out_val);
				mtk_i2c_dump_info(i2c);
			} else if (status & I2C_TRANSAC_COMP) {
				ret_code = I2C_OK;
				break;
			} else if (time_out_val > 100000) {
				ret_code = I2C_WRITE_FAIL_TIMEOUT;
				printf("[i2c%d write] transaction timeout:%d\n",
				       i2c->id, time_out_val);
				mtk_i2c_dump_info(i2c);
				break;
			}
			time_out_val++;
		}

		I2C_CLR_INTR_STATUS(i2c_base, I2C_TRANSAC_COMP | I2C_ACKERR
					| I2C_HS_NACKERR);

		/* clear bit mask */
		I2C_CLR_INTR_MASK(i2c_base, I2C_HS_NACKERR | I2C_ACKERR
					| I2C_TRANSAC_COMP);
		I2C_SOFTRESET(i2c_base);

		count--;
	}
	return ret_code;
}

uint32_t mtk_i2c_write_read(struct mtk_i2c_t *i2c, uint8_t *buffer,
				      uint32_t write_len, uint32_t read_len)
{
	uint32_t ret_code = I2C_OK;
	uint32_t i2c_clk;
	uint8_t *ptr = buffer;
	uint16_t status;
	uint32_t time_out_val = 0;
	uint32_t i2c_base;

	i2c_base = I2C0_BASE + i2c->id * 0x1000;

	/* CHECKME. mt65xx doesn't support len = 0. */
	if ((write_len == 0) || (read_len == 0) || (write_len > 255)
	    || (read_len > 255)) {
		printf("[i2c_write_read] I2C doesn't support w,r len = %d,%d.\n",
		       write_len, read_len);
		return I2C_WRITE_FAIL_ZERO_LENGTH;
	}
	if (i2c->dir == 0) {
		I2C_PATH_NORMAL(i2c_base);
		i2c_clk = I2C_CLK_RATE;
	} else {
		I2C_PATH_PMIC(i2c_base);
		i2c_clk = I2C_CLK_RATE_PMIC;
	}
	I2C_CLR_INTR_STATUS(i2c_base, I2C_TRANSAC_COMP | I2C_ACKERR
				| I2C_HS_NACKERR);
	/* setting speed */
	mtk_i2c_set_speed(i2c->id, i2c_clk, i2c->mode, i2c->speed);
	if (i2c->speed <= 100)
		writew(0x8001, (volatile void *)(uintptr_t)(i2c_base + MTK_I2C_EXT_CONF));

	/* control registers */
	I2C_SET_SLAVE_ADDR(i2c_base, i2c->addr << 1);
	I2C_SET_TRANS_LEN(i2c_base, write_len | read_len << 8);
	I2C_SET_TRANSAC_LEN(i2c_base, 1);
	I2C_SET_INTR_MASK(i2c_base, I2C_HS_NACKERR | I2C_ACKERR
				| I2C_TRANSAC_COMP);
	I2C_FIFO_CLR_ADDR(i2c_base);

	I2C_SET_TRANS_CTRL(i2c_base, ACK_ERR_DET_EN |
			   (i2c->is_clk_ext_disable ? 0 : CLK_EXT) |
			   (i2c->is_rs_enable ?
				REPEATED_START_FLAG : STOP_FLAG));

	/* start to write data */
	while (write_len--) {
		I2C_WRITE_BYTE(i2c_base, *ptr);
		ptr++;
	}

	/* start trnasfer transaction */
	I2C_START_TRANSAC(i2c_base);
	/* polling mode : see if transaction complete */
	while (1) {
		status = I2C_INTR_STATUS(i2c_base);
		if (status & I2C_HS_NACKERR) {
			ret_code = I2C_WRITE_FAIL_HS_NACKERR;
			printf("[i2c%d write_read] transaction NACK error\n",
			       i2c->id);
			mtk_i2c_dump_info(i2c);
			break;
		} else if (status & I2C_ACKERR) {
			ret_code = I2C_WRITE_FAIL_ACKERR;
			printf("[i2c%d write_read] transaction ACK error\n",
			       i2c->id);
			mtk_i2c_dump_info(i2c);
			break;
		} else if (status & I2C_TRANSAC_COMP) {
			ret_code = I2C_OK;
			break;
		} else if (time_out_val > 100000) {
			ret_code = I2C_WRITE_FAIL_TIMEOUT;
			printf("[i2c%d write_read] transaction timeout:%d\n",
			       i2c->id, time_out_val);
			mtk_i2c_dump_info(i2c);
			break;
		}
		time_out_val++;
	}

	if (ret_code == I2C_OK) {
		ptr = buffer;
		while (read_len--) {
			I2C_READ_BYTE(i2c_base, ptr);
			ptr++;
		}
	}

	I2C_CLR_INTR_STATUS(i2c_base, I2C_TRANSAC_COMP | I2C_ACKERR
				| I2C_HS_NACKERR);

	/* clear bit mask */
	I2C_CLR_INTR_MASK(i2c_base, I2C_HS_NACKERR | I2C_ACKERR
				| I2C_TRANSAC_COMP);
	I2C_SOFTRESET(i2c_base);

	return ret_code;
}

static int i2c_transfer(I2cOps *me, I2cSeg *segments, int seg_count)
{
	I2cSeg *seg = segments;
	MTKI2c *bus = container_of(me, MTKI2c, ops);
	struct mtk_i2c_t *i2c = bus->i2c;
	int ret = 0;

	for (int i = 0; i < seg_count; i++) {
		if (bus->wrflag == 1 && bus->readlen > 0) {
			ret = mtk_i2c_write_read(i2c, seg[i].buf, seg[i].len,
							bus->readlen);
		} else {
			if (seg[i].read == 1) {
				ret = mtk_i2c_read(i2c, seg[i].buf,
							seg[i].len);
			} else if (seg[i].read == 0) {
				ret = mtk_i2c_write(i2c, seg[i].buf,
							seg[i].len);
			}
		}

		if (ret)
			break;
	}

	return ret;
}

MTKI2c *new_mtk_i2c(struct mtk_i2c_t *i2c, uint32_t wrflag, uint32_t readlen)
{
	MTKI2c *bus = xzalloc(sizeof(*bus));
	bus->ops.transfer = &i2c_transfer;
	bus->i2c = i2c;
	bus->wrflag = wrflag;
	bus->readlen = readlen;
	return bus;
}
