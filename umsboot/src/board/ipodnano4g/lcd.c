#include "global.h"
#include "board/ipodnano4g/lcd.h"
#include "interface/lcd/lcd.h"
#include "interface/lcdif/lcdif.h"
#include "interface/backlight_manager/backlight_manager.h"
#include "soc/s5l87xx/regs.h"


static void n4g_lcd_send_cmd(const struct lcdif_instance* interface, int cmd)
{
    interface->driver->send_cmd(interface, cmd);
}

static void n4g_lcd_send_data(const struct lcdif_instance* interface, int data)
{
    interface->driver->send_data(interface, ((data & 0x7f00) << 1) | (data & 0xff));
}

static void n4g_lcd_init(const struct lcd_instance* instance)
{
    const struct ipodnano4g_lcd_config* data = (const struct ipodnano4g_lcd_config*)instance->driver_config;
    data->interface->driver->init(data->interface);
    if (data->backlight) data->backlight->driver->init(data->backlight);
}

enum framebuffer_format n4g_lcd_get_native_format(const struct lcd_instance* instance)
{
    return FRAMEBUFFER_FORMAT_R5G6B5_LE;
}

static void n4g_lcd_power(const struct lcd_instance* instance, bool on)
{
}

static void n4g_lcd_backlight(const struct lcd_instance* instance, int brightness)
{
    const struct ipodnano4g_lcd_config* data = (const struct ipodnano4g_lcd_config*)instance->driver_config;
    if (!data->backlight) return;
    data->backlight->driver->set_brightness(data->backlight, brightness);
}

static void n4g_lcd_setup_range(const struct lcd_instance* instance, int x, int y, int w, int h)
{
    const struct ipodnano4g_lcd_config* data = (const struct ipodnano4g_lcd_config*)instance->driver_config;
    const struct lcdif_instance* interface = data->interface;
    n4g_lcd_send_cmd(interface, 0x2a);
    n4g_lcd_send_data(interface, x);
    n4g_lcd_send_data(interface, x + w - 1);
    n4g_lcd_send_cmd(interface, 0x2b);
    n4g_lcd_send_data(interface, y);
    n4g_lcd_send_data(interface, y + h - 1);
    n4g_lcd_send_cmd(interface, 0x2c);
}

static void n4g_lcd_fill_pixels(const struct lcd_instance* instance, uint32_t color, int count)
{
    const struct ipodnano4g_lcd_config* data = (const struct ipodnano4g_lcd_config*)instance->driver_config;
    const struct lcdif_instance* interface = data->interface;
    interface->driver->send_repeat(interface, color, count);
}

static void n4g_lcd_blit_pixels(const struct lcd_instance* instance, void* buffer, int count)
{
    const struct ipodnano4g_lcd_config* data = (const struct ipodnano4g_lcd_config*)instance->driver_config;
    const struct lcdif_instance* interface = data->interface;
    interface->driver->send_bulk(interface, buffer, count);
}


const struct lcd_driver ipodnano4g_lcd_driver =
{
    .init = n4g_lcd_init,
    .get_native_format = n4g_lcd_get_native_format,
    .power = n4g_lcd_power,
    .backlight = n4g_lcd_backlight,
    .setup_range = n4g_lcd_setup_range,
    .fill_pixels = n4g_lcd_fill_pixels,
    .blit_pixels = n4g_lcd_blit_pixels,
};
