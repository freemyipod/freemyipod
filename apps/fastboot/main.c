#include "emcoreapp.h"
#include "libboot.h"


static void main()
{
    void* firmware = NULL;
    int size;
    if (!(clickwheel_get_state() & 0x1f))
    {
        struct emcorelib_header* libboot = get_library(LIBBOOT_IDENTIFIER, LIBBOOT_API_VERSION, LIBSOURCE_BOOTFLASH, "libboot ");
        if (!libboot) panicf(PANIC_KILLTHREAD, "Could not load booting library!");
        struct libboot_api* boot = (struct libboot_api*)libboot->api;
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
        release_library(libboot);
        library_unload(libboot);
    }
    if (firmware)
    {
        shutdown(false);
        execfirmware((void*)0x08000000, firmware, size);
    }
    else
    {
        int size = bootflash_filesize("bootmenu");
        if (size > 0)
        {
            void* buffer = memalign(0x10, size);
            if (buffer)
            {
                if (bootflash_read("bootmenu", buffer, 0, size) == size)
                {
                    if (execimage(buffer, false) != NULL) return;
                }
                else free(buffer);
            }
        }
        panic(PANIC_KILLTHREAD, "Failed to launch boot menu");
    }
}


EMCORE_APP_HEADER("Fastboot launcher", main, 127)
