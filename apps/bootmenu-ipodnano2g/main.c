#include "emcoreapp.h"
#include "libboot.h"
#include "libpng.h"
#include "libui.h"


void* framebuf;
    
    
bool mychooser_preblit(struct chooser_data* data)
{
    char buf[4];
    struct chooser_action_handler_wheel_data* adata;
    adata = (struct chooser_action_handler_wheel_data*)(data->actionhandlerdata);
    snprintf(buf, sizeof(buf), "%3d", adata->timeout_remaining / 1000000);
    rendertext(framebuf, 158, 124, 176, 0xffffcccc, 0, buf);
    return false;
}


struct chooser_renderer_iconflow_itemdata mychooser_rparams_0 =
{
    .icon = LIBUI_SURFACE(LIBUI_LOCATION(LIBUI_BUFFER(NULL, 44), LIBUI_POINT(0, 31)),
                          LIBUI_POINT(44, 40)),
    .icon_selected = LIBUI_SURFACE(LIBUI_LOCATION(LIBUI_BUFFER(NULL, 44), LIBUI_POINT(0, 31)),
                                   LIBUI_POINT(44, 40)),
    .text = "Power off",
    .text_color = 0xffffcccc,
};

struct chooser_renderer_iconflow_itemdata mychooser_rparams_1 =
{
    .icon = LIBUI_SURFACE(LIBUI_LOCATION(LIBUI_BUFFER(NULL, 44), LIBUI_POINT(0, 177)),
                          LIBUI_POINT(44, 43)),
    .icon_selected = LIBUI_SURFACE(LIBUI_LOCATION(LIBUI_BUFFER(NULL, 44), LIBUI_POINT(0, 177)),
                                   LIBUI_POINT(44, 43)),
    .text = "Original firmware",
    .text_color = 0xffffcccc,
};

struct chooser_renderer_iconflow_itemdata mychooser_rparams_2 =
{
    .icon = LIBUI_SURFACE(LIBUI_LOCATION(LIBUI_BUFFER(NULL, 44), LIBUI_POINT(0, 0)),
                          LIBUI_POINT(44, 14)),
    .icon_selected = LIBUI_SURFACE(LIBUI_LOCATION(LIBUI_BUFFER(NULL, 44), LIBUI_POINT(0, 0)),
                                   LIBUI_POINT(44, 14)),
    .text = "Rockbox",
    .text_color = 0xffffcccc,
};

struct chooser_renderer_iconflow_itemdata mychooser_rparams_3 =
{
    .icon = LIBUI_SURFACE(LIBUI_LOCATION(LIBUI_BUFFER(NULL, 44), LIBUI_POINT(0, 14)),
                          LIBUI_POINT(44, 17)),
    .icon_selected = LIBUI_SURFACE(LIBUI_LOCATION(LIBUI_BUFFER(NULL, 44), LIBUI_POINT(0, 14)),
                                   LIBUI_POINT(44, 17)),
    .text = "emCORE console",
    .text_color = 0xffffcccc,
};

struct chooser_renderer_iconflow_itemdata mychooser_rparams_4 =
{
    .icon = LIBUI_SURFACE(LIBUI_LOCATION(LIBUI_BUFFER(NULL, 44), LIBUI_POINT(0, 71)),
                          LIBUI_POINT(44, 31)),
    .icon_selected = LIBUI_SURFACE(LIBUI_LOCATION(LIBUI_BUFFER(NULL, 44), LIBUI_POINT(0, 71)),
                                   LIBUI_POINT(44, 31)),
    .text = "UMSboot",
    .text_color = 0xffffcccc,
};

struct chooser_renderer_iconflow_itemdata mychooser_rparams_5 =
{
    .icon = LIBUI_SURFACE(LIBUI_LOCATION(LIBUI_BUFFER(NULL, 44), LIBUI_POINT(0, 133)),
                          LIBUI_POINT(44, 44)),
    .icon_selected = LIBUI_SURFACE(LIBUI_LOCATION(LIBUI_BUFFER(NULL, 44), LIBUI_POINT(0, 133)),
                                   LIBUI_POINT(44, 44)),
    .text = "Diagnostics mode",
    .text_color = 0xffffcccc,
};

struct chooser_renderer_iconflow_itemdata mychooser_rparams_6 =
{
    .icon = LIBUI_SURFACE(LIBUI_LOCATION(LIBUI_BUFFER(NULL, 44), LIBUI_POINT(0, 102)),
                          LIBUI_POINT(44, 31)),
    .icon_selected = LIBUI_SURFACE(LIBUI_LOCATION(LIBUI_BUFFER(NULL, 44), LIBUI_POINT(0, 102)),
                                   LIBUI_POINT(44, 31)),
    .text = "Disk mode",
    .text_color = 0xffffcccc,
};

struct chooser_renderer_iconflow_params mychooser_rparams =
{
    .version = CHOOSER_RENDERER_LIST_PARAMS_VERSION,
    .copy_dest = LIBUI_LOCATION(LIBUI_BUFFER(NULL, 176), LIBUI_POINT(0, 0)),
    .copy_src = LIBUI_SURFACE(LIBUI_LOCATION(LIBUI_BUFFER(NULL, 176), LIBUI_POINT(0, 0)),
                              LIBUI_POINT(176, 132)),
    .bg_dest = LIBUI_LOCATION(LIBUI_BUFFER(NULL, 0), LIBUI_POINT(0, 0)),
    .bg_src = LIBUI_SURFACE(LIBUI_LOCATION(LIBUI_BUFFER(NULL, 0), LIBUI_POINT(0, 0)),
                            LIBUI_POINT(0, 0)),
    .bg_opacity = 0,
    .fill_dest = LIBUI_SURFACE(LIBUI_LOCATION(LIBUI_BUFFER(NULL, 0), LIBUI_POINT(0, 0)),
                               LIBUI_POINT(0, 0)),
    .fill_color = 0,
    .viewport = LIBUI_SURFACE(LIBUI_LOCATION(LIBUI_BUFFER(NULL, 176), LIBUI_POINT(0, 16)),
                              LIBUI_POINT(176, 72)),
    .text_pos = LIBUI_POINT(88, 118),
    .blit_dest = LIBUI_POINT(0, 0),
    .blit_src = LIBUI_SURFACE(LIBUI_LOCATION(LIBUI_BUFFER(NULL, 176), LIBUI_POINT(0, 0)),
                              LIBUI_POINT(176, 132)),
    .smoothness = 500000,
    .startposition = -3,
    .iconsinview = 4,
    .preblit = mychooser_preblit,
    .postblit = NULL
};

struct chooser_action_handler_wheel_params mychooser_aparams =
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

struct chooser_info mychooser =
{
    .version = CHOOSER_INFO_VERSION,
    .actionhandler = NULL,
    .actionhandlerparams = &mychooser_aparams,
    .renderer = NULL,
    .rendererparams = &mychooser_rparams,
    .userparams = NULL,
    .tickinterval = 990000,
    .itemcount = 7,
    .defaultitem = 2,
    .items =
    {
        {
            .user = (void*)0,
            .actionparams = NULL,
            .renderparams = &mychooser_rparams_0
        },
        {
            .user = (void*)1,
            .actionparams = NULL,
            .renderparams = &mychooser_rparams_1
        },
        {
            .user = (void*)2,
            .actionparams = NULL,
            .renderparams = &mychooser_rparams_2
        },
        {
            .user = (void*)3,
            .actionparams = NULL,
            .renderparams = &mychooser_rparams_3
        },
        {
            .user = (void*)4,
            .actionparams = NULL,
            .renderparams = &mychooser_rparams_4
        },
        {
            .user = (void*)5,
            .actionparams = NULL,
            .renderparams = &mychooser_rparams_5
        },
        {
            .user = (void*)6,
            .actionparams = NULL,
            .renderparams = &mychooser_rparams_6
        }
    }
};


static void main()
{
    struct emcorelib_header* libboot = get_library(0x4c424365, LIBBOOT_API_VERSION, LIBSOURCE_BOOTFLASH, "libboot ");
    if (!libboot) panicf(PANIC_KILLTHREAD, "Could not load booting library!");
    struct libboot_api* boot = (struct libboot_api*)libboot->api;
    struct emcorelib_header* libpng = get_library(0x64474e50, LIBPNG_API_VERSION, LIBSOURCE_BOOTFLASH, "libpng  ");
    if (!libpng) panicf(PANIC_KILLTHREAD, "Could not load PNG decoder library!");
    struct libpng_api* png = (struct libpng_api*)libpng->api;
    struct emcorelib_header* libui = get_library(0x49554365, LIBUI_API_VERSION, LIBSOURCE_BOOTFLASH, "libui   ");
    if (!libui) panicf(PANIC_KILLTHREAD, "Could not load user interface library!");
    struct libui_api* ui = (struct libui_api*)libui->api;
    int size = bootflash_filesize("backgrnd");
    if (size == -1) panicf(PANIC_KILLTHREAD, "Could not find background image!");
    void* buf = memalign(0x10, size);
    if (!buf) panicf(PANIC_KILLTHREAD, "Could not allocate buffer for background image!");
    bootflash_read("backgrnd", buf, 0, size);
    struct png_info* handle = png->png_open(buf, size);
    if (!handle) panicf(PANIC_KILLTHREAD, "Could not parse background image!");
    struct png_rgb* bg = png->png_decode_rgb(handle);
    if (!bg) panicf(PANIC_KILLTHREAD, "Could not decode background image!");
    png->png_destroy(handle);
    free(buf);
    size = bootflash_filesize("iconset ");
    if (size == -1) panicf(PANIC_KILLTHREAD, "Could not find icon set!");
    buf = memalign(0x10, size);
    if (!buf) panicf(PANIC_KILLTHREAD, "Could not allocate buffer for icon set!");
    bootflash_read("iconset ", buf, 0, size);
    handle = png->png_open(buf, size);
    if (!handle) panicf(PANIC_KILLTHREAD, "Could not parse icon set!");
    struct png_rgba* icons = png->png_decode_rgba(handle);
    if (!icons) panicf(PANIC_KILLTHREAD, "Could not decode icon set!");
    png->png_destroy(handle);
    free(buf);
    size = bootflash_filesize("rbxlogo ");
    if (size == -1) panicf(PANIC_KILLTHREAD, "Could not find Rockbox logo!");
    buf = memalign(0x10, size);
    if (!buf) panicf(PANIC_KILLTHREAD, "Could not allocate buffer for Rockbox logo!");
    bootflash_read("rbxlogo ", buf, 0, size);
    handle = png->png_open(buf, size);
    if (!handle) panicf(PANIC_KILLTHREAD, "Could not parse Rockbox logo!");
    struct png_rgb* rbxlogo = png->png_decode_rgb(handle);
    if (!rbxlogo) panicf(PANIC_KILLTHREAD, "Could not decode Rockbox logo!");
    png->png_destroy(handle);
    free(buf);
    size = bootflash_filesize("crapple ");
    if (size == -1) panicf(PANIC_KILLTHREAD, "Could not find OF logo!");
    buf = memalign(0x10, size);
    if (!buf) panicf(PANIC_KILLTHREAD, "Could not allocate buffer for OF logo!");
    bootflash_read("crapple ", buf, 0, size);
    handle = png->png_open(buf, size);
    if (!handle) panicf(PANIC_KILLTHREAD, "Could not parse OF logo!");
    struct png_rgba* crapple = png->png_decode_rgba(handle);
    if (!rbxlogo) panicf(PANIC_KILLTHREAD, "Could not decode OF logo!");
    png->png_destroy(handle);
    free(buf);
    framebuf = malloc(176 * 132 * 3);
    if (!framebuf) panicf(PANIC_KILLTHREAD, "Could not allocate framebuffer!");
    void* framebuf2 = malloc(176 * 132 * 3);
    if (!framebuf2) panicf(PANIC_KILLTHREAD, "Could not allocate framebuffer 2!");
    mychooser.actionhandler = ui->chooser_action_handler_wheel;
    mychooser.renderer = ui->chooser_renderer_iconflow;
    mychooser_rparams.copy_dest.buf.addr = framebuf;
    mychooser_rparams.copy_src.loc.buf.addr = bg;
    mychooser_rparams.viewport.loc.buf.addr = framebuf;
    mychooser_rparams.blit_src.loc.buf.addr = framebuf;
    mychooser_rparams_0.icon.loc.buf.addr = icons;
    mychooser_rparams_0.icon_selected.loc.buf.addr = icons;
    mychooser_rparams_1.icon.loc.buf.addr = icons;
    mychooser_rparams_1.icon_selected.loc.buf.addr = icons;
    mychooser_rparams_2.icon.loc.buf.addr = icons;
    mychooser_rparams_2.icon_selected.loc.buf.addr = icons;
    mychooser_rparams_3.icon.loc.buf.addr = icons;
    mychooser_rparams_3.icon_selected.loc.buf.addr = icons;
    mychooser_rparams_4.icon.loc.buf.addr = icons;
    mychooser_rparams_4.icon_selected.loc.buf.addr = icons;
    mychooser_rparams_5.icon.loc.buf.addr = icons;
    mychooser_rparams_5.icon_selected.loc.buf.addr = icons;
    mychooser_rparams_6.icon.loc.buf.addr = icons;
    mychooser_rparams_6.icon_selected.loc.buf.addr = icons;
    void* firmware = NULL;
    while (!firmware)
    {
        const struct chooser_item* result = ui->chooser_run(&mychooser);
        switch ((int)(result->user))
        {
        case 0:
            shutdown(true);
            power_off();
            break;
        case 1:
        {
            int i;
            for (i = 23; i <= 115; i += 23)
            {
                if (i < 115)
                    ui->blend(176, 132, 50, framebuf, 0, 0, 176,
                              framebuf, 0, 0, 176, bg, 0, 0, 176);
                else memcpy(framebuf, bg, 176 * 132 * 3);
                memcpy(framebuf2, framebuf, 176 * 132 * 3);
                ui->blenda(111, i, 255, framebuf2, 32, 0, 176,
                           framebuf2, 32, 0, 176, crapple, 0, 115 - i, 111);
                displaylcd(0, 0, 176, 132, framebuf2, 0, 0, 176);
            }
            int fd = file_open("/.boot/appleos.bin", O_RDONLY);
            if (fd > 0)
            {
                size = filesize(fd);
                if (size > 0)
                {
                    void* buf = memalign(0x10, size);
                    if (buf)
                    {
                        if (read(fd, buf, size) == size) firmware = buf;
                        else free(buf);
                    }
                }
                close(fd);
            }
            if (!firmware)
            {
                rendertext(framebuf2, 7, 73, 176, 0xff3333ff, 0xa0000000, "Loading appleos.bin failed!");
                rendertext(framebuf2, 7, 81, 176, 0xff3333ff, 0xa0000000, "  Returning to main menu.  ");
                displaylcd(0, 0, 176, 132, framebuf2, 0, 0, 176);
                sleep(5000000);
            }
            break;
        }
        case 2:
        {
            int i;
            for (i = 2; i <= 52; i += 10)
            {
                if (i < 52)
                    ui->blend(176, 132, 50, framebuf, 0, 0, 176,
                              framebuf, 0, 0, 176, bg, 0, 0, 176);
                else memcpy(framebuf, bg, 176 * 132 * 3);
                ui->blit(154, MIN(47, i), 3, framebuf, 11, MAX(0, i - 47), 176,
                         rbxlogo, 0, MAX(0, 47 - i), 154);
                displaylcd(0, 0, 176, 132, framebuf, 0, 0, 176);
            }
            int fd = file_open("/.rockbox/rockbox.ipod", O_RDONLY);
            if (fd > 0)
            {
                size = filesize(fd);
                if (size > 0)
                {
                    void* buf = memalign(0x10, size);
                    if (buf)
                    {
                        if (read(fd, buf, size) == size)
                            if (!boot->verify_rockbox_checksum(buf, size))
                                firmware = buf;
                        if (!firmware) free(buf);
                    }
                }
                close(fd);
            }
            if (!firmware)
            {
                rendertext(framebuf, 4, 73, 176, 0xff3333ff, 0xa0000000, "Loading rockbox.ipod failed!");
                rendertext(framebuf, 4, 81, 176, 0xff3333ff, 0xa0000000, "  Trying fallback image...  ");
                displaylcd(0, 0, 176, 132, framebuf, 0, 0, 132);
                sleep(5000000);
                size = bootflash_filesize("rockbox ");
                if (size > 0)
                {
                    void* buf = memalign(0x10, size);
                    if (buf)
                    {
                        bootflash_read("rockbox ", buf, 0, size);
                        if (bootflash_attributes("rockbox ") & 0x800)
                        {
                            void* buf2 = malloc(0x100000);
                            if (buf2)
                            {
                                if (!ucl_decompress(buf, size, buf2, (uint32_t*)&size))
                                {
                                    free(buf);
                                    buf = realloc(buf2, size);
                                    if (!buf) buf = buf2;
                                    if (!boot->verify_rockbox_checksum(buf, size))
                                        firmware = buf;
                                    else free(buf);
                                }
                                else
                                {
                                    free(buf2);
                                    free(buf);
                                }
                            }
                            else free(buf);
                        }
                        else
                        {
                            if (!boot->verify_rockbox_checksum(buf, size)) firmware = buf;
                            else free(buf);
                        }
                    }
                }
            }
            if (!firmware)
            {
                memcpy(framebuf, bg, 176 * 132 * 3);
                rendertext(framebuf, 19, 73, 176, 0xff3333ff, 0xa0000000, "Loading Rockbox failed!");
                rendertext(framebuf, 19, 81, 176, 0xff3333ff, 0xa0000000, "Returning to main menu.");
                displaylcd(0, 0, 176, 132, framebuf, 0, 0, 176);
                sleep(5000000);
            }
            break;
        }
        case 3:
            goto leave;
        case 4:
            size = bootflash_filesize("umsboot ");
            if (size > 0)
            {
                void* buf = memalign(0x10, size);
                if (buf)
                {
                    bootflash_read("umsboot ", buf, 0, size);
                    if (bootflash_attributes("umsboot ") & 0x800)
                    {
                        void* buf2 = malloc(0x10000);
                        if (buf2)
                        {
                            if (!ucl_decompress(buf, size, buf2, (uint32_t*)&size))
                            {
                                free(buf);
                                buf = realloc(buf2, size);
                                if (!buf) buf = buf2;
                                firmware = buf;
                            }
                            else
                            {
                                free(buf2);
                                free(buf);
                            }
                        }
                        else free(buf);
                    }
                    else firmware = buf;
                }
            }
            if (!firmware)
            {
                memcpy(framebuf, bg, 176 * 132 * 3);
                rendertext(framebuf, 19, 73, 176, 0xff3333ff, 0xa0000000, "Loading UMSboot failed!");
                rendertext(framebuf, 19, 81, 176, 0xff3333ff, 0xa0000000, "Returning to main menu.");
                displaylcd(0, 0, 176, 132, framebuf, 0, 0, 132);
                sleep(5000000);
            }
            break;
        case 5:
            size = bootflash_filesize("diagmode");
            if (size > 0)
            {
                void* buf = memalign(0x10, size);
                if (buf)
                {
                    bootflash_read("diagmode", buf, 0, size);
                    if (bootflash_attributes("diagmode") & 0x800)
                    {
                        void* buf2 = malloc(0x10000);
                        if (buf2)
                        {
                            if (!ucl_decompress(buf, size, buf2, (uint32_t*)&size))
                            {
                                free(buf);
                                buf = realloc(buf2, size);
                                if (!buf) buf = buf2;
                                firmware = buf;
                            }
                            else
                            {
                                free(buf2);
                                free(buf);
                            }
                        }
                        else free(buf);
                    }
                    else firmware = buf;
                }
            }
            if (!firmware)
            {
                memcpy(framebuf, bg, 176 * 132 * 3);
                rendertext(framebuf, 13, 73, 176, 0xff3333ff, 0xa0000000, "Loading diag mode failed!");
                rendertext(framebuf, 13, 81, 176, 0xff3333ff, 0xa0000000, " Returning to main menu. ");
                displaylcd(0, 0, 176, 132, framebuf, 0, 0, 176);
                sleep(5000000);
            }
            break;
        case 6:
            size = bootflash_filesize("diskmode");
            if (size > 0)
            {
                void* buf = memalign(0x10, size);
                if (buf)
                {
                    bootflash_read("diskmode", buf, 0, size);
                    if (bootflash_attributes("diskmode") & 0x800)
                    {
                        void* buf2 = malloc(0x10000);
                        if (buf2)
                        {
                            if (!ucl_decompress(buf, size, buf2, (uint32_t*)&size))
                            {
                                free(buf);
                                buf = realloc(buf2, size);
                                if (!buf) buf = buf2;
                                firmware = buf;
                            }
                            else
                            {
                                free(buf2);
                                free(buf);
                            }
                        }
                        else free(buf);
                    }
                    else firmware = buf;
                }
            }
            if (!firmware)
            {
                memcpy(framebuf, bg, 176 * 132 * 3);
                rendertext(framebuf, 13, 73, 176, 0xff3333ff, 0xa0000000, "Loading disk mode failed!");
                rendertext(framebuf, 13, 81, 176, 0xff3333ff, 0xa0000000, " Returning to main menu. ");
                displaylcd(0, 0, 176, 132, framebuf, 0, 0, 176);
                sleep(5000000);
            }
            break;
        }
    }
leave:
    free(framebuf2);
    free(framebuf);
    free(crapple);
    free(rbxlogo);
    free(icons);
    free(bg);
    release_library(libui);
    release_library(libpng);
    release_library(libboot);
    library_unload(libui);
    library_unload(libpng);
    library_unload(libboot);
    if (firmware)
    {
        shutdown(false);
        execfirmware((void*)0x08000000, firmware, size);
    }
    else cputs(3, "Dropped into emCORE console.\n");
}


EMCORE_APP_HEADER("Boot menu", main, 127)
