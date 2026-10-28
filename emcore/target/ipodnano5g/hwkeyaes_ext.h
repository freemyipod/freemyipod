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


#ifndef __HWKEYAES_EXT_H__
#define __HWKEYAES_EXT_H__

#include "global.h"
#include "hwkeyaes.h"

/* S5L8730 AES encryption interface. */

/* key selectors written to +0x6c (fw_disk aes_crypt 0x080304b8 accepts 0..3; others -> error 9) */
#define HWKEYAES_SEL_USER   0   /* key from +0x4c..+0x6c (written by the driver) */
#define HWKEYAES_SEL_1      1   /* hardware key selector 1 */
#define HWKEYAES_SEL_2      2   /* hardware key 2: UID on the 4G (fw_disk derives with it: 0x0803806c, 0x0804149c) */
#define HWKEYAES_SEL_KEEP   3   /* fw_disk skips the engine reset for this selector; meaning not RE'd */

#define HWKEYAES_FLAG_ECB   1   /* software ECB (block by block, zero IV): the hardware has no known ECB bit */
#define HWKEYAES_FLAG_IVOUT 2   /* write the CBC chaining value back to iv[] (for multi-call CBC) */

#define HWKEYAES_ERR_ZERO     0x28   /* fw_disk aes_crypt: len == 0 */
#define HWKEYAES_ERR_SIZE     9      /* fw_disk aes_crypt: len not a multiple of 16 / bad selector */
#define HWKEYAES_ERR_TIMEOUT  -1

int hwkeyaes_encrypt(uint32_t sel, uint32_t keybits, const void* key,
                   void* iv, void* out, const void* in, uint32_t size, uint32_t flags);

/* Buffers must be word aligned, physically addressed, and exclusively owned
   (including their 32-byte cache lines) until the call returns. */

/* USB command 25, flags byte (byte 5 of the request) == 0: legacy in-place,
   AES-128 CBC zero IV. Both forms return a signed driver status in reply word 1.
   flags byte != 0:
     bit0 = extended form, bits2:1 = key size (0: 128, 1: 192, 2: 256), bit3 = ECB, bit4 = IV write-back.
   addr -> struct hwkeyaes_usbparams (32-byte aligned) immediately followed by the payload, processed in place. */
struct hwkeyaes_usbparams
{
    uint32_t key[8];    /* user key bytes (selector 0), first keybits/8 bytes used */
    uint32_t iv[4];     /* CBC IV in, chaining value out (flag bit4) */
};

int hwkeyaes_usb_encrypt(uint32_t sel, uint32_t flagsbyte, void* addr, uint32_t size);

#endif
