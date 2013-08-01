#include "global.h"
#include "board/ipodclassic/lcd.h"
#include "interface/lcd/lcd.h"
#include "interface/lcdif/lcdif.h"
#include "interface/backlight_manager/backlight_manager.h"
#include "soc/s5l87xx/regs.h"


static void ipcl_lcd_send_cmd(const struct lcdif_instance* interface, int cmd)
{
    interface->driver->send_cmd(interface, cmd);
}

static void ipcl_lcd_send_data(const struct lcdif_instance* interface, int data)
{
    interface->driver->send_data(interface, data);
}

static void ipcl_lcd_init(const struct lcd_instance* instance)
{
    const struct ipodclassic_lcd_config* data = (const struct ipodclassic_lcd_config*)instance->driver_config;
    data->interface->driver->init(data->interface);
    if (data->backlight) data->backlight->driver->init(data->backlight);
}

enum framebuffer_format ipcl_lcd_get_native_format(const struct lcd_instance* instance)
{
    return FRAMEBUFFER_FORMAT_R5G6B5_LE;
}

static void ipcl_lcd_power(const struct lcd_instance* instance, bool on)
{
}

static void ipcl_lcd_backlight(const struct lcd_instance* instance, int brightness)
{
    const struct ipodclassic_lcd_config* data = (const struct ipodclassic_lcd_config*)instance->driver_config;
    if (!data->backlight) return;
    data->backlight->driver->set_brightness(data->backlight, brightness);
}

static void ipcl_lcd_setup_range(const struct lcd_instance* instance, int x, int y, int w, int h)
{
    const struct ipodclassic_lcd_config* data = (const struct ipodclassic_lcd_config*)instance->driver_config;
    const struct lcdif_instance* interface = data->interface;
    int type = (PDAT6 & 0x30) >> 4;
    if (type & 2)
    {
        ipcl_lcd_send_cmd(interface, 0x210);
        ipcl_lcd_send_data(interface, x);
        ipcl_lcd_send_cmd(interface, 0x211);
        ipcl_lcd_send_data(interface, x + w - 1);
        ipcl_lcd_send_cmd(interface, 0x212);
        ipcl_lcd_send_data(interface, y);
        ipcl_lcd_send_cmd(interface, 0x213);
        ipcl_lcd_send_data(interface, y + h - 1);
        ipcl_lcd_send_cmd(interface, 0x200);
        ipcl_lcd_send_data(interface, x);
        ipcl_lcd_send_cmd(interface, 0x201);
        ipcl_lcd_send_data(interface, y);
        ipcl_lcd_send_cmd(interface, 0x202);
    }
    else
    {
        ipcl_lcd_send_cmd(interface, 0x2a);
        ipcl_lcd_send_data(interface, x >> 8);
        ipcl_lcd_send_data(interface, x & 0xff);
        ipcl_lcd_send_data(interface, (x + w - 1) >> 8);
        ipcl_lcd_send_data(interface, (x + w - 1) & 0xff);
        ipcl_lcd_send_cmd(interface, 0x2b);
        ipcl_lcd_send_data(interface, y >> 8);
        ipcl_lcd_send_data(interface, y & 0xff);
        ipcl_lcd_send_data(interface, (y + h - 1) >> 8);
        ipcl_lcd_send_data(interface, (y + h - 1) & 0xff);
        ipcl_lcd_send_cmd(interface, 0x2c);
    }
}

static void ipcl_lcd_fill_pixels(const struct lcd_instance* instance, uint32_t color, int count)
{
    const struct ipodclassic_lcd_config* data = (const struct ipodclassic_lcd_config*)instance->driver_config;
    const struct lcdif_instance* interface = data->interface;
    interface->driver->send_repeat(interface, color, count);
}

static void ipcl_lcd_blit_pixels(const struct lcd_instance* instance, void* buffer, int count)
{
    const struct ipodclassic_lcd_config* data = (const struct ipodclassic_lcd_config*)instance->driver_config;
    const struct lcdif_instance* interface = data->interface;
    interface->driver->send_bulk(interface, buffer, count);
}


const struct lcd_driver ipodclassic_lcd_driver =
{
    .init = ipcl_lcd_init,
    .get_native_format = ipcl_lcd_get_native_format,
    .power = ipcl_lcd_power,
    .backlight = ipcl_lcd_backlight,
    .setup_range = ipcl_lcd_setup_range,
    .fill_pixels = ipcl_lcd_fill_pixels,
    .blit_pixels = ipcl_lcd_blit_pixels,
};
