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
#include "drivers/ec/cros/ec.h"

/******************************************************************************
 MACRO Definition
******************************************************************************/
#if LK_SUPPORT_EXT_GPIO
static inline int gpioext_write(void *addr, s64 data)
{
	return pwrap_write((u32)addr, data);
}

static inline int gpioext_read(void *addr)
{
	u32 ext_data;
	int ret;

	ret = pwrap_read((u32)addr, &ext_data);

	return (ret != 0) ? 0xFFFFFFFF : ext_data;
}

static inline int gpioext_set_bits(u32 bit, void *reg)
{
	return gpioext_write(reg, bit);
}

static inline int gpioext_clr_bits(u32 bit, void *reg)
{
	u32 ext_data;

	int ret;
	ret = gpioext_read(reg);
	ext_data = ret;

	return (ret < 0) ? 0xFFFFFFFF : (gpioext_write(reg, ext_data & ~((u32)(bit))));
}
#define GPIOEXT_BASE        (0xC000)			/* PMIC GPIO base. */
#endif

#define GPIO_BASE 0x10005000

enum {
	FALSE = 0,
	TRUE = 1,
};

#define MAX_GPIO_REG_BITS      16
#define MAX_GPIO_MODE_PER_REG  5
#define GPIO_MODE_BITS         3

#define GPIOTAG                "[GPIO] "
#define GPIOLOG(fmt, arg...)   printf(GPIOTAG fmt, ##arg)
#define GPIOMSG(fmt, arg...)   printf(fmt, ##arg)
#define GPIOERR(fmt, arg...)   printf(GPIOTAG "%5d: "fmt, __LINE__, ##arg)

/* -------for special kpad pupd----------- */
struct kpad_pupd {
	unsigned char pin;
	unsigned char reg;
	unsigned char bit;
};
static struct kpad_pupd kpad_pupd_spec[] = {
	{GPIO119,	0,	2},     /* KROW0 */
	{GPIO120,	0,	6},     /* KROW1 */
	{GPIO121,	0,	10},    /* KROW2 */
	{GPIO122,	1,	2},     /* KCOL0 */
	{GPIO123,	1,	6},     /* KCOL1 */
	{GPIO124,	1,	10}     /* KCOL2 */
};
/*-------for special msdc pupd-----------*/
#define MSDC0_MIN_PIN GPIO57
#define MSDC0_MAX_PIN GPIO67
#define MSDC1_MIN_PIN GPIO73
#define MSDC1_MAX_PIN GPIO78
#define MSDC2_MIN_PIN GPIO100
#define MSDC2_MAX_PIN GPIO105
#define MSDC3_MIN_PIN GPIO22
#define MSDC3_MAX_PIN GPIO27

#define MSDC0_PIN_NUM (MSDC0_MAX_PIN - MSDC0_MIN_PIN + 1)
#define MSDC1_PIN_NUM (MSDC1_MAX_PIN - MSDC1_MIN_PIN + 1)
#define MSDC2_PIN_NUM (MSDC2_MAX_PIN - MSDC2_MIN_PIN + 1)
#define MSDC3_PIN_NUM (MSDC3_MAX_PIN - MSDC3_MIN_PIN + 1)

static const struct msdc_pupd msdc_pupd_spec[] = 
{
    /* msdc0 pin:GPIO57~GPIO67 */
    {0xC20, 0},/* GPIO57 */
    {0xC20, 0},/* GPIO58 */
    {0xC20, 0},/* GPIO59 */
    {0xC20, 0},/* GPIO60 */
    {0xC20, 0},/* GPIO61 */
    {0xC20, 0},/* GPIO62 */
    {0xC20, 0},/* GPIO63 */
    {0xC20, 0},/* GPIO64 */
    {0xC00, 0},/* GPIO65 */
    {0xC10, 0},/* GPIO66 */
    {0xD10, 0},/* GPIO67 */
    {0xD00, 0},/* GPIO68 */
    /* msdc1 pin:GPIO73~GPIO78  */
    {0xD20, 0},/* GPIO73 */
    {0xD20, 4},/* GPIO74 */
    {0xD20, 8},/* GPIO75 */
    {0xD20, 12},/* GPIO76 */
    {0xC40, 0},/* GPIO77 */
    {0xC50, 0},/* GPIO78 */
    /* msdc2 pin:GPIO100~GPIO105 */
    {0xD40, 0},/* GPIO100 */
    {0xD40, 4},/* GPIO101 */
    {0xD40, 8},/* GPIO102 */
    {0xD40, 12},/* GPIO103 */
    {0xC80, 0},/* GPIO104 */
    {0xC90, 0},/* GPIO105 */
    /*msdc3 pin*/
    {0xD60, 0},/* GPIO22 */
    {0xD60, 4},/* GPIO23 */
    {0xD60, 8},/* GPIO24 */
    {0xD60, 12},/* GPIO25 */
    {0xCC0, 0},/* GPIO26 */
    {0xCD0, 0},/* GPIO27 */
};
#define PUPD_OFFSET_TO_REG(msdc_pupd_ctrl) ((VAL_REGS *)(uintptr_t)(GPIO_BASE + msdc_pupd_ctrl->reg))

/*---------------------------------------------------------------------------*/
const struct msdc_pupd *mt_get_msdc_ctrl(unsigned long pin)
{
	unsigned int idx = 255;

	if ((pin >= MSDC0_MIN_PIN) && (pin <= MSDC0_MAX_PIN)) {
		idx = pin - MSDC0_MIN_PIN;
	} else if ((pin >= MSDC1_MIN_PIN) && (pin <= MSDC1_MAX_PIN)) {
		idx = MSDC0_PIN_NUM + pin-MSDC1_MIN_PIN;/* pin-94 */
	} else if ((pin >= MSDC2_MIN_PIN) && (pin <= MSDC2_MAX_PIN)) {
		idx = MSDC0_PIN_NUM + MSDC1_PIN_NUM + pin-MSDC2_MIN_PIN;/* pin-68 */
	} else if ((pin >= MSDC3_MIN_PIN) && (pin <= MSDC3_MAX_PIN)) {
		idx = MSDC0_PIN_NUM + MSDC1_PIN_NUM + MSDC2_PIN_NUM + pin-MSDC3_MIN_PIN;/* pin-226 */
	} else {
	return NULL;
	}
	return &msdc_pupd_spec[idx];
}

/*---------------------------------------*/
struct mt_gpio_obj {
	GPIO_REGS       *reg;
};
static struct mt_gpio_obj gpio_dat = {
	.reg  = (GPIO_REGS *)(GPIO_BASE),
};
static struct mt_gpio_obj *gpio_obj = &gpio_dat;
/*---------------------------------------------------------------------------*/
#if LK_SUPPORT_EXT_GPIO
/*---------------------------------------*/
struct mt_gpioext_obj {
	GPIOEXT_REGS	*reg;
};
/*---------------------------------------*/
static struct mt_gpioext_obj gpioext_dat = {
	.reg = (GPIOEXT_REGS *)(GPIOEXT_BASE),
};
/*---------------------------------------*/
static struct mt_gpioext_obj *gpioext_obj = &gpioext_dat;
#endif


/*---------------------------------------------------------------------------*/
s32 mt_set_gpio_dir_chip(u32 pin, u32 dir)
{
	u32 pos;
	u32 bit;
	struct mt_gpio_obj *obj = gpio_obj;

	if (!obj)
		return -ERACCESS;

#if LK_SUPPORT_EXT_GPIO
	if (pin >= GPIO_EXTEND_START)
		return -ERINVAL;
#else
	if (pin >= MAX_GPIO_PIN)
		return -ERINVAL;
#endif

	if (dir >= GPIO_DIR_MAX)
		return -ERINVAL;

	pos = pin / MAX_GPIO_REG_BITS;
	bit = pin % MAX_GPIO_REG_BITS;

	if (dir == GPIO_DIR_IN)
		setbits_le32(&obj->reg->dir[pos].rst, (1L << bit));
	else
		setbits_le32(&obj->reg->dir[pos].set, (1L << bit));
	return RSUCCESS;

}
/*---------------------------------------------------------------------------*/
s32 mt_get_gpio_dir_chip(u32 pin)
{
	u32 pos;
	u32 bit;
	u32 reg;
	struct mt_gpio_obj *obj = gpio_obj;

	if (!obj)
		return -ERACCESS;

#if LK_SUPPORT_EXT_GPIO
	if (pin >= GPIO_EXTEND_START)
		return -ERINVAL;
#else
	if (pin >= MAX_GPIO_PIN)
		return -ERINVAL;
#endif

	pos = pin / MAX_GPIO_REG_BITS;
	bit = pin % MAX_GPIO_REG_BITS;

	reg = readl(&obj->reg->dir[pos].val);
	return (((reg & (1L << bit)) != 0) ? 1 : 0);
}
/*---------------------------------------------------------------------------*/
s32 mt_set_gpio_pull_enable_chip(u32 pin, u32 enable)
{
	u32 pos;
	u32 bit;
	u32 i;
	struct mt_gpio_obj *obj = gpio_obj;
	const struct msdc_pupd *msdc_pupd_ctrl;
	u16 *address;

	if (!obj)
		return -ERACCESS;

#if LK_SUPPORT_EXT_GPIO
	if (pin >= GPIO_EXTEND_START)
		return -ERINVAL;
#else
	if (pin >= MAX_GPIO_PIN)
		return -ERINVAL;
#endif

	if (enable >= GPIO_PULL_EN_MAX)
		return -ERINVAL;

	/*****************for special kpad pupd, NOTE DEFINITION REVERSE!!!*****************/
	for (i = 0; i < sizeof(kpad_pupd_spec)/sizeof(kpad_pupd_spec[0]); i++) {
		if (pin == kpad_pupd_spec[i].pin) {
			if (enable == GPIO_PULL_DISABLE) {
				address = &obj->reg->kpad_ctrl[kpad_pupd_spec[i].reg].rst;
				setbits_le32(address, (3L << (kpad_pupd_spec[i].bit-2)));
			} else {
				/* single key: 75K */
				address = &obj->reg->kpad_ctrl[kpad_pupd_spec[i].reg].set;
				setbits_le32(address, (1L << (kpad_pupd_spec[i].bit-2)));
			}
			return RSUCCESS;
		}
	}
	/********************************* MSDC special *********************************/
	if (NULL != (msdc_pupd_ctrl = mt_get_msdc_ctrl(pin)))
	{
		if (enable == GPIO_PULL_DISABLE)
		{
			address = &(PUPD_OFFSET_TO_REG(msdc_pupd_ctrl)->rst);
			setbits_le32(address, (3L << (msdc_pupd_ctrl->bit + R0R1)));
		} else {
			address = &((PUPD_OFFSET_TO_REG(msdc_pupd_ctrl))->set);
			setbits_le32(address, (1L << (msdc_pupd_ctrl->bit + R0R1)));
		}

	}
	pos = pin / MAX_GPIO_REG_BITS;
	bit = pin % MAX_GPIO_REG_BITS;

	if (enable == GPIO_PULL_DISABLE)
		setbits_le32(&obj->reg->pullen[pos].rst, (1L << bit));
	else
		setbits_le32(&obj->reg->pullen[pos].set, (1L << bit));

	return RSUCCESS;
}
/*---------------------------------------------------------------------------*/
s32 mt_get_gpio_pull_enable_chip(u32 pin)
{
	u32 pos;
	u32 bit;
	u32 reg;
	u32 i;
	struct mt_gpio_obj *obj = gpio_obj;
	const struct msdc_pupd *msdc_pupd_ctrl;
	unsigned long data = 0;


	if (!obj)
		return -ERACCESS;

#if LK_SUPPORT_EXT_GPIO
	if (pin >= GPIO_EXTEND_START)
		return -ERINVAL;
#else
	if (pin >= MAX_GPIO_PIN)
		return -ERINVAL;
#endif

	/*****************for special kpad pupd, NOTE DEFINITION REVERSE!!!*****************/
	for (i = 0; i < sizeof(kpad_pupd_spec)/sizeof(kpad_pupd_spec[0]); i++) {
		if (pin == kpad_pupd_spec[i].pin) {
			return (((readl(&obj->reg->kpad_ctrl[kpad_pupd_spec[i].reg].val) & (3L << (kpad_pupd_spec[i].bit-2))) != 0) ? 1 : 0);
		}
	}
	/*********************************MSDC special pupd*********************************/
	if (NULL != (msdc_pupd_ctrl = mt_get_msdc_ctrl(pin)))
	{
		data = readl(&PUPD_OFFSET_TO_REG(msdc_pupd_ctrl)->val);
		return (((data & (3L << (msdc_pupd_ctrl->bit + R0R1))) == 0) ? 0 : 1);
	}

	pos = pin / MAX_GPIO_REG_BITS;
	bit = pin % MAX_GPIO_REG_BITS;

	reg = readl(&obj->reg->pullen[pos].val);

	return (((reg & (1L << bit)) != 0) ? 1 : 0);
}
/*---------------------------------------------------------------------------*/
s32 mt_set_gpio_pull_select_chip(u32 pin, u32 select)
{
	u32 pos;
	u32 bit;
	u32 i;
	struct mt_gpio_obj *obj = gpio_obj;
	const struct msdc_pupd *msdc_pupd_ctrl;
	u16 *address;

	if (!obj)
		return -ERACCESS;

#if LK_SUPPORT_EXT_GPIO
	if (pin >= GPIO_EXTEND_START)
		return -ERINVAL;
#else
	if (pin >= MAX_GPIO_PIN)
		return -ERINVAL;
#endif

	if (select >= GPIO_PULL_MAX)
		return -ERINVAL;

	/***********************for special kpad pupd, NOTE DEFINITION REVERSE!!!**************************/
	for (i = 0; i < sizeof(kpad_pupd_spec)/sizeof(kpad_pupd_spec[0]); i++) {
		if (pin == kpad_pupd_spec[i].pin) {
			if (select == GPIO_PULL_DOWN)
				address = &obj->reg->kpad_ctrl[kpad_pupd_spec[i].reg].set;
			else
				address = &obj->reg->kpad_ctrl[kpad_pupd_spec[i].reg].rst;
			setbits_le32(address, (1L << kpad_pupd_spec[i].bit));
			return RSUCCESS;
		}
	}
	/*************************************MSDC special pupd*************************/
	if (NULL != (msdc_pupd_ctrl = mt_get_msdc_ctrl(pin)))
	{
		if (select == GPIO_PULL_DOWN)
			address = &(PUPD_OFFSET_TO_REG(msdc_pupd_ctrl))->set;
		else
			address = &(PUPD_OFFSET_TO_REG(msdc_pupd_ctrl))->rst;
		setbits_le32(address, (1L << (msdc_pupd_ctrl->bit+PUPD)));
		return RSUCCESS;
	}

	pos = pin / MAX_GPIO_REG_BITS;
	bit = pin % MAX_GPIO_REG_BITS;

	if (select == GPIO_PULL_DOWN)
		setbits_le32(&obj->reg->pullsel[pos].rst, (1L << bit));
	else
		setbits_le32(&obj->reg->pullsel[pos].set, (1L << bit));
	return RSUCCESS;
}
/*---------------------------------------------------------------------------*/
s32 mt_get_gpio_pull_select_chip(u32 pin)
{
	u32 pos;
	u32 bit;
	u32 reg;
	u32 i;
	struct mt_gpio_obj *obj = gpio_obj;
	const struct msdc_pupd *msdc_pupd_ctrl;
	unsigned long data = 0;

	if (!obj)
		return -ERACCESS;

#if LK_SUPPORT_EXT_GPIO
	if (pin >= GPIO_EXTEND_START)
		return -ERINVAL;
#else
	if (pin >= MAX_GPIO_PIN)
		return -ERINVAL;
#endif

	/*********************************for special kpad pupd*********************************/
	for (i = 0; i < sizeof(kpad_pupd_spec)/sizeof(kpad_pupd_spec[0]); i++) {
		if (pin == kpad_pupd_spec[i].pin) {
			reg = readl(&obj->reg->kpad_ctrl[kpad_pupd_spec[i].reg].val);
			return (((reg & (1L << kpad_pupd_spec[i].bit)) != 0) ? 0 : 1);
		}
	}
	/********************************* MSDC special pupd *********************************/
	if (NULL != (msdc_pupd_ctrl = mt_get_msdc_ctrl(pin)))
	{
		data = readl(&((PUPD_OFFSET_TO_REG(msdc_pupd_ctrl))->val));
		return (((data & (1L << (msdc_pupd_ctrl->bit+PUPD))) != 0) ? 0 : 1);
	}

	pos = pin / MAX_GPIO_REG_BITS;
	bit = pin % MAX_GPIO_REG_BITS;

	reg = readl(&obj->reg->pullsel[pos].val);
	return (((reg & (1L << bit)) != 0) ? 1 : 0);
}

/*---------------------------------------------------------------------------*/
s32 mt_set_gpio_out_chip(u32 pin, u32 output)
{
	u32 pos;
	u32 bit;
	struct mt_gpio_obj *obj = gpio_obj;

	if (!obj)
		return -ERACCESS;

#if LK_SUPPORT_EXT_GPIO
	if (pin >= GPIO_EXTEND_START)
		return -ERINVAL;
#else
	if (pin >= MAX_GPIO_PIN)
		return -ERINVAL;
#endif

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
s32 mt_get_gpio_out_chip(u32 pin)
{
	u32 pos;
	u32 bit;
	u32 reg;
	struct mt_gpio_obj *obj = gpio_obj;

	if (!obj)
		return -ERACCESS;

#if LK_SUPPORT_EXT_GPIO
	if (pin >= GPIO_EXTEND_START)
		return -ERINVAL;
#else
	if (pin >= MAX_GPIO_PIN)
		return -ERINVAL;
#endif

	pos = pin / MAX_GPIO_REG_BITS;
	bit = pin % MAX_GPIO_REG_BITS;

	reg = readl(&obj->reg->dout[pos].val);
	return (((reg & (1L << bit)) != 0) ? 1 : 0);
}
/*---------------------------------------------------------------------------*/
s32 mt_get_gpio_in_chip(u32 pin)
{
	u32 pos;
	u32 bit;
	u32 reg;
	struct mt_gpio_obj *obj = gpio_obj;

	if (!obj)
		return -ERACCESS;

#if LK_SUPPORT_EXT_GPIO
	if (pin >= GPIO_EXTEND_START)
		return -ERINVAL;
#else
	if (pin >= MAX_GPIO_PIN)
		return -ERINVAL;
#endif

	pos = pin / MAX_GPIO_REG_BITS;
	bit = pin % MAX_GPIO_REG_BITS;

	reg = readl(&obj->reg->din[pos].val);
	return (((reg & (1L << bit)) != 0) ? 1 : 0);
}
/*---------------------------------------------------------------------------*/
s32 mt_set_gpio_mode_chip(u32 pin, u32 mode)
{
	u32 pos;
	u32 bit;
	u32 mask = (1L << GPIO_MODE_BITS) - 1;
	struct mt_gpio_obj *obj = gpio_obj;
	u32 offset = 0;

	if (!obj)
		return -ERACCESS;

#if LK_SUPPORT_EXT_GPIO
	if (pin >= GPIO_EXTEND_START)
		return -ERINVAL;
#else
	if (pin >= MAX_GPIO_PIN)
		return -ERINVAL;
#endif

	if (mode >= GPIO_MODE_MAX)
		return -ERINVAL;

	pos = pin / MAX_GPIO_MODE_PER_REG;
	bit = pin % MAX_GPIO_MODE_PER_REG;

	offset = GPIO_MODE_BITS * bit;
	clrsetbits_le32(&obj->reg->mode[pos].val, mask << offset, mode << offset);

	return RSUCCESS;
}
/*---------------------------------------------------------------------------*/
s32 mt_get_gpio_mode_chip(u32 pin)
{
	u32 pos;
	u32 bit;
	u32 reg;
	u32 mask = (1L << GPIO_MODE_BITS) - 1;
	struct mt_gpio_obj *obj = gpio_obj;

	if (!obj)
		return -ERACCESS;

#if LK_SUPPORT_EXT_GPIO
	if (pin >= GPIO_EXTEND_START)
		return -ERINVAL;
#else
	if (pin >= MAX_GPIO_PIN)
		return -ERINVAL;
#endif

	pos = pin / MAX_GPIO_MODE_PER_REG;
	bit = pin % MAX_GPIO_MODE_PER_REG;

	reg = readl(&obj->reg->mode[pos].val);

	return ((reg >> (GPIO_MODE_BITS * bit)) & mask);
}
/*---------------------------------------------------------------------------*/
#if LK_SUPPORT_EXT_GPIO
s32 mt_set_gpio_dir_ext(u32 pin, u32 dir)
{
	u32 pos;
	u32 bit;
	int ret = 0;
	struct mt_gpioext_obj *obj = gpioext_obj;

	if (!obj)
		return -ERACCESS;

	if (pin >= MAX_GPIO_PIN)
		return -ERINVAL;

	if (dir >= GPIO_DIR_MAX)
		return -ERINVAL;
	pin -= GPIO_EXTEND_START;
	pos = pin / MAX_GPIO_REG_BITS;
	bit = pin % MAX_GPIO_REG_BITS;

	if (dir == GPIO_DIR_IN)
		ret = gpioext_set_bits((1L << bit), &obj->reg->dir[pos].rst);
	else
		ret = gpioext_set_bits((1L << bit), &obj->reg->dir[pos].set);

	if (ret != 0) return -ERWRAPPER;
	return RSUCCESS;
}
/*---------------------------------------------------------------------------*/
s32 mt_get_gpio_dir_ext(u32 pin)
{
	u32 pos;
	u32 bit;
	s64 reg;
	struct mt_gpioext_obj *obj = gpioext_obj;

	if (!obj)
		return -ERACCESS;

	if (pin >= MAX_GPIO_PIN)
		return -ERINVAL;

	pin -= GPIO_EXTEND_START;
	pos = pin / MAX_GPIO_REG_BITS;
	bit = pin % MAX_GPIO_REG_BITS;

	reg = gpioext_read(&obj->reg->dir[pos].val);
	if (reg < 0) return -ERWRAPPER;
	return (((reg & (1L << bit)) != 0) ? 1 : 0);
}
/*---------------------------------------------------------------------------*/
s32 mt_set_gpio_pull_enable_ext(u32 pin, u32 enable)
{
	u32 pos;
	u32 bit;
	int ret = 0;
	struct mt_gpioext_obj *obj = gpioext_obj;

	if (!obj)
		return -ERACCESS;

	if (pin >= MAX_GPIO_PIN)
		return -ERINVAL;

	if (enable >= GPIO_PULL_EN_MAX)
		return -ERINVAL;

	pin -= GPIO_EXTEND_START;
	pos = pin / MAX_GPIO_REG_BITS;
	bit = pin % MAX_GPIO_REG_BITS;

	if (enable == GPIO_PULL_DISABLE)
		ret = gpioext_set_bits((1L << bit), &obj->reg->pullen[pos].rst);
	else
		ret = gpioext_set_bits((1L << bit), &obj->reg->pullen[pos].set);
	if (ret != 0) return -ERWRAPPER;
	return RSUCCESS;
}
/*---------------------------------------------------------------------------*/
s32 mt_get_gpio_pull_enable_ext(u32 pin)
{
	u32 pos;
	u32 bit;
	s64 reg;
	struct mt_gpioext_obj *obj = gpioext_obj;

	if (!obj)
		return -ERACCESS;

	if (pin >= MAX_GPIO_PIN)
		return -ERINVAL;

	pin -= GPIO_EXTEND_START;
	pos = pin / MAX_GPIO_REG_BITS;
	bit = pin % MAX_GPIO_REG_BITS;

	reg = gpioext_read(&obj->reg->pullen[pos].val);
	if (reg < 0) return -ERWRAPPER;
	return (((reg & (1L << bit)) != 0) ? 1 : 0);
}
/*---------------------------------------------------------------------------*/
s32 mt_set_gpio_pull_select_ext(u32 pin, u32 select)
{
	u32 pos;
	u32 bit;
	int ret = 0;
	struct mt_gpioext_obj *obj = gpioext_obj;

	if (!obj)
		return -ERACCESS;

	if (pin >= MAX_GPIO_PIN)
		return -ERINVAL;

	if (select >= GPIO_PULL_MAX)
		return -ERINVAL;

	pin -= GPIO_EXTEND_START;
	pos = pin / MAX_GPIO_REG_BITS;
	bit = pin % MAX_GPIO_REG_BITS;

	if (select == GPIO_PULL_DOWN)
		ret = gpioext_set_bits((1L << bit), &obj->reg->pullsel[pos].rst);
	else
		ret = gpioext_set_bits((1L << bit), &obj->reg->pullsel[pos].set);
	if (ret != 0) return -ERWRAPPER;
	return RSUCCESS;
}
/*---------------------------------------------------------------------------*/
s32 mt_get_gpio_pull_select_ext(u32 pin)
{
	u32 pos;
	u32 bit;
	s64 reg;
	struct mt_gpioext_obj *obj = gpioext_obj;

	if (!obj)
		return -ERACCESS;

	if (pin >= MAX_GPIO_PIN)
		return -ERINVAL;

	pin -= GPIO_EXTEND_START;
	pos = pin / MAX_GPIO_REG_BITS;
	bit = pin % MAX_GPIO_REG_BITS;

	reg = gpioext_read(&obj->reg->pullsel[pos].val);
	if (reg < 0) return -ERWRAPPER;
	return (((reg & (1L << bit)) != 0) ? 1 : 0);
}
/*---------------------------------------------------------------------------*/
s32 mt_set_gpio_inversion_ext(u32 pin, u32 enable)
{
	u32 pos;
	u32 bit;
	int ret = 0;
	struct mt_gpioext_obj *obj = gpioext_obj;

	if (!obj)
		return -ERACCESS;

	if (pin >= MAX_GPIO_PIN)
		return -ERINVAL;

	if (enable >= GPIO_DATA_INV_MAX)
		return -ERINVAL;

	pin -= GPIO_EXTEND_START;
	pos = pin / MAX_GPIO_REG_BITS;
	bit = pin % MAX_GPIO_REG_BITS;

	if (enable == GPIO_DATA_UNINV)
		ret = gpioext_set_bits((1L << bit), &obj->reg->dinv[pos].rst);
	else
		ret = gpioext_set_bits((1L << bit), &obj->reg->dinv[pos].set);
	if (ret != 0) return -ERWRAPPER;
	return RSUCCESS;
}
/*---------------------------------------------------------------------------*/
s32 mt_get_gpio_inversion_ext(u32 pin)
{
	u32 pos;
	u32 bit;
	s64 reg;
	struct mt_gpioext_obj *obj = gpioext_obj;

	if (!obj)
		return -ERACCESS;

	if (pin >= MAX_GPIO_PIN)
		return -ERINVAL;

	pin -= GPIO_EXTEND_START;
	pos = pin / MAX_GPIO_REG_BITS;
	bit = pin % MAX_GPIO_REG_BITS;

	reg = gpioext_read(&obj->reg->dinv[pos].val);
	if (reg < 0) return -ERWRAPPER;
	return (((reg & (1L << bit)) != 0) ? 1 : 0);
}
/*---------------------------------------------------------------------------*/
s32 mt_set_gpio_out_ext(u32 pin, u32 output)
{
	u32 pos;
	u32 bit;
	int ret = 0;
	struct mt_gpioext_obj *obj = gpioext_obj;

	if (!obj)
		return -ERACCESS;

	if (pin >= MAX_GPIO_PIN)
		return -ERINVAL;

	if (output >= GPIO_OUT_MAX)
		return -ERINVAL;

	pin -= GPIO_EXTEND_START;
	pos = pin / MAX_GPIO_REG_BITS;
	bit = pin % MAX_GPIO_REG_BITS;

	if (output == GPIO_OUT_ZERO)
		ret = gpioext_set_bits((1L << bit), &obj->reg->dout[pos].rst);
	else
		ret = gpioext_set_bits((1L << bit), &obj->reg->dout[pos].set);
	if (ret != 0) return -ERWRAPPER;
	return RSUCCESS;
}
/*---------------------------------------------------------------------------*/
s32 mt_get_gpio_out_ext(u32 pin)
{
	u32 pos;
	u32 bit;
	s64 reg;
	struct mt_gpioext_obj *obj = gpioext_obj;

	if (!obj)
		return -ERACCESS;

	if (pin >= MAX_GPIO_PIN)
		return -ERINVAL;

	pin -= GPIO_EXTEND_START;
	pos = pin / MAX_GPIO_REG_BITS;
	bit = pin % MAX_GPIO_REG_BITS;

	reg = gpioext_read(&obj->reg->dout[pos].val);
	if (reg < 0) return -ERWRAPPER;
	return (((reg & (1L << bit)) != 0) ? 1 : 0);
}
/*---------------------------------------------------------------------------*/
s32 mt_get_gpio_in_ext(u32 pin)
{
	u32 pos;
	u32 bit;
	s64 reg;
	struct mt_gpioext_obj *obj = gpioext_obj;

	if (!obj)
		return -ERACCESS;

	if (pin >= MAX_GPIO_PIN)
		return -ERINVAL;

	pin -= GPIO_EXTEND_START;
	pos = pin / MAX_GPIO_REG_BITS;
	bit = pin % MAX_GPIO_REG_BITS;

	reg = gpioext_read(&obj->reg->din[pos].val);
	if (reg < 0) return -ERWRAPPER;
	return (((reg & (1L << bit)) != 0) ? 1 : 0);
}
/*---------------------------------------------------------------------------*/
s32 mt_set_gpio_mode_ext(u32 pin, u32 mode)
{
	u32 pos;
	u32 bit;
	s64 reg;
	int ret = 0;
	u32 mask = (1L << GPIO_MODE_BITS) - 1;
	struct mt_gpioext_obj *obj = gpioext_obj;

	if (!obj)
		return -ERACCESS;

	if (pin >= MAX_GPIO_PIN)
		return -ERINVAL;

	if (mode >= GPIO_MODE_MAX)
		return -ERINVAL;

	pin -= GPIO_EXTEND_START;
	pos = pin / MAX_GPIO_MODE_PER_REG;
	bit = pin % MAX_GPIO_MODE_PER_REG;


	reg = gpioext_read(&obj->reg->mode[pos].val);
	if (reg < 0) return -ERWRAPPER;

	reg &= ~(mask << (GPIO_MODE_BITS * bit));
	reg |= (mode << (GPIO_MODE_BITS * bit));

	ret = gpioext_write(&obj->reg->mode[pos].val, reg);
	if (ret != 0) return -ERWRAPPER;
	return RSUCCESS;
}
/*---------------------------------------------------------------------------*/
s32 mt_get_gpio_mode_ext(u32 pin)
{
	u32 pos;
	u32 bit;
	s64 reg;
	u32 mask = (1L << GPIO_MODE_BITS) - 1;
	struct mt_gpioext_obj *obj = gpioext_obj;

	if (!obj)
		return -ERACCESS;

	if (pin >= MAX_GPIO_PIN)
		return -ERINVAL;

	pin -= GPIO_EXTEND_START;
	pos = pin / MAX_GPIO_MODE_PER_REG;
	bit = pin % MAX_GPIO_MODE_PER_REG;

	reg = gpioext_read(&obj->reg->mode[pos].val);
	if (reg < 0) return -ERWRAPPER;

	return ((reg >> (GPIO_MODE_BITS * bit)) & mask);
}

#endif

/* set GPIO function in fact */
/*---------------------------------------------------------------------------*/
s32 mt_set_gpio_dir(u32 pin, u32 dir)
{
#if LK_SUPPORT_EXT_GPIO
	return (pin >= GPIO_EXTEND_START) ? mt_set_gpio_dir_ext(pin, dir) : mt_set_gpio_dir_chip(pin, dir);
#else
	return mt_set_gpio_dir_chip(pin, dir);
#endif
}
/*---------------------------------------------------------------------------*/
s32 mt_get_gpio_dir(u32 pin)
{
#if LK_SUPPORT_EXT_GPIO
	return (pin >= GPIO_EXTEND_START) ? mt_get_gpio_dir_ext(pin) : mt_get_gpio_dir_chip(pin);
#else
	return mt_get_gpio_dir_chip(pin);
#endif
}
/*---------------------------------------------------------------------------*/
s32 mt_set_gpio_pull_enable(u32 pin, u32 enable)
{
#if LK_SUPPORT_EXT_GPIO
	return (pin >= GPIO_EXTEND_START) ? mt_set_gpio_pull_enable_ext(pin, enable) : mt_set_gpio_pull_enable_chip(pin, enable);
#else
	return mt_set_gpio_pull_enable_chip(pin, enable);
#endif
}
/*---------------------------------------------------------------------------*/
s32 mt_get_gpio_pull_enable(u32 pin)
{
#if LK_SUPPORT_EXT_GPIO
	return (pin >= GPIO_EXTEND_START) ? mt_get_gpio_pull_enable_ext(pin) : mt_get_gpio_pull_enable_chip(pin);
#else
	return mt_get_gpio_pull_enable_chip(pin);
#endif
}
/*---------------------------------------------------------------------------*/
s32 mt_set_gpio_pull_select(u32 pin, u32 select)
{
#if LK_SUPPORT_EXT_GPIO
	return (pin >= GPIO_EXTEND_START) ? mt_set_gpio_pull_select_ext(pin, select) : mt_set_gpio_pull_select_chip(pin, select);
#else
	return mt_set_gpio_pull_select_chip(pin, select);
#endif
}
/*---------------------------------------------------------------------------*/
s32 mt_get_gpio_pull_select(u32 pin)
{
#if LK_SUPPORT_EXT_GPIO
	return (pin >= GPIO_EXTEND_START) ? mt_get_gpio_pull_select_ext(pin) : mt_get_gpio_pull_select_chip(pin);
#else
	return mt_get_gpio_pull_select_chip(pin);
#endif
}
/*---------------------------------------------------------------------------*/
s32 mt_set_gpio_out(GpioOps *me, u32 output)
{
	MtGpio *gpio = container_of(me, MtGpio, ops);
	unsigned pin = gpio->pin_num;

#if LK_SUPPORT_EXT_GPIO
	return (pin >= GPIO_EXTEND_START) ? mt_set_gpio_out_ext(pin, output) : mt_set_gpio_out_chip(pin, output);
#else
	return mt_set_gpio_out_chip(pin, output);
#endif
}
/*---------------------------------------------------------------------------*/
s32 mt_get_gpio_out(u32 pin)
{
#if LK_SUPPORT_EXT_GPIO
	return (pin >= GPIO_EXTEND_START) ? mt_get_gpio_out_ext(pin) : mt_get_gpio_out_chip(pin);
#else
	return mt_get_gpio_out_chip(pin);
#endif
}
/*---------------------------------------------------------------------------*/
s32 mt_get_gpio_in(GpioOps *me)
{
	MtGpio *gpio = container_of(me, MtGpio, ops);
	unsigned pin = gpio->pin_num;
	int status = 1;

	if (pin == GPIO_LID) {
		if (cros_ec_get_lid_status(&status)) {
			printf("%s: Could not read ChromeOS EC lid status\n", __func__);
			return -ERACCESS;
		}
		return status;
	}
	else if (pin == GPIO_PWRSW) {
		if (cros_ec_get_power_key_status(&status)) {
			printf("%s: Could not read ChromeOS EC power key status\n", __func__);
			return -ERACCESS;
		}
		return status;
	}
#if LK_SUPPORT_EXT_GPIO
	return (pin >= GPIO_EXTEND_START) ? mt_get_gpio_in_ext(pin) : mt_get_gpio_in_chip(pin);
#else
	return mt_get_gpio_in_chip(pin);
#endif
}
/*---------------------------------------------------------------------------*/
s32 mt_set_gpio_mode(u32 pin, u32 mode)
{
#if LK_SUPPORT_EXT_GPIO
	return (pin >= GPIO_EXTEND_START) ? mt_set_gpio_mode_ext(pin, mode) : mt_set_gpio_mode_chip(pin, mode);
#else
	return mt_set_gpio_mode_chip(pin, mode);
#endif
}
/*---------------------------------------------------------------------------*/
s32 mt_get_gpio_mode(u32 pin)
{
#if LK_SUPPORT_EXT_GPIO
	return (pin >= GPIO_EXTEND_START) ? mt_get_gpio_mode_ext(pin) : mt_get_gpio_mode_chip(pin);
#else
	return mt_get_gpio_mode_chip(pin);
#endif
}

static MtGpio *new_mtk_gpio(unsigned pin)
{
	die_if(pin > GPIO_MAX, "Bad GPIO pin number %d.\n", pin);
	MtGpio *gpio = xzalloc(sizeof(*gpio));
	gpio->pin_num = pin;
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



