/*
 * Copyright 2014 MediaTek Inc.
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
#include <stdint.h>
#include <libpayload.h>

#include "base/container_of.h"
#include "drivers/gpio/mtk_gpio.h"
#include "config.h"

enum {
	FALSE = 0,
	TRUE = 1,
};

enum {
	MAX_GPIO_REG_BITS = 16,
};

enum {
	MAX_GPIO_MODE_PER_REG = 5,
};

enum {
	GPIO_MODE_BITS = 3,
};

/*---------------------------------------*/
static struct mt_gpio_obj gpio_dat = {
	.reg  = (GPIO_REGS *)(CONFIG_DRIVER_GPIO_MTK_BASE),
};

/*---------------------------------------------------------------------------*/
int mt_set_gpio_out_chip(struct mt_gpio_obj *obj, u32 pin, u32 output)
{
	u32 pos;
	u32 bit;

	if (!obj)
		return -ERACCESS;

	if (pin >= MAX_GPIO_PIN)
		return -ERINVAL;

	if (output >= GPIO_OUT_MAX)
		return -ERINVAL;

	pos = pin / MAX_GPIO_REG_BITS;
	bit = pin % MAX_GPIO_REG_BITS;

	if (output == GPIO_OUT_ZERO)
		setbits_le32(&obj->reg->dout[pos].rst, (1L << bit));
	else
		setbits_le32(&obj->reg->dout[pos].set, (1L << bit));
	return RSUCCESS;
}

/*---------------------------------------------------------------------------*/
int mt_get_gpio_in_chip(struct mt_gpio_obj *obj, u32 pin)
{
	u32 pos;
	u32 bit;
	u32 reg;

	if (!obj)
		return -ERACCESS;

	if (pin >= MAX_GPIO_PIN)
		return -ERINVAL;

	pos = pin / MAX_GPIO_REG_BITS;
	bit = pin % MAX_GPIO_REG_BITS;

	reg = readl(&obj->reg->din[pos].val);
	return (((reg & (1L << bit)) != 0) ? 1 : 0);
}

/*---------------------------------------------------------------------------*/
int mt_set_gpio_out(GpioOps *me, u32 output)
{
	MtGpio *gpio = container_of(me, MtGpio, ops);
	unsigned pin = gpio->pin_num;
	struct mt_gpio_obj *gpio_obj = gpio->obj;

	return mt_set_gpio_out_chip(gpio_obj, pin, output);
}

/*---------------------------------------------------------------------------*/
int mt_get_gpio_in(GpioOps *me)
{
	MtGpio *gpio = container_of(me, MtGpio, ops);
	unsigned pin = gpio->pin_num;
	struct mt_gpio_obj *gpio_obj = gpio->obj;

	return mt_get_gpio_in_chip(gpio_obj, pin);
}

static MtGpio *new_mtk_gpio(unsigned pin)
{
	die_if(pin > GPIO_MAX, "Bad GPIO pin number %d.\n", pin);
	MtGpio *gpio = xzalloc(sizeof(*gpio));

	gpio->pin_num = pin;
	gpio->obj = &gpio_dat;

	return gpio;
}

MtGpio *new_mtk_gpio_input(unsigned pin)
{
	MtGpio *gpio = new_mtk_gpio(pin);

	gpio->ops.get = &mt_get_gpio_in;
	return gpio;
}

MtGpio *new_mtk_gpio_output(unsigned pin)
{
	MtGpio *gpio = new_mtk_gpio(pin);

	gpio->ops.set = &mt_set_gpio_out;
	return gpio;
}
