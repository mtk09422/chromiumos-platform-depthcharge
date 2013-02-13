/*
 * This file is part of the coreboot project.
 *
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

int i2c_read(uint8_t bus, uint8_t chip, uint32_t addr, int addr_len,
	     uint8_t *data, int data_len);
int i2c_write(uint8_t bus, uint8_t chip, uint32_t addr, int addr_len,
	      uint8_t *data, int data_len);

#endif /* __DRIVERS_BUS_I2C_I2C_H__ */