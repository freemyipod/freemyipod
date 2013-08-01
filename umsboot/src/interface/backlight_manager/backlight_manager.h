#ifndef __INTERFACE_BACKLIGHT_MANAGER_BACKLIGHT_MANAGER_H__
#define __INTERFACE_BACKLIGHT_MANAGER_BACKLIGHT_MANAGER_H__

#include "global.h"


struct __attribute__((packed,aligned(4))) backlight_manager_instance;


struct __attribute__((packed,aligned(4))) backlight_manager_driver
{
    void (*init)(const struct backlight_manager_instance* instance);
    void (*set_brightness)(const struct backlight_manager_instance* instance, uint8_t brightness);
    void (*set_fade)(const struct backlight_manager_instance* instance, uint8_t fade);
};

struct __attribute__((packed,aligned(4))) backlight_manager_instance
{
    const struct backlight_manager_driver* driver;
    const void* driver_config;
    void* driver_state;
};


#endif
