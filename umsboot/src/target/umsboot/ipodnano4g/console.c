#include "global.h"
#include "app/umsboot/console.h"
#include "protocol/i2c/i2c.h"
#include "interface/backlight_manager/backlight_manager.h"
#include "interface/lcdif/lcdif.h"
#include "interface/lcd/lcd.h"
#include "interface/framebuffer/framebuffer.h"
#include "interface/console/console.h"
#include "soc/s5l87xx/i2c.h"
#include "device/d1759/backlight_manager.h"
#include "soc/s5l87xx/lcdif.h"
#include "board/ipodnano4g/lcd.h"
#include "lib/simpletextrenderer/simpletextrenderer.h"
#include "lib/fbconsole/fbconsole.h"


static const struct s5l87xx_i2c_driver_config umsboot_i2c_config =
{
    .index = 0,
};

static const struct i2c_driver_instance umsboot_i2c =
{
    .driver = &s5l87xx_i2c_driver,
    .driver_config = &umsboot_i2c_config,
};

static const struct d1759_backlight_manager_driver_config umsboot_backlight_config =
{
    .i2c = &umsboot_i2c,
    .max_current = 255,
    .default_fade = 1,
};

static const struct backlight_manager_instance umsboot_backlight =
{
    .driver = &d1759_backlight_manager_driver,
    .driver_config = &umsboot_backlight_config,
};

static const struct lcdif_instance umsboot_lcdif =
{
    .driver = &s5l87xx_lcdif_driver,
};

static const struct ipodnano4g_lcd_config umsboot_lcd_config =
{
    .interface = &umsboot_lcdif,
    .backlight = &umsboot_backlight,
};

static const struct lcd_instance umsboot_lcd =
{
    .driver = &ipodnano4g_lcd_driver,
    .driver_config = &umsboot_lcd_config,
};

static uint8_t umsboot_fb_data[320][30];
static const struct framebuffer_instance fb =
{
    .width = 240,
    .height = 320,
    .format = FRAMEBUFFER_FORMAT_I1,
    .data = umsboot_fb_data,
    .update_handler = lcd_fb_update_handler,
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
    .w = 40,
    .h = 40,
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
    lcd_fill(&umsboot_lcd, 0, 0, 240, 320, FRAMEBUFFER_FORMAT_I1, 0, FRAMEBUFFER_CONVERSION_QUALITY_CLIP);
    lcd_backlight(&umsboot_lcd, 100);
    console_init(&umsboot_console);
    console_puts(&umsboot_console,
        "UMSboot v0.2\n\n"
        "Please copy a UBI file to the mass\n"
        "storage device and safely eject\n"
        "it when you're done.\n"
        "If you booted this accidentally, just\n"
        "press and hold \e[7mMENU\e[27m+\e[7mSELECT\e[27m to reboot.\n"
    );
    console_flush(&umsboot_console);
}
