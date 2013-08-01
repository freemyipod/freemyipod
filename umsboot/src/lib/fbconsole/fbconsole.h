#ifndef __LIB_FBCONSOLE_FBCONSOLE_H__
#define __LIB_FBCONSOLE_FBCONSOLE_H__

#include "global.h"
#include "interface/framebuffer/framebuffer.h"
#include "interface/textrenderer/textrenderer.h"


struct __attribute__((packed,aligned(4))) fbconsole_config
{
    const struct framebuffer_instance* fb;
    const struct textrenderer_instance* renderer;
    uint16_t x;
    uint16_t y;
    uint16_t w;
    uint16_t h;
};

struct __attribute__((packed,aligned(4))) fbconsole_state
{
    uint32_t fgcolor;
    uint32_t bgcolor;
    uint16_t cwidth;
    uint16_t cheight;
    uint16_t cpos_x;
    uint16_t cpos_y;
    uint16_t dirty_xs;
    uint16_t dirty_xe;
    uint16_t dirty_ys;
    uint16_t dirty_ye;
    uint16_t meta_arg1;
    uint16_t meta_arg2;
    uint32_t dirty : 1;
    uint32_t reserved1 : 1;
    uint32_t escape : 1;
    uint32_t meta : 1;
    uint32_t badmeta : 1;
    uint32_t secondarg : 1;
    uint32_t arg1_present : 1;
    uint32_t arg2_present : 1;
    uint32_t bgcolcode : 3;
    uint32_t fgcolcode : 3;
    uint32_t intensity : 1;
    uint32_t inverse : 1;
    uint16_t reserved2;
};

extern const struct console_driver fbconsole_driver;


#endif
