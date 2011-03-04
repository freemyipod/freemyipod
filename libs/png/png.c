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


//#define DEBUG_CONSOLES 2
//#define DEBUG_PRINT_SOURCE_LINE


#include "emcorelib.h"
#include "export/libpng.h"
#include "tinf.h"
#include "png_format.h"


static uint32_t read32be(const uint8_t* in)
{
    return (in[0] << 24) | (in[1] << 16) | (in[2] << 8) | in[3];
}

struct png_info* png_open(const void* data, size_t size)
{
    DEBUGF("png_open: Processing file at 0x%08X, size: %d", data, size);
    const uint8_t* in = (const uint8_t*)data;
    struct png_info info;
    memset(&info, 0, sizeof(info));
    if (memcmp(in, "\x89PNG\r\n\x1a\n", 8))
    {
        DEBUGF("png_open: Invalid signature: %02X %02X %02X %02X %02X %02X %02X %02X",
               in[0], in[1], in[2], in[3], in[4], in[5], in[6], in[7]);
        return NULL;
    }
    in += 8;
    size -= 8;
    bool first = true;
    bool ihdr = false;
    int idat = 0;
    bool iend = false;
    while (size)
    {
        if (iend)
        {
            DEBUGF("png_open: %d bytes of garbage after IEND chunk", size);
            return NULL;
        }
        uint32_t length = read32be(in);
        DEBUGF("png_open: %08X (%d remaining): %c%c%c%c (%08X, %d+12 bytes)",
               in, size, in[4], in[5], in[6], in[7], read32be(in + 4), length);
        in += 4;
        if (length + 12 > size)
        {
            DEBUGF("png_open: Chunk reaches beyond end of file", size);
            return NULL;
        }
        if (crc32(in, length + 4) != read32be(in + length + 4))
        {
            DEBUGF("png_open: Bad chunk CRC");
            return NULL;
        }
        uint32_t chunk = read32be(in);
        if (chunk & 0x2000)
        {
            DEBUGF("png_open: Chunk name has reserved bit set: %08X", chunk);
            return NULL;
        }
        if (first && chunk != PNG_CHUNK_IHDR)
        {
            DEBUGF("png_open: First chunk is not IHDR");
            return NULL;
        }
        if (idat && chunk != PNG_CHUNK_IDAT) idat = 2;
        in += 4;
        switch (chunk)
        {
        case PNG_CHUNK_IHDR:
            if (length != 13)
            {
                DEBUGF("png_open: Bad IHDR length: %d bytes", length);
                return NULL;
            }
            if (in[10])
            {
                DEBUGF("png_open: Unknown compression type 0x%02X", in[10]);
                return NULL;
            }
            if (in[11])
            {
                DEBUGF("png_open: Unknown filter type 0x%02X", in[11]);
                return NULL;
            }
            switch (in[9])
            {
            case 0:
                if (in[8] == 16) break;
            case 3:
                if (in[8] == 1 || in[8] == 2 || in[8] == 4 || in[8] == 8)
                    break;
                DEBUGF("png_open: Invalid depth 0x%02X for color type 0x%02X", in[8], in[9]);
                return NULL;
            case 2:
            case 4:
            case 6:
                if (in[8] == 8 || in[8] == 16) break;
                DEBUGF("png_open: Invalid depth %02X for color type 0x%02X", in[8], in[9]);
                return NULL;
            default:
                DEBUGF("png_open: Unknown color type 0x%02X", in[9]);
                return NULL;
            }
            if (in[12])
            {
                DEBUGF("png_open: Only non-interlaced files are supported");
                return NULL;
            }
            switch (in[9])
            {
            case 0:
            case 3:
                info.channels = 1;
                break;
            case 4:
                info.channels = 2;
                break;
            case 2:
                info.channels = 3;
                break;
            case 6:
                info.channels = 4;
                break;
            default:
                DEBUGF("png_open: Unknown color type 0x%02X", info.colortype);
                return NULL;
            }
            info.width = read32be(in);
            info.height = read32be(in + 4);
            info.depth = in[8];
            info.colortype = in[9];
            info.comprtype = in[10];
            info.filtertype = in[11];
            in += length;
            break;
        case PNG_CHUNK_IDAT:
            if (idat == 2)
            {
                DEBUGF("png_open: IDAT chunks are not consecutive");
                return NULL;
            }
            if (!idat)
            {
                info.idat = in - 8;
                idat = 1;
            }
            info.idatlen += length + 12;
            in += length;
            break;
        case PNG_CHUNK_IEND:
            if (length)
            {
                DEBUGF("png_open: IEND chunk has non-zero size");
                return NULL;
            }
            iend = true;
            break;
        case PNG_CHUNK_PLTE:
            if (info.colortype != 3)
            {
                DEBUGF("png_open: Color type 0x%02X may not have a palette", info.colortype);
                return NULL;
            }
            if (length % 3)
            {
                DEBUGF("png_open: Palette size is not a multiple of 3");
                return NULL;
            }
            info.palette = (const struct png_rgb*)in;
            in += length;
            break;
        case PNG_CHUNK_tRNS:
            switch (info.colortype)
            {
            case 2:
                info.key[0] = (in[0] << 8) | in[1];
                info.key[1] = (in[2] << 8) | in[3];
                info.key[2] = (in[4] << 8) | in[5];
                break;
            case 0:
                info.key[0] = (in[0] << 8) | in[1];
                info.key[1] = (in[0] << 8) | in[1];
                info.key[2] = (in[0] << 8) | in[1];
                info.keyvalid = 1;
                break;
            case 3:
                info.palalpha = in;
                info.palalphacnt = length;
                break;
            default:
                DEBUGF("png_open: Color type 0x%02X may not have a tRNS chunk", info.colortype);
                return NULL;
            }
            in += length;
            break;
        case PNG_CHUNK_bKGD:
            switch (info.colortype)
            {
            case 2:
            case 6:
                if (info.depth == 16)
                {
                    info.bg.r = in[0];
                    info.bg.g = in[2];
                    info.bg.b = in[4];
                }
                else
                {
                    info.bg.r = in[1] << (8 - info.depth);
                    info.bg.g = in[3] << (8 - info.depth);
                    info.bg.b = in[5] << (8 - info.depth);
                }
                break;
            case 0:
            case 4:
                if (info.depth == 16)
                {
                    info.bg.r = in[0];
                    info.bg.g = in[0];
                    info.bg.b = in[0];
                }
                else
                {
                    info.bg.r = in[1] << (8 - info.depth);
                    info.bg.g = in[1] << (8 - info.depth);
                    info.bg.b = in[1] << (8 - info.depth);
                }
                break;
            case 3:
                if (!info.palette)
                {
                    DEBUGF("png_open: Indexed mode, but no palette before bKGD chunk");
                    return NULL;
                }
                info.bg = info.palette[*in];
                break;
            default:
                DEBUGF("png_open: Unknown color type 0x%02X", info.colortype);
                return NULL;
            }
            in += length;
            break;
        default:
            if (!(chunk & 0x20000000))
            {
                DEBUGF("png_open: Unknown critical chunk: %08X", chunk);
                return NULL;
            }
            in += length;
        }
        first = false;
        in += 4;
        size -= length + 12;
    }
    if (!iend)
    {
        DEBUGF("png_open: No IEND chunk");
        return NULL;
    }
    if (info.colortype == 3 && !info.palette)
    {
        DEBUGF("png_open: Indexed mode, but no palette present");
        return NULL;
    }
    struct png_info* ret = (struct png_info*)malloc(sizeof(info));
    if (ret) memcpy(ret, &info, sizeof(info));
    else DEBUGF("png_open: Could not allocate memory for png_info struct");
    return ret;
}

uint32_t png_get_width(struct png_info* info)
{
    return info->width;
}

uint32_t png_get_height(struct png_info* info)
{
    return info->height;
}

void png_set_background(struct png_info* info, uint32_t color)
{
    info->bg.r = (color >> 0) & 0xff;
    info->bg.g = (color >> 8) & 0xff;
    info->bg.b = (color >> 16) & 0xff;
}

uint8_t* png_decode(struct png_info* info, bool alpha)
{
    int i, rc;
    int filterchannels = info->channels * ((info->depth + 7) / 8);
    int scanlinesize = (info->width * info->channels * info->depth + 7) / 8;
    int insize = info->height * (1 + scanlinesize);
    int unfilteredsize = info->height * scanlinesize;
    int outsize = info->height * info->width * (alpha ? 4 : 3);
    int bufsize = MAX(insize, outsize);
    DEBUGF("png_decode: Input data size: 0x%X", insize);
    DEBUGF("png_decode: Unfiltered data size: 0x%X", unfilteredsize);
    DEBUGF("png_decode: Output data size: 0x%X", outsize);
    DEBUGF("png_decode: Allocating 0x%X bytes of memory for decoding", bufsize);
    uint8_t* buf = (uint8_t*)malloc(bufsize);
    if (!buf)
    {
        DEBUGF("png_decode: Could not allocate memory for decompression");
        return NULL;
    }
    uint8_t* in = buf + bufsize - insize;
    uint8_t* out = buf;
    DEBUGF("png_decode: Allocated decoding buffer at 0x%08X", buf);
    DEBUGF("png_decode: Decompressing input data to 0x%08X", in);
    if (rc = tinf_zlib_uncompress(in, &insize, info->idat, info->idatlen))
    {
        DEBUGF("png_decode: Decompression failed: %d", rc);
        return NULL;
    }
    DEBUGF("png_decode: Unfiltering input data to 0x%08X", out);
    int row;
    for (row = 0; row < info->height; row++)
    {
        uint8_t filtertype = *in++;
        for (i = 0; i < scanlinesize; i++)
        {
            int predict;
            switch (filtertype)
            {
            case 1:
                if (i >= filterchannels) predict = *(out - filterchannels);
                else predict = 0;
                break;
            case 2:
                if (row) predict = *(out - scanlinesize);
                else predict = 0;
                break;
            case 3:
                predict = 0;
                if (i >= filterchannels) predict += *(out - filterchannels);
                if (row) predict += *(out - scanlinesize);
                predict >>= 1;
                break;
            case 4:
            {
                int a, b, c, p, pa, pb, pc;
                if (i >= filterchannels) a = *(out - filterchannels);
                else a = 0;
                if (row > 0) b = *(out - scanlinesize);
                else b = 0;
                if (row > 0 && i >= filterchannels) c = *(out - scanlinesize - filterchannels);
                else c = 0;
                p = a + b - c;
                pa = ABS(p - a);
                pb = ABS(p - b);
                pc = ABS(p - c);
                if (pa <= pb && pa <= pc) predict = a;
                else if (pb <= pc) predict = b;
                else predict = c;
                break;
            }
            default:
                predict = 0;
            }
            *out++ = predict + *in++;
        }
    }
    if (!(alpha && info->colortype == 6 && info->depth == 8)
     && !(!alpha && info->colortype == 2 && info->depth == 8 && !info->keyvalid))
    {
        uint8_t* in = buf + bufsize - unfilteredsize;
        uint8_t* out = buf;
        DEBUGF("png_decode: Moving unfiltered data from 0x%08X to 0x%08X", out, in);
        memmove(in, out, unfilteredsize);
        DEBUGF("png_decode: Converting unfiltered data to 0x%08X", out);
        for (row = 0; row < info->height; row++)
        {
            int col = 0;
            while (col < info->width)
            {
                int i, a, d;
                if (info->depth < 8)
                {
                    uint8_t byte = *buf++;
                    for (i = 0; i < 8 && col < info->width; i += info->depth)
                    {
                        if (info->colortype == 3)
                        {
                            int index = byte >> (8 - info->depth);
                            struct png_rgb color = info->palette[index];
                            if (index < info->palalphacnt) a = info->palalpha[index];
                            else a = 255;
                            if (!alpha && a != 255)
                            {
                                *out++ = (color.r * a + info->bg.r * (255 - a)) >> 8;
                                *out++ = (color.g * a + info->bg.g * (255 - a)) >> 8;
                                *out++ = (color.b * a + info->bg.b * (255 - a)) >> 8;
                            }
                            else
                            {
                                *out++ = color.r;
                                *out++ = color.g;
                                *out++ = color.b;
                                if (alpha) *out++ = a;
                            }
                        }
                        else
                        {
                            int color = byte >> (8 - info->depth);
                            for (d = info->depth; d < 8; d <<= 1)
                                color |= color << d;
                            if (info->keyvalid && color == info->key[0]) a = 0;
                            else a = 255;
                            if (!alpha && !a)
                            {
                                *out++ = info->bg.r;
                                *out++ = info->bg.r;
                                *out++ = info->bg.r;
                            }
                            else
                            {
                                *out++ = color;
                                *out++ = color;
                                *out++ = color;
                                if (alpha) *out++ = a;
                            }
                        }
                        byte <<= info->depth;
                        col++;
                    }
                }
                else
                {
                    if (info->colortype == 3)
                    {
                        int index = *in++;
                        struct png_rgb color = info->palette[index];
                        if (index < info->palalphacnt) a = info->palalpha[index];
                        else a = 255;
                        if (!alpha && a != 255)
                        {
                            *out++ = (color.r * a + info->bg.r * (255 - a)) >> 8;
                            *out++ = (color.g * a + info->bg.g * (255 - a)) >> 8;
                            *out++ = (color.b * a + info->bg.b * (255 - a)) >> 8;
                        }
                        else
                        {
                            *out++ = color.r;
                            *out++ = color.g;
                            *out++ = color.b;
                            if (alpha) *out++ = a;
                        }
                    }
                    else
                    {
                        int chan[4];
                        for (i = 0; i < info->channels; i++)
                        {
                            chan[i] = *in++;
                            if (info->depth == 16) chan[i] = (chan[i] << 8) | *in++;
                        }
                        if (!(info->colortype & 2))
                        {
                            chan[3] = chan[1];
                            chan[2] = chan[0];
                            chan[1] = chan[0];
                        }
                        if (!(info->colortype & 4))
                        {
                            if (info->keyvalid && chan[0] == info->key[0]
                             && chan[1] == info->key[1] && chan[2] == info->key[2]) a = 0;
                            else a = 255;
                        }
                        else if (info->depth == 16) a = chan[3] >> 8;
                        else a = chan[3];
                        if (info->depth == 16)
                        {
                            chan[0] >>= 8;
                            chan[1] >>= 8;
                            chan[2] >>= 8;
                        }
                        if (!alpha && a != 255)
                        {
                            *out++ = (chan[0] * a + info->bg.r * (255 - a)) >> 8;
                            *out++ = (chan[1] * a + info->bg.g * (255 - a)) >> 8;
                            *out++ = (chan[2] * a + info->bg.b * (255 - a)) >> 8;
                        }
                        else
                        {
                            *out++ = chan[0];
                            *out++ = chan[1];
                            *out++ = chan[2];
                            if (alpha) *out++ = a;
                        }
                    }
                    col++;
                }
            }
        }
    }
    if (outsize != bufsize)
    {
        DEBUGF("png_decode: Shrinking buffer to 0x%X bytes", outsize);
        out = (uint8_t*)realloc(buf, outsize);
        if (!out)
        {
            free(buf);
            DEBUGF("png_decode: Failed to trim output buffer");
        }
        DEBUGF("png_decode: Buffer is now at 0x%08X", out);
        return out;
    }
    return buf;
}

struct png_rgba* png_decode_rgba(struct png_info* info)
{
    return (struct png_rgba*)png_decode(info, true);
}

struct png_rgb* png_decode_rgb(struct png_info* info)
{
    return (struct png_rgb*)png_decode(info, false);
}

void png_destroy(struct png_info* info)
{
    free(info);
}
