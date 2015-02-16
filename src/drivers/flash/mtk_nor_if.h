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

#ifndef NOR_IF_H
#define NOR_IF_H
#include <stdint.h>


#define EXTERN  extern
typedef unsigned char UINT8;
typedef unsigned short UINT16;
typedef unsigned int UINT32;
typedef unsigned long long UINT64;
typedef unsigned short USHORT;
typedef signed char INT8;
typedef signed short INT16;
typedef signed int INT32;

#define MAX_NOR_PART  8


#ifndef BOOL
typedef unsigned char BOOL;
#endif

#define IO_READ8(base, offset)            ((*(volatile unsigned int*)(uintptr_t)(base + offset)) & 0xFF)
#define IO_WRITE8(offset, value)          (*(volatile unsigned int*)(uintptr_t)(offset)) = (value)

#define SFLASHNAME_LEN  48

#define MAX_FLASHCOUNT  1
#define SFLASHHWNAME_LEN    48

extern u32 nor_init_device(void);
extern INT32 NOR_Init(void);
extern INT32 NOR_Read(UINT64 u8Offset, UINT32 u4MemPtr, UINT32 u4MemLen);
extern INT32 NOR_Write(UINT32 u8Offset, UINT32 u4MemPtr, UINT32 u4ByteCount);
extern INT32 NOR_WriteByte(UINT32 u4Offset, UINT32 u4Data);
extern INT32 NOR_EraseChip(UINT32 u4FlashIndex);
extern INT32 NOR_Erase(UINT32 u4DevId, UINT32 u4SectIdx, UINT32 u4SectNum);
extern INT32 NOR_EraseAddr(UINT32 u4Offset, UINT32 u4ByteCount);

/* defined at serialflash_func.c */

extern INT32 SFLASH_Init(void);
extern INT32 SFLASH_GetID(UINT32 u4Index, UINT8 *pu1MenuID, UINT8 *pu1DevID1, UINT8 *pu1DevID2);
extern INT32 SFLASH_Read(UINT32 u4Offset, UINT64 u4MemPtr, UINT32 u4ByteCount);
extern INT32 SFLASH_Write(UINT32 u4Offset, UINT64 u4MemPtr, UINT32 u4ByteCount);
extern INT32 SFLASH_WriteOneByte(UINT32 u4Addr, UINT8 u1Data);
extern INT32 SFLASH_EraseAddr(UINT32 u4Offset, UINT32 u4ByteLen);
extern INT32 SFLASH_EraseChip(UINT32 u4Index);
extern INT32 SFLASH_ReadFlashStatus(UINT32 u4Index, UINT8* pu1Status);
extern INT32 SFLASH_WriteProtect(UINT32 u4Index, BOOL fgEnable);
extern char* SFLASH_GetFlashName(UINT8 u1MenuID, UINT8 u1DevID1, UINT8 u1DevID2);


#endif /* DRV_NOR_H */

