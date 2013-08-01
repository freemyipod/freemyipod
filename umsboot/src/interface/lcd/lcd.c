#include "global.h"
#include "interface/lcd/lcd.h"
#include "interface/framebuffer/framebuffer.h"
#include "sys/util.h"

void lcd_init(const struct lcd_instance* instance)
{
    instance->driver->init(instance);
}

enum framebuffer_format lcd_get_native_format(const struct lcd_instance* instance)
{
    return instance->driver->get_native_format(instance);
}

void lcd_power(const struct lcd_instance* instance, bool on)
{
    instance->driver->power(instance, on);
}

void lcd_backlight(const struct lcd_instance* instance, int brightness)
{
    instance->driver->backlight(instance, brightness);
}

void lcd_setup_range(const struct lcd_instance* instance, int x, int y, int w, int h)
{
    instance->driver->setup_range(instance, x, y, w, h);
}

void lcd_fill_pixels(const struct lcd_instance* instance, uint32_t color, int count)
{
    instance->driver->fill_pixels(instance, color, count);
}

void lcd_blit_pixels(const struct lcd_instance* instance, void* buffer, int count)
{
    instance->driver->blit_pixels(instance, buffer, count);
}

void lcd_fill(const struct lcd_instance* instance, int x, int y, int w, int h,
              enum framebuffer_format format, uint32_t color, enum framebuffer_conversion_quality quality)
{
    enum framebuffer_format native = lcd_get_native_format(instance);
    if (format != native)
    {
        if (framebuffer_format_wordsize[format] == 16) color = swap16(color);
        else if (framebuffer_format_wordsize[format] == 32) color = swap32(color);
        color = framebuffer_convert_color(format, native, color);
        if (framebuffer_format_wordsize[native] == 16) color = swap16(color);
        else if (framebuffer_format_wordsize[native] == 32) color = swap32(color);
    }
    lcd_setup_range(instance, x, y, w, h);
    lcd_fill_pixels(instance, color, w * h);
}

void lcd_blit(const struct lcd_instance* instance, int x, int y, int w, int h,
              const struct framebuffer_instance* framebuffer, int fx, int fy, enum framebuffer_conversion_quality quality)
{
    int stride = framebuffer->width;
    enum framebuffer_format infmt = framebuffer->format;
    enum framebuffer_format outfmt = lcd_get_native_format(instance);
    int inwsize = framebuffer_format_wordsize[infmt];
    int outwsize = framebuffer_format_wordsize[outfmt];

    lcd_setup_range(instance, x, y, w, h);

    if (w == stride)
    {
        w *= h;
        h = 1;
    }

    if (infmt == outfmt && inwsize >= 8)
    {
        void* in = framebuffer->data + (fy * stride + fx) * (inwsize >> 3);
        while (h--)
        {
            lcd_blit_pixels(instance, in, w);
            in += (stride - w) * (inwsize >> 3);
        }
        return;
    }

    int inmask = 0xffffffff >> (32 - inwsize);
    uint32_t* in = framebuffer->data;
    int inbit = (fy * stride + fx) * inwsize;
    in += inbit >> 5;
    inbit &= 31;
    uint32_t buf[64];
    while (h--)
    {
        int pixels = w;
        uint32_t idata = swap32(*in) << inbit;
        while (pixels > 0)
        {
            int processed = 0;
            uint32_t* out = buf;
            int outbit = 0;
            uint32_t odata;
            while (((void*)out) < ((void*)buf) + sizeof(buf) && processed < pixels)
            {
                if (inbit == 32)
                {
                    idata = swap32(*++in);
                    inbit = 0;
                }
                if (!outbit) odata = 0;
                idata = ((idata << inwsize) | (idata >> (32 - inwsize)));
                odata <<= outwsize;
                uint32_t data = idata & inmask;
                if (infmt != outfmt) data = framebuffer_convert_color(infmt, outfmt, data);
                odata |= data;
                inbit += inwsize;
                outbit += outwsize;
                if (outbit == 32)
                {
                    *out++ = swap32(odata);
                    outbit = 0;
                }
                processed++;
            }
            if (outbit) *out = swap32(odata << (32 - outbit));
            lcd_blit_pixels(instance, buf, processed);
            pixels -= processed;
        }
        inbit += (stride - w) * inwsize;
        in += inbit >> 5;
        inbit &= 31;
    }
}

void lcd_fb_update_handler(const void* arg, const struct framebuffer_instance* fb, int x, int y, int w, int h)
{
    const struct lcd_instance* instance = (const struct lcd_instance*)arg;
    lcd_blit(instance, x, y, w, h, fb, x, y, FRAMEBUFFER_CONVERSION_QUALITY_CLIP);
}
