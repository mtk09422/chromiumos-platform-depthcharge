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

#ifndef __MTK_I2C_H__
#define __MTK_I2C_H__

#include "drivers/bus/i2c/i2c.h"
#include <libpayload.h>
/* ==============================================================================
   I2C Configuration
   ==============================================================================
*/

#define I2C0_BASE (0x1100D000)

enum {

	I2C_CLK_RATE = 12350,	/* EMI/16 (khz) */
	I2C_CLK_RATE_PMIC = 24000	/* kHz for wrapper I2C work frequency */
};

#define I2C_FIFO_SIZE			8

#define MAX_DMA_TRANS_SIZE		252	/* Max(255) aligned to 4 bytes = 252 */
#define MAX_DMA_TRANS_NUM		256

#define MAX_SAMPLE_CNT_DIV		8
#define MAX_STEP_CNT_DIV		64
#define MAX_HS_STEP_CNT_DIV		8

#define I2C_TIMEOUT_TH			200	/* i2c wait for response timeout value, 200ms */

typedef struct MTKI2c {
	I2cOps ops;
	struct mtk_i2c_t *i2c;
	uint32_t wrflag;
	uint32_t readlen;
} MTKI2c;

MTKI2c *new_mtk_i2c(struct mtk_i2c_t *i2c, uint32_t wrflag, uint32_t readlen);

enum {
	I2C0 = 0,
	I2C1 = 1,
	I2C2 = 2,
	I2C3 = 3,
	I2C4 = 4,
	I2C5 = 5,
	I2C6 = 6
};

typedef enum {
	ST_MODE,
	FS_MODE,
	HS_MODE,
} I2C_SPD_MODE;

struct mtk_i2c_t {
	uint8_t id;	/* select which one i2c controller */
	uint8_t dir;	/* Transaction direction, 1,PMIC or 0,6589 */
	uint8_t addr;	/* The address of the slave device, 7bit */
	uint8_t mode;	/* i2c mode, stand mode or High speed mode */
	uint16_t speed;	/* The speed (Kb) */
	uint8_t is_rs_enable;	/* repeat start enable or stop condition */

	/* reserved funtion */
	uint8_t is_push_pull_enable;	/* IO push-pull or open-drain */
	uint8_t is_clk_ext_disable;	/* clk entend default enable */
	uint8_t delay_len;	/* number of half pulse between transfers in a trasaction */
	uint8_t is_dma_enabled;	/* Transaction via DMA instead of 8-byte FIFO */
};

/* ==============================================================================
   I2C Register
   ==============================================================================
*/
enum {
	MTK_I2C_DATA_PORT = 0x0000,
	MTK_I2C_SLAVE_ADDR = 0x0004,
	MTK_I2C_INTR_MASK = 0x0008,
	MTK_I2C_INTR_STAT = 0x000c,
	MTK_I2C_CONTROL = 0x0010,
	MTK_I2C_TRANSFER_LEN = 0x0014,
	MTK_I2C_TRANSAC_LEN = 0x0018,
	MTK_I2C_DELAY_LEN = 0x001c,
	MTK_I2C_TIMING = 0x0020,
	MTK_I2C_START = 0x0024,
	MTK_I2C_EXT_CONF = 0x0028,
	MTK_I2C_FIFO_STAT = 0x0030,
	MTK_I2C_FIFO_THRESH = 0x0034,
	MTK_I2C_FIFO_ADDR_CLR = 0x0038,
	MTK_I2C_IO_CONFIG = 0x0040,
	MTK_I2C_DEBUG = 0x0044,
	MTK_I2C_HS = 0x0048,
	MTK_I2C_SOFTRESET = 0x0050,
	MTK_I2C_PATH_DIR = 0x0060,
	MTK_I2C_DEBUGSTAT = 0x0064,
	MTK_I2C_DEBUGCTRL = 0x0068
};

#define I2C_TRANS_LEN_MASK		(0xff)
#define I2C_TRANS_AUX_LEN_MASK		(0x1f << 8)
#define I2C_CONTROL_MASK		(0x3f << 1)

/* ----------- Register mask ------------------- */
#define I2C_3_BIT_MASK			0x07
#define I2C_4_BIT_MASK			0x0f
#define I2C_8_BIT_MASK			0xff
#define I2C_6_BIT_MASK			0x3f
#define I2C_MASTER_READ			0x01
#define I2C_MASTER_WRITE		0x00
#define I2C_FIFO_THRESH_MASK		0x07
#define I2C_AUX_LEN_MASK		0x1f00
#define I2C_CTL_RS_STOP_BIT		0x02
#define I2C_CTL_DMA_EN_BIT		0x04
#define I2C_CTL_ACK_ERR_DET_BIT		0x20
#define I2C_CTL_CLK_EXT_EN_BIT		0x08
#define I2C_CTL_DIR_CHANGE_BIT		0x10
#define I2C_CTL_TRANSFER_LEN_CHG_BIT	0x40
#define I2C_DATA_READ_ADJ_BIT		0x8000
#define I2C_SDA_MODE_BIT		0x02
#define I2C_SCL_MODE_BIT		0x01
#define I2C_ARBITRATION_BIT		0x01
#define I2C_CLOCK_SYNC_BIT		0x02
#define I2C_BUS_DETECT_EN_BIT		0x04
#define I2C_HS_EN_BIT			0x01
#define I2C_HS_NACK_ERR_DET_EN_BIT	0x02
#define I2C_BUS_BUSY_DET_BIT		0x04
#define I2C_HS_MASTER_CODE_MASK		0x70
#define I2C_HS_STEP_CNT_DIV_MASK	0x700
#define I2C_HS_SAMPLE_CNT_DIV_MASK	0x7000
#define I2C_FIFO_FULL_STATUS		0x01
#define I2C_FIFO_EMPTY_STATUS		0x02

#define I2C_DEBUG			(1 << 3)
#define I2C_HS_NACKERR			(1 << 2)
#define I2C_ACKERR			(1 << 1)
#define I2C_TRANSAC_COMP		(1 << 0)

#define I2C_TX_THR_OFFSET		8
#define I2C_RX_THR_OFFSET		0

/* i2c control bits */
enum {
	TRANS_LEN_CHG = (1 << 6),
	ACK_ERR_DET_EN = (1 << 5),
	DIR_CHG	= (1 << 4),
	CLK_EXT = (1 << 3),
	DMA_EN = (1 << 2),
	REPEATED_START_FLAG = (1 << 1),
	STOP_FLAG = (0 << 1)
};

/* ---------------------- Register Settings ----------------------------------- */
static inline void I2C_START_TRANSAC(uint32_t i2c_base)
{
	writew(0x1, (volatile void *)(uintptr_t)(i2c_base + MTK_I2C_START));
}

static inline void I2C_FIFO_CLR_ADDR(uint32_t i2c_base)
{
	writew(0x1, (volatile void *)(uintptr_t)(i2c_base + MTK_I2C_FIFO_ADDR_CLR));
}

static inline uint16_t I2C_FIFO_OFFSET(uint32_t i2c_base)
{
	return (readw((volatile void *)(uintptr_t)(i2c_base + MTK_I2C_FIFO_STAT))>>4&0xf);
}

static inline uint16_t I2C_FIFO_IS_EMPTY(uint32_t i2c_base)
{
	return (readw((volatile void *)(uintptr_t)(i2c_base + MTK_I2C_FIFO_STAT))>>0&0x1);
}

static inline void I2C_SOFTRESET(uint32_t i2c_base)
{
	writew(0x1, (volatile void *)(uintptr_t)(i2c_base + MTK_I2C_SOFTRESET));
}

static inline void I2C_PATH_PMIC(uint32_t i2c_base)
{
	writew(0x1, (volatile void *)(uintptr_t)(i2c_base + MTK_I2C_PATH_DIR));
}

static inline void I2C_PATH_NORMAL(uint32_t i2c_base)
{
	writew(0x0, (volatile void *)(uintptr_t)(i2c_base + MTK_I2C_PATH_DIR));
}

static inline uint16_t I2C_INTR_STATUS(uint32_t i2c_base)
{
	return readw((volatile void *)(uintptr_t)(i2c_base + MTK_I2C_INTR_STAT));
}

static inline void I2C_SET_FIFO_THRESH(uint32_t i2c_base, uint8_t tx, uint8_t rx)
{
	uint16_t tmp = (((tx) & 0x7) << I2C_TX_THR_OFFSET) |
				(((rx) & 0x7) << I2C_RX_THR_OFFSET);
	writew(tmp, (volatile void *)(uintptr_t)(i2c_base + MTK_I2C_FIFO_THRESH));
}

static inline void I2C_SET_INTR_MASK(uint32_t i2c_base, uint16_t mask)
{
	writew(mask, (volatile void *)(uintptr_t)(i2c_base + MTK_I2C_INTR_MASK));
}

static inline void I2C_CLR_INTR_MASK(uint32_t i2c_base, uint16_t mask)
{
	uint16_t tmp = readw((volatile void *)(uintptr_t)(i2c_base + MTK_I2C_INTR_MASK));
	tmp &= ~(mask);
	writew(tmp, (volatile void *)(uintptr_t)(i2c_base + MTK_I2C_INTR_MASK));
}

static inline void I2C_SET_SLAVE_ADDR(uint32_t i2c_base, uint16_t addr)
{
	writew(addr, (volatile void *)(uintptr_t)(i2c_base + MTK_I2C_SLAVE_ADDR));
}

static inline void I2C_SET_TRANS_LEN(uint32_t i2c_base, uint16_t len)
{
	uint16_t tmp = readw((volatile void *)(uintptr_t)(i2c_base + MTK_I2C_TRANSFER_LEN))
				& ~I2C_TRANS_LEN_MASK;
	tmp |= ((len) & I2C_TRANS_LEN_MASK);
	writew(tmp, (volatile void *)(uintptr_t)(i2c_base + MTK_I2C_TRANSFER_LEN));
}

static inline void I2C_SET_TRANS_AUX_LEN(uint32_t i2c_base, uint16_t len)
{
	uint16_t tmp = readw((volatile void *)(uintptr_t)(i2c_base + MTK_I2C_TRANSFER_LEN)) &
				 ~(I2C_TRANS_AUX_LEN_MASK);
	tmp |= (((len) << 8) & I2C_TRANS_AUX_LEN_MASK);
	writew(tmp, (volatile void *)(uintptr_t)(i2c_base + MTK_I2C_TRANSFER_LEN));
}

static inline void I2C_SET_TRANSAC_LEN(uint32_t i2c_base, uint16_t len)
{
	writew(len, (volatile void *)(uintptr_t)(i2c_base + MTK_I2C_TRANSAC_LEN));
}

static inline void I2C_SET_TRANS_DELAY(uint32_t i2c_base, uint16_t delay)
{
	writew(delay, (volatile void *)(uintptr_t)(i2c_base + MTK_I2C_DELAY_LEN));
}

static inline void I2C_SET_TRANS_CTRL(uint32_t i2c_base, uint16_t ctrl)
{
	uint16_t tmp = readw((volatile void *)(uintptr_t)(i2c_base + MTK_I2C_CONTROL))
				& ~I2C_CONTROL_MASK;
	tmp |= ((ctrl) & I2C_CONTROL_MASK);
	writew(tmp, (volatile void *)(uintptr_t)(i2c_base + MTK_I2C_CONTROL));
}

static inline void I2C_SET_HS_MODE(uint32_t i2c_base, uint16_t on_off)
{
	uint16_t tmp = readw((volatile void *)(uintptr_t)(i2c_base + MTK_I2C_HS)) & ~0x1;
	tmp |= (on_off & 0x1);
	writew(tmp, (volatile void *)(uintptr_t)(i2c_base + MTK_I2C_HS));
}

static inline void I2C_READ_BYTE(uint32_t i2c_base, uint8_t *byte)
{
	*byte = readw((volatile void *)(uintptr_t)(i2c_base + MTK_I2C_DATA_PORT));
}

static inline void I2C_WRITE_BYTE(uint32_t i2c_base, uint8_t byte)
{
	writew(byte, (volatile void *)(uintptr_t)(i2c_base + MTK_I2C_DATA_PORT));
}

static inline void I2C_CLR_INTR_STATUS(uint32_t i2c_base, uint16_t status)
{
	writew(status, (volatile void *)(uintptr_t)(i2c_base + MTK_I2C_INTR_STAT));
}
/* ==============================================================================
   I2C Status Code
   ==============================================================================
*/

enum {
	I2C_OK = 0x0000,
	I2C_SET_SPEED_FAIL_OVER_SPEED = 0xA001,
	I2C_READ_FAIL_ZERO_LENGTH = 0xA002,
	I2C_READ_FAIL_HS_NACKERR = 0xA003,
	I2C_READ_FAIL_ACKERR = 0xA004,
	I2C_READ_FAIL_TIMEOUT = 0xA005,
	I2C_WRITE_FAIL_ZERO_LENGTH = 0xA012,
	I2C_WRITE_FAIL_HS_NACKERR = 0xA013,
	I2C_WRITE_FAIL_ACKERR = 0xA014,
	I2C_WRITE_FAIL_TIMEOUT = 0xA015
};

/*-----------------------------------------------------------------------
 * Set I2C Speend interface:    Set internal I2C speed,
 *                              Goal is that get sample_cnt_div and step_cnt_div
 *   clock: Depends on the current MCU/AHB/APB clock frequency
 *   mode:  ST_MODE. (fixed setting for stable I2C transaction)
 *   khz:   MAX_ST_MODE_SPEED. (fixed setting for stable I2C transaction)
 *
 *   Returns: ERROR_CODE
 */
uint32_t mtk_i2c_set_speed(uint8_t, uint32_t clock,
			       I2C_SPD_MODE mode, uint32_t khz);
/*-----------------------------------------------------------------------
 * new read interface: Read bytes
 *   i2c:    I2C chip config, see struct mtk_i2c_t.
 *   buffer:  Where to read/write the data.
 *   len:     How many bytes to read/write
 *   Returns: ERROR_CODE
 */
uint32_t mtk_i2c_read(struct mtk_i2c_t *i2c,
				     uint8_t *buffer, uint32_t len);

/*-----------------------------------------------------------------------
 * New write interface: Write bytes
 *   i2c:    I2C chip config, see struct mtk_i2c_t.
 *   buffer:  Where to read/write the data.
 *   len:     How many bytes to read/write
 *   Returns: ERROR_CODE
 */
uint32_t mtk_i2c_write(struct mtk_i2c_t *i2c,
				    uint8_t *buffer, uint32_t len);

/*-----------------------------------------------------------------------
 * New write then read back interface: Write bytes then read bytes
 *   i2c:    I2C chip config, see struct mtk_i2c_t.
 *   buffer:  Where to read/write the data.
 *   write_len:     How many bytes to write
 *   read_len:     How many bytes to read
 *   Returns: ERROR_CODE
 */
uint32_t mtk_i2c_write_read(struct mtk_i2c_t *i2c, uint8_t *buffer,
					    uint32_t write_len,
					    uint32_t read_len);
#endif /* __MTK_I2C_H__ */
