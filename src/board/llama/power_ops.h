/*
 * Copyright 2013 Google Inc.  All rights reserved.
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

#ifndef __BOARD_LLAMA_POWER_OPS_H__
#define __BOARD_LLAMA_POWER_OPS_H__

#include "drivers/power/mt6397.h"
#include "drivers/power/power.h"

typedef struct {
	PowerOps ops;
	PowerOps *pass_through;
	uint8_t wdt_mode;
} LlamaPowerOps;

LlamaPowerOps *new_llama_power_ops(PowerOps *pass_through, uint8_t mode);

#endif /* __BOARD_LLAMA_POWER_OPS_H__ */
