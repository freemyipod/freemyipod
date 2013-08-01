#ifndef __DEVICE_PCF50635_BACKLIGHT_MANAGER_H__
#define __DEVICE_PCF50635_BACKLIGHT_MANAGER_H__

#include "global.h"
#include "interface/backlight_manager/backlight_manager.h"
#include "protocol/i2c/i2c.h"


struct __attribute__((packed,aligned(4))) pcf50635_backlight_manager_driver_config
{
    const struct i2c_driver_instance* i2c;
    uint8_t max_current;
    uint8_t default_fade;
    uint8_t reserved[2];
};

struct __attribute__((packed,aligned(4))) pcf50635_backlight_manager_driver_state
{
};

extern const struct backlight_manager_driver pcf50635_backlight_manager_driver;


#endif
