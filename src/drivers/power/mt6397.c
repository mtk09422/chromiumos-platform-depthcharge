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

#include "base/container_of.h"
#include "drivers/power/mt6397.h"

/* RTC registers */
enum {
	RTC_BASE = 0xE000,
	RTC_BBPU = RTC_BASE + 0x0000,
	RTC_PROT = RTC_BASE + 0x0036,
	RTC_WRTGR = RTC_BASE + 0x003c
};

enum {
	RTC_BBPU_PWREN = 1U << 0,	/* BBPU = 1 when alarm occurs */
	RTC_BBPU_BBPU = 1U << 2,	/* 1: power on, 0: power down */
	RTC_BBPU_AUTO = 1U << 3,	/* BBPU = 0 when xreset_rstb goes low */
	RTC_BBPU_CLRPKY = 1U << 4,
	RTC_BBPU_RELOAD = 1U << 5,
	RTC_BBPU_CBUSY = 1U << 6,
	RTC_BBPU_KEY = 0x43 << 8
};
/* PWRAP registers */
/* For MT8135 */
enum {
	PMIC_WRAP_BASE = 0x1000F000,
	PMIC_WRAP_WACS2_CMD = PMIC_WRAP_BASE + 0xA4,
	PMIC_WRAP_WACS2_RDATA = PMIC_WRAP_BASE + 0xA8,
	PMIC_WRAP_WACS2_VLDCLR = PMIC_WRAP_BASE + 0xAC
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

#define GET_INIT_DONE0(x)		(((x)>>21) & 0x00000001)
#define GET_WACS0_FSM(x)		(((x)>>16) & 0x00000007)
#define GET_WACS0_RDATA(x)		(((x)>>0) & 0x0000ffff)

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

/* debug */
#define power_debug(format...)	\
		printf("mt6397: " format)
#define power_error(format...)	\
		printf("mt6397: ERROR: " format)

static inline uint32_t is_fsm_idle(uint32_t x)
{
	return (GET_WACS0_FSM(x) == WACS_FSM_IDLE);
}

static inline uint32_t is_fsm_vldclr(uint32_t x)
{
	return (GET_WACS0_FSM(x) == WACS_FSM_WFVLDCLR);
}

static inline uint32_t wait_for_state_ready(uint32_t(*fp) (uint32_t),
					    uint32_t timeout_us,
					    uint32_t wacs_register,
					    uint32_t *read_val)
{
	uint32_t reg_rdata;
	uint64_t start = timer_us(0);
	uint32_t timeout_retry = 0;

	do {
		reg_rdata = readl((void *)(uintptr_t)wacs_register);
		if (GET_INIT_DONE0(reg_rdata) != WACS_INIT_DONE) {
			power_error("initialization isn't finished\n");
			return E_PWR_NOT_INIT_DONE;
		}
		if (timer_us(start) > timeout_us) {
			if (timeout_retry) {
				power_error("timeout when waiting for idle\n");
				return E_PWR_WAIT_IDLE_TIMEOUT;
			}
			timeout_retry = 1;
		}
	} while (!fp(reg_rdata));

	if (read_val)
		*read_val = reg_rdata;

	return 0;
}

static int32_t pwrap_read(uint32_t adr, uint32_t *rdata)
{
	int32_t ret;
	uint32_t wacs_cmd = 0;
	uint32_t reg_rdata = 0;

	if ((adr & ~(0xffff)) != 0)
		return E_PWR_INVALID_ADDR;

	if (NULL == rdata) {
		power_error("rdata is a NULL pointer\n");
		return E_PWR_INVALID_ARG;
	}

	ret = wait_for_state_ready(is_fsm_idle,
				   TIMEOUT_WAIT_IDLE, PMIC_WRAP_WACS2_RDATA, 0);
	if (ret)
		return ret;

	wacs_cmd = (0 << 31) | ((adr >> 1) << 16);
	writel(wacs_cmd, (uint32_t *)(uintptr_t)PMIC_WRAP_WACS2_CMD);

	ret = wait_for_state_ready(is_fsm_vldclr,
				   TIMEOUT_READ, PMIC_WRAP_WACS2_RDATA,
				   &reg_rdata);
	if (ret) {
		power_error("wait_for_fsm_vldclr fail, ret=%d\n", ret);
		return E_PWR_WAIT_IDLE_TIMEOUT_READ;
	}

	*rdata = GET_WACS0_RDATA(reg_rdata);
	writel(1,  (uint32_t *)(uintptr_t)PMIC_WRAP_WACS2_VLDCLR);

	return 0;
}

static int32_t pwrap_write(uint32_t adr, uint32_t wdata)
{
	int32_t ret;
	uint32_t wacs_cmd = 0;

	if ((adr & ~(0xffff)) != 0)
		return E_PWR_INVALID_ADDR;

	if ((wdata & ~(0xffff)) != 0)
		return E_PWR_INVALID_WDAT;

	ret = wait_for_state_ready(is_fsm_idle,
				   TIMEOUT_WAIT_IDLE, PMIC_WRAP_WACS2_RDATA, 0);
	if (ret) {
		power_error("pwrap_write fail, ret=%d\n", ret);
		return ret;
	}
	wacs_cmd = (1 << 31) | ((adr >> 1) << 16) | wdata;
	writel(wacs_cmd, (uint32_t *)(uintptr_t) PMIC_WRAP_WACS2_CMD);

	return 0;
}

static uint32_t RTC_Read(uint32_t addr)
{
	uint32_t rdata = 0;
	pwrap_read(addr, &rdata);
	return rdata;
}

static void RTC_Write(uint32_t addr, uint32_t data)
{
	pwrap_write(addr, data);
}

static void rtc_write_trigger(void)
{
	RTC_Write(RTC_WRTGR, 1);
	do {
		while (RTC_Read(RTC_BBPU) & RTC_BBPU_CBUSY);
	} while (0);
}

static void rtc_writeif_unlock(void)
{
	RTC_Write(RTC_PROT, 0x586a);
	rtc_write_trigger();
	RTC_Write(RTC_PROT, 0x9136);
	rtc_write_trigger();
}

static void rtc_bbpu_power_down(void)
{
	uint32_t bbpu;

	/* pull PWRBB low */
	bbpu = RTC_BBPU_KEY | RTC_BBPU_AUTO | RTC_BBPU_PWREN;
	rtc_writeif_unlock();
	RTC_Write(RTC_BBPU, bbpu);
	rtc_write_trigger();
}

static int mt6397_set_reg(Mt6397Pmic *pmic,
			  uint32_t reg, uint32_t val, uint32_t mask,
			  uint32_t shift)
{
	uint32_t ret;
	uint32_t rdata;
	uint32_t wdata;

	ret = pwrap_read(reg, &rdata);
	if (!ret)
		return ret;

	wdata = rdata;
	wdata &= ~(mask << shift);
	wdata |= (val << shift);
	ret = pwrap_write(reg, wdata);
	if (!ret)
		return ret;

	return 0;
}

static int mt6397_get_reg(Mt6397Pmic *pmic,
			  uint32_t reg, uint32_t *val, uint32_t mask,
			  uint32_t shift)
{
	uint32_t ret;
	uint32_t rdata;
	uint32_t outdata;

	ret = pwrap_read(reg, &rdata);
	if (!ret)
		return ret;

	outdata = rdata;
	outdata &= (mask << shift);
	*val = outdata >> shift;

	return 0;
}

/*
  * state1: RG_SYSRSTB_EN = 1, RG_STRUP_MAN_RST_EN=1, RG_RST_PART_SEL=1
  * state2: RG_SYSRSTB_EN = 1, RG_STRUP_MAN_RST_EN=0, RG_RST_PART_SEL=1
  * state3: RG_SYSRSTB_EN = 1, RG_STRUP_MAN_RST_EN=x, RG_RST_PART_SEL=0
  */
static int mt6397_cold_reboot(PowerOps *me)
{
	Mt6397Pmic *pmic = container_of(me, Mt6397Pmic, ops);
	uint8_t rst_mode = pmic->rst_mode;

	switch (rst_mode) {
	case 1:
		mt6397_set_reg(pmic, MT6397_TOP_RST_MISC, 0x1,
			       RG_SYSRSTB_EN_MASK, RG_SYSRSTB_EN_SHIFT);
		mt6397_set_reg(pmic, MT6397_TOP_RST_MISC, 0x1,
			       RG_STRUP_MAN_RST_EN_MASK,
			       RG_STRUP_MAN_RST_EN_SHIFT);
		mt6397_set_reg(pmic, MT6397_TOP_RST_MISC, 0x1,
			       RG_RST_PART_SEL_MASK, RG_RST_PART_SEL_SHIFT);
		break;

	case 2:
		mt6397_set_reg(pmic, MT6397_TOP_RST_MISC, 0x1,
			       RG_SYSRSTB_EN_MASK, RG_SYSRSTB_EN_SHIFT);
		mt6397_set_reg(pmic, MT6397_TOP_RST_MISC, 0x0,
			       RG_STRUP_MAN_RST_EN_MASK,
			       RG_STRUP_MAN_RST_EN_SHIFT);
		mt6397_set_reg(pmic, MT6397_TOP_RST_MISC, 0x1,
			       RG_RST_PART_SEL_MASK, RG_RST_PART_SEL_SHIFT);
		break;

	case 3:
		mt6397_set_reg(pmic, MT6397_TOP_RST_MISC, 0x1,
			       RG_SYSRSTB_EN_MASK, RG_SYSRSTB_EN_SHIFT);
		mt6397_set_reg(pmic, MT6397_TOP_RST_MISC, 0x0,
			       RG_RST_PART_SEL_MASK, RG_RST_PART_SEL_SHIFT);
		break;

	default:
		power_error("Invalid rest mode(%d)\n", rst_mode);
		break;
	}

	return 0;
}

static int mt6397_power_off(PowerOps *me)
{
	rtc_bbpu_power_down();
	halt();
}

Mt6397Pmic *new_mt6397_pmic(uint8_t rst_mode, void *data)
{
	Mt6397Pmic *pmic = xzalloc(sizeof(*pmic));
	pmic->ops.cold_reboot = &mt6397_cold_reboot;
	pmic->ops.power_off = &mt6397_power_off;
	pmic->set_reg = mt6397_set_reg;
	pmic->get_reg = mt6397_get_reg;
	pmic->rst_mode = rst_mode;
	pmic->priv_data = data;
	return pmic;
}
