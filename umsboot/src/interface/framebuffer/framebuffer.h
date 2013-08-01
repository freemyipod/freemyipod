#ifndef __INTERFACE_FRAMEBUFFER_FRAMEBUFFER_H__
#define __INTERFACE_FRAMEBUFFER_FRAMEBUFFER_H__

#include "global.h"


enum __attribute__((packed)) framebuffer_format
{
    FRAMEBUFFER_FORMAT_I1 = 0,
    FRAMEBUFFER_FORMAT_X1R1G1B1,
    FRAMEBUFFER_FORMAT_I1R1G1B1,
    FRAMEBUFFER_FORMAT_X7I1,
    FRAMEBUFFER_FORMAT_X5R1G1B1,
    FRAMEBUFFER_FORMAT_X2R2G2B2,
    FRAMEBUFFER_FORMAT_R2G3B3,
    FRAMEBUFFER_FORMAT_X1R5G5B5_LE,
    FRAMEBUFFER_FORMAT_X1R5G5B5_BE,
    FRAMEBUFFER_FORMAT_R5G6B5_LE,
    FRAMEBUFFER_FORMAT_R5G6B5_BE,
    FRAMEBUFFER_FORMAT_R8G8B8_LE,
    FRAMEBUFFER_FORMAT_R8G8B8_BE,
    FRAMEBUFFER_FORMAT_X31I1_LE,
    FRAMEBUFFER_FORMAT_X31I1_BE,
    FRAMEBUFFER_FORMAT_X8R8G8B8_LE,
    FRAMEBUFFER_FORMAT_X8R8G8B8_BE,
};

extern const uint8_t framebuffer_format_wordsize[];

enum __attribute__((packed)) framebuffer_conversion_quality
{
    FRAMEBUFFER_CONVERSION_QUALITY_CLIP,
    FRAMEBUFFER_CONVERSION_QUALITY_DITHER,
};

struct __attribute__((packed,aligned(4))) framebuffer_instance
{
    uint16_t width;
    uint16_t height;
    enum framebuffer_format format;
    uint32_t reserved : 24;
    void* data;
    void (*update_handler)(const void* arg, const struct framebuffer_instance* instance, int x, int y, int w, int h);
    const void* update_handler_arg;
};


extern uint32_t framebuffer_color_to_rgb888_be(enum framebuffer_format format, uint32_t color);
extern uint32_t framebuffer_color_from_rgb888_be(enum framebuffer_format format, uint32_t color);
extern uint32_t framebuffer_convert_color(enum framebuffer_format from, enum framebuffer_format to, uint32_t color);
extern uint32_t framebuffer_get_pixel(const struct framebuffer_instance* instance, int x, int y);
extern void framebuffer_set_pixel(const struct framebuffer_instance* instance, int x, int y, uint32_t color);
extern void framebuffer_fill(const struct framebuffer_instance* instance, int x, int y, int w, int h,
                             enum framebuffer_format format, uint32_t color, enum framebuffer_conversion_quality quality);
extern void framebuffer_blit(const struct framebuffer_instance* infb, int inx, int iny,
                             const struct framebuffer_instance* outfb, int outx, int outy,
                             int w, int h, enum framebuffer_conversion_quality quality);
extern void framebuffer_update(const struct framebuffer_instance* instance, int x, int y, int w, int h);


#endif
