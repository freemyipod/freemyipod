//
//
//    Copyright 2010 TheSeven
//
//
//    This file is part of emCORE.
//
//    emCORE is free software: you can redistribute it and/or
//    modify it under the terms of the GNU General Public License as
//    published by the Free Software Foundation, either version 2 of the
//    License, or (at your option) any later version.
//
//    emCORE is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//    See the GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License along
//    with emCORE.  If not, see <http://www.gnu.org/licenses/>.
//
//

//    iPod nano 5G (S5L8730, ARM1176JZF-S) target configuration.
//    Peripheral drivers and the Rockbox NAND harness are selected below.

#ifndef __TARGET_H__
#define __TARGET_H__


#define PLATFORM_ID 0x47354e49   /* 'IN5G' (4G: 0x47344e49 'IN4G') */


#define ARM_ARCH 6
#define LITTLE_ENDIAN
#define CACHEALIGN_BITS 5
/* FCLK after the stage-1 loader: PLL0 = 24 MHz x 206 / (9 << 1) = 274.67 MHz (bl_PreEfi
   literal 0x2200b21c.. PLL0PMS 0x0900ce01; formula bl_ClockAndReset:0x5f6). HCLK = PCLK = FCLK/2 = 137.3 MHz (C13/C14). */
#define CPU_FREQ 274670000


#define CONSOLE_BOOT 3
#define CONSOLE_PANIC 3
#define CONSOLE_PANICDUMP 0


#include "s5l8730.h"
#define HAVE_USB
#define USB_DRIVER_HEADER "usb/synopsysotg.h"
#define USB_DRIVER synopsysotg_driver
#define USB_DRIVER_CONFIG_TYPE const struct synopsysotg_config
/* fw_disk usb_otg_core_init 0x08048ba4: 16-bit UTMI+ PHY (GUSBCFG 0x1408), DMA (GAHBCFG 0x2b,
   DOEPDMA0 0x080432ac), dedicated TX FIFOs (GHWCFG4 b25 path 0x0804daa8).
   PROVISIONAL: fifosize/txfifosize are the 4G's. Apple computes the FIFO layout from GHWCFG1..4 (fw_disk:0x08048de8),
   which are not in any image: read 0x38400044..0x38400050 on the device and keep
   fifosize <= GHWCFG3[31:16]. RX = 0x1c0 words here, Apple's formula gives >= 0x110. */
#define USB_DRIVER_CONFIG \
{ \
    .core = (struct synopsysotg_core_regs*)OTGBASE, \
    .phy_16bit = true, \
    .phy_ulpi = false, \
    .use_dma = true, \
    .shared_txfifo = false, \
    .disable_double_buffering = false, \
    .fifosize = 1024, \
    .txfifosize = { 0x40, 0x100, 0, 0x100, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, \
}
#define USB_DRIVER_STATE_TYPE struct synopsysotg_state
#define USB_DRIVER_STATE \
{ \
    .endpoints = { {}, {}, {}, {}, {} }, \
}
#define USB_ENDPOINTS 0b0000000000010110000000000010101   /* = 4G (IN 0,1,3 / OUT 0,2,4); check GHWCFG1 on the device */
#define USB_MAXCURRENT 500      /* config bMaxPower 0xfa, fw_disk:0x0807f3e4; applied to the PMU only after SET_CONFIGURATION (power.c) */
//#define USB_HAVE_TARGET_SPECIFIC_REQUESTS


#define HAVE_LCD
#define LCD_WIDTH 240            /* bl_DisplayPlatform:0x568 {0xf0, 0x178}, fw_disk:0x0807d498  */
#define LCD_HEIGHT 376
#define LCD_FORMAT 0x004154b4  // rgb565, red in bits 15..11 (bl_Gop:0x2e6)
#define LCD_BYTESPERPIXEL 2
#define LCDCONSOLE_FGCOLOR 0
#define LCDCONSOLE_BGCOLOR -1

#define HAVE_BACKLIGHT

#define HAVE_I2C

/* HAVE_HWKEYAES is enabled with its driver. */
/* HAVE_HWKEYAES_EXT disabled. */
/* user keys 128/192/256, IV, ECB: hwkeyaes_ext.h, USB cmd 25 flags byte */

/* HAVE_ACCELEROMETER is enabled with its driver. */

/* HAVE_BUTTON is enabled with its driver. */
/* HAVE_CLICKWHEEL is enabled with its driver. */

/* targetinit_late(): click wheel start + a register report once I2C/USB are up (targetinit.c) */
#define HAVE_TARGETINIT_LATE

/* no HAVE_STORAGE: the NAND harness (nand5g.c on the Rockbox driver copy) is reached over USB only */


#endif
