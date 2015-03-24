/*
 * Copyright 2012 Google Inc.
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

#ifndef __DRIVERS_POWER_MT6397_H__
#define __DRIVERS_POWER_MT6397_H__

#include <stdint.h>

#include "drivers/power/power.h"

enum {
	RTC_BBPU_PWREN = 1U << 0,	/* BBPU = 1 when alarm occurs */
	RTC_BBPU_BBPU = 1U << 2,	/* 1: power on, 0: power down */
	RTC_BBPU_AUTO = 1U << 3,	/* BBPU = 0 when xreset_rstb goes low */
	RTC_BBPU_CLRPKY = 1U << 4,
	RTC_BBPU_RELOAD = 1U << 5,
	RTC_BBPU_CBUSY = 1U << 6,
	RTC_BBPU_KEY = 0x43 << 8
};

/* macro for WACS_FSM */
enum {
	WACS_FSM_IDLE = 0x00,
	WACS_FSM_REQ = 0x02,
	WACS_FSM_WFDLE = 0x04,
	WACS_FSM_WFVLDCLR = 0x06,
	WACS_INIT_DONE = 0x01,
	WACS_SYNC_IDLE = 0x01,
	WACS_SYNC_BUSY = 0x00,

	TIMEOUT_READ = 10000,
	TIMEOUT_WAIT_IDLE = 10000,
};

/* Errors */
enum {
	E_PWR_INVALID_ARG = 1,
	E_PWR_INVALID_RW,
	E_PWR_INVALID_ADDR,
	E_PWR_INVALID_WDAT,
	E_PWR_INVALID_OP_MANUAL,
	E_PWR_NOT_IDLE_STATE,
	E_PWR_NOT_INIT_DONE,
	E_PWR_NOT_INIT_DONE_READ,
	E_PWR_WAIT_IDLE_TIMEOUT,
	E_PWR_WAIT_IDLE_TIMEOUT_READ
};

/* PMIC RST control type */
enum {
	MT6397_TOP_RST_MISC = 0x0126,
	RG_SYSRSTB_EN_MASK = 1,
	RG_SYSRSTB_EN_SHIFT = 1,
	RG_STRUP_MAN_RST_EN_MASK = 1,
	RG_STRUP_MAN_RST_EN_SHIFT = 2,
	RG_RST_PART_SEL_MASK = 1,
	RG_RST_PART_SEL_SHIFT = 4
};

enum {
	RTC_PROT_UNLOCK_1 = 0x586a,
	RTC_PROT_UNLOCK_2 = 0x9136
};

/*WDT_MODE*/
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

/*WDT_RESTART*/
enum {
	MTK_WDT_RESTART_KEY = 0x1971
};

/*WDT_SWRST*/
enum {
	MTK_WDT_SWRST_KEY = 0x1209
};

/* DIGLDO Register Definition */
enum{
	DIGLDO_CON0 = 0x0410,
	DIGLDO_CON1 = 0x0412,
	DIGLDO_CON2 = 0x0414,
	DIGLDO_CON3 = 0x0416,
	DIGLDO_CON4 = 0x0418,
	DIGLDO_CON5 = 0x041A,
	DIGLDO_CON6 = 0x041C,
	DIGLDO_CON7 = 0x041E,
	DIGLDO_CON8 = 0x0420,
	DIGLDO_CON9 = 0x0422,
	DIGLDO_CON10  = 0x0424,
	DIGLDO_CON11  = 0x0426,
	DIGLDO_CON12  = 0x0428,
	DIGLDO_CON13  = 0x042A,
	DIGLDO_CON14  = 0x042C,
	DIGLDO_CON15  = 0x042E,
	DIGLDO_CON16  = 0x0430,
	DIGLDO_CON17  = 0x0432,
	DIGLDO_CON18  = 0x0434,
	DIGLDO_CON19  = 0x0436,
	DIGLDO_CON20  = 0x0438,
	DIGLDO_CON21  = 0x043A,
	DIGLDO_CON22  = 0x043C,
	DIGLDO_CON23  = 0x043E,
	DIGLDO_CON24  = 0x0440,
	DIGLDO_CON25  = 0x0442,
	DIGLDO_CON26  = 0x0444,
	DIGLDO_CON27  = 0x0446,
	DIGLDO_CON28  = 0x0448,
	DIGLDO_CON29  = 0x044A,
	DIGLDO_CON30  = 0x044C,
	DIGLDO_CON31  = 0x044E,
	DIGLDO_CON32  = 0x0450,
	DIGLDO_CON33  = 0x45A
};

/* DIGLDO MASK and SHIFT Definition */
enum{
	PMIC_RG_VCAMD_SW_EN_MASK = 0x1,
	PMIC_RG_VCAMD_SW_EN_SHIFT = 15,
	PMIC_RG_VGP4_SW_EN_MASK = 0x1,
	PMIC_RG_VGP4_SW_EN_SHIFT = 15,
	PMIC_RG_VCAMD_VOSEL_MASK = 0x7,
	PMIC_RG_VCAMD_VOSEL_SHIFT = 5,
	PMIC_RG_VGP4_VOSEL_MASK = 0x7,
	PMIC_RG_VGP4_VOSEL_SHIFT = 5,
	PMIC_VCAMD_ON_CTRL_MASK = 0x1,
	PMIC_VCAMD_ON_CTRL_SHIFT = 15,
	PMIC_VGP4_ON_CTRL_MASK = 0x1,
	PMIC_VGP4_ON_CTRL_SHIFT = 12,
};

typedef struct Mt6397Pmic {
	PowerOps ops;
	int (*set_reg)(struct Mt6397Pmic *me, uint32_t reg, uint32_t val,
		       uint32_t mask, uint32_t shift);
	int (*get_reg)(struct Mt6397Pmic *me, uint32_t reg, uint32_t *val,
		       uint32_t mask, uint32_t shift);
	void *wdt_base;
	void *wacs_base;
	void *rtc_base;
} Mt6397Pmic;

Mt6397Pmic *new_mt6397_power(uintptr_t pwrap_reg_addr, uintptr_t wdt_reg_addr,
			    uintptr_t rtc_reg_addr);

#endif /* __DRIVERS_POWER_MT6397_H__ */
