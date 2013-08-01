#ifndef __INTERFACE_LCD_LCD_H__
#define __INTERFACE_LCD_LCD_H__

#include "global.h"
#include "interface/framebuffer/framebuffer.h"


struct __attribute__((packed,aligned(4))) lcd_instance;


struct __attribute__((packed,aligned(4))) lcd_driver
{
    void (*init)(const struct lcd_instance* instance);
    enum framebuffer_format (*get_native_format)(const struct lcd_instance* instance);
    void (*power)(const struct lcd_instance* instance, bool on);
    void (*backlight)(const struct lcd_instance* instance, int brightness);
    void (*setup_range)(const struct lcd_instance* instance, int x, int y, int w, int h);
    void (*fill_pixels)(const struct lcd_instance* instance, uint32_t color, int count);
    void (*blit_pixels)(const struct lcd_instance* instance, void* buffer, int count);
};


struct __attribute__((packed,aligned(4))) lcd_instance
{
    const struct lcd_driver* driver;
    const void* driver_config;
    void* driver_state;
};


extern void lcd_init(const struct lcd_instance* instance);
extern enum framebuffer_format lcd_get_native_format(const struct lcd_instance* instance);
extern void lcd_power(const struct lcd_instance* instance, bool on);
extern void lcd_backlight(const struct lcd_instance* instance, int brightness);
extern void lcd_setup_range(const struct lcd_instance* instance, int x, int y, int w, int h);
extern void lcd_fill_pixels(const struct lcd_instance* instance, uint32_t color, int count);
extern void lcd_blit_pixels(const struct lcd_instance* instance, void* buffer, int count);
extern void lcd_fill(const struct lcd_instance* instance, int x, int y, int w, int h,
                     enum framebuffer_format format, uint32_t color, enum framebuffer_conversion_quality quality);
extern void lcd_blit(const struct lcd_instance* instance, int x, int y, int w, int h,
                     const struct framebuffer_instance* framebuffer, int fx, int fy, enum framebuffer_conversion_quality quality);
extern void lcd_fb_update_handler(const void* arg, const struct framebuffer_instance* instance, int x, int y, int w, int h);


#endif
