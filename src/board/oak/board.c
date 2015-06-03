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
#include "drivers/bus/i2s/mt8173.h"
#include "drivers/bus/usb/usb.h"
#include "drivers/ec/cros/ec.h"
#include "drivers/ec/cros/spi.h"
#include "drivers/bus/spi/mt8173.h"
#include "drivers/flash/spi.h"
#include "drivers/gpio/sysinfo.h"
#include "drivers/gpio/mtk_gpio.h"
#include "drivers/power/mt6397.h"
#include "drivers/sound/i2s.h"
#include "drivers/sound/max98090.h"
#include "drivers/storage/mtk_mmc.h"
#include "drivers/flash/mtk_emmc_flash.h"
#include "drivers/tpm/slb9635_i2c.h"
#include "drivers/tpm/tpm.h"
#include "vboot/util/flag.h"

#include "drivers/video/display.h"
#include "drivers/video/mt8173_ddp.h"

static int board_setup(void)
{
	sysinfo_install_flags(new_mtk_gpio_input);

	fit_set_compat_by_rev("google,oak-rev%d", lib_sysinfo.board_id);

	MTKI2c *i2c2 = new_mtk_i2c(0x11009000, 0x11000200, 2, 0, 0x20,
				   ST_MODE, 100, 0);
	tpm_set_ops(&new_slb9635_i2c(&i2c2->ops, 0x20)->base.ops);

	MT8173Spi *spiBus = new_mt8173_spi(0x1100A000);
	CrosEcSpiBus *cros_ec_spi_bus = new_cros_ec_spi_bus(&spiBus->ops);
	cros_ec_set_bus(&cros_ec_spi_bus->ops);

	Mt6397Pmic *pmic = new_mt6397_power(0x1000D000, 0x10007000, 0xE000);
	power_set_ops(&pmic->ops);

	MtkMmcHost *emmc = new_mtk_mmc_host(0x11230000, 8, 0, NULL);
	GpioOps *card_detect_ops = new_gpio_not(new_mtk_gpio_input(1));
	MtkMmcHost *sd_card = new_mtk_mmc_host(0x11240000, 4, 1, card_detect_ops);

	list_insert_after(&emmc->mmc.ctrlr.list_node,
			  &fixed_block_dev_controllers);
	list_insert_after(&sd_card->mmc.ctrlr.list_node,
			  &removable_block_dev_controllers);

	/* set display ops */
	if (lib_sysinfo.framebuffer &&
	    lib_sysinfo.framebuffer->physical_address != 0) {
		display_set_ops(new_mt8173_display());
	}

	UsbHostController *usb_host = new_usb_hc(XHCI, 0x11270000);

	list_insert_after(&usb_host->list_node, &usb_host_controllers);

	/* Setup sound components */
	MtkI2s *i2s0 = new_mtk_i2s(0x11220000, 16, 2, 48000,
				   0x10209000, 0x10000000);
	I2sSource *i2s_source = new_i2s_source(&i2s0->ops, 48000, 2, 16000);
	SoundRoute *sound_route = new_sound_route(&i2s_source->ops);

	MTKI2c *i2c0 = new_mtk_i2c(0x11007000, 0x11000100, 0, 0, 0x10,
				   ST_MODE, 100, 0);

	Max98090Codec *codec = new_max98090_codec(&i2c0->ops, 0x10, 16, 48000,
						  256, 1);
	list_insert_after(&codec->component.list_node,
			  &sound_route->components);
	sound_set_ops(&sound_route->ops);

	MtkEmmcFlash *emmc_flash = new_mtk_emmc_flash(emmc, 0x400000);

	if (emmc_flash)
		flash_set_ops(&emmc_flash->ops);

	return 0;
}

INIT_FUNC(board_setup);
