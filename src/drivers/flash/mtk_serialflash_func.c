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

#include <string.h>

#include <assert.h>
#include <libpayload.h>
#include <stddef.h>
#include <stdint.h>

#include "mtk_nor_if.h"
#include "typedefs.h"
#include "mtk_serialflash_hw.h"

#define NOR_SAVE_ERROR_LOG 1

/* Constant definitions */


#define DEBUG_NOR(level, x ...)          printk(level, x)


typedef struct {
	uint32_t u4RegVal;
	char acName[25];
} SFLASH_TESTITEM_T;
#ifndef NULL
#define NULL 0
#endif

/* The information of each erase region of all chips. Each entry contains the sector size and the number of sectors of each  erase region.*/

static SFLASH_INFO_T _rSFInfo;

#define TRUE                   1
#define FALSE                  0

static BOOL _fgInit = FALSE;

/* Brief of SFLASH_Lock.  Details of SFLASH_Lock ().  @retval 0   Success @retval 1   Fail */
INT32 SFLASH_Lock(void)
{
	SFLASHHW_WriteSfProtect(0x00);
	if (SFLASHHW_ReadSfProtect()) {
		return 1;
	}else {
		return 0;
	}
}

/** Brief of SFLASH_UnLock.  Details of SFLASH_UnLock ().@retval 0   Success @retval 1   Fail*/
INT32 SFLASH_UnLock(void)
{
	SFLASHHW_WriteSfProtect(0x30);
	if (SFLASHHW_ReadSfProtect()) {
		return 0;
	}else {
		return 1;
	}
}

/* Brief of SFLASH_Init. Details of SFLASH_Init (optional). @retval 0    Success  @retval 1   Fail*/

INT32 SFLASH_Init()
{
	static BOOL fgInit = FALSE;

	fgInit = _fgInit;

	if (fgInit) {
		return 0;
	}
	SFLASH_UnLock();
	memset((void*)&_rSFInfo, 0x0, sizeof(SFLASH_INFO_T));
	if (SFLASHHW_Init() != 0) {
		return -1;
	}
	SFLASHHW_GetFlashInfo(&_rSFInfo);
	_fgInit = TRUE;

	return 0;
}

/* Brief of SFLASH_GetID. Details of SFLASH_GetID (optional).  @retval 0    Success  @retval 1   Fail*/

INT32 SFLASH_GetID(uint32_t u4Index, UINT8 *pu1MenuID, UINT8 *pu1DevID1, UINT8 *pu1DevID2)
{
	INT32 i4Ret;

	if ((pu1MenuID == NULL) || (pu1DevID1 == NULL) || (pu1DevID2 == NULL)) {
		return -1;
	}
	i4Ret = SFLASHHW_GetID(u4Index, pu1MenuID, pu1DevID1, pu1DevID2);
	return i4Ret;
}

/* Brief of SFLASH_Write.  Details of SFLASH_Write (optional).  @retval TRUE    Success  @retval FALSE   Fail*/
INT32 SFLASH_Write(uint32_t u4Offset, uint64_t u4MemPtr, uint32_t u4ByteCount)
{
	uint8_t *pu1Buf;
	uint32_t i, j, u4WriteAddr, u4Len, u4WriteByte, u4ChipOffset, u4SectStart, u4SectEnd;

	if (SFLASH_Init() != 0) {
		return -1;
	}
	pu1Buf = (uint8_t*)u4MemPtr; //for build pass
	u4WriteAddr = u4Offset;
	u4WriteByte = u4ByteCount;
	u4ChipOffset = 0;
	u4SectStart = 0;
	u4SectEnd = 0;

	for (i = 0; i < _rSFInfo.u1FlashCount; i++) {
		if (SFLASH_WriteProtect(i, FALSE) != 0) {
			nor_debug("Disable Flash write protect fail!\n");
			return -1;
		}
		for (j = 0; j < _rSFInfo.arFlashInfo[i].u4SecCount; j++) {
			u4SectEnd = u4SectStart + _rSFInfo.arFlashInfo[i].u4SecSize;
			if ((u4SectStart <= u4WriteAddr) && (u4WriteAddr < u4SectEnd)) {
				u4Len = _rSFInfo.arFlashInfo[i].u4SecSize - (u4WriteAddr - u4SectStart);
				if (u4Len >= u4WriteByte) {
					u4Len = u4WriteByte;
				}
				if (SFLASHHW_WriteSector(i, u4WriteAddr - u4ChipOffset, u4Len, pu1Buf) != 0) {
					nor_debug("Write flash error !\n");
					if (SFLASH_WriteProtect(i, TRUE) != 0) {
						nor_debug("Enable flash write protect fail!\n");
						return -1;
					}
					return -1;
				}
				u4WriteAddr += u4Len;
				u4WriteByte -= u4Len;
				pu1Buf += u4Len;

				if (u4WriteByte == 0) {
					break;
				}
			}
			u4SectStart += _rSFInfo.arFlashInfo[i].u4SecSize;
		}
		u4ChipOffset += _rSFInfo.arFlashInfo[i].u4ChipSize;
		if (SFLASH_WriteProtect(i, TRUE) != 0) {
			nor_debug("Enable flash write protect fail!\n");
			return -1;
		}
	}
	return 0;
}


/** Brief of SFLASH_WriteOneByte  Details of SFLASH_WriteOneByte (optional).  @retval 0   Success  @retval 1   Fail*/
INT32 SFLASH_WriteOneByte(uint32_t u4Addr, UINT8 u1Data)
{
	UINT8 u1Val = 0;

	if (SFLASH_Init() != 0) {
		return -1;
	}

	if (SFLASH_Read(u4Addr, (uint64_t)&u1Val, 1) != 0) {
		nor_debug("Read flash fail!\n");
		return -1;
	}

	if (u1Val == u1Data) {
		return 0;
	}
	if (((~u1Val) & u1Data) != 0) {
		nor_debug("Erase first !\n");
		return -1;
	}
	if (SFLASH_Write(u4Addr, (uint64_t)&u1Data, 1) != 0) {
		return -1;
	}
	return 0;
}

/** Brief of SFLASH_EraseAddr.
 *  Details of SFLASH_EraseAddr (optional).
 *  @retval TRUE    Success
 *  @retval FALSE   Fail
 */
INT32 SFLASH_EraseAddr(uint32_t u4Offset, uint32_t u4ByteLen)
{
	uint32_t i, j, u4EraseAddr, u4ChipOffset, u4SectStart, u4SectEnd;

	if (SFLASH_Init() != 0) {
		return -1;
	}
	u4EraseAddr = u4Offset;
	u4ChipOffset = 0;
	u4SectStart = 0;
	u4SectEnd = 0;

	for (i = 0; i < _rSFInfo.u1FlashCount; i++) {
		if (SFLASH_WriteProtect(i, FALSE) != 0) {
			nor_debug("Disable Flash write protect fail!\n");
			return -1;
		}

		for (j = 0; j < _rSFInfo.arFlashInfo[i].u4SecCount; j++) {
			u4SectEnd = u4SectStart + _rSFInfo.arFlashInfo[i].u4SecSize;
			if ((u4SectStart <= u4EraseAddr) && (u4EraseAddr < u4SectEnd)) {
				if (SFLASHHW_EraseSector(i, u4SectStart - u4ChipOffset) != 0) {
					nor_debug("Erase chip #%d, sector 0x%08X ~ 0x%08X FAIL\n", i,
						  u4SectStart - u4ChipOffset,
						  (u4SectStart - u4ChipOffset) + (_rSFInfo.arFlashInfo[i].u4SecSize - 1));

					if (SFLASH_WriteProtect(i, TRUE) != 0) {
						nor_debug("Enable Flash write protect fail!\n");
						return -1;
					}
					return -1;
				}else {
					nor_debug("Erase chip #%d, sector 0x%08X ~ 0x%08X OK\n", i, u4SectStart - u4ChipOffset, (u4SectStart - u4ChipOffset) + (_rSFInfo.arFlashInfo[i].u4SecSize - 1));
				}
				u4EraseAddr = u4SectEnd;
			}

			if (u4EraseAddr >= (u4Offset + u4ByteLen)) {
				break;
			}

			u4SectStart += _rSFInfo.arFlashInfo[i].u4SecSize;
		}
		u4ChipOffset += _rSFInfo.arFlashInfo[i].u4ChipSize;

		if (SFLASH_WriteProtect(i, TRUE) != 0) {
			nor_debug("Enable flash write protect fail!\n");
			return -1;
		}
	}
	return 0;
}


/* Brief of SFLASH_GetInfo. Details of SFLASH_GetInfo (optional). @retval TRUE    Success  @retval FALSE   Fail  */

INT32 SFLASH_EraseChip(uint32_t u4Index)
{
	INT32 i4Ret;

	assert(u4Index < MAX_FLASHCOUNT);


	if (SFLASH_Init() != 0) {
		return -1;
	}

	if (SFLASH_WriteProtect(u4Index, FALSE) != 0) {
		nor_debug("Disable flash write protect fail!\n");
		return -1;
	}
	i4Ret = SFLASHHW_EraseChip(u4Index);

	if (SFLASH_WriteProtect(u4Index, TRUE) != 0) {
		nor_debug("Enable flash write protect fail!\n");
		return -1;
	}
	return i4Ret;
}

/* Brief of SFLASH_WriteProtect.
 *  Details of SFLASH_WriteProtect (optional).
 *  @retval 1    Success
 *  @retval 0   Fail
 */
INT32 SFLASH_WriteProtect(uint32_t u4Index, BOOL fgEnable)
{
	return SFLASHHW_WriteProtect(u4Index, fgEnable);
}

/** Brief of SFLASH_GetFlashName.
 *  Details of SFLASH_GetFlashName (optional).
 */

char* SFLASH_GetFlashName(UINT8 u1MenuID, UINT8 u1DevID1, UINT8 u1DevID2)
{
	return SFLASHHW_GetFlashName(u1MenuID, u1DevID1, u1DevID2);
}

/** Brief of SFLASH_WriteProtect.
 *  Details of SFLASH_WriteProtect (optional).
 *  @retval 1    Success
 *  @retval 0   Fail
 */
INT32 SFLASH_ReadFlashStatus(uint32_t u4Index, UINT8* pu1Status)
{
	return SFLASHHW_ReadFlashStatus(u4Index, pu1Status);
}


/* Brief of SFLASH_Read. Details of SFLASH_Read (optional). @retval TRUE    Success @retval FALSE   Fail*/
INT32 SFLASH_Read(uint32_t u4Offset, uint64_t u4MemPtr, uint32_t u4ByteCount)
{
	if (SFLASHHW_Read(u4Offset, u4ByteCount, (UINT8*)u4MemPtr) != 0)
		return -1;

	return 0;
}


