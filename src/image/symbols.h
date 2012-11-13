/*
 * Copyright (c) 2011-2012 The Chromium OS Authors.
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

#ifndef __IMAGE_SYMBOLS_H__
#define __IMAGE_SYMBOLS_H__

#include <stdint.h>

// C level variable definitions for symbols defined in the linker script.

extern const uint8_t _start;
extern const uint8_t _edata;
extern const uint8_t _heap;
extern const uint8_t _eheap;
extern const uint8_t _estack;
extern const uint8_t _stack;
extern const uint8_t _end;
extern const uint8_t _tramp_start;
extern const uint8_t _tramp_end;
extern const uint8_t _kernel_start;
extern const uint8_t _kernel_end;

#define ENTRY __attribute__((section(".text._entry")))
#define CPARAMS __attribute__((section(".cparams")))
#define SHARED_DATA __attribute__((section(".shared_data")))

#endif /* __IMAGE_SYMBOLS_H__ */