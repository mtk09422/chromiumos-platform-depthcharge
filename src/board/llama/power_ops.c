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

#include <libpayload.h>

#include "base/container_of.h"
#include "board/llama/power_ops.h"

/* WDT registers */
enum {
	MTK_WDT_BASE = 0x10000000,
	MTK_WDT_MODE = MTK_WDT_BASE + 0x0000,
	MTK_WDT_LENGTH = MTK_WDT_BASE + 0x0004,
	MTK_WDT_RESTART = MTK_WDT_BASE + 0x0008,
	MTK_WDT_STATUS = MTK_WDT_BASE + 0x000C,
	MTK_WDT_INTERVAL = MTK_WDT_BASE + 0x0010,
	MTK_WDT_SWRST = MTK_WDT_BASE + 0x0014,
	MTK_WDT_SWSYSRST = MTK_WDT_BASE + 0x0018
};

/* WDT_MODE */
enum {
	MTK_WDT_MODE_KEYMASK = 0xff00,
	MTK_WDT_MODE_KEY = 0x22000000,
	MTK_WDT_MODE_DUAL_MODE = 0x0040,
	MTK_WDT_MODE_IN_DIS = 0x0020,
	MTK_WDT_MODE_AUTO_RESTART = 0x0010,
	MTK_WDT_MODE_IRQ = 0x0008,
	MTK_WDT_MODE_EXTEN = 0x0004,
	MTK_WDT_MODE_EXT_POL = 0x0002,
	MTK_WDT_MODE_ENABLE = 0x0001
};

/* WDT_RESTART */
enum {
	MTK_WDT_RESTART_KEY = 0x1971
};

/* WDT_SWRST */
enum {
	MTK_WDT_SWRST_KEY = 0x1209
};

static int llama_wdt_reboot(PowerOps *me)
{
	uint8_t mode;
	LlamaPowerOps *power = container_of(me, LlamaPowerOps, ops);

	mode = power->wdt_mode;

	setbits_le32((uint32_t *)MTK_WDT_RESTART, MTK_WDT_RESTART_KEY);
	setbits_le32((uint32_t *)MTK_WDT_MODE,
		     (MTK_WDT_MODE_KEY | MTK_WDT_MODE_EXTEN |
				(mode ? MTK_WDT_MODE_AUTO_RESTART : 0)));
	/*
	 * Watchdog will trigger reset signal to PMIC as soon as
	 * setbits_le32(MTK_WDT_SWRST, MTK_WDT_SWRST_KEY) executes,
	 * but write register MTK_WDT_MODE has latency, so we add delay here.
	 */
	udelay(100);
	setbits_le32((uint32_t *)MTK_WDT_SWRST, MTK_WDT_SWRST_KEY);

	halt();
}

static int llama_pass_through_power_off(PowerOps *me)
{
	LlamaPowerOps *power = container_of(me, LlamaPowerOps, ops);

	return power->pass_through->power_off(power->pass_through);
}

LlamaPowerOps *new_llama_power_ops(PowerOps *pass_through, uint8_t mode)
{
	LlamaPowerOps *power = xzalloc(sizeof(*power));

	power->ops.cold_reboot = &llama_wdt_reboot;
	power->ops.power_off = &llama_pass_through_power_off;
	power->pass_through = pass_through;
	power->wdt_mode = mode;
	return power;
}
