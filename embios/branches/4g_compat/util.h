//
//
//    Copyright 2010 TheSeven
//
//
//    This file is part of emBIOS.
//
//    emBIOS is free software: you can redistribute it and/or
//    modify it under the terms of the GNU General Public License as
//    published by the Free Software Foundation, either version 2 of the
//    License, or (at your option) any later version.
//
//    emBIOS is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//    See the GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License along
//    with emBIOS.  If not, see <http://www.gnu.org/licenses/>.
//
//


#ifndef __UTIL_H__
#define __UTIL_H__


#include "global.h"


#ifndef NULL
#define NULL ((void*)0)
#endif

#ifndef MIN
#define MIN(a, b) (((a)<(b))?(a):(b))
#endif

#ifndef MAX
#define MAX(a, b) (((a)>(b))?(a):(b))
#endif

/* return number of elements in array a */
#define ARRAYLEN(a) (sizeof(a)/sizeof((a)[0]))

/* return p incremented by specified number of bytes */
#define SKIPBYTES(p, count) ((typeof (p))((char *)(p) + (count)))

#define P2_M1(p2)  ((1 << (p2))-1)

/* align up or down to nearest 2^p2 */
#define ALIGN_DOWN_P2(n, p2) ((n) & ~P2_M1(p2))
#define ALIGN_UP_P2(n, p2)   ALIGN_DOWN_P2((n) + P2_M1(p2),p2)

/* align up or down to nearest integer multiple of a */
#define ALIGN_DOWN(n, a)     ((n)/(a)*(a))
#define ALIGN_UP(n, a)       ALIGN_DOWN((n)+((a)-1),a)

/* align start and end of buffer to nearest integer multiple of a */
#define ALIGN_BUFFER(ptr,len,align) \
{\
    uintptr_t tmp_ptr1 = (uintptr_t)ptr; \
    uintptr_t tmp_ptr2 = tmp_ptr1 + len;\
    tmp_ptr1 = ALIGN_UP(tmp_ptr1,align); \
    tmp_ptr2 = ALIGN_DOWN(tmp_ptr2,align); \
    len = tmp_ptr2 - tmp_ptr1; \
    ptr = (typeof(ptr))tmp_ptr1; \
}


/* live endianness conversion */
#ifdef LITTLE_ENDIAN
#define letoh16(x) (x)
#define letoh32(x) (x)
#define htole16(x) (x)
#define htole32(x) (x)
#define betoh16(x) swap16(x)
#define betoh32(x) swap32(x)
#define htobe16(x) swap16(x)
#define htobe32(x) swap32(x)
#define swap_odd_even_be32(x) (x)
#define swap_odd_even_le32(x) swap_odd_even32(x)
#else
#define letoh16(x) swap16(x)
#define letoh32(x) swap32(x)
#define htole16(x) swap16(x)
#define htole32(x) swap32(x)
#define betoh16(x) (x)
#define betoh32(x) (x)
#define htobe16(x) (x)
#define htobe32(x) (x)
#define swap_odd_even_be32(x) swap_odd_even32(x)
#define swap_odd_even_le32(x) (x)
#endif


/* static endianness conversion */
#define SWAP_16(x) ((typeof(x))(unsigned short)(((unsigned short)(x) >> 8) | \
                                                ((unsigned short)(x) << 8)))

#define SWAP_32(x) ((typeof(x))(unsigned long)( ((unsigned long)(x) >> 24) | \
                                               (((unsigned long)(x) & 0xff0000ul) >> 8) | \
                                               (((unsigned long)(x) & 0xff00ul) << 8) | \
                                                ((unsigned long)(x) << 24)))

#ifdef RLITTLE_ENDIAN
#define LE_TO_H16(x) (x)
#define LE_TO_H32(x) (x)
#define H_TO_LE16(x) (x)
#define H_TO_LE32(x) (x)
#define BE_TO_H16(x) SWAP_16(x)
#define BE_TO_H32(x) SWAP_32(x)
#define H_TO_BE16(x) SWAP_16(x)
#define H_TO_BE32(x) SWAP_32(x)
#else
#define LE_TO_H16(x) SWAP_16(x)
#define LE_TO_H32(x) SWAP_32(x)
#define H_TO_LE16(x) SWAP_16(x)
#define H_TO_LE32(x) SWAP_32(x)
#define BE_TO_H16(x) (x)
#define BE_TO_H32(x) (x)
#define H_TO_BE16(x) (x)
#define H_TO_BE32(x) (x)
#endif

/* Get the byte offset of a type's member */
#define OFFSETOF(type, membername) ((off_t)&((type *)0)->membername)

/* Get the type pointer from one of its members */
#define TYPE_FROM_MEMBER(type, memberptr, membername) \
    ((type *)((intptr_t)(memberptr) - OFFSETOF(type, membername)))

static inline uint16_t swap16(uint16_t value)
    /*
      result[15..8] = value[ 7..0];
      result[ 7..0] = value[15..8];
    */
{
    return (value >> 8) | (value << 8);
}

static inline uint32_t swap32(uint32_t value)
    /*
      result[31..24] = value[ 7.. 0];
      result[23..16] = value[15.. 8];
      result[15.. 8] = value[23..16];
      result[ 7.. 0] = value[31..24];
    */
{
    uint32_t hi = swap16(value >> 16);
    uint32_t lo = swap16(value & 0xffff);
    return (lo << 16) | hi;
}

static inline uint32_t swap_odd_even32(uint32_t value)
{
    /*
      result[31..24],[15.. 8] = value[23..16],[ 7.. 0]
      result[23..16],[ 7.. 0] = value[31..24],[15.. 8]
    */
    uint32_t t = value & 0xff00ff00;
    return (t >> 8) | ((t ^ value) << 8);
}

#ifndef BIT_N
#define BIT_N(n) (1U << (n))
#endif

#ifndef CACHEALIGN_SIZE /* could be elsewhere for a particular reason */
#ifdef CACHEALIGN_BITS
/* 2^CACHEALIGN_BITS = the byte size */
#define CACHEALIGN_SIZE (1u << CACHEALIGN_BITS)
#else
#define CACHEALIGN_SIZE 16  /* FIXME */
#endif
#endif /* CACHEALIGN_SIZE */

#define CACHEALIGN_ATTR __attribute__((aligned(CACHEALIGN_SIZE)))
/* Aligns x up to a CACHEALIGN_SIZE boundary */
#define CACHEALIGN_UP(x) \
    ((typeof (x))ALIGN_UP_P2((uintptr_t)(x), CACHEALIGN_BITS))
/* Aligns x down to a CACHEALIGN_SIZE boundary */
#define CACHEALIGN_DOWN(x) \
    ((typeof (x))ALIGN_DOWN_P2((uintptr_t)(x), CACHEALIGN_BITS))
/* Aligns at least to the greater of size x or CACHEALIGN_SIZE */
#define CACHEALIGN_AT_LEAST_ATTR(x) \
    __attribute__((aligned(CACHEALIGN_UP(x))))
/* Aligns a buffer pointer and size to proper boundaries */
#define CACHEALIGN_BUFFER(start, size) \
    ALIGN_BUFFER((start), (size), CACHEALIGN_SIZE)

/* Double-cast to avoid 'dereferencing type-punned pointer will
 * break strict aliasing rules' B.S. */
#define PUN_PTR(type, p) ((type)(intptr_t)(p))


#endif
