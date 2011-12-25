//
//
//    Copyright 2011 TheSeven, user890104
//
//
//    This file is part of emCORE.
//
//    emCORE is free software: you can redistribute it and/or
//    modify it under the terms of the GNU General Public License as
//    published by the Free Software Foundation, either version 2 of the
//    License, or (at your option) any later version.
//
//    emCORE is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//    See the GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License along
//    with emCORE.  If not, see <http://www.gnu.org/licenses/>.
//
//


#include "emcoreapp.h"
#include "libboot.h"
#include "libpng.h"
#include "libui.h"
#include "libmkfat32.h"

extern char background_png[];
extern uint32_t background_png_size;
extern char icons_png[];
extern uint32_t icons_png_size;
extern char rockbox_png[];
extern uint32_t rockbox_png_size;
extern char crapple_png[];
extern uint32_t crapple_png_size;

struct libpng_api* png;
struct libboot_api* boot;
struct libui_api* ui;
void* framebuf;
void* framebuf2;
void* bg;
void* icons;
//void* rbxlogo;
//void* crapple;

static struct emcorelib_header* loadlib(uint32_t identifier, uint32_t version, char* filename)
{
    struct emcorelib_header* lib = get_library(identifier, version, LIBSOURCE_BOOTFLASH, filename);
    if (!lib) panicf(PANIC_KILLTHREAD, "Could not load %s", filename);
    return lib;
}

static void* loadpng(const char* buf, const uint32_t size, void* (*decoder)(struct png_info* handle))
{
    if (size == 0) panicf(PANIC_KILLTHREAD, "Could not load PNG at 0x%08X", buf);
    struct png_info* handle = png->png_open(buf, size);
    if (!handle) panicf(PANIC_KILLTHREAD, "Could not parse PNG at 0x%08X", buf);
    void* out = decoder(handle);
    if (!out) panicf(PANIC_KILLTHREAD, "Could not decode PNG at 0x%08X", buf);
    png->png_destroy(handle);
    return out;
}

static void message(int x, const char* line1, const char* line2)
{
    rendertext(framebuf, x, 73, 240, 0xff3333ff, 0xa0000000, line1);
    rendertext(framebuf, x, 81, 240, 0xff3333ff, 0xa0000000, line2);
    displaylcd(0, 0, 240, 320, framebuf, 0, 0, 240);
    sleep(5000000);
}

struct chooser_renderer_list_itemdata toolchooser_rparams_mainchooser =
{
    .size = LIBUI_POINT(224, 10),
    .fill_box = LIBUI_BOX(LIBUI_POINT(0, 0), LIBUI_POINT(224, 10)),
    .fill_color = 0xa0000000,
    .fill_color_selected = 0x60ffffff,
    .icon_pos = LIBUI_POINT(0, 0),
    .icon = LIBUI_SURFACE(LIBUI_LOCATION(LIBUI_BUFFER(NULL, 0), LIBUI_POINT(0, 0)),
                          LIBUI_POINT(0, 0)),
    .icon_opacity = 0,
    .icon_selected = LIBUI_SURFACE(LIBUI_LOCATION(LIBUI_BUFFER(NULL, 0), LIBUI_POINT(0, 0)),
                                   LIBUI_POINT(0, 0)),
    .icon_selected_opacity = 0,
    .text = "Return to main menu",
    .text_pos = LIBUI_POINT(1, 1),
    .text_color = 0xffffffff,
    .text_color_selected = 0xff7fffff
};

struct chooser_renderer_list_itemdata toolchooser_rparams_umsboot =
{
    .size = LIBUI_POINT(224, 10),
    .fill_box = LIBUI_BOX(LIBUI_POINT(0, 0), LIBUI_POINT(224, 10)),
    .fill_color = 0xa0000000,
    .fill_color_selected = 0x60ffffff,
    .icon_pos = LIBUI_POINT(0, 0),
    .icon = LIBUI_SURFACE(LIBUI_LOCATION(LIBUI_BUFFER(NULL, 0), LIBUI_POINT(0, 0)),
                          LIBUI_POINT(0, 0)),
    .icon_opacity = 0,
    .icon_selected = LIBUI_SURFACE(LIBUI_LOCATION(LIBUI_BUFFER(NULL, 0), LIBUI_POINT(0, 0)),
                                   LIBUI_POINT(0, 0)),
    .icon_selected_opacity = 0,
    .text = "Run UMSboot",
    .text_pos = LIBUI_POINT(1, 1),
    .text_color = 0xffffffff,
    .text_color_selected = 0xff7fffff
};

void run_umsboot(void** firmware, void** app, int* size)
{
    boot->load_from_flash(firmware, size, false, "umsboot ", 0x10000);
    if (!*firmware)
    {
        memcpy(framebuf, bg, 240 * 320 * 3);
        message(19, "Loading UMSboot failed!", "Returning to main menu.");
    }
}

struct chooser_renderer_list_itemdata toolchooser_rparams_diagmode =
{
    .size = LIBUI_POINT(224, 10),
    .fill_box = LIBUI_BOX(LIBUI_POINT(0, 0), LIBUI_POINT(224, 10)),
    .fill_color = 0xa0000000,
    .fill_color_selected = 0x60ffffff,
    .icon_pos = LIBUI_POINT(0, 0),
    .icon = LIBUI_SURFACE(LIBUI_LOCATION(LIBUI_BUFFER(NULL, 0), LIBUI_POINT(0, 0)),
                          LIBUI_POINT(0, 0)),
    .icon_opacity = 0,
    .icon_selected = LIBUI_SURFACE(LIBUI_LOCATION(LIBUI_BUFFER(NULL, 0), LIBUI_POINT(0, 0)),
                                   LIBUI_POINT(0, 0)),
    .icon_selected_opacity = 0,
    .text = "Run diagnostics mode",
    .text_pos = LIBUI_POINT(1, 1),
    .text_color = 0xffffffff,
    .text_color_selected = 0xff7fffff
};

void run_diagmode(void** firmware, void** app, int* size)
{
    boot->load_from_flash(firmware, size, false, "diagmode", 0x100000);
    if (!*firmware)
    {
        memcpy(framebuf, bg, 240 * 320 * 3);
        message(16, "Loading diag mode failed!", " Returning to main menu. ");
    }
}

struct chooser_renderer_list_itemdata toolchooser_rparams_rockbox_fallback =
{
    .size = LIBUI_POINT(224, 10),
    .fill_box = LIBUI_BOX(LIBUI_POINT(0, 0), LIBUI_POINT(224, 10)),
    .fill_color = 0xa0000000,
    .fill_color_selected = 0x60ffffff,
    .icon_pos = LIBUI_POINT(0, 0),
    .icon = LIBUI_SURFACE(LIBUI_LOCATION(LIBUI_BUFFER(NULL, 0), LIBUI_POINT(0, 0)),
                          LIBUI_POINT(0, 0)),
    .icon_opacity = 0,
    .icon_selected = LIBUI_SURFACE(LIBUI_LOCATION(LIBUI_BUFFER(NULL, 0), LIBUI_POINT(0, 0)),
                                   LIBUI_POINT(0, 0)),
    .icon_selected_opacity = 0,
    .text = "Run Rockbox fallback image",
    .text_pos = LIBUI_POINT(1, 1),
    .text_color = 0xffffffff,
    .text_color_selected = 0xff7fffff
};

void run_rockbox_fallback(void** firmware, void** app, int* size)
{
    boot->load_from_flash(firmware, size, true, "rockbox ", 0x100000);
    if (!*firmware)
    {
        memcpy(framebuf, bg, 240 * 320 * 3);
        message(19, "Loading Rockbox failed!", "Returning to main menu.");
    }
}

struct chooser_renderer_list_itemdata toolchooser_rparams_clearcfg =
{
    .size = LIBUI_POINT(224, 10),
    .fill_box = LIBUI_BOX(LIBUI_POINT(0, 0), LIBUI_POINT(224, 10)),
    .fill_color = 0xa0000000,
    .fill_color_selected = 0x60ffffff,
    .icon_pos = LIBUI_POINT(0, 0),
    .icon = LIBUI_SURFACE(LIBUI_LOCATION(LIBUI_BUFFER(NULL, 0), LIBUI_POINT(0, 0)),
                          LIBUI_POINT(0, 0)),
    .icon_opacity = 0,
    .icon_selected = LIBUI_SURFACE(LIBUI_LOCATION(LIBUI_BUFFER(NULL, 0), LIBUI_POINT(0, 0)),
                                   LIBUI_POINT(0, 0)),
    .icon_selected_opacity = 0,
    .text = "Clear Rockbox configuration",
    .text_pos = LIBUI_POINT(1, 1),
    .text_color = 0xffffffff,
    .text_color_selected = 0xff7fffff
};

void run_clearcfg(void** firmware, void** app, int* size)
{
    remove("/.rockbox/config.cfg");
    memcpy(framebuf, bg, 240 * 320 * 3);
    message(25, "Rockbox configuration", "  has been cleared.  ");
}

struct chooser_renderer_list_itemdata toolchooser_rparams_cleardb =
{
    .size = LIBUI_POINT(224, 10),
    .fill_box = LIBUI_BOX(LIBUI_POINT(0, 0), LIBUI_POINT(224, 10)),
    .fill_color = 0xa0000000,
    .fill_color_selected = 0x60ffffff,
    .icon_pos = LIBUI_POINT(0, 0),
    .icon = LIBUI_SURFACE(LIBUI_LOCATION(LIBUI_BUFFER(NULL, 0), LIBUI_POINT(0, 0)),
                          LIBUI_POINT(0, 0)),
    .icon_opacity = 0,
    .icon_selected = LIBUI_SURFACE(LIBUI_LOCATION(LIBUI_BUFFER(NULL, 0), LIBUI_POINT(0, 0)),
                                   LIBUI_POINT(0, 0)),
    .icon_selected_opacity = 0,
    .text = "Clear Rockbox database",
    .text_pos = LIBUI_POINT(1, 1),
    .text_color = 0xffffffff,
    .text_color_selected = 0xff7fffff
};

void run_cleardb(void** firmware, void** app, int* size)
{
    remove("/.rockbox/database_0.tcd");
    remove("/.rockbox/database_1.tcd");
    remove("/.rockbox/database_2.tcd");
    remove("/.rockbox/database_3.tcd");
    remove("/.rockbox/database_4.tcd");
    remove("/.rockbox/database_5.tcd");
    remove("/.rockbox/database_6.tcd");
    remove("/.rockbox/database_7.tcd");
    remove("/.rockbox/database_8.tcd");
    remove("/.rockbox/database_9.tcd");
    remove("/.rockbox/database_idx.tcd");
    remove("/.rockbox/database_tmp.tcd");
    remove("/.rockbox/database_state.tcd");
    remove("/.rockbox/database_changelog.txt");
    memcpy(framebuf, bg, 240 * 320 * 3);
    message(37, "Rockbox  database", "has been cleared.");
}

struct chooser_renderer_list_itemdata toolchooser_rparams_reformat =
{
    .size = LIBUI_POINT(224, 10),
    .fill_box = LIBUI_BOX(LIBUI_POINT(0, 0), LIBUI_POINT(224, 10)),
    .fill_color = 0xa0000000,
    .fill_color_selected = 0x60ffffff,
    .icon_pos = LIBUI_POINT(0, 0),
    .icon = LIBUI_SURFACE(LIBUI_LOCATION(LIBUI_BUFFER(NULL, 0), LIBUI_POINT(0, 0)),
                          LIBUI_POINT(0, 0)),
    .icon_opacity = 0,
    .icon_selected = LIBUI_SURFACE(LIBUI_LOCATION(LIBUI_BUFFER(NULL, 0), LIBUI_POINT(0, 0)),
                                   LIBUI_POINT(0, 0)),
    .icon_selected_opacity = 0,
    .text = "Reformat data partition",
    .text_pos = LIBUI_POINT(1, 1),
    .text_color = 0xffffffff,
    .text_color_selected = 0xff7fffff
};

void fat32_progressbar_init(void* user, int max)
{
    progressbar_init((struct progressbar_state*)user,
                     10, 165, 74, 83, 0x77ff, 0xe8, 0x125f, 0, max);
}

void fat32_progressbar_update(void* user, int current)
{
    progressbar_setpos((struct progressbar_state*)user, current, false);
}

void run_reformat(void** firmware, void** app, int* size)
{
    memcpy(framebuf, bg, 240 * 320 * 3);
    rendertext(framebuf, 7, 65, 240, 0xff7fffff, 0, "Reformatting data partition");
    displaylcd(0, 0, 240, 320, framebuf, 0, 0, 240);
    struct emcorelib_header* libmkfat32 = loadlib(LIBMKFAT32_IDENTIFIER,
                                                  LIBMKFAT32_API_VERSION, "libmkf32");
    struct libmkfat32_api* mf32 = (struct libmkfat32_api*)libmkfat32->api;
    struct storage_info storageinfo;
    storage_get_info(0, &storageinfo);
    struct progressbar_state progressbar;
    int rc = mf32->mkfat32(0, 0, storageinfo.num_sectors, 2048, 1, "iPod Nano2G",
                           &progressbar, fat32_progressbar_init, fat32_progressbar_update);
    if (rc < 0) panicf(PANIC_KILLTHREAD, "Error formatting data partition: %08X", rc);
    release_library(libmkfat32);
    library_unload(libmkfat32);
    memcpy(framebuf, bg, 240 * 320 * 3);
    message(34, "Data partition has", "been  reformatted.");
}

struct chooser_renderer_list_itemdata toolchooser_rparams_uninstaller =
{
    .size = LIBUI_POINT(224, 10),
    .fill_box = LIBUI_BOX(LIBUI_POINT(0, 0), LIBUI_POINT(224, 10)),
    .fill_color = 0xa0000000,
    .fill_color_selected = 0x60ffffff,
    .icon_pos = LIBUI_POINT(0, 0),
    .icon = LIBUI_SURFACE(LIBUI_LOCATION(LIBUI_BUFFER(NULL, 0), LIBUI_POINT(0, 0)),
                          LIBUI_POINT(0, 0)),
    .icon_opacity = 0,
    .icon_selected = LIBUI_SURFACE(LIBUI_LOCATION(LIBUI_BUFFER(NULL, 0), LIBUI_POINT(0, 0)),
                                   LIBUI_POINT(0, 0)),
    .icon_selected_opacity = 0,
    .text = "Uninstall emCORE",
    .text_pos = LIBUI_POINT(1, 1),
    .text_color = 0xffffffff,
    .text_color_selected = 0xff7fffff
};

void run_uninstaller(void** firmware, void** app, int* size)
{
    boot->load_from_flash(app, size, false, "uninst  ", 0);
    if (!*app)
    {
        memcpy(framebuf, bg, 240 * 320 * 3);
        message(7, "Loading uninstaller failed!", "  Returning to main menu.  ");
    }
}

bool update_display(struct chooser_data* data)
{
    char buf[6];
    struct rtc_datetime dt;
    //rtc_read_datetime(&dt);
    dt.hour = 13; dt.minute = 37; // Leet :)
    snprintf(buf, sizeof(buf), "%02d:%02d", dt.hour, dt.minute);
    // clock
    rendertext(framebuf, 207, 4, 240, 0xffffcccc, 0, buf);
    unsigned int batt_level = 11; // set level to 50%
    // unsigned int batt_level = 22 * read_battery_mwh_current(0) / read_battery_mwh_full(0);
    // remaining battery level
    ui->blendcolor(batt_level, 6, 0xc0ffcccc, framebuf, 5, 5, 240, framebuf, 5, 5, 240);
    // background of the rest space
    ui->blendcolor(22 - batt_level, 6, 0x40000000, framebuf, 5 + batt_level, 5, 240, framebuf, 5 + batt_level, 5, 240);
    return false;
}

struct chooser_renderer_list_params toolchooser_rparams =
{
    .version = CHOOSER_RENDERER_LIST_PARAMS_VERSION,
    .copy_dest = LIBUI_LOCATION(LIBUI_BUFFER(NULL, 240), LIBUI_POINT(0, 0)),
    .copy_src = LIBUI_SURFACE(LIBUI_LOCATION(LIBUI_BUFFER(NULL, 240), LIBUI_POINT(0, 0)),
                              LIBUI_POINT(240, 320)),
    .bg_dest = LIBUI_LOCATION(LIBUI_BUFFER(NULL, 0), LIBUI_POINT(0, 0)),
    .bg_src = LIBUI_SURFACE(LIBUI_LOCATION(LIBUI_BUFFER(NULL, 0), LIBUI_POINT(0, 0)),
                            LIBUI_POINT(0, 0)),
    .bg_opacity = 0,
    .fill_dest = LIBUI_SURFACE(LIBUI_LOCATION(LIBUI_BUFFER(NULL, 0), LIBUI_POINT(0, 0)),
                               LIBUI_POINT(0, 0)),
    .fill_color = 0,
    .viewport = LIBUI_SURFACE(LIBUI_LOCATION(LIBUI_BUFFER(NULL, 240), LIBUI_POINT(7, 30)),
                              LIBUI_POINT(226, 80)),
    .blit_dest = LIBUI_POINT(0, 0),
    .blit_src = LIBUI_SURFACE(LIBUI_LOCATION(LIBUI_BUFFER(NULL, 240), LIBUI_POINT(0, 0)),
                              LIBUI_POINT(240, 320)),
    .preblit = update_display,
    .postblit = NULL
};

struct chooser_action_handler_wheel_params toolchooser_aparams =
{
    .version = CHOOSER_ACTION_HANDLER_WHEEL_PARAMS_VERSION,
    .stepsperitem = 128,
    .eventfilter = NULL,
    .timeout_initial = TIMEOUT_BLOCK,
    .timeout_idle = TIMEOUT_BLOCK,
    .timeout_item = 0,
    .tick_force_redraw = false,
    .buttoncount = 5,
    .buttonmap =
    {
        CHOOSER_ACTION_HANDLER_WHEEL_ACTION_SELECT,
        CHOOSER_ACTION_HANDLER_WHEEL_ACTION_NEXT,
        CHOOSER_ACTION_HANDLER_WHEEL_ACTION_PREV,
        CHOOSER_ACTION_HANDLER_WHEEL_ACTION_NEXT,
        CHOOSER_ACTION_HANDLER_WHEEL_ACTION_PREV
    }
};

struct chooser_info toolchooser =
{
    .version = CHOOSER_INFO_VERSION,
    .actionhandler = NULL,
    .actionhandlerparams = &toolchooser_aparams,
    .renderer = NULL,
    .rendererparams = &toolchooser_rparams,
    .userparams = NULL,
    .tickinterval = 10000000,
    .itemcount = 8,
    .defaultitem = 0,
    .items =
    {
        {
            .user = NULL,
            .actionparams = NULL,
            .renderparams = &toolchooser_rparams_mainchooser
        },
        {
            .user = run_umsboot,
            .actionparams = NULL,
            .renderparams = &toolchooser_rparams_umsboot
        },
        {
            .user = run_diagmode,
            .actionparams = NULL,
            .renderparams = &toolchooser_rparams_diagmode
        },
        {
            .user = run_rockbox_fallback,
            .actionparams = NULL,
            .renderparams = &toolchooser_rparams_rockbox_fallback
        },
        {
            .user = run_clearcfg,
            .actionparams = NULL,
            .renderparams = &toolchooser_rparams_clearcfg
        },
        {
            .user = run_cleardb,
            .actionparams = NULL,
            .renderparams = &toolchooser_rparams_cleardb
        },
        {
            .user = run_reformat,
            .actionparams = NULL,
            .renderparams = &toolchooser_rparams_reformat
        },
        {
            .user = run_uninstaller,
            .actionparams = NULL,
            .renderparams = &toolchooser_rparams_uninstaller
        }
    }
};

bool mainchooser_preblit(struct chooser_data* data)
{
    char buf[4];
    struct chooser_action_handler_wheel_data* adata;
    adata = (struct chooser_action_handler_wheel_data*)(data->actionhandlerdata);
    snprintf(buf, sizeof(buf), "%3d", adata->timeout_remaining / 1000000);
    rendertext(framebuf, 219, 309, 240, 0xffffcccc, 0, buf);
    update_display(data);
    return false;
}

struct chooser_renderer_iconflow_itemdata mainchooser_rparams_powerdown =
{
    .icon = LIBUI_SURFACE(LIBUI_LOCATION(LIBUI_BUFFER(NULL, 80), LIBUI_POINT(0, 0)),
                          LIBUI_POINT(80, 80)),
    .icon_selected = LIBUI_SURFACE(LIBUI_LOCATION(LIBUI_BUFFER(NULL, 80), LIBUI_POINT(0, 0)),
                                   LIBUI_POINT(80, 80)),
    .text = "Power off",
    .text_color = 0xffffcccc,
};

void run_powerdown(void** firmware, void** app, int* size)
{
    shutdown(true);
    power_off();
}
/*
struct chooser_renderer_iconflow_itemdata mainchooser_rparams_crapple =
{
    .icon = LIBUI_SURFACE(LIBUI_LOCATION(LIBUI_BUFFER(NULL, 44), LIBUI_POINT(0, 123)),
                          LIBUI_POINT(44, 43)),
    .icon_selected = LIBUI_SURFACE(LIBUI_LOCATION(LIBUI_BUFFER(NULL, 44), LIBUI_POINT(0, 123)),
                                   LIBUI_POINT(44, 43)),
    .text = "Original firmware",
    .text_color = 0xffffcccc,
};

void run_crapple(void** firmware, void** app, int* size)
{
    int i;
    for (i = 23; i <= 115; i += 23)
    {
        if (i < 115)
            ui->blend(240, 320, 50, framebuf, 0, 0, 240,
                      framebuf, 0, 0, 240, bg, 0, 0, 240);
        else memcpy(framebuf, bg, 240 * 320 * 3);
        memcpy(framebuf2, framebuf, 240 * 320 * 3);
        ui->blenda(111, i, 255, framebuf2, 32, 0, 240,
                   framebuf2, 32, 0, 240, crapple, 0, 115 - i, 111);
        displaylcd(0, 0, 240, 320, framebuf2, 0, 0, 240);
    }
    boot->load_from_file(firmware, size, false, "/.boot/appleos.ucl", 0x800000);
    if (!*firmware) boot->load_from_file(firmware, size, false, "/.boot/appleos.bin", 0);
    if (!*firmware) message(7, "Loading appleos.bin failed!", "  Returning to main menu.  ");
}
*/
struct chooser_renderer_iconflow_itemdata mainchooser_rparams_rockbox =
{
    .icon = LIBUI_SURFACE(LIBUI_LOCATION(LIBUI_BUFFER(NULL, 80), LIBUI_POINT(0, 80)),
                          LIBUI_POINT(80, 80)),
    .icon_selected = LIBUI_SURFACE(LIBUI_LOCATION(LIBUI_BUFFER(NULL, 80), LIBUI_POINT(0, 80)),
                                   LIBUI_POINT(80, 80)),
    .text = "Rockbox",
    .text_color = 0xffffcccc,
};

void run_rockbox(void** firmware, void** app, int* size)
{
/*
    int i;
    for (i = 2; i <= 52; i += 10)
    {
        if (i < 52)
            ui->blend(240, 320, 50, framebuf, 0, 0, 240,
                      framebuf, 0, 0, 240, bg, 0, 0, 240);
        else memcpy(framebuf, bg, 240 * 320 * 3);
        ui->blit(154, MIN(47, i), 3, framebuf, 11, MAX(0, i - 47), 240,
                 rbxlogo, 0, MAX(0, 47 - i), 154);
        displaylcd(0, 0, 240, 320, framebuf, 0, 0, 240);
    }
*/
    boot->load_from_file(firmware, size, true, "/.rockbox/rockbox.ipod", 0);
    if (!*firmware)
    {
        message(4, "Loading rockbox.ipod failed!", "  Trying fallback image...  ");
        boot->load_from_flash(firmware, size, true, "rockbox ", 0x100000);
    }
    if (!*firmware)
    {
        memcpy(framebuf, bg, 240 * 320 * 3);
        message(19, "Loading Rockbox failed!", "Returning to main menu.");
    }
}

struct chooser_renderer_iconflow_itemdata mainchooser_rparams_console =
{
    .icon = LIBUI_SURFACE(LIBUI_LOCATION(LIBUI_BUFFER(NULL, 80), LIBUI_POINT(0, 160)),
                          LIBUI_POINT(80, 80)),
    .icon_selected = LIBUI_SURFACE(LIBUI_LOCATION(LIBUI_BUFFER(NULL, 80), LIBUI_POINT(0, 160)),
                                   LIBUI_POINT(80, 80)),
    .text = "emCORE console",
    .text_color = 0xffffcccc,
};
/*
struct chooser_renderer_iconflow_itemdata mainchooser_rparams_diskmode =
{
    .icon = LIBUI_SURFACE(LIBUI_LOCATION(LIBUI_BUFFER(NULL, 44), LIBUI_POINT(0, 166)),
                          LIBUI_POINT(44, 31)),
    .icon_selected = LIBUI_SURFACE(LIBUI_LOCATION(LIBUI_BUFFER(NULL, 44), LIBUI_POINT(0, 166)),
                                   LIBUI_POINT(44, 31)),
    .text = "Disk mode",
    .text_color = 0xffffcccc,
};

void run_diskmode(void** firmware, void** app, int* size)
{
    boot->load_from_flash(firmware, size, false, "diskmode", 0x100000);
    if (!*firmware)
    {
        memcpy(framebuf, bg, 240 * 320 * 3);
        message(13, "Loading disk mode failed!", " Returning to main menu. ");
    }
}
*/
struct chooser_renderer_iconflow_itemdata mainchooser_rparams_toolchooser =
{
    .icon = LIBUI_SURFACE(LIBUI_LOCATION(LIBUI_BUFFER(NULL, 80), LIBUI_POINT(0, 240)),
                          LIBUI_POINT(80, 80)),
    .icon_selected = LIBUI_SURFACE(LIBUI_LOCATION(LIBUI_BUFFER(NULL, 80), LIBUI_POINT(0, 240)),
                                   LIBUI_POINT(80, 80)),
    .text = "Tools",
    .text_color = 0xffffcccc,
};

static void run_toolchooser(void** firmware, void** app, int* size)
{
    const struct chooser_item* result = ui->chooser_run(&toolchooser);
    if (!result->user) return;
    void (*selected_function)(void** firmware, void** app, int* size);
    selected_function = (void(*)(void** firmware, void** app, int* size))(result->user);
    selected_function(firmware, app, size);
}

struct chooser_renderer_iconflow_params mainchooser_rparams =
{
    .version = CHOOSER_RENDERER_ICONFLOW_PARAMS_VERSION,
    .copy_dest = LIBUI_LOCATION(LIBUI_BUFFER(NULL, 240), LIBUI_POINT(0, 0)),
    .copy_src = LIBUI_SURFACE(LIBUI_LOCATION(LIBUI_BUFFER(NULL, 240), LIBUI_POINT(0, 0)),
                              LIBUI_POINT(240, 320)),
    .bg_dest = LIBUI_LOCATION(LIBUI_BUFFER(NULL, 0), LIBUI_POINT(0, 0)),
    .bg_src = LIBUI_SURFACE(LIBUI_LOCATION(LIBUI_BUFFER(NULL, 0), LIBUI_POINT(0, 0)),
                            LIBUI_POINT(0, 0)),
    .bg_opacity = 0,
    .fill_dest = LIBUI_SURFACE(LIBUI_LOCATION(LIBUI_BUFFER(NULL, 0), LIBUI_POINT(0, 0)),
                               LIBUI_POINT(0, 0)),
    .fill_color = 0,
    .viewport = LIBUI_SURFACE(LIBUI_LOCATION(LIBUI_BUFFER(NULL, 240), LIBUI_POINT(0, 66)),
                              LIBUI_POINT(240, 160)),
    .text_pos = LIBUI_POINT(120, 306),
    .blit_dest = LIBUI_POINT(0, 0),
    .blit_src = LIBUI_SURFACE(LIBUI_LOCATION(LIBUI_BUFFER(NULL, 240), LIBUI_POINT(0, 0)),
                              LIBUI_POINT(240, 320)),
    .smoothness = 500000,
    .startposition = -3,
    .iconsinview = 4,
    .preblit = mainchooser_preblit,
    .postblit = NULL
};

struct chooser_action_handler_wheel_params mainchooser_aparams =
{
    .version = CHOOSER_ACTION_HANDLER_WHEEL_PARAMS_VERSION,
    .stepsperitem = 512,
    .eventfilter = NULL,
    .timeout_initial = 30500000,
    .timeout_idle = 300500000,
    .timeout_item = 0,
    .tick_force_redraw = true,
    .buttoncount = 3,
    .buttonmap =
    {
        CHOOSER_ACTION_HANDLER_WHEEL_ACTION_SELECT,
        CHOOSER_ACTION_HANDLER_WHEEL_ACTION_NEXT,
        CHOOSER_ACTION_HANDLER_WHEEL_ACTION_PREV
    }
};

struct chooser_info mainchooser =
{
    .version = CHOOSER_INFO_VERSION,
    .actionhandler = NULL,
    .actionhandlerparams = &mainchooser_aparams,
    .renderer = NULL,
    .rendererparams = &mainchooser_rparams,
    .userparams = NULL,
    .tickinterval = 990000,
    .itemcount = 4,
    .defaultitem = 1,
    .items =
    {
        {
            .user = run_powerdown,
            .actionparams = NULL,
            .renderparams = &mainchooser_rparams_powerdown
        },
        {
            .user = run_rockbox,
            .actionparams = NULL,
            .renderparams = &mainchooser_rparams_rockbox
        },
        {
            .user = NULL,
            .actionparams = NULL,
            .renderparams = &mainchooser_rparams_console
        },
        {
            .user = run_toolchooser,
            .actionparams = NULL,
            .renderparams = &mainchooser_rparams_toolchooser
        }
    }
};

static void run_mainchooser(void** firmware, void** app, int* size)
{
    while (!*firmware && !*app)
    {
        const struct chooser_item* result = ui->chooser_run(&mainchooser);
        if (!result->user) return;
        void (*selected_function)(void** firmware, void** app, int* size);
        selected_function = (void(*)(void** firmware, void** app, int* size))(result->user);
        selected_function(firmware, app, size);
    }
}

static void main(int argc, const char** argv)
{
    struct emcorelib_header* libpng = loadlib(LIBPNG_IDENTIFIER, LIBPNG_API_VERSION, "libpng  ");
    png = (struct libpng_api*)libpng->api;
    bg = loadpng(background_png, background_png_size, (void* (*)(struct png_info*))(png->png_decode_rgb));
    icons = loadpng(icons_png, icons_png_size, (void* (*)(struct png_info*))(png->png_decode_rgba));
    //rbxlogo = loadpng(rockbox_png, rockbox_png_size, (void* (*)(struct png_info*))(png->png_decode_rgb));
    //crapple = loadpng(crapple_png, crapple_png_size, (void* (*)(struct png_info*))(png->png_decode_rgba));
    release_library(libpng);
    library_unload(libpng);
    struct emcorelib_header* libboot = loadlib(LIBBOOT_IDENTIFIER,
                                               LIBBOOT_API_VERSION, "libboot ");
    boot = (struct libboot_api*)libboot->api;
    struct emcorelib_header* libui = loadlib(LIBUI_IDENTIFIER, LIBUI_API_VERSION, "libui   ");
    ui = (struct libui_api*)libui->api;
    // draw the battery meter box
    // top line
    ui->blendcolor(24, 1, 0xffffcccc, bg, 4, 4, 240, bg, 4, 4, 240);
    // bottom line
    ui->blendcolor(24, 1, 0xffffcccc, bg, 4, 11, 240, bg, 4, 11, 240);
    // left line
    ui->blendcolor(1, 6, 0xffffcccc, bg, 4, 5, 240, bg, 4, 5, 240);
    // right line
    ui->blendcolor(1, 6, 0xffffcccc, bg, 27, 5, 240, bg, 27, 5, 240);
    // tip - right
    ui->blendcolor(1, 4, 0xffffcccc, bg, 28, 6, 240, bg, 28, 6, 240);
    framebuf = malloc(240 * 320 * 3);
    if (!framebuf) panicf(PANIC_KILLTHREAD, "Could not allocate framebuffer!");
    framebuf2 = malloc(240 * 320 * 3);
    if (!framebuf2) panicf(PANIC_KILLTHREAD, "Could not allocate framebuffer 2!");
    mainchooser.actionhandler = ui->chooser_action_handler_wheel;
    mainchooser.renderer = ui->chooser_renderer_iconflow;
    mainchooser_rparams.copy_dest.buf.addr = framebuf;
    mainchooser_rparams.copy_src.loc.buf.addr = bg;
    mainchooser_rparams.viewport.loc.buf.addr = framebuf;
    mainchooser_rparams.blit_src.loc.buf.addr = framebuf;
    mainchooser_rparams_powerdown.icon.loc.buf.addr = icons;
    mainchooser_rparams_powerdown.icon_selected.loc.buf.addr = icons;
    //mainchooser_rparams_crapple.icon.loc.buf.addr = icons;
    //mainchooser_rparams_crapple.icon_selected.loc.buf.addr = icons;
    mainchooser_rparams_rockbox.icon.loc.buf.addr = icons;
    mainchooser_rparams_rockbox.icon_selected.loc.buf.addr = icons;
    mainchooser_rparams_console.icon.loc.buf.addr = icons;
    mainchooser_rparams_console.icon_selected.loc.buf.addr = icons;
    //mainchooser_rparams_diskmode.icon.loc.buf.addr = icons;
    //mainchooser_rparams_diskmode.icon_selected.loc.buf.addr = icons;
    mainchooser_rparams_toolchooser.icon.loc.buf.addr = icons;
    mainchooser_rparams_toolchooser.icon_selected.loc.buf.addr = icons;

    toolchooser.actionhandler = ui->chooser_action_handler_wheel;
    toolchooser.renderer = ui->chooser_renderer_list;
    toolchooser_rparams.copy_dest.buf.addr = framebuf;
    toolchooser_rparams.copy_src.loc.buf.addr = bg;
    toolchooser_rparams.viewport.loc.buf.addr = framebuf;
    toolchooser_rparams.blit_src.loc.buf.addr = framebuf;

    backlight_set_brightness(177);
    void* firmware = NULL;
    void* app = NULL;
    int size;
    run_mainchooser(&firmware, &app, &size);

    free(framebuf2);
    free(framebuf);
    //free(crapple);
    //free(rbxlogo);
    free(icons);
    free(bg);
    release_library(libui);
    release_library(libboot);
    library_unload(libui);
    library_unload(libboot);

    if (firmware)
    {
        shutdown(false);
        execfirmware((void*)0x08000000, firmware, size);
    }
    else if (app) execimage(app, false);
    else cputs(3, "Dropped into emCORE console.\n");
}


EMCORE_APP_HEADER("Boot menu", main, 127)
