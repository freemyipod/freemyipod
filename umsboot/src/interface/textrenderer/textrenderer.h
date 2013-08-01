#ifndef __INTERFACE_TEXTRENDERER_TEXTRENDERER_H__
#define __INTERFACE_TEXTRENDERER_TEXTRENDERER_H__

#include "global.h"
#include "interface/framebuffer/framebuffer.h"


struct __attribute__((packed,aligned(4))) textrenderer_instance;


struct __attribute__((packed,aligned(4))) textrenderer_driver
{
    void (*init)(const struct textrenderer_instance* instance);
    int (*get_line_height)(const struct textrenderer_instance* instance);
    int (*get_max_width)(const struct textrenderer_instance* instance);
    int (*render_char)(const struct textrenderer_instance* instance,
                       const struct framebuffer_instance* fb, int x, int y, uint32_t color, char c);
};


struct __attribute__((packed,aligned(4))) textrenderer_instance
{
    const struct textrenderer_driver* driver;
    const void* driver_config;
    void* driver_state;
};


extern void textrenderer_init(const struct textrenderer_instance* instance);
extern int textrenderer_get_line_height(const struct textrenderer_instance* instance);
extern int textrenderer_get_max_width(const struct textrenderer_instance* instance);
extern int textrenderer_render_char(const struct textrenderer_instance* instance,
                                    const struct framebuffer_instance* fb, int x, int y, uint32_t color, char c);


#endif
