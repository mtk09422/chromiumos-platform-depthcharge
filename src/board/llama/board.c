/*
 * Copyright 2013 Google Inc.
 *
 * See file CREDITS for list of people who contributed to this
 * project.
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

#include "base/init_funcs.h"
#include "drivers/bus/i2c/mtk_i2c.h"
#include "drivers/storage/mtk_mmc.h"
#include "drivers/tpm/slb9635_i2c.h"
#include "drivers/tpm/tpm.h"

static int board_setup(void)
{
	struct mtk_i2c_t *i2c = xzalloc(sizeof(*i2c));
	MTKI2c *i2cBus = xzalloc(sizeof(*i2cBus));
	i2c->id = 6;
	/* for use pmic i2c. */
	i2c->dir = 1;
	i2c->addr = 0x20;
	i2c->mode = ST_MODE;
	i2c->speed = 100;
	i2cBus = new_mtk_i2c(i2c, 0, 0);
	tpm_set_ops(&new_slb9635_i2c(&i2cBus->ops, 0x20)->base.ops);

	MtkMmcHost *emmc = new_mtk_mmc_host(0x11230000, 8, 0);
	list_insert_after(&emmc->mmc.ctrlr.list_node,
			  &fixed_block_dev_controllers);

	return 0;
}

INIT_FUNC(board_setup);
