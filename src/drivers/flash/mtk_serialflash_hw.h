/*
 * This file is part of the coreboot project.
 *
 * Copyright 2014 MediaTek Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef SERIAL_FLASH_HW_H
#define SERIAL_FLASH_HW_H

#include "drivers/storage/blockdev.h"
#include "config.h"

#include <assert.h>
#include <stdint.h>
#include <endian.h>
#include <stddef.h>
#include <stdlib.h>

#define BD_NOR_ENABLE

typedef unsigned char UINT8;
typedef unsigned short UINT16;
typedef unsigned int UINT32;
typedef unsigned long long UINT64;
typedef unsigned short USHORT;
typedef signed char INT8;
typedef signed short INT16;
typedef signed int INT32;

/* register definitions */

#define ENABLE_DUALREAD
#define SFLASH_MEM_BASE         ((UINT32)0x30000000) //mmu on
#define SFLASH_REG_BASE         ((UINT32)(0x11000000 + 0xD000))

#define SFLASH_CMD_REG          ((UINT32)0x00)
#define SFLASH_CNT_REG          ((UINT32)0x04)
#define SFLASH_RDSR_REG         ((UINT32)0x08)
#define SFLASH_RDATA_REG        ((UINT32)0x0C)

#define SFLASH_RADR0_REG        ((UINT32)0x10)
#define SFLASH_RADR1_REG        ((UINT32)0x14)
#define SFLASH_RADR2_REG        ((UINT32)0x18)
#define SFLASH_WDATA_REG        ((UINT32)0x1C)
#define SFLASH_PRGDATA0_REG     ((UINT32)0x20)
#define SFLASH_PRGDATA1_REG     ((UINT32)0x24)
#define SFLASH_PRGDATA2_REG     ((UINT32)0x28)
#define SFLASH_PRGDATA3_REG     ((UINT32)0x2C)
#define SFLASH_PRGDATA4_REG     ((UINT32)0x30)
#define SFLASH_PRGDATA5_REG     ((UINT32)0x34)
#define SFLASH_SHREG0_REG       ((UINT32)0x38)
#define SFLASH_SHREG1_REG       ((UINT32)0x3C)
#define SFLASH_SHREG2_REG       ((UINT32)0x40)
#define SFLASH_SHREG3_REG       ((UINT32)0x44)
#define SFLASH_SHREG4_REG       ((UINT32)0x48)
#define SFLASH_SHREG5_REG       ((UINT32)0x4C)
#define SFLASH_SHREG6_REG       ((UINT32)0x50)
#define SFLASH_SHREG7_REG       ((UINT32)0x54)
#define SFLASH_SHREG8_REG       ((UINT32)0x58)
#define SFLASH_SHREG9_REG       ((UINT32)0x5C)
#define SFLASH_FLHCFG_REG       ((UINT32)0x84)
#define SFLASH_PP_DATA_REG      ((UINT32)0x98)
#define SFLASH_PREBUF_STUS_REG  ((UINT32)0x9C)
#define SFLASH_SF_INTRSTUS_REG  ((UINT32)0xA8)
#define SFLASH_SF_INTREN_REG    ((UINT32)0xAC)
#define SFLASH_SF_TIME_REG      ((UINT32)0x94)
#define SFLASH_CHKSUM_CTL_REG   ((UINT32)0xB8)
#define SFLASH_CHECKSUM_REG     ((UINT32)0xBC)
#define SFLASH_CMD2_REG         ((UINT32)0xC0)
#define SFLASH_WRPROT_REG       ((UINT32)0xC4)
#define SFLASH_RADR3_REG        ((UINT32)0xC8)
#define SFLASH_READ_DUAL_REG    ((UINT32)0xCC)
#define SFLASH_DELSEL0_REG      ((UINT32)0xA0)
#define SFLASH_DELSEL1_REG      ((UINT32)0xA4)
#define SFLASH_DELSEL2_REG      ((UINT32)0xD0)
#define SFLASH_DELSEL3_REG      ((UINT32)0xD4)
#define SFLASH_DELSEL4_REG      ((UINT32)0xD8)

#define SFLASH_CFG1_REG         ((UINT32)0x60)
#define SFLASH_CFG2_REG         ((UINT32)0x64)
#define SFLASH_CFG3_REG         ((UINT32)0x68)
#define SFLASH_STATUS0_REG      ((UINT32)0x70)
#define SFLASH_STATUS1_REG      ((UINT32)0x74)
#define SFLASH_STATUS2_REG      ((UINT32)0x78)
#define SFLASH_STATUS3_REG      ((UINT32)0x7C)


#define SFLASH_WRBUF_SIZE       128
#define SFLASHHWNAME_LEN    48

#ifndef BOOL
typedef unsigned char BOOL;
#endif

typedef struct {
	UINT8 u1MenuID;
	UINT8 u1DevID1;
	UINT8 u1DevID2;
	UINT8 u1PPType;
	UINT32 u4ChipSize;
	UINT32 u4SecSize;
	UINT32 u4CEraseTimeoutMSec;

	UINT8 u1WRENCmd;
	UINT8 u1WRDICmd;
	UINT8 u1RDSRCmd;
	UINT8 u1WRSRCmd;
	UINT8 u1READCmd;
	UINT8 u1FASTREADCmd;
	UINT8 u1READIDCmd;
	UINT8 u1SECERASECmd;
	UINT8 u1CHIPERASECmd;
	UINT8 u1PAGEPROGRAMCmd;
	UINT8 u1AAIPROGRAMCmd;
	UINT8 u1DualREADCmd;
	UINT8 u1Protection;
	char pcFlashStr[SFLASHHWNAME_LEN];
} SFLASHHW_VENDOR_T;

extern int __nor_debug;
extern int __nor_trace;

#define nor_debug(format ...) \
	while (__nor_debug) { printf("nor: " format); break; }
 #define mmc_trace(format ...) \
	while (__nor_trace) { printf(format); break; }
#define nor_error(format ...) printf("nor: ERROR: " format)



/* Inter-file functions  */

#define IO_BASE         0x70000000
#define BIM_BASE        (IO_BASE + 0x08000) //fixme here!!!!!!!!!

#define BIM_READ8(offset)               readb((void*)(BIM_BASE + offset))
#define BIM_READ16(offset)              readw((void*)(BIM_BASE + offset))
#define BIM_READ32(offset)              readl((void*)(BIM_BASE + offset))

#define BIM_WRITE8(offset, value)               writeb((value), ((void*))(BIM_BASE + offset))
#define BIM_WRITE16(offset, value)              writew((value), (void*)(BIM_BASE + offset))
#define BIM_WRITE32(offset, value)              writel((value), (void*)(BIM_BASE + offset))

#ifndef MAX_FLASHCOUNT
#define MAX_FLASHCOUNT  1
#endif

#ifndef SFLASHNAME_LEN
#define SFLASHNAME_LEN  48
#endif



typedef struct {
	UINT32 u4Seconds;               //Number of seconds from startup
	UINT32 u4Micros;                //Remainder in microsecond
} HAL_TIME_T;

#define OSR_TIMEOUT               ((INT32)-3)
typedef struct {
	UINT8 u1MenuID;
	UINT8 u1DevID1;
	UINT8 u1DevID2;
	UINT32 u4ChipSize;
	UINT32 u4SecSize;
	UINT32 u4SecCount;

	char pcFlashStr[SFLASHNAME_LEN];
} SFLASH_CHIPINFO_T;

typedef struct {
	UINT8 u1FlashCount;
	SFLASH_CHIPINFO_T arFlashInfo[MAX_FLASHCOUNT];
} SFLASH_INFO_T;


/* Public functions */

extern void SF_NOR_GPIO_INIT(void);
extern void SF_NOR_GPIO_DEINIT(void);
//extern int nor_bread(struct blkdev * bdev, u32 blknr, u32 blks, u8 * buf, u32 part_id);
//extern int nor_bwrite(struct blkdev * bdev, u32 blknr, u32 blks, u8 * buf, u32 part_id);
extern INT32 SFLASH_Lock(void);
extern INT32 SFLASH_UnLock(void);
extern INT32 SFLASHHW_Init(void);



extern INT32 SFLASHHW_GetID(UINT32 u4Index, UINT8 *pu1MenuID,
			    UINT8 *pu1DevID1, UINT8 *pu1DevID2);
extern void SFLASHHW_GetFlashInfo(SFLASH_INFO_T *prInfo);

extern INT32 SFLASHHW_Read(UINT32 u4Addr, UINT32 u4len, UINT8* pu1buf);

extern INT32 SFLASHHW_WriteSector(UINT32 u4Index, UINT32 u4Addr,
				  UINT32 u4Len, const UINT8* pu1Buf);

extern INT32 SFLASHHW_EraseSector(UINT32 u1Index, UINT32 u4Addr);

extern INT32 SFLASHHW_EraseChip(UINT32 u4Index);


extern void SFLASHHW_SetClk(UINT32 u4Val);
extern UINT32 SFLASHHW_GetClk(void);
extern void SFLASHHW_SetSampleEdge(UINT32 u4Val);
extern UINT32 SFLASHHW_GetSampleEdge(void);

extern INT32 SFLASHHW_ReadFlashStatus(UINT32 u4Index, UINT8 *pu1Val);
extern INT32 SFLASHHW_WriteProtect(UINT32 u4Index, BOOL fgEnable);

extern char* SFLASHHW_GetFlashName(UINT8 u1MenuID, UINT8 u1DevID1, UINT8 u1DevID2);

extern INT32 SFLASHHW_EnableDMA(UINT32 u4SrcAddr, UINT32 u4DestAddr, UINT32 u4Size);

extern UINT32 SFLASHHW_GetFlashSize(UINT8 u1MenuID, UINT8 u1DevID1, UINT8 u1DevID2);

extern void SFLASHHW_WriteSfProtect(UINT32 u4Val);
extern UINT32 SFLASHHW_ReadSfProtect(void);
extern void SFLASHHW_WriteReg(UINT32 uAddr, UINT32 u4Val);
extern UINT32 SFLASHHW_ReadReg(UINT32 uAddr);
extern UINT32 SFLASHHW_SwitchChip(UINT32 uAddr);



#endif

