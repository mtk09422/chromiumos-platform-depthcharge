/*
 * Copyright 2012 Google Inc.  All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA, 02110-1301 USA
 */

#ifndef __DRIVERS_BUS_I2C_I2C_H__
#define __DRIVERS_BUS_I2C_I2C_H__

#include <stdint.h>

/*
 * transfer operation consumes I2cSeg struct one by one and finishes a
 * transaction by a stop bit at the end.
 *
 *   frame = [segment] ... [segment][stop]
 *   segment = [start][slave addr][r/w][ack][data][ack][data][ack] ...
 *
 * Two adjacent segments are connected by a repeated start bit.
 */
typedef struct I2cSeg
{
	int read;	// 0:write 1:read
	uint8_t chip;	// slave address
	uint8_t *buf;	// buffer for read or write
	int len;	// buffer size
} I2cSeg;

typedef struct I2cOps
{
	/*
	 * read and write will be deprecated. New i2c drivers should implement
	 * transfer and new slave drivers should use transfer (or utility funcs
	 * in i2c.c).
	 */
	int (*read)(struct I2cOps *me, uint8_t chip,
		    uint32_t addr, int addr_len, uint8_t *data, int data_len);
	int (*write)(struct I2cOps *me, uint8_t chip,
		     uint32_t addr, int addr_len, uint8_t *data, int data_len);
	int (*transfer)(struct I2cOps *me, I2cSeg *segments, int seg_count);
} I2cOps;

/**
 * Read a byte by two segments in one frame
 *
 * [start][slave addr][w][register addr][start][slave addr][r][data][stop]
 */
int i2c_readb(I2cOps *ops, uint8_t chip, uint8_t reg, uint8_t *data);

/**
 * Write a byte by one segment in one frame.
 *
 * [start][slave addr][w][register addr][data][stop]
 */
int i2c_writeb(I2cOps *ops, uint8_t chip, uint8_t reg, uint8_t data);

#endif /* __DRIVERS_BUS_I2C_I2C_H__ */
