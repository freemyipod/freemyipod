#include "global.h"
#include "interface/framebuffer/framebuffer.h"
#include "sys/util.h"

const uint8_t framebuffer_format_wordsize[] =
{
    [FRAMEBUFFER_FORMAT_I1] = 1,
    [FRAMEBUFFER_FORMAT_X1R1G1B1] = 4,
    [FRAMEBUFFER_FORMAT_I1R1G1B1] = 4,
    [FRAMEBUFFER_FORMAT_X7I1] = 8,
    [FRAMEBUFFER_FORMAT_X5R1G1B1] = 8,
    [FRAMEBUFFER_FORMAT_X2R2G2B2] = 8,
    [FRAMEBUFFER_FORMAT_R2G3B3] = 8,
    [FRAMEBUFFER_FORMAT_X1R5G5B5_LE] = 16,
    [FRAMEBUFFER_FORMAT_X1R5G5B5_BE] = 16,
    [FRAMEBUFFER_FORMAT_R5G6B5_LE] = 16,
    [FRAMEBUFFER_FORMAT_R5G6B5_BE] = 16,
    [FRAMEBUFFER_FORMAT_R8G8B8_LE] = 24,
    [FRAMEBUFFER_FORMAT_R8G8B8_BE] = 24,
    [FRAMEBUFFER_FORMAT_X31I1_LE] = 32,
    [FRAMEBUFFER_FORMAT_X31I1_BE] = 32,
    [FRAMEBUFFER_FORMAT_X8R8G8B8_LE] = 32,
    [FRAMEBUFFER_FORMAT_X8R8G8B8_BE] = 32,
};

uint32_t framebuffer_color_to_rgb888_be(enum framebuffer_format format, uint32_t color)
{
    uint32_t result = 0xff;
    switch (format)
    {
    case FRAMEBUFFER_FORMAT_X31I1_LE:
        color = swap32(color);
    case FRAMEBUFFER_FORMAT_I1:
    case FRAMEBUFFER_FORMAT_X7I1:
    case FRAMEBUFFER_FORMAT_X31I1_BE:
        result = (color & 1) ? 0xffffff : 0;
        break;
    case FRAMEBUFFER_FORMAT_I1R1G1B1:
        result = (color & 8) ? 0xff : 0x7f;
    case FRAMEBUFFER_FORMAT_X1R1G1B1:
    case FRAMEBUFFER_FORMAT_X5R1G1B1:
        result = ((color & 4) ? (result << 16) : 0) | ((color & 2) ? (result << 8) : 0) | ((color & 1) ? result : 0);
        break;
    case FRAMEBUFFER_FORMAT_X2R2G2B2:
        result = ((color & 0x30) << 18) | ((color & 0xc) << 14) | ((color & 3) << 6);
        result |= result >> 2;
        result |= result >> 4;
        break;
    case FRAMEBUFFER_FORMAT_R2G3B3:
        result = ((color & 0xc0) << 16) | ((color & 0x38) << 10) | ((color & 7) << 5);
        result |= (result & 0xc00000) >> 2;
        result |= (result & 0xf00000) >> 4;
        result |= (result & 0xe0e0) >> 3;
        result |= (result & 0xc0c0) >> 6;
        break;
    case FRAMEBUFFER_FORMAT_X1R5G5B5_LE:
        color = swap16(color);
    case FRAMEBUFFER_FORMAT_X1R5G5B5_BE:
        result = ((color & 0x7c00) << 9) | ((color & 0x3e0) << 6) | ((color & 0x1f) << 3);
        result |= (result & 0xe0e0e0) >> 5;
        break;
    case FRAMEBUFFER_FORMAT_R5G6B5_LE:
        color = swap16(color);
    case FRAMEBUFFER_FORMAT_R5G6B5_BE:
        result = ((color & 0xf800) << 8) | ((color & 0x7e0) << 5) | ((color & 0x1f) << 3);
        result |= (result & 0xe000e0) >> 5;
        result |= (result & 0xc000) >> 6;
        break;
    case FRAMEBUFFER_FORMAT_R8G8B8_LE:
        color <<= 8;
    case FRAMEBUFFER_FORMAT_X8R8G8B8_LE:
        color = swap32(color);
    case FRAMEBUFFER_FORMAT_R8G8B8_BE:
    case FRAMEBUFFER_FORMAT_X8R8G8B8_BE:
        result = color;
    }
    return result;
}

uint32_t framebuffer_color_from_rgb888_be(enum framebuffer_format format, uint32_t color)
{
    uint32_t result = 0;
    uint8_t r = (color & 0xff0000) >> 16;
    uint8_t g = (color & 0xff00) >> 8;
    uint8_t b = color & 0xff;
    switch (format)
    {
    case FRAMEBUFFER_FORMAT_I1:
    case FRAMEBUFFER_FORMAT_X7I1:
    case FRAMEBUFFER_FORMAT_X31I1_BE:
        result = r + g + b >= 0x180 ? 1 : 0;
        break;
    case FRAMEBUFFER_FORMAT_X31I1_LE:
        result = r + g + b >= 0x180 ? 0x01000000 : 0;
        break;
    case FRAMEBUFFER_FORMAT_X1R1G1B1:
    case FRAMEBUFFER_FORMAT_X5R1G1B1:
        result = ((r & 0x80) >> 5) | ((g & 0x80) >> 6) | ((b & 0x80) >> 7);
        break;
    case FRAMEBUFFER_FORMAT_I1R1G1B1:
        result = r + g + b >= 0x180 ? (8 | (r >= 0x80 ? 4 : 0) | (g >= 0x80 ? 2 : 0) | (b >= 0x80 ? 1 : 0))
                                    : ((r >= 0x40 ? 4 : 0) | (g >= 0x40 ? 2 : 0) | (b >= 0x40 ? 1 : 0));
        break;
    case FRAMEBUFFER_FORMAT_X2R2G2B2:
        result = (((r - (r >> 2)) >> 6) << 4) | (((g - (g >> 2)) >> 6) << 2) | ((b - (b >> 2)) >> 6);
        break;
    case FRAMEBUFFER_FORMAT_R2G3B3:
        result = (((r - (r >> 2)) >> 6) << 6) | (((g - (g >> 3)) >> 5) << 3) | ((b - (b >> 3)) >> 5);
        break;
    case FRAMEBUFFER_FORMAT_X1R5G5B5_BE:
        result = (((r - (r >> 5)) >> 3) << 10) | (((g - (g >> 5)) >> 3) << 5) | ((b - (b >> 5)) >> 3);
        break;
    case FRAMEBUFFER_FORMAT_X1R5G5B5_LE:
        result = swap16((((r - (r >> 5)) >> 3) << 10) | (((g - (g >> 5)) >> 3) << 5) | ((b - (b >> 5)) >> 3));
        break;
    case FRAMEBUFFER_FORMAT_R5G6B5_BE:
        result = (((r - (r >> 5)) >> 3) << 11) | (((g - (g >> 6)) >> 2) << 5) | ((b - (b >> 5)) >> 3);
        break;
    case FRAMEBUFFER_FORMAT_R5G6B5_LE:
        result = swap16((((r - (r >> 5)) >> 3) << 11) | (((g - (g >> 6)) >> 2) << 5) | ((b - (b >> 5)) >> 3));
        break;
    case FRAMEBUFFER_FORMAT_R8G8B8_LE:
        color <<= 8;
    case FRAMEBUFFER_FORMAT_X8R8G8B8_LE:
        color = swap32(color);
    case FRAMEBUFFER_FORMAT_R8G8B8_BE:
    case FRAMEBUFFER_FORMAT_X8R8G8B8_BE:
        result = color;
        break;
    }
    return result;
}

uint32_t framebuffer_convert_color(enum framebuffer_format from, enum framebuffer_format to, uint32_t color)
{
    return framebuffer_color_from_rgb888_be(to, framebuffer_color_to_rgb888_be(from, color));
}

uint32_t framebuffer_get_pixel(const struct framebuffer_instance* instance, int x, int y)
{
    int wsize = framebuffer_format_wordsize[instance->format];
    int bit = (instance->width * y + x) * wsize;
    uint32_t color = (swap32(((uint32_t*)instance->data)[bit >> 5]) >> (32 - (bit & 31) - wsize)) & (0xffffffff >> (32 - wsize));
    if (wsize == 16) return swap16(color);
    if (wsize == 32) return swap32(color);
    return color;
}

void framebuffer_set_pixel(const struct framebuffer_instance* instance, int x, int y, uint32_t color)
{
    int wsize = framebuffer_format_wordsize[instance->format];
    if (wsize == 16) color = swap16(color);
    else if (wsize == 32) color = swap32(color);
    int bit = (instance->width * y + x) * wsize;
    int byte = bit >> 5;
    bit = 32 - (bit & 31) - wsize;
    uint32_t mask = 0xffffffff >> (32 - wsize);
    uint32_t data = swap32(((uint32_t*)instance->data)[byte]);
    ((uint32_t*)instance->data)[byte] = swap32((data & ~(mask << bit)) | ((color & mask) << bit));
}

void framebuffer_fill(const struct framebuffer_instance* instance, int x, int y, int w, int h,
                      enum framebuffer_format format, uint32_t color, enum framebuffer_conversion_quality quality)
{
    int stride = instance->width;
    enum framebuffer_format fmt = instance->format;
    int wsize = framebuffer_format_wordsize[fmt];
    if (format != fmt)
    {
        if (framebuffer_format_wordsize[format] == 16) color = swap16(color);
        else if (framebuffer_format_wordsize[format] == 32) color = swap32(color);
        color = framebuffer_convert_color(format, fmt, color);
        if (wsize == 16) color = swap16(color);
        else if (wsize == 32) color = swap32(color);
    }

    if (w == stride)
    {
        w *= h;
        h = 1;
    }

    color &= 0xffffffff >> (32 - wsize);
    while (wsize < 32 && !(w & 1) && !(x & 1) && !(stride & 1))
    {
        color |= color << wsize;
        wsize <<= 1;
        w >>= 1;
        x >>= 1;
        stride >>= 1;
    }
    if (wsize >= 8)
    {
        void* out = instance->data + (y * stride + x) * (wsize >> 3);
        while (h--)
        {
            if (wsize == 32)
            {
                uint32_t* ptr = out;
                int pixels = w;
                while (pixels--) *ptr++ = color;
            }
            else if (wsize == 16)
            {
                uint16_t* ptr = out;
                int pixels = w;
                while (pixels--) *ptr++ = color;
            }
            else memset(out, color, w);
            out += stride * (wsize >> 3);
        }
        return;
    }

    int mask = 0xffffffff >> (32 - wsize);
    uint32_t* out = instance->data;
    int bit = (y * stride + x) * wsize;
    out += bit >> 5;
    bit &= 31;
    while (h--)
    {
        int pixels = w;
        uint32_t data = swap32(*out);
        while (pixels--)
        {
            if (!bit) data = swap32(*out);
            data = (data & ~(mask << (32 - bit - wsize))) | (color << (32 - bit - wsize));
            bit += wsize;
            if (bit == 32)
            {
                *out++ = swap32(data);
                bit = 0;
            }
        }
        if (bit) *out = swap32(data);
        bit += (stride - w) * wsize;
        out += bit >> 5;
        bit &= 31;
    }
}

void framebuffer_blit(const struct framebuffer_instance* infb, int inx, int iny,
                      const struct framebuffer_instance* outfb, int outx, int outy,
                      int w, int h, enum framebuffer_conversion_quality quality)
{
    int instride = infb->width;
    int outstride = outfb->width;
    enum framebuffer_format infmt = infb->format;
    enum framebuffer_format outfmt = outfb->format;
    int inwsize = framebuffer_format_wordsize[infmt];
    int outwsize = framebuffer_format_wordsize[outfmt];

    if (w == instride && w == outstride)
    {
        w *= h;
        h = 1;
    }

    if (infmt == outfmt)
    {
        while (inwsize < 32 && !(w & 1) && !(inx & 1) && !(outx & 1) && !(instride & 1) && !(outstride & 1))
        {
            inwsize <<= 1;
            outwsize <<= 1;
            w >>= 1;
            inx >>= 1;
            outx >>= 1;
            instride >>= 1;
            outstride >>= 1;
        }
        if (inwsize >= 8)
        {
            void* in = infb->data + (iny * instride + inx) * (inwsize >> 3);
            void* out = outfb->data + (outy * outstride + outx) * (outwsize >> 3);
            while (h--)
            {
                memcpy(out, in, w * (outwsize >> 3));
                in += (instride - w) * (inwsize >> 3);
                out += (outstride - w) * (outwsize >> 3);
            }
            return;
        }
    }

    int inmask = 0xffffffff >> (32 - inwsize);
    int outmask = 0xffffffff >> (32 - outwsize);
    uint32_t* in = infb->data;
    uint32_t* out = outfb->data;
    int inbit = (iny * instride + inx) * inwsize;
    int outbit = (outy * outstride + outx) * outwsize;
    in += inbit >> 5;
    out += outbit >> 5;
    inbit &= 31;
    outbit &= 31;
    while (h--)
    {
        int pixels = w;
        uint32_t idata = swap32(*in) << inbit;
        uint32_t odata = swap32(*out);
        while (pixels--)
        {
            if (inbit == 32)
            {
                idata = swap32(*++in);
                inbit = 0;
            }
            if (!outbit) odata = swap32(*out);
            idata = ((idata << inwsize) | (idata >> (32 - inwsize)));
            uint32_t data = idata & inmask;
            if (infmt != outfmt) data = framebuffer_convert_color(infmt, outfmt, data);
            odata = (odata & ~(outmask << (32 - outbit - outwsize))) | (data << (32 - outbit - outwsize));
            inbit += inwsize;
            outbit += outwsize;
            if (outbit == 32)
            {
                *out++ = swap32(odata);
                outbit = 0;
            }
        }
        if (outbit) *out = swap32(odata);
        inbit += (instride - w) * inwsize;
        outbit += (outstride - w) * outwsize;
        in += inbit >> 5;
        out += outbit >> 5;
        inbit &= 31;
        outbit &= 31;
    }
}

void framebuffer_update(const struct framebuffer_instance* instance, int x, int y, int w, int h)
{
    instance->update_handler(instance->update_handler_arg, instance, x, y, w, h);
}
