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

#ifndef __DRIVERS_FLASH_MTK_NOR_H__
#define __DRIVERS_FLASH_MTK_NOR_H__

#include <stdint.h>
#include "config.h"
#include "drivers/storage/blockdev.h"

#include "drivers/flash/flash.h"


typedef struct {
	uint32_t sflash_cmd;
	uint32_t sflash_cnt;
	uint32_t sflash_rdsr;
	uint32_t sflash_rdata;
	uint32_t sflash_radr0;
	uint32_t sflash_radr1;
	uint32_t sflash_radr2;
	uint32_t sflash_wdata;
	uint32_t sflash_prgdata0;
	uint32_t sflash_prgdata1;
	uint32_t sflash_prgdata2;
	uint32_t sflash_prgdata3;
	uint32_t sflash_prgdata4;
	uint32_t sflash_prgdata5;
} MtkNorReg;



//typedef struct NorCtrlr {
//       BlockDevCtrlr ctrlr;
//       uint32_t bus_width;
//int (*send_cmd)(struct MmcCtrlr *me, MmcCommand *cmd, MmcData *data);
//void (*set_ios)(struct MmcCtrlr *me);
//} NorCtrlr;
/*

   typedef struct {
        NorCtrlr nor;
        MtkNorReg *reg;
        int initialized;
        int removable;
   } MtkNorHost;
 */

typedef struct MtkNorFlash {
	FlashOps ops;
	//MtkNorHost *nor_host;
	uint32_t rom_size;
	uint8_t *buffer;
} MtkNorFlash;



MtkNorFlash *new_mtk_nor_flash(uint32_t rom_size);
//extern MtkNorHost *new_mtk_nor_host(uintptr_t ioaddr, int bus_width, int removable);

#endif /* __DRIVERS_FLASH_MTK_EMMC_H__ */
