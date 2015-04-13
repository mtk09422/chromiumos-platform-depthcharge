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
#include <assert.h>
#include <libpayload.h>
#include <stddef.h>
#include <stdint.h>

#include "mtk_nor_if.h"
#include "typedefs.h"
#include "mtk_serialflash_hw.h"


#define SFLASH_POLLINGREG_COUNT     500000
#define SFLASH_WRITECMD_TIMEOUT     100000
#define SFLASH_WRITEBUSY_TIMEOUT    500000
#define SFLASH_ERASESECTOR_TIMEOUT  200000
#define SFLASH_CHIPERASE_TIMEOUT    500000

#define DEBUG_NOR(level, x ...)          printk(level, x)

#ifndef FALSE
#define FALSE   0
#endif

#ifndef TRUE
#define TRUE    1
#endif

#ifndef NULL
#define NULL    0
#endif


#define SFLASH_HW_ALIGNMENT         4
#ifndef SFLASH_WRITE_PROTECTION_VAL
  #define SFLASH_WRITE_PROTECTION_VAL 0x00
#endif
#define DUAL_READ 0x01
#define DUAL_IO   0x02
#define QUAD_READ 0x04
#define QUAD_IO   0x08

int __nor_debug = 0;
int __nor_trace = 0;
/* Type definitions */
static SFLASHHW_VENDOR_T _aVendorFlash[] =
{
	{ 0x01, 0x02, 0x12, 0x0, 0x80000,   0x10000,	  60000,  0x06, 0x04, 0x05, 0x01, 0x03, 0x0B, 0x9F, 0xD8, 0xC7, 0x02, 0x00, 0x00, SFLASH_WRITE_PROTECTION_VAL,	"Spansion(S25FL004A)"  },
	{ 0x01, 0x02, 0x13, 0x0, 0x100000,  0x10000,	  60000,  0x06, 0x04, 0x05, 0x01, 0x03, 0x0B, 0x9F, 0xD8, 0xC7, 0x02, 0x00, 0x00, SFLASH_WRITE_PROTECTION_VAL,	"Spansion(S25FL008A)"  },
	{ 0x01, 0x02, 0x14, 0x0, 0x200000,  0x10000,	  60000,  0x06, 0x04, 0x05, 0x01, 0x03, 0x0B, 0x9F, 0xD8, 0xC7, 0x02, 0x00, 0x00, SFLASH_WRITE_PROTECTION_VAL,	"Spansion(S25FL016A)"  },
	{ 0x01, 0x02, 0x15, 0x0, 0x400000,  0x10000,	  60000,  0x06, 0x04, 0x05, 0x01, 0x03, 0x0B, 0x9F, 0xD8, 0xC7, 0x02, 0x00, 0x00, SFLASH_WRITE_PROTECTION_VAL,	"Spansion(S25FL032A)"  },
	{ 0x01, 0x02, 0x16, 0x0, 0x800000,  0x10000,	  60000,  0x06, 0x04, 0x05, 0x01, 0x03, 0x0B, 0x9F, 0xD8, 0xC7, 0x02, 0x00, 0x00, SFLASH_WRITE_PROTECTION_VAL,	"Spansion(S25FL064A)"  },
	{ 0x01, 0x20, 0x18, 0x0, 0x1000000, 0x10000,	  60000,  0x06, 0x04, 0x05, 0x01, 0x03, 0x0B, 0x9F, 0xD8, 0xC7, 0x02, 0x00, 0x00, SFLASH_WRITE_PROTECTION_VAL,	"Spansion(S25FL128P)"  },
	{ 0x01, 0x02, 0x19, 0x0, 0x2000000, 0x10000,	  60000,  0x06, 0x04, 0x05, 0x01, 0x03, 0x0B, 0x9F, 0xD8, 0xC7, 0x02, 0x00, 0x00, SFLASH_WRITE_PROTECTION_VAL,	"Spansion(S25FL256S)"  },
	{ 0x01, 0x40, 0x15, 0x0, 0x200000,  0x10000,	  60000,  0x06, 0x04, 0x05, 0x01, 0x03, 0x0B, 0x9F, 0xD8, 0xC7, 0x02, 0x00, 0x00, SFLASH_WRITE_PROTECTION_VAL,	"Spansion(S25FL016A)"  },

	{ 0xC2, 0x20, 0x13, 0x0, 0x80000,   0x10000,	  60000,  0x06, 0x04, 0x05, 0x01, 0x03, 0x0B, 0x9F, 0xD8, 0xC7, 0x02, 0x00, 0x00, SFLASH_WRITE_PROTECTION_VAL,	"MXIC(25L400)"	       },
	{ 0xC2, 0x20, 0x14, 0x0, 0x100000,  0x10000,	  60000,  0x06, 0x04, 0x05, 0x01, 0x03, 0x0B, 0x9F, 0xD8, 0xC7, 0x02, 0x00, 0x00, SFLASH_WRITE_PROTECTION_VAL,	"MXIC(25L80)"	       },
	{ 0xC2, 0x20, 0x15, 0x0, 0x200000,  0x10000,	  60000,  0x06, 0x04, 0x05, 0x01, 0x03, 0x0B, 0x9F, 0xD8, 0xC7, 0x02, 0x00, 0x00, SFLASH_WRITE_PROTECTION_VAL,	"MXIC(25L160)"	       },
	{ 0xC2, 0x24, 0x15, 0x0, 0x200000,  0x10000,	  60000,  0x06, 0x04, 0x05, 0x01, 0x03, 0x0B, 0x9F, 0xD8, 0xC7, 0x02, 0x00, 0x00, SFLASH_WRITE_PROTECTION_VAL,	"MXIC(25L1635D)"       },
	{ 0xC2, 0x20, 0x16, 0x0, 0x400000,  0x10000,	  60000,  0x06, 0x04, 0x05, 0x01, 0x03, 0x0B, 0x9F, 0xD8, 0xC7, 0x02, 0x00, 0x00, SFLASH_WRITE_PROTECTION_VAL,	"MXIC(25L320)"	       },
	{ 0xC2, 0x20, 0x17, 0x0, 0x800000,  0x10000,	  60000,  0x06, 0x04, 0x05, 0x01, 0x03, 0x0B, 0x9F, 0xD8, 0xC7, 0x02, 0x00, 0x00, SFLASH_WRITE_PROTECTION_VAL,	"MXIC(25L640)"	       },
	{ 0xC2, 0x20, 0x18, 0x0, 0x1000000, 0x10000,	  60000,  0x06, 0x04, 0x05, 0x01, 0x03, 0x0B, 0x9F, 0xD8, 0xC7, 0x02, 0x00, 0x00, SFLASH_WRITE_PROTECTION_VAL,	"MXIC(25L1280)"	       },
	{ 0xC2, 0x5E, 0x16, 0x0, 0x400000,  0x10000,	  60000,  0x06, 0x04, 0x05, 0x01, 0x03, 0x0B, 0x9F, 0xD8, 0xC7, 0x02, 0x00, 0x00, SFLASH_WRITE_PROTECTION_VAL,	"MXIC(25L3235D)"       },
	{ 0xC2, 0x20, 0x19, 0x0, 0x2000000, 0x10000,	  60000,  0x06, 0x04, 0x05, 0x01, 0x03, 0x0B, 0x9F, 0xD8, 0xC7, 0x02, 0x00, 0x3b, SFLASH_WRITE_PROTECTION_VAL,	"MXIC(25L256)"	       },
	{ 0xC2, 0x20, 0x1A, 0x0, 0x2000000, 0x10000,	  60000,  0x06, 0x04, 0x05, 0x01, 0x03, 0x0B, 0x9F, 0xD8, 0xC7, 0x02, 0x00, 0x3b, SFLASH_WRITE_PROTECTION_VAL,	"MXIC(25L512)"	       },
	{ 0xC2, 0x25, 0x39, 0x0, 0x2000000, 0x10000,	  60000,  0x06, 0x04, 0x05, 0x01, 0x03, 0x0B, 0x9F, 0xD8, 0xC7, 0x02, 0x00, 0x3b, SFLASH_WRITE_PROTECTION_VAL,	"MXIC(25U256)"	       },

	{ 0x20, 0x20, 0x14, 0x0, 0x100000,  0x10000,	  60000,  0x06, 0x04, 0x05, 0x01, 0x03, 0x0B, 0x9F, 0xD8, 0xC7, 0x02, 0x00, 0x00, SFLASH_WRITE_PROTECTION_VAL,	"ST(M25P80)"	       },
	{ 0x20, 0x20, 0x15, 0x0, 0x200000,  0x10000,	  60000,  0x06, 0x04, 0x05, 0x01, 0x03, 0x0B, 0x9F, 0xD8, 0xC7, 0x02, 0x00, 0x00, SFLASH_WRITE_PROTECTION_VAL,	"ST(M25P16)"	       },
	{ 0x20, 0x20, 0x16, 0x0, 0x400000,  0x10000,	  60000,  0x06, 0x04, 0x05, 0x01, 0x03, 0x0B, 0x9F, 0xD8, 0xC7, 0x02, 0x00, 0x00, SFLASH_WRITE_PROTECTION_VAL,	"ST(M25P32)"	       },
	{ 0x20, 0x20, 0x17, 0x0, 0x800000,  0x10000,	  60000,  0x06, 0x04, 0x05, 0x01, 0x03, 0x0B, 0x9F, 0xD8, 0xC7, 0x02, 0x00, 0x00, SFLASH_WRITE_PROTECTION_VAL,	"ST(M25P64)"	       },
	{ 0x20, 0x20, 0x18, 0x0, 0x1000000, 0x40000,	  60000,  0x06, 0x04, 0x05, 0x01, 0x03, 0x0B, 0x9F, 0xD8, 0xC7, 0x02, 0x00, 0x00, SFLASH_WRITE_PROTECTION_VAL,	"ST(M25P128)"	       },
	{ 0x20, 0x71, 0x16, 0x0, 0x400000,  0x10000,	  60000,  0x06, 0x04, 0x05, 0x01, 0x03, 0x0B, 0x9F, 0xD8, 0xC7, 0x02, 0x00, 0x00, SFLASH_WRITE_PROTECTION_VAL,	"ST(M25PX32)"	       },
	{ 0x20, 0x71, 0x17, 0x0, 0x800000,  0x10000,	  60000,  0x06, 0x04, 0x05, 0x01, 0x03, 0x0B, 0x9F, 0xD8, 0xC7, 0x02, 0x00, 0x00, SFLASH_WRITE_PROTECTION_VAL,	"ST(M25PX64)"	       },
	{ 0x20, 0xBA, 0x17, 0x0, 0x800000,  0x10000,	  60000,  0x06, 0x04, 0x05, 0x01, 0x03, 0x0B, 0x9F, 0xD8, 0xC7, 0x02, 0x00, 0x00, SFLASH_WRITE_PROTECTION_VAL,	"ST(M25P64)"	       },

	{ 0xBF, 0x25, 0x41, 0x0, 0x200000,  0x10000,	  60000,  0x06, 0x04, 0x05, 0x01, 0x03, 0x0B, 0x9F, 0xD8, 0xC7, 0x02, 0xAD, 0x00, SFLASH_WRITE_PROTECTION_VAL,	"SST(25VF016B)"	       },

	{ 0xEF, 0x30, 0x13, 0x0, 0x80000,   0x10000,	  60000,  0x06, 0x04, 0x05, 0x01, 0x03, 0x0B, 0x9F, 0xD8, 0xC7, 0x02, 0x00, 0x00, SFLASH_WRITE_PROTECTION_VAL,	"WINBOND(W25X40)"      },
	{ 0xEF, 0x30, 0x14, 0x0, 0x100000,  0x10000,	  60000,  0x06, 0x04, 0x05, 0x01, 0x03, 0x0B, 0x9F, 0xD8, 0xC7, 0x02, 0x00, 0x00, SFLASH_WRITE_PROTECTION_VAL,	"WINBOND(W25X80)"      },
	{ 0xEF, 0x30, 0x15, 0x0, 0x200000,  0x10000,	  60000,  0x06, 0x04, 0x05, 0x01, 0x03, 0x0B, 0x9F, 0xD8, 0xC7, 0x02, 0x00, 0x00, SFLASH_WRITE_PROTECTION_VAL,	"WINBOND(W25X16)"      },
	{ 0xEF, 0x30, 0x16, 0x0, 0x400000,  0x10000,	  60000,  0x06, 0x04, 0x05, 0x01, 0x03, 0x0B, 0x9F, 0xD8, 0xC7, 0x02, 0x00, 0x00, SFLASH_WRITE_PROTECTION_VAL,	"WINBOND(W25X32)"      },
	{ 0xEF, 0x60, 0x16, 0x0, 0x400000,  0x10000,	  60000,  0x06, 0x04, 0x05, 0x01, 0x03, 0x0B, 0x9F, 0xD8, 0xC7, 0x02, 0x00, 0x00, SFLASH_WRITE_PROTECTION_VAL,	"WINBOND(W25X32)"      },

	{ 0xEF, 0x30, 0x17, 0x0, 0x800000,  0x10000,	  60000,  0x06, 0x04, 0x05, 0x01, 0x03, 0x0B, 0x9F, 0xD8, 0xC7, 0x02, 0x00, 0x00, SFLASH_WRITE_PROTECTION_VAL,	"WINBOND(W25X64)"      },
	{ 0xEF, 0x40, 0x19, 0x0, 0x2000000, 0x10000,	  60000,  0x06, 0x04, 0x05, 0x01, 0x03, 0x0B, 0x9F, 0xD8, 0xC7, 0x02, 0x00, 0x00, SFLASH_WRITE_PROTECTION_VAL,	"WINBOND(W25Q256FV)"   },
	{ 0xEF, 0x40, 0x15, 0x0, 0x200000,  0x10000,	  60000,  0x06, 0x04, 0x05, 0x01, 0x03, 0x0B, 0x9F, 0xD8, 0xC7, 0x02, 0x00, 0xBB, SFLASH_WRITE_PROTECTION_VAL,	"WINBOND(W25Q16CV)"    },
	{ 0xEF, 0x40, 0x16, 0x0, 0x400000,  0x10000,	  60000,  0x06, 0x04, 0x05, 0x01, 0x03, 0x0B, 0x9F, 0xD8, 0xC7, 0x02, 0x00, 0x00, SFLASH_WRITE_PROTECTION_VAL,	"WINBOND(W25Q32BV)"    },
	{ 0xEF, 0x40, 0x17, 0x0, 0x800000,  0x10000,	  60000,  0x06, 0x04, 0x05, 0x01, 0x03, 0x0B, 0x9F, 0xD8, 0xC7, 0x02, 0x00, 0xBB, SFLASH_WRITE_PROTECTION_VAL,	"WINBOND(W25Q64BV)"    },
	{ 0xEF, 0x40, 0x18, 0x0, 0x1000000, 0x10000,	  60000,  0x06, 0x04, 0x05, 0x01, 0x03, 0x0B, 0x9F, 0xD8, 0xC7, 0x02, 0x00, 0x00, SFLASH_WRITE_PROTECTION_VAL,	"WINBOND(W25Q128BV)"   },

	{ 0xC8, 0x40, 0x15, 0x0, 0x200000,  0x10000,	  60000,  0x06, 0x04, 0x05, 0x01, 0x03, 0x0B, 0x9F, 0xD8, 0xC7, 0x02, 0x00, 0x00, SFLASH_WRITE_PROTECTION_VAL,	"GD(GD25Q16BSIG)"      },
	{ 0xC8, 0x40, 0x16, 0x0, 0x400000,  0x10000,	  60000,  0x06, 0x04, 0x05, 0x01, 0x03, 0x0B, 0x9F, 0xD8, 0xC7, 0x02, 0x00, 0x00, SFLASH_WRITE_PROTECTION_VAL,	"GD(GD25Q32BSIG)"      },
	{ 0xC8, 0x40, 0x17, 0x0, 0x800000,  0x10000,	  60000,  0x06, 0x04, 0x05, 0x01, 0x03, 0x0B, 0x9F, 0xD8, 0xC7, 0x02, 0x00, 0x00, SFLASH_WRITE_PROTECTION_VAL,	"GD(GD25Q64BSIG)"      },
	{ 0xC8, 0x40, 0x18, 0x0, 0x1000000, 0x10000,	  60000,  0x06, 0x04, 0x05, 0x01, 0x03, 0x0B, 0x9F, 0xD8, 0xC7, 0x02, 0x00, 0x00, SFLASH_WRITE_PROTECTION_VAL,	"GD(GD25Q128BSIG)"     },

	{ 0xBF, 0x43, 0x10, 0x1, 0x40000,   0x8000,	  60000,  0x06, 0x04, 0x05, 0x01, 0x03, 0x0B, 0x9F, 0x52, 0x60, 0x02, 0xAF, 0x00, SFLASH_WRITE_PROTECTION_VAL,	"SST(SST25VF020)"      },
	{ 0xBF, 0x25, 0x8D, 0x1, 0x80000,   0x10000,	  60000,  0x06, 0x04, 0x05, 0x01, 0x03, 0x0B, 0x9F, 0xD8, 0xC7, 0x02, 0xAD, 0x00, SFLASH_WRITE_PROTECTION_VAL,	"SST(SST25VF040B)"     },
	{ 0xBF, 0x25, 0x8E, 0x1, 0x100000,  0x10000,	  60000,  0x06, 0x04, 0x05, 0x01, 0x03, 0x0B, 0x9F, 0xD8, 0xC7, 0x02, 0xAD, 0x00, SFLASH_WRITE_PROTECTION_VAL,	"SST(SST25VF080B)"     },
	{ 0xBF, 0x25, 0x41, 0x1, 0x200000,  0x10000,	  60000,  0x06, 0x04, 0x05, 0x01, 0x03, 0x0B, 0x9F, 0xD8, 0xC7, 0x02, 0xAD, 0x00, SFLASH_WRITE_PROTECTION_VAL,	"SST(SST25VF016B)"     },
	{ 0xBF, 0x25, 0x4A, 0x1, 0x400000,  0x10000,	  60000,  0x06, 0x04, 0x05, 0x01, 0x03, 0x0B, 0x9F, 0xD8, 0xC7, 0x02, 0xAD, 0x00, SFLASH_WRITE_PROTECTION_VAL,	"SST(SST25VF032B)"     },
	{ 0xBF, 0x25, 0x4B, 0x1, 0x800000,  0x10000,	  60000,  0x06, 0x04, 0x05, 0x01, 0x03, 0x0B, 0x9F, 0xD8, 0xC7, 0x02, 0xAD, 0x00, SFLASH_WRITE_PROTECTION_VAL,	"SST(SST25VF064C)"     },

	{ 0x1F, 0x47, 0x00, 0x0, 0x400000,  0x10000,	  60000,  0x06, 0x04, 0x05, 0x01, 0x03, 0x0B, 0x9F, 0xD8, 0xC7, 0x02, 0x00, 0x00, SFLASH_WRITE_PROTECTION_VAL,	"ATMEL(AT25DF321)"     },
	{ 0x1F, 0x48, 0x00, 0x0, 0x800000,  0x10000,	  60000,  0x06, 0x04, 0x05, 0x01, 0x03, 0x0B, 0x9F, 0xD8, 0xC7, 0x02, 0x00, 0x00, SFLASH_WRITE_PROTECTION_VAL,	"ATMEL(AT25DF641)"     },
	{ 0x1F, 0x45, 0x01, 0x0, 0x100000,  0x10000,	  60000,  0x06, 0x04, 0x05, 0x01, 0x03, 0x0B, 0x9F, 0xD8, 0xC7, 0x02, 0x00, 0x00, SFLASH_WRITE_PROTECTION_VAL,	"ATMEL(AT26DF081A)"    },
	{ 0x1C, 0x20, 0x13, 0x0, 0x80000,   0x10000,	  60000,  0x06, 0x04, 0x05, 0x01, 0x03, 0x0B, 0x9F, 0xD8, 0xC7, 0x02, 0x00, 0x00, SFLASH_WRITE_PROTECTION_VAL,	"EON(EN25B40)"	       },
	{ 0x1C, 0x31, 0x14, 0x0, 0x100000,  0x10000,	  60000,  0x06, 0x04, 0x05, 0x01, 0x03, 0x0B, 0x9F, 0xD8, 0xC7, 0x02, 0x00, 0x00, SFLASH_WRITE_PROTECTION_VAL,	"EON(EN25F80)"	       },
	{ 0x1C, 0x20, 0x15, 0x0, 0x200000,  0x10000,	  60000,  0x06, 0x04, 0x05, 0x01, 0x03, 0x0B, 0x9F, 0xD8, 0xC7, 0x02, 0x00, 0x00, SFLASH_WRITE_PROTECTION_VAL,	"EON(EN25P16)"	       },
	{ 0x1C, 0x20, 0x16, 0x0, 0x400000,  0x10000,	  60000,  0x06, 0x04, 0x05, 0x01, 0x03, 0x0B, 0x9F, 0xD8, 0xC7, 0x02, 0x00, 0x00, SFLASH_WRITE_PROTECTION_VAL,	"EON(EN25P32)"	       },
	{ 0x1C, 0x20, 0x17, 0x0, 0x800000,  0x10000,	  60000,  0x06, 0x04, 0x05, 0x01, 0x03, 0x0B, 0x9F, 0xD8, 0xC7, 0x02, 0x00, 0x00, SFLASH_WRITE_PROTECTION_VAL,	"EON(EN25P64)"	       },
	{ 0x1C, 0x30, 0x17, 0x0, 0x800000,  0x10000,	  60000,  0x06, 0x04, 0x05, 0x01, 0x03, 0x0B, 0x9F, 0xD8, 0xC7, 0x02, 0x00, 0x00, SFLASH_WRITE_PROTECTION_VAL,	"EON(EN25Q64)"	       },
	{ 0x1C, 0x20, 0x14, 0x0, 0x100000,  0x10000,	  60000,  0x06, 0x04, 0x05, 0x01, 0x03, 0x0B, 0x9F, 0xD8, 0xC7, 0x02, 0x00, 0x00, SFLASH_WRITE_PROTECTION_VAL,	"EON(EN25P80/EN25B80)" },
	{ 0x7F, 0x37, 0x20, 0x0, 0x200000,  0x10000,	  60000,  0x06, 0x04, 0x05, 0x01, 0x03, 0x0B, 0x9F, 0xD8, 0xC7, 0x02, 0x00, 0x00, SFLASH_WRITE_PROTECTION_VAL,	"AMIC(A25L40P)"	       },
	{ 0x37, 0x30, 0x13, 0x0, 0x100000,  0x10000,	  60000,  0x06, 0x04, 0x05, 0x01, 0x03, 0x0B, 0x9F, 0xD8, 0xC7, 0x02, 0x00, 0x00, SFLASH_WRITE_PROTECTION_VAL,	"AMIC(A25L040)"	       },
	{ 0x37, 0x30, 0x16, 0x0, 0x400000,  0x10000,	  60000,  0x06, 0x04, 0x05, 0x01, 0x03, 0x0B, 0x9F, 0xD8, 0xC7, 0x02, 0x00, 0x00, SFLASH_WRITE_PROTECTION_VAL,	"AMIC(A25L032)"	       },
	{ 0xFF, 0xFF, 0x10, 0x0, 0x10000,   0x10000,	  60000,  0x06, 0x04, 0x05, 0x01, 0x03, 0x0B, 0x9F, 0xC7, 0xC7, 0x02, 0x00, 0x00, SFLASH_WRITE_PROTECTION_VAL,	"512Kb"		       },
	{ 0xFF, 0xFF, 0x12, 0x0, 0x40000,   0x10000,	  60000,  0x06, 0x04, 0x05, 0x01, 0x03, 0x0B, 0x9F, 0xD8, 0xC7, 0x02, 0x00, 0x00, SFLASH_WRITE_PROTECTION_VAL,	"2Mb"		       },
	{ 0xFF, 0xFF, 0x13, 0x0, 0x80000,   0x10000,	  60000,  0x06, 0x04, 0x05, 0x01, 0x03, 0x0B, 0x9F, 0xD8, 0xC7, 0x02, 0x00, 0x00, SFLASH_WRITE_PROTECTION_VAL,	"4Mb"		       },
	{ 0xFF, 0xFF, 0x14, 0x0, 0x100000,  0x10000,	  60000,  0x06, 0x04, 0x05, 0x01, 0x03, 0x0B, 0x9F, 0xD8, 0xC7, 0x02, 0x00, 0x00, SFLASH_WRITE_PROTECTION_VAL,	"8Mb"		       },
	{ 0xFF, 0xFF, 0x15, 0x0, 0x200000,  0x10000,	  60000,  0x06, 0x04, 0x05, 0x01, 0x03, 0x0B, 0x9F, 0xD8, 0xC7, 0x02, 0x00, 0x00, SFLASH_WRITE_PROTECTION_VAL,	"16Mb"		       },
	{ 0xFF, 0xFF, 0x16, 0x0, 0x400000,  0x10000,	  60000,  0x06, 0x04, 0x05, 0x01, 0x03, 0x0B, 0x9F, 0xD8, 0xC7, 0x02, 0x00, 0x00, SFLASH_WRITE_PROTECTION_VAL,	"32Mb"		       },
	{ 0xFF, 0xFF, 0x17, 0x0, 0x800000,  0x10000,	  60000,  0x06, 0x04, 0x05, 0x01, 0x03, 0x0B, 0x9F, 0xD8, 0xC7, 0x02, 0x00, 0x00, SFLASH_WRITE_PROTECTION_VAL,	"64Mb"		       },
	{ 0xFF, 0xFF, 0x18, 0x0, 0x1000000, 0x10000,	  60000,  0x06, 0x04, 0x05, 0x01, 0x03, 0x0B, 0x9F, 0xD8, 0xC7, 0x02, 0x00, 0x00, SFLASH_WRITE_PROTECTION_VAL,	"128Mb"		       },
	{ 0x00, 0x00, 0x00, 0x0, 0x000000,  0x00000,	  0x0000, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, SFLASH_WRITE_PROTECTION_VAL,	"NULL Device"	       },
};

/* Macro definitions */

#define SFLASH_WREG8(offset, value)      writeb( value, (void*)((volatile uint32_t*)((uintptr_t)(SFLASH_REG_BASE + offset))))
#define SFLASH_RREG8(offset)             readb((const void*)((volatile uint32_t*)((uintptr_t)(SFLASH_REG_BASE + offset))))
#define SFLASH_WREG32(offset, value)     writel(value, (void*)((volatile uint32_t*)((uintptr_t)(SFLASH_REG_BASE + offset))))
#define SFLASH_RREG32(offset)            readl((const void*)((volatile uint32_t*)((uintptr_t)(SFLASH_REG_BASE + offset))))

//#define SFLASH_WREG8(offset, value)       write8((void *)((volatile uint32_t*)((uintptr_t)(SFLASH_REG_BASE+ offset))), value)
//#define SFLASH_RREG8(offset)               read8((const void *)((volatile uint32_t*)((uintptr_t)(SFLASH_REG_BASE+offset))))
//#define SFLASH_WREG32(offset, value)     write32((void *)((volatile uint32_t*)((uintptr_t)(SFLASH_REG_BASE + offset))), value)
//#define SFLASH_RREG32(offset)            read32((const void *)((volatile uint32_t*)((uintptr_t)(SFLASH_REG_BASE + offset))))
//#define MSDC_WRITE32(addr, data)	write32((void *)addr, data)

//#define MSDC_READ32(addr)		read32((const void *)addr)
//#define MSDC_WRITE16(addr, data)	write16((void *)addr, data)
//#define MSDC_READ16(addr)		read16((const void *)addr)
//#define MSDC_WRITE8(addr, data)		write8((void *)addr, data)
//#define MSDC_READ8(addr)		read8((const void *)addr)


#define LoWord(d)     ((UINT16)(d & 0x0000ffffL))
#define HiWord(d)     ((UINT16)((d >> 16) & 0x0000ffffL))
#define LoByte(w)     ((UINT8)(w & 0x00ff))
#define HiByte(w)     ((UINT8)((w >> 8) & 0x00ff))
#define MidWord(d)    ((UINT16)((d >> 8) & 0x00ffff))
#define BYTE0(arg)    (*(UINT8*)&arg)
#define BYTE1(arg)    (*((UINT8*)&arg + 1))
#define BYTE2(arg)    (*((UINT8*)&arg + 2))
#define BYTE3(arg)    (*((UINT8*)&arg + 3))

/* Static variables  */


#define EFUSE_base                  (0x10206000)    // EFUSE
 #define EFUSE_HW2_RES0_IO_33V       0x00000008
 #define EFUSE_HW2_RES0              ((volatile unsigned int*)((uintptr_t)(EFUSE_base + 0x04B0)))
 #define EFUSE_Is_IO_33V()         (((*EFUSE_HW2_RES0) & EFUSE_HW2_RES0_IO_33V) ? FALSE : TRUE) // 0 : 3.3v (MT8130 default), 1 : 1.8v


 #define GPIO_CFG_BIT16(reg, field, val) \
	do {	\
		unsigned short tv = (unsigned short)(*(volatile unsigned short*)(reg));	\
		tv &= ~(field);	\
		tv |= val; \
		(*(volatile unsigned short*)(reg) = (unsigned short)(tv)); \
	} while (0)

#define GPIO_base_nor       (0x10005000)    // GPIO
#define GPIO_BASE_nor        GPIO_base_nor
#define EXMD_CTRL           (GPIO_BASE_nor + 0X0DC0)

#define GPIO_DRVSEL(x)      (0x10005000 + 0x0B00 + 0x10 * x)            /* x = 0 ~ 9*/
#define MODE_BIT(ID)        (0x7 << (ID * 3))                           // calculate the gpio control bits  for specific pin in certain GPIO_MODE register
#define MODE_DAT(ID, VAL)    (VAL << (ID * 3))                          // calculate the gpio mode value    for specific pin in certain GPIO_MODE register
#define PULL_BIT(ID)        (0x1 << (ID))                               // calculate the gpio pullen bit    for specific pin in certain GPIO_PULLEN register
#define PULL_DAT(ID, VAL)    (VAL << (ID))                              // calculate the gpio pullsel value for specific pin in certain GPIO_PULLSEL register
#define DRVS_BIT(ID)        (0xf << (ID * 4))                           // calculate the gpio driving bits  for specific pin in certain GPIO_MODE register
#define DRVS_DAT(ID, VAL)    (VAL << (ID * 4))                          // calculate the gpio driving value for specific pin in certain GPIO_MODE register
#define MSDC_DRV_DAT(VAL)   (VAL << 8)                                  // calculate the MSDC driving value for special MSDC pin in MSDC Ctrl register
#define GPIO_PULLSEL(x)     (GPIO_BASE_nor + 0x0200 + 0x10 * (x - 1))   /* x = 1 ~ 9 */

#define EXMD_TDSEL          (0xf << 0)                                  // Bit [3:0]:   TDSEL
#define EXMD_RDSEL          (0x3f << 4)                                 // Bit [9:4]:   RDSEL
#define EXMD_BIAS           (0xf << 12)                                 // Bit [15:12]: BIAS

#define GPIO_MODE(x)        (GPIO_BASE_nor + 0x0600 + 0x10 * (x - 1))   /* x = 1 ~ 28 */

typedef enum {
	GPIO_MODE0    = 0,
	GPIO_MODE1    = 1,
	GPIO_MODE2    = 2,
	GPIO_MODE3    = 3,
	GPIO_MODE4    = 4,
	GPIO_MODE5    = 5,
	GPIO_MODE6    = 6,
	GPIO_MODE7    = 7,
} GMODE_ID;

void SF_NOR_GPIO_INIT(void)
{

	//DEBUG_NOR(BIOS_DEBUG, "NOR GPIO Init here !\n");

	// driving strength
	GPIO_CFG_BIT16(GPIO_DRVSEL(2),  DRVS_BIT(4), DRVS_DAT(4, 0x2));                 // 0x10005B20[15:12] = 0x2;  // SFWP_B 8mA
	GPIO_CFG_BIT16(GPIO_DRVSEL(3),  DRVS_BIT(0), DRVS_DAT(0, 0x2));                 // 0x10005B30[4:0]   = 0x2;  // SFOUT/SFCS0/SFHOLD/SFIN/SFCK 8mA

	// Pull up and Pull down¡ê?NOR flash need to be set as PU for all related GPIO
	GPIO_CFG_BIT16(GPIO_PULLSEL(1),  PULL_BIT(4) | PULL_BIT(5) | PULL_BIT(6) | PULL_BIT(7) | PULL_BIT(8) | PULL_BIT(9),
		       PULL_DAT(4, 0x1) |                                               // 0x10005200[4]     = 0x1;  // SFWP_B
		       PULL_DAT(5, 0x1) |                                               // 0x10005200[5]     = 0x1;  // SFOUT
		       PULL_DAT(6, 0x1) |                                               // 0x10005200[6]     = 0x1;  // SFCS0
		       PULL_DAT(7, 0x1) |                                               // 0x10005200[7]     = 0x1;  // SFHOLD
		       PULL_DAT(8, 0x1) |                                               // 0x10005200[8]     = 0x1;  // SFIN
		       PULL_DAT(9, 0x1) );                                              // 0x10005200[9]     = 0x1;  // SFCK

	// TDSEL, 3.3v/1.8v, TDSEL[3:0] = 1010 (Overwrite RTL design : 0000)
	GPIO_CFG_BIT16(EXMD_CTRL, EXMD_TDSEL, 0xa);                                     // 0x10005DC0[3:0] = 0xa;  // TDSEL

	// RDSEL, 3.3v, RDSEL[5:0] = 001100; 1.8v, RDSEL[5:0] = 000000 (RTL design default: 000000)
	if (EFUSE_Is_IO_33V()) {
		GPIO_CFG_BIT16(EXMD_CTRL, EXMD_RDSEL, 0x0c0);                           // 0x10005DC0[9:4] = 0xc;  // RDSEL
	} else{
		// NA
	}

	// BIAS
	if (EFUSE_Is_IO_33V()) {
		// 3.3v suggest value: TUNE[3:0] = [0101] (Overwrite RTL design: 0000)
		//    DEBUG_NOR(BIOS_DEBUG, "IO is 3.3V !\n");
		GPIO_CFG_BIT16(EXMD_CTRL, EXMD_BIAS, 0x5000);                           // 0x10005DC0[15:12] = 0x5;  // BIAS
	} else{
		//	DEBUG_NOR(BIOS_DEBUG, "IO isnot 3.3V !\n");
		// 1.8v suggest value: TUNE[3:0] = [xxxx] (use RTL design default 0000)
	}

	// Switch GPIO_MODE to SPI NAND mode
	GPIO_CFG_BIT16(GPIO_MODE(1), MODE_BIT(4),
		       MODE_DAT(4, GPIO_MODE6) );                                       // 0x10005600[14:12]  = 0x6;  // SFWP_B
	GPIO_CFG_BIT16(GPIO_MODE(2), MODE_BIT(0) | MODE_BIT(1) | MODE_BIT(2) | MODE_BIT(3) | MODE_BIT(4),
		       MODE_DAT(0, GPIO_MODE6) |                                        // 0x10005610[2:0]    = 0x6;  // SFOUT
		       MODE_DAT(1, GPIO_MODE6) |                                        // 0x10005610[5:3]    = 0x6;  // SFCS0
		       MODE_DAT(2, GPIO_MODE6) |                                        // 0x10005610[8:6]    = 0x6;  // SFHOLD
		       MODE_DAT(3, GPIO_MODE6) |                                        // 0x10005610[11:9]   = 0x6;  // SFIN
		       MODE_DAT(4, GPIO_MODE6) );                                       // 0x10005610[14:12]  = 0x6;  // SFCK


}





SFLASHHW_VENDOR_T _arFlashChip[MAX_FLASHCOUNT];

static uint32_t _u4ChipCount;

static uint32_t _u4SFIsrStatus;
static BOOL _fgSFIsrEn = TRUE;
static BOOL _fgDoProtect = FALSE;
BOOL _fgWriteProtect = TRUE;

#if (defined(ENABLE_DUALREAD))
static UINT8 _u1DualReadCmd = 0;
#endif

static BOOL _fgAAIWrite = FALSE;
uint32_t _fgNorPartialProtection = 0;
uint32_t SZ_OPT_UNIT_SIZE = 0x10000;
static UINT8 u1MenuID, u1DevID1, u1DevID2;

/*for gpio init and deinit for nor flash  */
static inline unsigned int NFI_gpio_uffs( unsigned int x)
{
	unsigned int r = 1;

	if (!x)
		return 0;
	if (!(x & 0xffff)) {
		x >>= 16;
		r += 16;
	}
	if (!(x & 0xff)) {
		x >>= 8;
		r += 8;
	}

	if (!(x & 0xf)) {
		x >>= 4;
		r += 4;
	}

	if (!(x & 3)) {
		x >>= 2;
		r += 2;
	}

	if (!(x & 1)) {
		x >>= 1;
		r += 1;
	}

	return r;
}


static void _SendFlashCommand(uint8_t u1Val)
{
	SFLASH_WREG8(SFLASH_CMD_REG, u1Val);

}

static INT32 _PollingReg(uint8_t u1RegOffset, uint8_t u8Compare)
{
	uint32_t u4Polling;
	uint8_t u1Reg;

	u4Polling = 0;
	while (1) {
		u1Reg = SFLASH_RREG8(u1RegOffset);
		if (0x00 == (u1Reg & u8Compare)) {
			break;
		}
		u4Polling++;
		if (u4Polling > SFLASH_POLLINGREG_COUNT) {
			return 1;
		}
	}

	return 0;
}

#ifndef UNUSED
#define UNUSED(x)               (void)x
#endif
#define VERIFY(x)               ((void)(x))


static void _SetFlashCountReg(uint32_t u4Count)
{
	UINT8 u1Reg;

	u1Reg = SFLASH_RREG8(SFLASH_CFG1_REG);
	u1Reg &= (~0xC);

	if (u4Count == 1) {
		SFLASH_WREG8(SFLASH_CFG1_REG, u1Reg);
	}else {
		assert(0);
	}
}

static INT32 _DoIdentify(uint32_t u4Index, uint32_t *pu4Vendor)
{

	uint32_t i;

	if (pu4Vendor == NULL) {
		return 1;
	}
	*pu4Vendor = 0xFFFFFFFF;

	if (SFLASHHW_GetID(u4Index, &u1MenuID, &u1DevID1, &u1DevID2) != 0) {
		return 1;
	}
	nor_debug("Flash index :%d,MenuID: 0x%x, DeviceID1: 0x%x, DeviceID2: 0x%x\n", u4Index, u1MenuID, u1DevID1, u1DevID2);
	i = 0;
	while (_aVendorFlash[i].u1MenuID != (UINT8)0x0) {
		if ( (_aVendorFlash[i].u1MenuID == u1MenuID) &&
		     (_aVendorFlash[i].u1DevID1 == u1DevID1) &&
		     (_aVendorFlash[i].u1DevID2 == u1DevID2)) {
			*pu4Vendor = i;

			if (_aVendorFlash[i].u1DualREADCmd) {
				_u1DualReadCmd = _aVendorFlash[i].u1DualREADCmd;
				nor_debug( "dualread flash _u1DualReadCmd = 0x%x\n", _u1DualReadCmd);
			}
			return 0;
		}
		i++;
	}

	return 1;
}

static INT32 _WBEnable(void)
{
	UINT32 u4Polling;
	UINT8 u1Reg;
	UINT8 u1Tmp;

	u1Tmp = 0x01;

	if (_fgAAIWrite == TRUE) {
		if (_arFlashChip[0].u1AAIPROGRAMCmd != 0xAF) {
			u1Tmp |= 0x40;
		}
	}
	SFLASH_WREG8(SFLASH_CFG2_REG, u1Tmp);

	u4Polling = 0;
	while (1) {
		u1Reg = SFLASH_RREG8(SFLASH_CFG2_REG);
		if (0x01 == (u1Reg & 0x1)) {
			break;
		}
		u4Polling++;
		if (u4Polling > SFLASH_POLLINGREG_COUNT) {
			return 1;
		}
	}
	return 0;
}


static INT32 _WBDisable(void)
{
	uint32_t u4Polling;
	uint8_t u1Reg;
	uint8_t u1Tmp;

	u1Tmp = 0x00;

	SFLASH_WREG8(SFLASH_CFG2_REG, u1Tmp);
	u4Polling = 0;
	while (1) { // Wait for finish write buffer
		u1Reg = SFLASH_RREG8(SFLASH_CFG2_REG);
		if (u1Tmp == (u1Reg & 0xF)) {
			break;
		}

		u4Polling++;
		if (u4Polling > SFLASH_POLLINGREG_COUNT) {
			return 1;
		}
	}
	return 0;
}

static INT32 _WaitForWriteBusy(uint32_t u4Index, uint32_t u4Timeout)
{
	uint32_t u4Count;
	uint8_t u1Reg;

	u4Count = 0;
	while (1) {
		if (SFLASHHW_ReadFlashStatus(u4Index, &u1Reg) != 0) {
			return 1;
		}
		if (0 == (u1Reg & 0x1)) {
			break;
		}
		u4Count++;
		if (u4Count > u4Timeout) {
			return 1;
		}
	}
	return 0;
}

static INT32 _SetFlashWriteEnable(uint32_t u4Index)
{

	SFLASH_WREG8(SFLASH_PRGDATA5_REG, _arFlashChip[u4Index].u1WRENCmd);
	SFLASH_WREG8(SFLASH_CNT_REG, 8); // Write SF Bit Count

	if (u4Index == 0) {
		_SendFlashCommand(0x4); // Write PRG
	}else {
		assert(0);
	}
	if ( _PollingReg(SFLASH_CMD_REG, 0x04) != 0) { // timeout
		return 1;
	}
	return 0;
}
/* Brief of SFLASHHW_Init. Details of SFLASHHW_Init (optional). @retval TRUE    Success  @retval FALSE   Fail*/

INT32 SFLASHHW_Init(void)
{
	uint32_t u4Loop, u4VendorIdx;

	SFLASHHW_WriteSfProtect(0x30);
	memset((void*)_arFlashChip, 0x0, MAX_FLASHCOUNT * sizeof(SFLASHHW_VENDOR_T));
	SF_NOR_GPIO_INIT();

	_u4ChipCount = 0;
	for (u4Loop = 0; u4Loop < MAX_FLASHCOUNT; u4Loop++) {
		if (_DoIdentify(u4Loop, &u4VendorIdx) != 0) {
			continue;
		}
		memcpy((void*)&(_arFlashChip[_u4ChipCount]), (void*)&(_aVendorFlash[u4VendorIdx]), sizeof(SFLASHHW_VENDOR_T));
		_u4ChipCount++;
		nor_debug("Detect flash #%d: %s\n", u4Loop, _arFlashChip[u4Loop].pcFlashStr);
	}
	if (_u4ChipCount == 0) {
		nor_debug("There is no flash!\n");
		return 1;
	}
	/* Set flash number register */
	_SetFlashCountReg(_u4ChipCount);
#if !defined (CC_MTK_LOADER) && !defined(CC_MTK_LOADER)

	SFLASH_WREG32(SFLASH_SF_INTREN_REG, 0x50); //enable pp Interrupt & aai
#endif /* CC_MTK_LOADER */
	_fgSFIsrEn = FALSE;
	_u4SFIsrStatus = 0;
	//Guarantee every time nor init can WriteProtect
	_fgWriteProtect = TRUE;

	if (_fgWriteProtect) {
		_fgDoProtect = TRUE;
		if (_fgDoProtect) {
			for (u4Loop = 0; u4Loop < _u4ChipCount; u4Loop++) {
				if (SFLASHHW_WriteProtect(u4Loop, TRUE) != 0) {
					return -1;
				}
			}
		}
	}

	return 0;
}

/* Brief of SFLASHHW_ReadFlashStatus.  Details of SFLASHHW_ReadFlashStatus (optional).  @retval 1    Success  @retval 0   Fail*/
INT32 SFLASHHW_ReadFlashStatus(uint32_t u4Index, uint8_t *pu1Val)
{
	if (pu1Val == NULL) {
		return 1;
	}
	if (u4Index == 0) {
		_SendFlashCommand(0x2);
	}else {
		assert(0);
	}
	if (_PollingReg(SFLASH_CMD_REG, 0x2) != 0) {
		return 1;
	}

	*pu1Val = SFLASH_RREG8(SFLASH_RDSR_REG);
	return 0;
}


static INT32 SFLASHHW_Send2Byte(uint32_t u4Index, uint8_t u1Data1, uint8_t u1Data2)
{
	SFLASH_WREG8(SFLASH_PRGDATA5_REG, u1Data1);
	SFLASH_WREG8(SFLASH_PRGDATA4_REG, u1Data2);
	SFLASH_WREG8(SFLASH_CNT_REG, 0x10); // Write SF Bit Count

	SFLASHHW_SwitchChip(u4Index);
	if (u4Index == 0) {
		_SendFlashCommand(0x4); // Write PRG
	}else {
		assert(0);
	}
	if ( _PollingReg(SFLASH_CMD_REG, 0x04) != 0) { // timeout
		SFLASHHW_SwitchChip(0);
		return -1;
	}
	SFLASHHW_SwitchChip(0);
	return 0;

}

/* Brief of SFLASHHW_SendByte.  Details @retval TRUE    Success  @retval FALSE   Fail*/
static INT32 SFLASHHW_SendByte(uint32_t u4Index, uint8_t u1Data)
{
	SFLASH_WREG8(SFLASH_PRGDATA5_REG, u1Data);
	SFLASH_WREG8(SFLASH_CNT_REG, 8); // Write SF Bit Count
	SFLASHHW_SwitchChip(u4Index);
	if (u4Index == 0) {
		_SendFlashCommand(0x4); // Write PRG

	}else {
		assert(0);
	}
	if ( _PollingReg(SFLASH_CMD_REG, 0x04) != 0) { // timeout
		SFLASHHW_SwitchChip(0);
		return -1;
	}
	SFLASHHW_SwitchChip(0);
	return 0;

}
/* Brief of SFLASHHW_GetSstID. Details of SFLASHHW_Identify (optional).  @retval TRUE    Success @retval FALSE   Fail*/
static INT32 SFLASHHW_GetSstID(uint32_t u4Index, uint8_t *pu1MenuID, uint8_t *pu1DevID1, uint8_t *pu1DevID2)
{
	SFLASH_WREG8(SFLASH_PRGDATA5_REG, 0x90);        // Write
	SFLASH_WREG8(SFLASH_PRGDATA4_REG, 0x00);        // Write
	SFLASH_WREG8(SFLASH_PRGDATA3_REG, 0x00);        // Write
	SFLASH_WREG8(SFLASH_PRGDATA2_REG, 0x00);        // Write
	SFLASH_WREG8(SFLASH_PRGDATA1_REG, 0x00);        // Write
	SFLASH_WREG8(SFLASH_PRGDATA0_REG, 0x00);        // Write
	SFLASH_WREG8(SFLASH_CNT_REG, 0x30);             // Write SF Bit Count

	if (u4Index == 0) {
		_SendFlashCommand(0x4);
	}else {
		assert(0);
	}
	if ( _PollingReg(SFLASH_CMD_REG, 0x04) != 0) { // timeout
		return -1;
	}

	if (pu1DevID2 != NULL) {
		*pu1DevID2 = 0x10;
	}

	if (pu1DevID1 != NULL) {
		*pu1DevID1 = SFLASH_RREG8(SFLASH_SHREG0_REG);
	}

	if (pu1MenuID != NULL) {
		*pu1MenuID = SFLASH_RREG8(SFLASH_SHREG1_REG);
	}

	_SendFlashCommand(0x0);
	return 0;
}
/* Brief of SFLASHHW_GetID.  Details of SFLASHHW_Identify (optional).  @retval TRUE    Success  @retval FALSE   Fail*/
INT32 SFLASHHW_GetID(uint32_t u4Index, uint8_t *pu1MenuID, uint8_t *pu1DevID1, uint8_t *pu1DevID2)
{
	nor_debug("Try SFLASHHW_GetID  ,u4Index = 0x%x \n", u4Index);
	SFLASH_WREG8(SFLASH_PRGDATA5_REG, 0x9F);        // Write
	SFLASH_WREG8(SFLASH_PRGDATA4_REG, 0x00);        // Write
	SFLASH_WREG8(SFLASH_PRGDATA3_REG, 0x00);        // Write
	SFLASH_WREG8(SFLASH_PRGDATA2_REG, 0x00);        // Write
	SFLASH_WREG8(SFLASH_CNT_REG, 32);               // Write SF Bit Count

	if (u4Index == 0) {
		_SendFlashCommand(0x4);
	}else {
		assert(0);
	}

	if ( _PollingReg(SFLASH_CMD_REG, 0x04) != 0) { // timeout
		return 1;
	}
	if (pu1DevID2 != NULL) {
		*pu1DevID2 = SFLASH_RREG8(SFLASH_SHREG0_REG);
	}

	if (pu1DevID1 != NULL) {
		*pu1DevID1 = SFLASH_RREG8(SFLASH_SHREG1_REG);
	}

	if (pu1MenuID != NULL) {
		*pu1MenuID = SFLASH_RREG8(SFLASH_SHREG2_REG);
	}
	_SendFlashCommand(0x0);
	if ((pu1MenuID != NULL) && (pu1DevID1 != NULL)) {
		if ((*pu1MenuID == 0xFF) || (*pu1DevID1 == 0x00)) {
			nor_debug("Try not JEDEC ID\n");
			if (SFLASHHW_GetSstID(u4Index, pu1MenuID, pu1DevID1, pu1DevID2) != 0) {
				return -1;
			}
		}
	}
	return 0;
}

/* Brief of SFLASHHW_GetFlashInfo.  Details of SFLASHHW_GetInfo (optional).  @retval TRUE    Success  @retval FALSE   Fail*/

void SFLASHHW_GetFlashInfo(SFLASH_INFO_T *prInfo)
{
	uint32_t i;

	if (prInfo == NULL) {
		return;
	}
	prInfo->u1FlashCount = (UINT8)(_u4ChipCount & 0xFF);
	for (i = 0; i < _u4ChipCount; i++) {
		prInfo->arFlashInfo[i].u1MenuID = _arFlashChip[i].u1MenuID;
		prInfo->arFlashInfo[i].u1DevID1 = _arFlashChip[i].u1DevID1;
		prInfo->arFlashInfo[i].u1DevID2 = _arFlashChip[i].u1DevID2;
		prInfo->arFlashInfo[i].u4ChipSize = _arFlashChip[i].u4ChipSize;
		prInfo->arFlashInfo[i].u4SecSize = _arFlashChip[i].u4SecSize;
		prInfo->arFlashInfo[i].u4SecCount =
			_arFlashChip[i].u4ChipSize / _arFlashChip[i].u4SecSize;

		memcpy(prInfo->arFlashInfo[i].pcFlashStr, _arFlashChip[i].pcFlashStr, SFLASHHWNAME_LEN);
	}

}
/* Brief of SFLASHHW_Read. Details of SFLASHHW_Read (optional).  @retval TRUE    Success @retval FALSE   Fail*/

INT32 SFLASHHW_Read(uint32_t u4Addr, uint32_t u4Len, UINT8* pu1buf)
{
	uint32_t i;

	//uint8_t _u1FlagVaule;

	if (pu1buf == NULL) {
		return 1;
	}
	SFLASH_WREG8(SFLASH_CFG2_REG, 0x0C);                                            // Disable pretch write buffer

	SFLASH_WREG8(SFLASH_READ_DUAL_REG, SFLASH_RREG8(SFLASH_READ_DUAL_REG) & 0x10);  //clear read mode setting
	//_u1FlagVaule = IO_READ8(0xCA000000, 0);
	//if(_u1FlagVaule == 1)
	//{
	//    DEBUG_NOR(BIOS_DEBUG, "...cdd SFLASHHW_Read _u1DualReadCmd is 0x3B u4Len = 0x%x!\n",u4Len);
	//    SFLASH_WREG8(SFLASH_PRGDATA3_REG, 0x3b);
	//    SFLASH_WREG8(SFLASH_READ_DUAL_REG, SFLASH_RREG8(SFLASH_READ_DUAL_REG) | 0x1);
	//}
	// else if(_u1FlagVaule == 2)
	//{
	//   DEBUG_NOR(BIOS_DEBUG, "...cdd SFLASHHW_Read _u1DualReadCmd is 0xBB u4Len = 0x%x!\n",u4Len);
	//  SFLASH_WREG8(SFLASH_PRGDATA3_REG, 0xbb);
	//  SFLASH_WREG8(SFLASH_READ_DUAL_REG, SFLASH_RREG8(SFLASH_READ_DUAL_REG) | 0x3);
	//}
	//else
	{
		SFLASH_WREG8(SFLASH_READ_DUAL_REG, SFLASH_RREG8(SFLASH_READ_DUAL_REG) | 0x0);
	}
	SFLASH_WREG8(SFLASH_RADR2_REG, LoByte(HiWord(u4Addr))); // Set Addr
	SFLASH_WREG8(SFLASH_RADR1_REG, HiByte(LoWord(u4Addr)));
	SFLASH_WREG8(SFLASH_RADR0_REG, LoByte(LoWord(u4Addr)));

	for (i = 0; i < u4Len; i++) {
		_SendFlashCommand(0x81); // Read and autoinc address
		if ( _PollingReg(SFLASH_CMD_REG, 0x01) != 0) { // timeout
			nor_debug( "...cdd SFLASHHW_Read timeout !\n");
			return 1;
		}
		#ifdef ENABLE_CONTROLLER_STATUS
		nor_debug("Serial flash control starus:0x87b4=0x%08x!\n", SFLASH_RREG32(SFLASH_SAMPLE_EDGE_REG));
		#endif
		pu1buf[i] = SFLASH_RREG8(SFLASH_RDATA_REG);                             // Get data
	}
	SFLASH_WREG8(SFLASH_READ_DUAL_REG, SFLASH_RREG8(SFLASH_READ_DUAL_REG) & 0x10);  //clear read mode setting

	return 0;
}



static INT32 _ExecuteWriteCmd(uint32_t u4Index)
{
	UINT8 u1Reg;

	if (_fgSFIsrEn) {
		_u4SFIsrStatus = 0;
		if (_fgAAIWrite == FALSE) {
			SFLASH_WREG32(SFLASH_SF_INTRSTUS_REG, 0x10);    // Clear interrupt
			SFLASH_WREG32(SFLASH_SF_INTREN_REG, 0x10);
		}else {
			SFLASH_WREG32(SFLASH_SF_INTRSTUS_REG, 0x40);    // Clear interrupt
			SFLASH_WREG32(SFLASH_SF_INTREN_REG, 0x40);
		}
	}
	//SFLASHHW_SwitchChip(u4Index);
	if (u4Index == 0) {
		if (_fgAAIWrite == FALSE) {
			_SendFlashCommand(0x10);
		}       else{
			SFLASH_WREG8(SFLASH_CMD2_REG, 0x01);
		}
	}else {
		assert(0);
	}

	if (_fgSFIsrEn) {
		while (((_fgAAIWrite == FALSE) && (!(_u4SFIsrStatus & 0x10))) || ((_fgAAIWrite == TRUE) && (!(_u4SFIsrStatus & 0x40)))) {
		}
	}else {
		while (1) {
			if (_fgAAIWrite == FALSE) {
				u1Reg = SFLASH_RREG8(SFLASH_CMD_REG);
				if (0x0 == (u1Reg & 0x10)) {
					break;
				}
			}else {
				u1Reg = SFLASH_RREG8(SFLASH_CMD2_REG);
				if (0x0 == (u1Reg & 0x01)) {
					break;
				}
			}
		}
	}
	if (_fgSFIsrEn) {
		SFLASH_WREG32(SFLASH_SF_INTREN_REG, 0x0);   // Disable interrupt
	}
	_SendFlashCommand(0x0);
	//SFLASHHW_SwitchChip(0);
	return 0;
}

static INT32 _WriteBuffer(uint32_t u4Index, uint32_t u4Addr, uint32_t u4Len, const uint8_t* pu1Buf)
{
	uint32_t i, j, u4BufIdx, u4Data;

	if (pu1Buf == NULL) {
		return 1;
	}
	assert(u4Len <= SFLASH_WRBUF_SIZE);
	assert((u4Addr % SFLASH_HW_ALIGNMENT) == 0);
	assert((u4Len % SFLASH_HW_ALIGNMENT) == 0);

	SFLASH_WREG8(SFLASH_RADR2_REG, LoByte(HiWord(u4Addr))); // Write
	SFLASH_WREG8(SFLASH_RADR1_REG, HiByte(LoWord(u4Addr))); // Write
	SFLASH_WREG8(SFLASH_RADR0_REG, LoByte(LoWord(u4Addr))); // Write

	u4BufIdx = 0;
	for (i = 0; i < u4Len; i += 4) {
		for (j = 0; j < 4; j++) {
			(*((UINT8*)&u4Data + j)) = pu1Buf[u4BufIdx];
			u4BufIdx++;
		}
		SFLASH_WREG32(SFLASH_PP_DATA_REG, u4Data);
	}
	if (_ExecuteWriteCmd(u4Index) != 0) {
		return 1;
	}

	if (_WaitForWriteBusy(u4Index, SFLASH_WRITEBUSY_TIMEOUT) != 0) {
		return 1;
	}
	return 0;
}
static INT32 _WriteSingleByte(uint32_t u4Index, uint32_t u4Addr, uint8_t u1Data)
{
	SFLASH_WREG8(SFLASH_RADR2_REG, LoByte(HiWord(u4Addr)));
	SFLASH_WREG8(SFLASH_RADR1_REG, HiByte(LoWord(u4Addr)));
	SFLASH_WREG8(SFLASH_RADR0_REG, LoByte(LoWord(u4Addr)));
	SFLASH_WREG8(SFLASH_WDATA_REG, u1Data);
	if (_ExecuteWriteCmd(u4Index) != 0) {
		return 1;
	}
	if (_WaitForWriteBusy(u4Index, SFLASH_WRITEBUSY_TIMEOUT) != 0) {
		return 1;
	}

	return 0;
}
/* Brief of SFLASHHW_WriteSector.  Details of SFLASHHW_WriteSector (optional).  @retval TRUE    Success  @retval FALSE   Fail*/

INT32 SFLASHHW_WriteSector(uint32_t u4Index, uint32_t u4Addr, uint32_t u4Len,
			   const UINT8* pu1Buf)
{
	uint32_t i, u4Count, u4PgAlign;

	if (u4Index >= MAX_FLASHCOUNT) {
		nor_debug("Nor Flash Index is out fo Max Flash Count\n");
		return -1;
	}
	assert(_arFlashChip[u4Index].u1MenuID != 0x00);
	assert(u4Len <= _arFlashChip[u4Index].u4SecSize);

	if (u4Len == 0) {
		return 0;
	}

	if (pu1Buf == NULL) {
		return 1;
	}
	if (_SetFlashWriteEnable(u4Index) != 0) {
		return 1;
	}
	u4PgAlign = u4Addr % SFLASH_WRBUF_SIZE;
	if (u4PgAlign != 0) {
		_fgAAIWrite = FALSE;
		for (i = 0; i < (SFLASH_WRBUF_SIZE - u4PgAlign); i++) {
			if (_WriteSingleByte(u4Index, u4Addr, *pu1Buf) != 0) {
				return 1;
			}
			u4Addr++;
			pu1Buf++;
			u4Len--;

			if (u4Len == 0) {
				return 0;
			}
		}
	}
	if (_WBEnable() != 0) {
		return 1;
	}
	if (_arFlashChip[u4Index].u1PPType == 0) {
		while ((INT32)u4Len > 0) {
			if (u4Len >= SFLASH_WRBUF_SIZE) {
				u4Count = SFLASH_WRBUF_SIZE;
			}else {
				/* Not write-buffer alignment */
				break;
			}
			if (_WriteBuffer(u4Index, u4Addr, u4Count, pu1Buf) != 0) {
				nor_debug("Write flash error!! faddr = 0x%x, len = 0x%x\n", u4Addr, u4Count);
				if (_WBDisable() != 0) {
					return 1;
				}
				return 1;
			}
			u4Len -= u4Count;
			u4Addr += u4Count;
			pu1Buf += u4Count;

		}
		if (_WBDisable() != 0) {
			return 1;
		}
	}
	if ((INT32)u4Len > 0) {
		_fgAAIWrite = FALSE;
		for (i = 0; i < u4Len; i++) {
			if (_WriteSingleByte(u4Index, u4Addr, *pu1Buf) != 0) {
				if (_WBDisable() != 0) {
					return 1;
				}
				return 1;
			}
			u4Addr++;
			pu1Buf++;
		}
	}
	return 0;
}

/* Brief of SFLASHHW_EraseSector. Details of SFLASHHW_EraseSector (optional).  @retval TRUE    Success  @retval FALSE   Fail*/
INT32 SFLASHHW_EraseSector(uint32_t u4Index, uint32_t u4Addr)
{
	uint32_t u4Polling;
	UINT8 u1Reg;

	if (u4Index >= MAX_FLASHCOUNT) {
		nor_debug("Nor Flash Index is out fo Max Flash Count\n");
		return -1;
	}
	assert(_arFlashChip[u4Index].u1MenuID != 0x00);
	if (_WaitForWriteBusy(u4Index, SFLASH_WRITEBUSY_TIMEOUT) != 0) {
		return 1;
	}
	if (_SetFlashWriteEnable(u4Index) != 0) {
		return 1;
	}
	// Execute sector erase command
	SFLASH_WREG8(SFLASH_PRGDATA5_REG, _arFlashChip[u4Index].u1SECERASECmd);
	SFLASH_WREG8(SFLASH_PRGDATA4_REG, LoByte(HiWord(u4Addr)));      // Write
	SFLASH_WREG8(SFLASH_PRGDATA3_REG, HiByte(LoWord(u4Addr)));      // Write
	SFLASH_WREG8(SFLASH_PRGDATA2_REG, LoByte(LoWord(u4Addr)));      // Write
	SFLASH_WREG8(SFLASH_CNT_REG, 32);                               // Write SF Bit Count
	//use cpu polling
	if (u4Index == 0) {
		_SendFlashCommand(0x4);
	}else {
		assert(0);
	}
	// Can not use ISR mode, because there is not erase sector interrupt
	u4Polling = 0;
	while (1) {
		if (SFLASHHW_ReadFlashStatus(u4Index, &u1Reg) != 0) {
			//SFLASHHW_SwitchChip(0);
			return 1;
		}
		if (0 == (u1Reg & 0x1)) {
			break;
		}
		u4Polling++;
		if (u4Polling > SFLASH_ERASESECTOR_TIMEOUT) {
			//	SFLASHHW_SwitchChip(0);

			return 1;
		}
		mdelay(10);
	}
	_SendFlashCommand(0x0);
	//SFLASHHW_SwitchChip(0);
	return 0;
}


/* Brief of SFLASHHW_EraseChip.  Details of SFLASHHW_EraseChip (optional).  @retval TRUE    Success  @retval FALSE   Fail */

INT32 SFLASHHW_EraseChip(uint32_t u4Index)
{
	UINT8 u1Reg;

	assert(_arFlashChip[u4Index].u1MenuID != 0x00);

	if (u4Index >= _u4ChipCount) {
		nor_debug("Flash chip not exist for %d\n", u4Index);
		return 1;
	}
	if (_WaitForWriteBusy(u4Index, SFLASH_WRITEBUSY_TIMEOUT) != 0) {
		return 1;
	}
	if (_SetFlashWriteEnable(u4Index) != 0) {
		return 1;
	}

	if (_fgSFIsrEn) {
		_u4SFIsrStatus = 0;
		SFLASH_WREG32(SFLASH_SF_INTRSTUS_REG, 0x8); // Clear interrupt
		SFLASH_WREG32(SFLASH_SF_INTREN_REG, 0x8);
	}
	if (u4Index == 0) {
		_SendFlashCommand(0x8);
	}else {
		assert(0);
	}

	if (_fgSFIsrEn) {
		while (!(_u4SFIsrStatus & 0x8)) {

		}
	}else {
		while (1) {
			u1Reg = SFLASH_RREG8(SFLASH_CMD_REG);
			if (0x0 == (u1Reg & 0x8)) {
				break;
			}
		}
	}

	if (_fgSFIsrEn) {
		SFLASH_WREG32(SFLASH_SF_INTREN_REG, 0x0); // Disable interrupt
	}

	_SendFlashCommand(0x0);


	return 0;
}

#define OSR_OK                    ((INT32)0)
#define VECTOR_FLASH              35            //Serial Flash

/* Brief of SFLASHHW_WriteSfProtect.  Details of SFLASHHW_WriteSfProtect (u4Val).*/
void SFLASHHW_WriteSfProtect(uint32_t u4Val)
{
	nor_debug("...cdd SFLASHHW_WriteSfProtect u4Val = 0x%x!\n", u4Val);

	SFLASH_WREG32(SFLASH_WRPROT_REG, u4Val);
}

/* Brief of SFLASHHW_ReadSfProtect. Details of SFLASHHW_ReadSfProtect ().*/
uint32_t SFLASHHW_ReadSfProtect(void)
{
	return SFLASH_RREG32(SFLASH_WRPROT_REG);
}

/* Brief of SFLASHHW_WriteReg.  Details of SFLASHHW_WriteReg .*/

void SFLASHHW_WriteReg(uint32_t uAddr, uint32_t u4Val)
{
	SFLASH_WREG32(uAddr, u4Val);
}

/* Brief of SFLASHHW_ReadReg. Details of SFLASHHW_ReadReg ().*/

uint32_t SFLASHHW_ReadReg(uint32_t uAddr)
{
	return SFLASH_RREG32(uAddr);
}

/* Brief of SFLASHHW_SwitchChip.  Details of SFLASHHW_SwitchChip ().*/
uint32_t SFLASHHW_SwitchChip(uint32_t uAddr)
{
	uint32_t uDualReg = SFLASH_RREG32(SFLASH_READ_DUAL_REG);

	if (uAddr == 0) {
		uDualReg &= 0xFFFFFF7F;
		SFLASH_WREG32(SFLASH_READ_DUAL_REG, uDualReg);
	}else if (uAddr == 1) {
		uDualReg |= 0x80;
		SFLASH_WREG32(SFLASH_READ_DUAL_REG, uDualReg);
	}else {
		return SFLASH_RREG32(uAddr);
	}
	return 0;
}
/*
   int nor_bread(struct blkdev * bdev, u32 blknr, u32 blks, u8 * buf, u32 part_id)
   {
        u32 i,ret;
        u32 offset = blknr * bdev->blksz;

        u8 * ptr = buf;
        u32 u4Bytelen = blks * bdev->blksz;
        DEBUG_NOR(BIOS_DEBUG, "we are in the nor_bread,blknr:0x%x, blks:0x%x, offset: 0x%x ,bdev->blksz: %x \n",blknr,blks,offset,bdev->blksz);
        DEBUG_NOR(BIOS_DEBUG,  "part_id : 0x%x \n",part_id);

        if((ret = NOR_Read((unsigned int )offset,(unsigned int)(uintptr_t)buf,u4Bytelen)) != 0 )
        {
                DEBUG_NOR(BIOS_DEBUG, "partitial Read test fail !!! \n");
        }
        for(i=0; i<100 ;i++)
                DEBUG_NOR(BIOS_DEBUG, "0x%x ",*(ptr+i));
        DEBUG_NOR(BIOS_DEBUG, "\n");
        DEBUG_NOR(BIOS_DEBUG, "printf buf over in nand_bread  \n");
    return 0;
   }

   int nor_bwrite(struct blkdev * bdev, u32 blknr, u32 blks, u8 * buf, u32 part_id)
   {
    u32 ret;
    u32 offset = blknr * bdev->blksz;
        u32 u4MemPtr = (u32)(uintptr_t)buf;
        u32 u4Bytelen = blks * bdev->blksz;

   if((ret = NOR_Write(offset,u4MemPtr,u4Bytelen)) != 0)
   {
        DEBUG_NOR(BIOS_DEBUG, "partitial write test fail !!! \n");
   }
    return 0;
   }
 */

/* Brief of SFLASHHW_WriteProtect.Details of SFLASHHW_WriteProtect (optional).*/
INT32 SFLASHHW_WriteProtect(uint32_t u4Index, BOOL fgEnable)
{
	if (u4Index >= MAX_FLASHCOUNT) {
		nor_debug("Nor Flash Index is out fo Max Flash Count\n");
		return -1;
	}
	if (_fgWriteProtect) {
		if (!_fgDoProtect) {
			return 0;
		}
		if (_WaitForWriteBusy(u4Index, SFLASH_WRITEBUSY_TIMEOUT) != 0) {
			return -1;
		}
		if (_SetFlashWriteEnable(u4Index) != 0) {
			return -1;
		}
		if (_fgAAIWrite) {
			nor_debug("AAI Write Protect Enable flag %d\n", fgEnable);
			SFLASHHW_SendByte(u4Index, 0x50);                                               // EWSR
			if (fgEnable) {
				SFLASHHW_Send2Byte(u4Index, _arFlashChip[u4Index].u1WRSRCmd, 0x3C);     // WRSR
			}else {
				if (_fgNorPartialProtection) {
					SFLASHHW_Send2Byte(u4Index, _arFlashChip[u4Index].u1WRSRCmd, _arFlashChip[u4Index].u1Protection);       // WRSR
				}else {
					SFLASHHW_Send2Byte(u4Index, _arFlashChip[u4Index].u1WRSRCmd, 0x00);                                     // WRSR
				}
			}
			if (_WaitForWriteBusy(u4Index, SFLASH_WRITEBUSY_TIMEOUT) != 0) {
				return -1;
			}
		}
		if (fgEnable) {
			SFLASH_WREG8(SFLASH_PRGDATA5_REG, 0x3C);
		}else {
			if (_fgNorPartialProtection) { //_fgNorPartialProtection is TV used only
				SFLASH_WREG32(SFLASH_PRGDATA5_REG, _arFlashChip[u4Index].u1Protection);
			}else {
				SFLASH_WREG32(SFLASH_PRGDATA5_REG, 0x0);
			}
		}
		SFLASH_WREG8(SFLASH_CNT_REG, 8); // Write SF Bit Count

		if (u4Index == 0) {
			_SendFlashCommand(0x20);
		}else {
			assert(0);
		}

		if (_PollingReg(SFLASH_CMD_REG, 0x20) != 0) {
			return -1;
		}

		if (_WaitForWriteBusy(u4Index, SFLASH_WRITEBUSY_TIMEOUT) != 0) {
			return -1;
		}
	}

	return 0;
}
/* Brief of SFLASHHW_GetFlashName.  Details of SFLASHHW_GetFlashName (optional).*/
char* SFLASHHW_GetFlashName(uint8_t u1MenuID_1, uint8_t u1DevID1_1, uint8_t u1DevID2_1)
{
	char *pStr;
	uint32_t i;

	i = 0;
	while (_aVendorFlash[i].u1MenuID != (uint8_t)0x0) {
		if ( (_aVendorFlash[i].u1MenuID == u1MenuID_1) &&
		     (_aVendorFlash[i].u1DevID1 == u1DevID1_1) &&
		     (_aVendorFlash[i].u1DevID2 == u1DevID2_1)) {
			pStr = _aVendorFlash[i].pcFlashStr;
			return pStr;
		}
		i++;
	}
	return NULL;
}
/* Brief of SFLASHHW_GetFlashSize.  Details of SFLASHHW_GetFlashSize (optional) */

uint32_t SFLASHHW_GetFlashSize(uint8_t u1MenuID_1, uint8_t u1DevID1_1, uint8_t u1DevID2_1)
{
	uint32_t u4Size;
	uint32_t i;

	i = 0;
	while (_aVendorFlash[i].u1MenuID != (uint8_t)0x0) {
		if ( (_aVendorFlash[i].u1MenuID == u1MenuID_1) &&
		     (_aVendorFlash[i].u1DevID1 == u1DevID1_1) &&
		     (_aVendorFlash[i].u1DevID2 == u1DevID2_1)) {
			u4Size = _aVendorFlash[i].u4ChipSize;
			return u4Size;
		}
		i++;
	}
	return 0;
}
