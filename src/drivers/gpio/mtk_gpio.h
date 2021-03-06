#ifndef __DRIVERS_MT_GPIO_H__
#define __DRIVERS_MT_GPIO_H__

#include "drivers/gpio/gpio.h"

enum {
	GPIO_MAX = 135,
};

/*----------------------------------------------------------------------------*/
/* GPIO OUTPUT */
typedef enum {
	GPIO_OUT_UNSUPPORTED = -1,
	GPIO_OUT_ZERO = 0,
	GPIO_OUT_ONE  = 1,

	GPIO_OUT_MAX,
	GPIO_OUT_DEFAULT = GPIO_OUT_ZERO,
	GPIO_DATA_OUT_DEFAULT = GPIO_OUT_ZERO,  /* compatible with DCT */
} GPIO_OUT;
/* GPIO INPUT */
typedef enum {
	GPIO_IN_UNSUPPORTED = -1,
	GPIO_IN_ZERO = 0,
	GPIO_IN_ONE  = 1,

	GPIO_IN_MAX,
} GPIO_IN;

/*----------------------------------------------------------------------------*/
typedef struct {
	uint16_t val;
	uint16_t _align1;
	uint16_t set;
	uint16_t _align2;
	uint16_t rst;
	uint16_t _align3[3];
} VAL_REGS;
/*----------------------------------------------------------------------------*/
typedef struct {
	VAL_REGS dir[9];	        /* 0x0000 ~ 0x008F: 144 bytes */
	uint8_t rsv00[112];	/* 0x0090 ~ 0x00FF: 112 bytes */
	VAL_REGS pullen[9];	        /* 0x0100 ~ 0x018F: 144 bytes */
	uint8_t rsv01[112];	/* 0x0190 ~ 0x01FF: 112 bytes */
	VAL_REGS pullsel[9];		/* 0x0200 ~ 0x028F: 144 bytes */
	uint8_t rsv02[112];	/* 0x0290 ~ 0x02FF: 112 bytes */
	uint8_t rsv03[256];	/* 0x0300 ~ 0x03FF: 256 bytes */
	VAL_REGS dout[9];	        /* 0x0400 ~ 0x048F: 144 bytes */
	uint8_t rsv04[112];	/* 0x0490 ~ 0x04FF: 112 bytes */
	VAL_REGS din[9];	        /* 0x0500 ~ 0x058F: 114 bytes */
	uint8_t rsv05[112];	/* 0x0590 ~ 0x05FF: 112 bytes */
	VAL_REGS mode[27];	        /* 0x0600 ~ 0x07AF: 432 bytes */
	uint8_t rsv06[336];	/* 0x07B0 ~ 0x08FF: 336 bytes */
	VAL_REGS ies[3];	        /* 0x0900 ~ 0x092F:  48 bytes */
	VAL_REGS smt[3];	        /* 0x0930 ~ 0x095F:  48 bytes */
	uint8_t rsv07[160];	/* 0x0960 ~ 0x09FF: 160 bytes */
	VAL_REGS tdsel[8];	        /* 0x0A00 ~ 0x0A7F: 128 bytes */
	VAL_REGS rdsel[6];	        /* 0x0A80 ~ 0x0ADF:  96 bytes */
	uint8_t rsv08[32];	/* 0x0AE0 ~ 0x0AFF:  32 bytes */
	VAL_REGS drv_mode[10];		/* 0x0B00 ~ 0x0B9F: 160 bytes */
	uint8_t rsv09[96];	/* 0x0BA0 ~ 0x0BFF:  96 bytes */
	VAL_REGS msdc0_ctrl0;		/* 0x0C00 ~ 0x0C0F:  16 bytes */
	VAL_REGS msdc0_ctrl1;		/* 0x0C10 ~ 0x0C1F:  16 bytes */
	VAL_REGS msdc0_ctrl2;		/* 0x0C20 ~ 0x0C2F:  16 bytes */
	VAL_REGS msdc0_ctrl5;		/* 0x0C30 ~ 0x0C3F:  16 bytes */
	VAL_REGS msdc1_ctrl0;		/* 0x0C40 ~ 0x0C4F:  16 bytes */
	VAL_REGS msdc1_ctrl1;		/* 0x0C50 ~ 0x0C5F:  16 bytes */
	VAL_REGS msdc1_ctrl2;		/* 0x0C60 ~ 0x0C6F:  16 bytes */
	VAL_REGS msdc1_ctrl5;		/* 0x0C70 ~ 0x0C7F:  16 bytes */
	VAL_REGS msdc2_ctrl0;		/* 0x0C80 ~ 0x0C8F:  16 bytes */
	VAL_REGS msdc2_ctrl1;		/* 0x0C90 ~ 0x0C9F:  16 bytes */
	VAL_REGS msdc2_ctrl2;		/* 0x0CA0 ~ 0x0CAF:  16 bytes */
	VAL_REGS msdc2_ctrl5;		/* 0x0CB0 ~ 0x0CBF:  16 bytes */
	VAL_REGS msdc3_ctrl0;		/* 0x0CC0 ~ 0x0CCF:  16 bytes */
	VAL_REGS msdc3_ctrl1;		/* 0x0CD0 ~ 0x0CDF:  16 bytes */
	VAL_REGS msdc3_ctrl2;		/* 0x0CE0 ~ 0x0CEF:  16 bytes */
	VAL_REGS msdc3_ctrl5;		/* 0x0CF0 ~ 0x0CFF:  16 bytes */
	VAL_REGS msdc0_ctrl3;		/* 0x0D00 ~ 0x0D0F:  16 bytes */
	VAL_REGS msdc0_ctrl4;		/* 0x0D10 ~ 0x0D1F:  16 bytes */
	VAL_REGS msdc1_ctrl3;		/* 0x0D20 ~ 0x0D2F:  16 bytes */
	VAL_REGS msdc1_ctrl4;		/* 0x0D30 ~ 0x0D3F:  16 bytes */
	VAL_REGS msdc2_ctrl3;		/* 0x0D40 ~ 0x0D4F:  16 bytes */
	VAL_REGS msdc2_ctrl4;		/* 0x0D50 ~ 0x0D5F:  16 bytes */
	VAL_REGS msdc3_ctrl3;		/* 0x0D60 ~ 0x0D6F:  16 bytes */
	VAL_REGS msdc3_ctrl4;		/* 0x0D70 ~ 0x0D7F:  16 bytes */
	uint8_t rsv10[64];	/* 0x0D80 ~ 0x0DBF:  64 bytes */
	VAL_REGS exmd_ctrl[1];		/* 0x0DC0 ~ 0x0DCF:  16 bytes */
	uint8_t rsv11[48];	/* 0x0DD0 ~ 0x0DFF:  48 bytes */
	VAL_REGS kpad_ctrl[2];		/* 0x0E00 ~ 0x0E1F:  32 bytes */
	VAL_REGS hsic_ctrl[4];		/* 0x0E20 ~ 0x0E5F:  64 bytes */
} GPIO_REGS;
/*----------------------------------------------------------------------------*/
struct mt_gpio_obj {
	GPIO_REGS	*reg;
};

typedef struct MtGpio {
	GpioOps ops;
	u32 pin_num;
	struct mt_gpio_obj obj;
} MtGpio;

GpioOps *new_mtk_gpio_input(u32 pin);
GpioOps *new_mtk_gpio_output(u32 pin);

#endif /* __DRIVERS_MT_GPIO_H__ */

