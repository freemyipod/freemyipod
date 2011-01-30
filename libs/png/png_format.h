//
//
//    Copyright 2011 TheSeven
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


#ifndef __PNG_DECODER_H__
#define __PNG_DECODER_H__


#include "emcorelib.h"


/* PNG chunk types signatures */
/* critical chunks */
#define PNG_CHUNK_IHDR 0x49484452
#define PNG_CHUNK_PLTE 0x504c5445
#define PNG_CHUNK_IDAT 0x49444154
#define PNG_CHUNK_IEND 0x49454e44

/* ancillary chunks */
#define PNG_CHUNK_bKGD 0x624b4744
#define PNG_CHUNK_cHRM 0x6348524d
#define PNG_CHUNK_gAMA 0x67414d41
#define PNG_CHUNK_hIST 0x68495354
#define PNG_CHUNK_iCCP 0x69434350
#define PNG_CHUNK_pHYs 0x70485973
#define PNG_CHUNK_sBIT 0x73424954
#define PNG_CHUNK_sPLT 0x73504c54
#define PNG_CHUNK_sRGB 0x73524742
#define PNG_CHUNK_tEXt 0x74455874
#define PNG_CHUNK_tIME 0x74494d45
#define PNG_CHUNK_tRNS 0x74524e53
#define PNG_CHUNK_zTXt 0x7a545874

/* PNG color types */
#define PNG_COLORTYPE_GREY    0
#define PNG_COLORTYPE_RGB     2
#define PNG_COLORTYPE_PALETTE 3
#define PNG_COLORTYPE_GREYA   4
#define PNG_COLORTYPE_RGBA    6

/* PNG filter types */
#define PNG_FILTERTYPE_NONE    0
#define PNG_FILTERTYPE_SUB     1
#define PNG_FILTERTYPE_UP      2
#define PNG_FILTERTYPE_AVERAGE 3
#define PNG_FILTERTYPE_PAETH   4


#endif