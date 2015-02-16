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

#if !defined(CC_MTK_LOADER) && !defined(CC_MTK_LOADER)
#define NOR_SHARE_WITH_CI
#endif

#define NOR_FLASH_BASE_ADDR     ((UINT32)0x30000000)


///struct blkdev g_nor_bdev;



#if defined(CONFIG_DEBUG_CBFS_NOR) && CONFIG_DEBUG_CBFS_NOR
#define DEBUG_NOR(x ...) printk(BIOS_DEBUG, "MTK_NOR_CBFS: " x)
#endif


static UINT32 _u4NORXDelay = 4 * 1024;

/* NOR_Init() to read data from nor flash.  @return 0 successful, otherwise failed. */

INT32 NOR_Init(void)
{
	INT32 i4Ret;
	static BOOL _fgNORInit = FALSE;

	if (_fgNORInit) {
		return 0;
	}
	_fgNORInit = TRUE;

	i4Ret = SFLASH_Init();

	return i4Ret;
}


/* NOR_Read() to read data from nor flash.  @return 0 successful, otherwise failed. */

INT32 NOR_Read(UINT64 u8Offset, UINT32 u4MemPtr, UINT32 u4MemLen)
{
	UINT32 u4Offset = (UINT32)u8Offset;

	SFLASH_Read( 0, (UINT64)u4MemPtr, 1);
	SFLASH_Read( u4Offset, (UINT64)u4MemPtr, u4MemLen);

	mdelay(200);

	return 0;
}

/* NOR_EraseAddr() to erase data to 0xff on nor flash  @return 0 successful, otherwise failed. */

INT32 NOR_EraseAddr(UINT32 u4Offset, UINT32 u4ByteCount)
{
	INT32 i4Ret;

	i4Ret = SFLASH_EraseAddr(u4Offset, u4ByteCount);
	return i4Ret;
}


/* NOR_EraseChip() to erase data to 0xff on nor flash. @return 0 successful, otherwise failed. */
INT32 NOR_EraseChip(UINT32 u4FlashIndex)
{
	INT32 i4Ret;

	i4Ret = SFLASH_EraseChip(u4FlashIndex);

	return i4Ret;
}


/* NOR_Write to write data to nor flash. @return 0 successful, otherwise failed. */

INT32 NOR_Write(UINT32 u8Offset, UINT32 u4MemPtr, UINT32 u4ByteCount)
{
	INT32 i4Ret;
	UINT32 u4WriteCnt;
	UINT32 u4WriteOffset;

	u4WriteOffset = 0;
	do {
		if (u4ByteCount > _u4NORXDelay) {
			u4WriteCnt = _u4NORXDelay;
		}else  {
			u4WriteCnt = u4ByteCount;
		}
		i4Ret = SFLASH_Write((UINT32)(u8Offset + u4WriteOffset), (uint64_t)(u4MemPtr + u4WriteOffset), u4WriteCnt);
		u4WriteOffset += u4WriteCnt;
		u4ByteCount -= u4WriteCnt;
	} while (u4ByteCount);

	return i4Ret;
}

/* NOR_WriteByte() to write single byte on nor flash. @return 0 successful, otherwise failed.*/
INT32 NOR_WriteByte(UINT32 u4Offset, UINT32 u4Data)
{
	INT32 i4Ret;

	i4Ret = SFLASH_WriteOneByte(u4Offset, (UINT8)(u4Data & 0xFF));
	return i4Ret;
}



#define SFLASH_WREG8(offset, value)       IO_WRITE8(SFLASH_REG_BASE + (offset), (value))
#define SFLASH_RREG8(offset)               IO_READ8(SFLASH_REG_BASE, (offset))



#define storage_buffer g_dram_buf->storage_buffer

extern SFLASHHW_VENDOR_T _arFlashChip[MAX_FLASHCOUNT];
/*
   u32 nor_init_device(void)
   {
        if (!blkdev_get(BOOTDEV_NOR))
        {
                NOR_Init();
                SFLASH_WREG8(SFLASH_READ_DUAL_REG,0);// clear bit mode

                memset(&g_nor_bdev, 0, sizeof(struct blkdev));
                g_nor_bdev.blksz = _arFlashChip[0].u4SecSize;// page change to sector

                g_nor_bdev.erasesz = _arFlashChip[0].u4SecSize;
                g_nor_bdev.blks = _arFlashChip[0].u4ChipSize; //blks equals to chip size
                g_nor_bdev.bread = nor_bread;
                g_nor_bdev.bwrite = nor_bwrite;
                g_nor_bdev.blkbuf = NULL;
                g_nor_bdev.type = BOOTDEV_NOR;
                blkdev_register(&g_nor_bdev);
    }
        return 0;
   }

 */

