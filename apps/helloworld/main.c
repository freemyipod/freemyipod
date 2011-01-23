#include "emcoreapp.h"
#include "libui.h"


void main();
EMCORE_APP_HEADER("Hello world", main, 127)


void main()
{
    struct emcorelib_header* libui = get_library(0x49554365, LIBUI_API_VERSION, LIBSOURCE_FILESYSTEM, "/ui.emcorelib");
    if (!libui) panicf(PANIC_KILLTHREAD, "Could not load UI library!");
    struct libui_api* ui = (struct libui_api*)libui->api;
    void* srcbuf = memalign(0x10, 92160);
    int fd = file_open("/test.lcd", O_RDONLY);
    read(fd, srcbuf, 92160);
    close(fd);
    displaylcd(194, 319, 0, 239, srcbuf, 0);
    fd = file_open("/test.raw", O_RDONLY);
    read(fd, srcbuf, 92160);
    close(fd);
    void* framebuf = malloc(61440);
    int count = 30720;
    char* in = (char*)srcbuf;
    short* out = (short*)framebuf;
    while (count--)
    {
        int b = *in++ >> 3;
        int g = *in++ >> 2;
        int r = *in++ >> 3;
        *out++ = (r << 11) | (g << 5) | b;
    }
    displaylcd(0, 127, 0, 239, framebuf, 0);
    ui->dither(128, 240, srcbuf, 0, 0, 128, framebuf, 0, 0, 128);
    free(srcbuf);
    displaylcd(128, 255, 0, 239, framebuf, 0);
    displaylcd_sync();
    free(framebuf);
    release_library(libui);
    library_unload(libui);
}
