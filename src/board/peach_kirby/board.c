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

#include "base/init_funcs.h"
#include "drivers/bus/i2c/exynos5_usi.h"
#include "drivers/bus/spi/exynos5.h"
#include "drivers/bus/usb/exynos.h"
#include "drivers/bus/usb/usb.h"
#include "drivers/ec/cros/spi.h"
#include "drivers/flash/spi.h"
#include "drivers/gpio/exynos5420.h"
#include "drivers/gpio/sysinfo.h"
#include "drivers/power/exynos.h"
#include "drivers/storage/blockdev.h"
#include "drivers/storage/dw_mmc.h"
#include "drivers/tpm/slb9635_i2c.h"
#include "drivers/tpm/tpm.h"
#include "vboot/util/flag.h"

static int board_setup(void)
{
	if (sysinfo_install_flags())
		return 1;

	Exynos5420Gpio *lid_switch = new_exynos5420_gpio_input(GPIO_X, 3, 4);
	Exynos5420Gpio *ec_in_rw = new_exynos5420_gpio_input(GPIO_X, 2, 3);

	if (!lid_switch || !ec_in_rw)
		return 1;

	if (flag_replace(FLAG_LIDSW, &lid_switch->ops) ||
	    flag_install(FLAG_ECINRW, &ec_in_rw->ops))
		return 1;

	// The power switch is active low and needs to be inverted.
	Exynos5420Gpio *power_switch_l =
		new_exynos5420_gpio_input(GPIO_X, 1, 2);
	GpioOps *power_switch = new_gpio_not(&power_switch_l->ops);
	if (!power_switch_l || !power_switch ||
	    flag_replace(FLAG_PWRSW, power_switch))
		return 1;

	Exynos5UsiI2c *i2c9 =
		new_exynos5_usi_i2c(0x12e10000, 400000);
	if (!i2c9)
		return 1;

	Slb9635I2c *tpm = new_slb9635_i2c(&i2c9->ops, 0x20);
	if (!tpm || tpm_set_ops(&tpm->base.ops))
		return 1;

	Exynos5Spi *spi1 = new_exynos5_spi(0x12d30000);
	Exynos5Spi *spi2 = new_exynos5_spi(0x12d40000);
	if (!spi1 || !spi2)
		return 1;

	CrosEcSpiBus *cros_ec_spi_bus = new_cros_ec_spi_bus(&spi2->ops);
	if (!cros_ec_spi_bus)
		return 1;
	cros_ec_set_bus(&cros_ec_spi_bus->ops);

	SpiFlash *flash = new_spi_flash(&spi1->ops, 0x400000);
	if (!flash || flash_set_ops(&flash->ops))
		return 1;

	DwmciHost *emmc = new_dwmci_host(0x12200000, 100000000, 8, 0,
					 DWMCI_SET_SAMPLE_CLK(1) |
					 DWMCI_SET_DRV_CLK(3) |
					 DWMCI_SET_DIV_RATIO(3));
	if (!emmc)
		return 1;
	list_insert_after(&emmc->mmc.ctrlr.list_node,
			  &fixed_block_dev_controllers);

	if (power_set_ops(&exynos_power_ops))
		return 1;

	UsbHostController *usb_drd0 = new_usb_hc(XHCI, 0x12000000);
	UsbHostController *usb_drd1 = new_usb_hc(XHCI, 0x12400000);

	if (!usb_drd0 || !usb_drd1)
		return 1;

	set_usb_init_callback(usb_drd0, exynos5420_usbss_phy_tune);
	set_usb_init_callback(usb_drd1, exynos5420_usbss_phy_tune);

	list_insert_after(&usb_drd0->list_node, &usb_host_controllers);
	list_insert_after(&usb_drd1->list_node, &usb_host_controllers);

	return 0;
}

INIT_FUNC(board_setup);
