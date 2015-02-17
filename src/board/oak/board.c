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

#include <assert.h>
#include <libpayload.h>

#include "base/init_funcs.h"
#include "boot/fit.h"
#include "boot/ramoops.h"
#include "config.h"
#include "drivers/bus/i2c/mtk_i2c.h"
#include "drivers/bus/usb/usb.h"
#include "drivers/flash/spi.h"
#include "drivers/gpio/sysinfo.h"
#include "drivers/gpio/mtk_gpio.h"
#include "drivers/sound/i2s.h"
#include "drivers/sound/max98090.h"
#include "drivers/tpm/slb9635_i2c.h"
#include "drivers/tpm/tpm.h"
#include "vboot/util/flag.h"

static int board_setup(void)
{
	sysinfo_install_flags(NULL);
	fit_set_compat("mediatek,mt8173-crosnb");

	MtGpio *ec_in_rw = new_mtk_gpio_input(10);

	flag_install(FLAG_ECINRW, &ec_in_rw->ops);

	struct mtk_i2c_t *i2c = xzalloc(sizeof(*i2c));
	MTKI2c *i2cBus = xzalloc(sizeof(*i2cBus));

	i2c->base = 0x11009000;
	i2c->id = 2;
	i2c->addr = 0x20;
	i2c->mode = ST_MODE;
	i2c->speed = 100;
	i2cBus = new_mtk_i2c(i2c, 0, 0);
	tpm_set_ops(&new_slb9635_i2c(&i2cBus->ops, 0x20)->base.ops);

	return 0;
}

INIT_FUNC(board_setup);
