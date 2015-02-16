/*
 * Copyright 2015 MediaTek Inc.
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

/* debug */
#define power_error(format ...)	printf("mt6397: ERROR: " format)

typedef struct {
	u32	mode;
	u32	length;
	u32	restart;
	u32	status;
	u32	interval;
	u32	swrst;
	u32	swsysrst;
} MtkWdtReg;

typedef struct {
	u32	reserved1[40];
	u32	cmd;		/* 0xA0 */
	u32	rdata;		/* 0xA4 */
	u32	vldclr;		/* 0xA8 */
} MtkPwrapReg;

enum {
	bbpu = 0x00,
	prot = 0x36,
	wrtgr = 0x3c,
};

static inline uint32_t get_init_done0(uint32_t x)
{
	return (((x)>>21) & 0x00000001);
}

static inline uint32_t get_wacs0_fsm(uint32_t x)
{
	return (((x)>>16) & 0x00000007);
}

static inline uint32_t get_wacs0_rdata(uint32_t x)
{
	return (((x)>>0) & 0x0000ffff);
}

static inline uint32_t wait_for_state_ready(uint32_t expected_wacs_fsm,
					    uint32_t timeout_us,
					    uint32_t *wacs_register,
					    uint32_t *read_val)
{
	uint32_t reg_rdata;
	uint64_t start = timer_us(0);
	uint32_t timeout_retry = 0;

	do {
		reg_rdata = readl((void *)(uintptr_t)wacs_register);
		if (get_init_done0(reg_rdata) != WACS_INIT_DONE) {
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
	} while (get_wacs0_fsm(reg_rdata) != expected_wacs_fsm);

	if (read_val)
		*read_val = reg_rdata;

	return 0;
}

static int32_t pwrap_read(Mt6397Pmic *pmic, uint32_t adr, uint32_t *rdata)
{
	int32_t ret;
	uint32_t wacs_cmd = 0;
	uint32_t reg_rdata = 0;
	MtkPwrapReg *wacs_regs = pmic->wacs_base;

	ret = wait_for_state_ready(WACS_FSM_IDLE, TIMEOUT_WAIT_IDLE,
				   &wacs_regs->rdata, 0);
	if (ret) {
		power_error("WACS_FSM_IDLE fail, ret=%d\n", ret);
		return ret;
	}
	wacs_cmd = (0 << 31) | ((adr >> 1) << 16);
	writel(wacs_cmd, &wacs_regs->cmd);

	ret = wait_for_state_ready(WACS_FSM_WFVLDCLR, TIMEOUT_READ,
				   &wacs_regs->rdata, &reg_rdata);
	if (ret) {
		power_error("WACS_FSM_WFVLDCLR failed, ret=%d\n", ret);
		return E_PWR_WAIT_IDLE_TIMEOUT_READ;
	}

	*rdata = get_wacs0_rdata(reg_rdata);
	writel(1, &wacs_regs->vldclr);

	return 0;
}

static int32_t pwrap_write(Mt6397Pmic *pmic, uint32_t adr, uint32_t wdata)
{
	int32_t ret;
	uint32_t wacs_cmd = 0;
	MtkPwrapReg *wacs_regs = pmic->wacs_base;

	ret = wait_for_state_ready(WACS_FSM_IDLE, TIMEOUT_WAIT_IDLE,
				   &wacs_regs->rdata, 0);
	if (ret) {
		power_error("%s failed, ret=%d\n", __func__, ret);
		return ret;
	}

	wacs_cmd |= (1 << 31) | ((adr >> 1) << 16) | wdata;
	writel(wacs_cmd, &wacs_regs->cmd);

	return 0;
}

static void rtc_write_trigger(Mt6397Pmic *pmic)
{
	uint32_t rdata;

	pwrap_write(pmic, (uintptr_t)(pmic->rtc_base + wrtgr), 1);

	do {
		pwrap_read(pmic, (uintptr_t)(pmic->rtc_base + bbpu), &rdata);
	} while (rdata & RTC_BBPU_CBUSY);
}

/*
 * It's unlock scheme to prevent RTC miswriting. The RTC write interface is
 * protected by RTC_PROT. Whether the RTC writing interface is enabled or not
 * is decided by RTC_PROT contents. Users have to perform unlock flow to enable
 * the writing interface.
 */

static void rtc_writeif_unlock(Mt6397Pmic *pmic)
{
	pwrap_write(pmic, (uintptr_t)(pmic->rtc_base + prot), RTC_PROT_UNLOCK_1);
	rtc_write_trigger(pmic);
	pwrap_write(pmic, (uintptr_t)(pmic->rtc_base + prot), RTC_PROT_UNLOCK_2);
	rtc_write_trigger(pmic);
}

static void rtc_bbpu_power_down(Mt6397Pmic *pmic)
{
	rtc_writeif_unlock(pmic);
	pwrap_write(pmic, (uintptr_t)(pmic->rtc_base + bbpu),
		    RTC_BBPU_KEY | RTC_BBPU_AUTO | RTC_BBPU_PWREN);
	rtc_write_trigger(pmic);
}

static int mt6397_set_reg(Mt6397Pmic *pmic,
			  uint32_t reg, uint32_t val, uint32_t mask,
			  uint32_t shift)
{
	uint32_t ret;
	uint32_t rdata;
	uint32_t wdata;

	ret = pwrap_read(pmic, reg, &rdata);
	if (ret)
		return ret;

	wdata = rdata;
	wdata &= ~(mask << shift);
	wdata |= (val << shift);

	return pwrap_write(pmic, reg, wdata);
}

static int mt6397_cold_reboot(PowerOps *me)
{
	Mt6397Pmic *pmic = container_of(me, Mt6397Pmic, ops);
	MtkWdtReg *wdt_regs = pmic->wdt_base;

	/* Set PMIC reboot mode to cold reboot */
	mt6397_set_reg(pmic, MT6397_TOP_RST_MISC, 0x1,
		       RG_SYSRSTB_EN_MASK, RG_SYSRSTB_EN_SHIFT);
	mt6397_set_reg(pmic, MT6397_TOP_RST_MISC, 0x1,
		       RG_STRUP_MAN_RST_EN_MASK, RG_STRUP_MAN_RST_EN_SHIFT);
	mt6397_set_reg(pmic, MT6397_TOP_RST_MISC, 0x1,
		       RG_RST_PART_SEL_MASK, RG_RST_PART_SEL_SHIFT);

	writel(MTK_WDT_RESTART_KEY, &wdt_regs->restart);
	clrsetbits_le32(&wdt_regs->mode,
			MTK_WDT_MODE_DUAL_MODE | MTK_WDT_MODE_IRQ,
			(MTK_WDT_MODE_KEY | MTK_WDT_MODE_EXTEN |
			 MTK_WDT_MODE_AUTO_RESTART));
	/*
	 * Watchdog will trigger reset signal to PMIC as soon as
	 * clrsetbits_le32() executes. But write register MTK_WDT_MODE has
	 * latency, so we add delay here.
	 */
	mdelay(5);
	writel(MTK_WDT_SWRST_KEY, &wdt_regs->swrst);

	while (1);

	return -1;
}

static int mt6397_power_off(PowerOps *me)
{
	Mt6397Pmic *pmic = container_of(me, Mt6397Pmic, ops);

	rtc_bbpu_power_down(pmic);

	while (1);

	return -1;
}

Mt6397Pmic *new_mt6397_power(uintptr_t pwrap_reg_addr, uintptr_t wdt_reg_addr,
			    uintptr_t rtc_reg_addr)
{
	Mt6397Pmic *pmic = xzalloc(sizeof(*pmic));

	pmic->ops.power_off = &mt6397_power_off;
	pmic->ops.cold_reboot = &mt6397_cold_reboot;
	pmic->wdt_base = (void *)wdt_reg_addr;
	pmic->wacs_base = (void *)pwrap_reg_addr;
	pmic->rtc_base = (void *)rtc_reg_addr;

	return pmic;
}
