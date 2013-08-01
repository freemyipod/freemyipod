#include "global.h"
#include "device/d1759/backlight_manager.h"
#include "interface/backlight_manager/backlight_manager.h"
#include "protocol/i2c/i2c.h"


static void backlight_set_brightness(const struct backlight_manager_instance* instance, uint8_t brightness)
{
    const struct d1759_backlight_manager_driver_config* data = (const struct d1759_backlight_manager_driver_config*)instance->driver_config;
    struct d1759_backlight_manager_driver_state* state = (struct d1759_backlight_manager_driver_state*)instance->driver_state;
    uint8_t on = (!!brightness) | ((!!state->fade) << 1);
    brightness = (data->max_current * (brightness & 0xff)) >> 8;
    if (on) i2c_write_regs(data->i2c, 0x73, 0x30, &brightness, 1);
    i2c_write_regs(data->i2c, 0x73, 0x31, &on, 1);
}

static void backlight_set_fade(const struct backlight_manager_instance* instance, uint8_t fade)
{
    struct d1759_backlight_manager_driver_state* state = (struct d1759_backlight_manager_driver_state*)instance->driver_state;
    state->fade = fade;
}

static void backlight_init(const struct backlight_manager_instance* instance)
{
    const struct d1759_backlight_manager_driver_config* data = (const struct d1759_backlight_manager_driver_config*)instance->driver_config;
    backlight_set_fade(instance, data->default_fade);
}

const struct backlight_manager_driver d1759_backlight_manager_driver =
{
    .init = backlight_init,
    .set_brightness = backlight_set_brightness,
    .set_fade = backlight_set_fade,
};
