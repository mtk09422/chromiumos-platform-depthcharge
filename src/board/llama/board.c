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
#include "board/llama/power_ops.h"
#include "drivers/bus/i2c/mtk_i2c.h"
#include "drivers/ec/cros/ec.h"
#include "drivers/ec/cros/spi.h"
#include "drivers/bus/spi/mt8135.h"
#include "drivers/gpio/sysinfo.h"
#include "drivers/gpio/mtk_gpio.h"
#include "drivers/storage/mtk_mmc.h"
#include "drivers/flash/mtk_emmc_flash.h"
#include "drivers/tpm/slb9635_i2c.h"
#include "drivers/tpm/tpm.h"
#include "vboot/util/flag.h"

static int board_setup(void)
{
	sysinfo_install_flags(NULL);

	MtGpio *ec_in_rw = new_mtk_gpio_input(10);
	flag_install(FLAG_ECINRW, &ec_in_rw->ops);

	MtGpio *lid_switch = new_mtk_gpio_input(GPIO_LID);
	flag_replace(FLAG_LIDSW, &lid_switch->ops);
	MtGpio *power_switch_l = new_mtk_gpio_input(GPIO_PWRSW);
	flag_replace(FLAG_PWRSW, &power_switch_l->ops);

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

	struct mt8135_spi_cfg *spi_cfg = xzalloc(sizeof(*spi_cfg));

	spi_cfg->high_time = 8;
	spi_cfg->low_time = 8;
	spi_cfg->setuptime = 4;
	spi_cfg->holdtime = 4;
	spi_cfg->cs_idletime = 4;
	spi_cfg->cpha = 0;
	spi_cfg->cpol = 0;
	spi_cfg->rx_endian = 0;
	spi_cfg->tx_endian = 0;
	spi_cfg->rx_mlsb = 1;
	spi_cfg->tx_mlsb = 1;
	spi_cfg->tckdly = 3;
	spi_cfg->pause = 1;
	spi_cfg->deassert = 0;
	MT8135Spi *spiBus = new_mt8135_spi(0x11016000, spi_cfg);
	CrosEcSpiBus *cros_ec_spi_bus = new_cros_ec_spi_bus(&spiBus->ops);
	cros_ec_set_bus(&cros_ec_spi_bus->ops);

	Mt6397Pmic *pmic = new_mt6397_pmic(2, NULL);
	LlamaPowerOps *power = new_llama_power_ops(&pmic->ops, 1);
	power_set_ops(&power->ops);

	MtkMmcHost *emmc = new_mtk_mmc_host(0x11230000, 8, 0);
	list_insert_after(&emmc->mmc.ctrlr.list_node,
			  &fixed_block_dev_controllers);

	MtkEmmcFlash *emmc_flash = new_mtk_emmc_flash(emmc, 0x400000);
	if (emmc_flash)
		flash_set_ops(&emmc_flash->ops);

	return 0;
}

INIT_FUNC(board_setup);
