#include "global.h"
#include "app/umsboot/console.h"
#include "protocol/i2c/i2c.h"
#include "interface/backlight_manager/backlight_manager.h"
#include "interface/lcdif/lcdif.h"
#include "interface/lcd/lcd.h"
#include "interface/framebuffer/framebuffer.h"
#include "interface/console/console.h"
#include "soc/s5l87xx/i2c.h"
#include "device/pcf50635/backlight_manager.h"
#include "soc/s5l87xx/lcdif.h"
#include "board/ipodnano2g/lcd.h"
#include "lib/simpletextrenderer/simpletextrenderer.h"
#include "lib/fbconsole/fbconsole.h"


static void umsboot_fb_update_handler(const void* arg, const struct framebuffer_instance* fb, int x, int y, int w, int h)
{
    const struct lcd_instance* instance = (const struct lcd_instance*)arg;
    lcd_blit(instance, x + 4, y + 2, w, h, fb, x, y, FRAMEBUFFER_CONVERSION_QUALITY_CLIP);
}

static const struct s5l87xx_i2c_driver_config umsboot_i2c_config =
{
    .index = 0,
};

static const struct i2c_driver_instance umsboot_i2c =
{
    .driver = &s5l87xx_i2c_driver,
    .driver_config = &umsboot_i2c_config,
};

static const struct pcf50635_backlight_manager_driver_config umsboot_backlight_config =
{
    .i2c = &umsboot_i2c,
    .max_current = 46,
    .default_fade = 20,
};

static const struct backlight_manager_instance umsboot_backlight =
{
    .driver = &pcf50635_backlight_manager_driver,
    .driver_config = &umsboot_backlight_config,
};

static const struct lcdif_instance umsboot_lcdif =
{
    .driver = &s5l87xx_lcdif_driver,
};

static const struct ipodnano2g_lcd_config umsboot_lcd_config =
{
    .interface = &umsboot_lcdif,
    .backlight = &umsboot_backlight,
};

static const struct lcd_instance umsboot_lcd =
{
    .driver = &ipodnano2g_lcd_driver,
    .driver_config = &umsboot_lcd_config,
};

static uint8_t umsboot_fb_data[128][21];
static const struct framebuffer_instance fb =
{
    .width = 168,
    .height = 128,
    .format = FRAMEBUFFER_FORMAT_I1,
    .data = umsboot_fb_data,
    .update_handler = umsboot_fb_update_handler,
    .update_handler_arg = &umsboot_lcd,
};

static const struct textrenderer_instance umsboot_renderer =
{
    .driver = &simpletextrenderer_driver,
};

static const struct fbconsole_config umsboot_console_config =
{
    .fb = &fb,
    .renderer = &umsboot_renderer,
    .x = 0,
    .y = 0,
    .w = 28,
    .h = 16,
};

static struct fbconsole_state umsboot_console_state =
{
};

const struct console_instance umsboot_console =
{
    .driver = &fbconsole_driver,
    .driver_config = &umsboot_console_config,
    .driver_state = &umsboot_console_state,
};

void umsboot_console_init()
{
    lcd_init(&umsboot_lcd);
    lcd_fill(&umsboot_lcd, 0, 0, 176, 132, FRAMEBUFFER_FORMAT_I1, 0, FRAMEBUFFER_CONVERSION_QUALITY_CLIP);
    lcd_backlight(&umsboot_lcd, 100);
    console_init(&umsboot_console);
    console_puts(&umsboot_console,
        "UMSboot v0.2.1\n\n"
        "Please copy a UBI file to\n"
        "the mass storage device and\n"
        "safely eject it when you're\n"
        "done. If you booted this\n"
        "accidentally, just press and\n"
        "hold \e[7mMENU\e[27m+\e[7mSELECT\e[27m to reboot.\n"
    );
    console_flush(&umsboot_console);
}
