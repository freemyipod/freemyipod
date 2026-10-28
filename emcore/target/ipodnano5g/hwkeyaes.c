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


#include "global.h"
#include "hwkeyaes.h"
#include "target/ipodnano5g/hwkeyaes_ext.h"
#include "s5l8730.h"
#include "thread.h"
#include "timer.h"
#include "clockgates.h"
#include "mmu.h"
#include "util.h"
#include "libc/include/string.h"


/* S5L8730 AES encryption: user/hardware keys, CBC and software ECB. */

struct mutex hwkeyaes_mutex;

#define AES_MODE_BASE    0xf          /* bits 3:1 constant (fw_disk 0x0803052c, bootrom 0x20001e34); bit 0 = encrypt */
#define AES_POLL_US      200000       /* + 2 us per byte: hang guard, Apple polls forever */


/* One engine run over [in, in+size) -> out. Caller holds the mutex and the clock gate. 0 = ok, -1 = timeout.
   Order of the register writes = fw_disk aes_crypt (0x080304d8..0x08030574), differences marked. */
static int hwkeyaes_run(uint32_t sel, uint32_t keycode,
                        const uint32_t* key, uint32_t keywords, const uint32_t* iv,
                        void* out, const void* in, uint32_t size)
{
    int i;
    if (sel != HWKEYAES_SEL_KEEP)          /* fw skips the reset only for selector 3 */
    {
        uint32_t start = USEC_TIMER;
        AESUNKREG0 = 1;                    /* +0x08 = 1: engine reset, polled back to 0 */
        while (AESUNKREG0)
        {
            if (TIME_AFTER(USEC_TIMER, start + AES_POLL_US))
            { AESCONTROL = 0; return HWKEYAES_ERR_TIMEOUT; }
            yield();
        }
    }
    AESUNKREG3 = 1;                        /* +0x70 = 1 (fw 0x080304f0) */
    if (sel == HWKEYAES_SEL_USER)          /* key words right aligned so that they end at +0x6c (bootrom 0x20001e34) */
        for (i = 0; i < (int)keywords; i++) AESKEY[8 - keywords + i] = key[i];
    AESTYPE = sel;                         /* +0x6c key selector, +0x88 = ~selector */
    AESTYPE2 = ~sel;
    AESCONTROL = 1;                        /* +0x00 = 1 */
    AESKEYLEN = AES_MODE_BASE | (keycode << 4);   /* +0x14 */
    AESOUTSIZE = size;                     /* +0x18 */
    AESINADDR = in;                        /* +0x28 */
    AESAUXSIZE = size;                     /* +0x2c */
    AESOUTADDR = out;                      /* +0x20 */
    AESINSIZE = size;                      /* +0x24 */
    AESAUXADDR = (void*)in;                       /* +0x30 */
    AESSIZE3 = size;                       /* +0x34 */
    for (i = 0; i < 4; i++) AESIV[i] = iv ? iv[i] : 0;   /* +0x74..+0x80 */
    AESSTATUS = 7;                         /* clear status */
    /* Publish CPU writes before DMA and preserve bytes sharing the end lines.
       The caller must exclusively own the buffer's cache lines during DMA. */
    clean_dcache_range((uint32_t)in, size);
    clean_dcache_range((uint32_t)out, size);
    invalidate_dcache_range((uint32_t)out, size);
    AESGO = 1;                             /* +0x04 */
    {
        uint32_t start = USEC_TIMER;
        while (!(AESSTATUS & 1))
            if (TIME_AFTER(USEC_TIMER, start + AES_POLL_US + 2 * size))
            {
                AESCONTROL = 0;
                invalidate_dcache_range((uint32_t)out, size);
                return HWKEYAES_ERR_TIMEOUT;
            }
    }
    AESCONTROL = 0;                        /* +0x00 = 0 */
    invalidate_dcache_range((uint32_t)out, size);
    return 0;
}


/* Full engine. Returns 0, HWKEYAES_ERR_SIZE (0x28 / 9 as in Apple's routine) or HWKEYAES_ERR_TIMEOUT.
   in may equal out. iv (16 bytes, may be NULL = zero) is updated with the chaining value when HWKEYAES_FLAG_IVOUT. */
int hwkeyaes_encrypt(uint32_t sel, uint32_t keybits, const void* key,
                   void* iv, void* out, const void* in, uint32_t size, uint32_t flags)
{
    uint32_t keycode, keywords, ivbuf[4], next[4];
    int rc = 0;
    if (size == 0) return HWKEYAES_ERR_ZERO;
    if (size & 15) return HWKEYAES_ERR_SIZE;
    if (size > (0x7fffffffu - AES_POLL_US) / 2) return HWKEYAES_ERR_SIZE;
    if (!in || !out || ((uint32_t)in & 3) || ((uint32_t)out & 3)
        || (uint32_t)in + size < (uint32_t)in || (uint32_t)out + size < (uint32_t)out)
        return HWKEYAES_ERR_SIZE;
    if (flags & ~(HWKEYAES_FLAG_ECB | HWKEYAES_FLAG_IVOUT)) return HWKEYAES_ERR_SIZE;
    if (sel > HWKEYAES_SEL_KEEP) return HWKEYAES_ERR_SIZE;
    if (keybits == 0) keybits = 128;
    if (keybits != 128 && keybits != 192 && keybits != 256) return HWKEYAES_ERR_SIZE;
    if (sel == HWKEYAES_SEL_USER && !key) return HWKEYAES_ERR_SIZE;
    keycode = (keybits - 128) >> 6;        /* 128 -> 0, 192 -> 1, 256 -> 2 (bootrom 0x20001e34: (+0x14 & 0x30) >> 4) */
    keywords = keybits >> 5;
    if (iv) memcpy(ivbuf, iv, 16); else memset(ivbuf, 0, 16);

    mutex_lock(&hwkeyaes_mutex, TIMEOUT_BLOCK);
    clockgate_enable(CLOCKGATE_AES, true);   /* PWRCON0 b7, fw_disk:0x080304b8 gate id 9  */
    if (flags & HWKEYAES_FLAG_ECB)
    {
        /* ECB in software: a one-block CBC run with a zero IV is ECB (no hardware ECB bit is known) */
        uint32_t off;
        for (off = 0; off < size && !rc; off += 16)
            rc = hwkeyaes_run( sel, keycode, key, keywords, NULL,
                              (uint8_t*)out + off, (const uint8_t*)in + off, 16);
    }
    else
    {
        /* The last ciphertext block is the next CBC IV. */
        rc = hwkeyaes_run( sel, keycode, key, keywords, ivbuf, out, in, size);
        if (!rc) memcpy(next, (const uint8_t*)out + size - 16, 16);
        if (!rc && iv && (flags & HWKEYAES_FLAG_IVOUT)) memcpy(iv, next, 16);
    }
    /* do not leave the user key behind (the ROM wipes it as well, bootrom 0x2000226c) */
    if (sel == HWKEYAES_SEL_USER) { int i; for (i = 0; i < 8; i++) AESKEY[i] = 0; }
    clockgate_enable(CLOCKGATE_AES, false);
    mutex_unlock(&hwkeyaes_mutex);
    return rc;
}


/* Preserve the kernel service signature; this target implements encryption. */
void hwkeyaes(enum hwkeyaes_direction direction, uint32_t keyidx, void* data, uint32_t size)
{
    uint32_t zero[4] = {0, 0, 0, 0};
    if (direction == HWKEYAES_ENCRYPT)
        hwkeyaes_encrypt(keyidx, 128, keyidx == HWKEYAES_SEL_USER ? zero : NULL,
                         NULL, data, data, size, 0);
}

/* USB command 25: flags == 0 retains the legacy buffer layout. Otherwise addr
   points at params followed by the payload. Both return the driver result. */
int hwkeyaes_usb_encrypt(uint32_t sel, uint32_t flagsbyte, void* addr, uint32_t size)
{
    if (!flagsbyte)
    {
        uint32_t zero[4] = {0, 0, 0, 0};
        return hwkeyaes_encrypt( sel, 128, sel == HWKEYAES_SEL_USER ? zero : NULL,
                             NULL, addr, addr, size, 0);
    }
    struct hwkeyaes_usbparams* p = (struct hwkeyaes_usbparams*)addr;
    uint32_t keybits = 128 + 64 * ((flagsbyte >> 1) & 3);
    uint32_t flags = ((flagsbyte & 8) ? HWKEYAES_FLAG_ECB : 0) | ((flagsbyte & 16) ? HWKEYAES_FLAG_IVOUT : 0);
    if (!addr || ((uint32_t)addr & 31) || !(flagsbyte & 1) || (flagsbyte & ~31u)
        || ((flagsbyte >> 1) & 3) == 3
        || (uint32_t)addr > 0xffffffffu - sizeof(*p)) return HWKEYAES_ERR_SIZE;
    return hwkeyaes_encrypt( sel, keybits, p->key, p->iv, p + 1, p + 1, size, flags);
}
