//
//
//    Copyright 2013 user890104
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

#include "global.h"

#include "usb.h"
#include "cache.h"
#include "emcore.h"
#include "fuse.h"
#include "util.h"

int main() {
    int32_t res, res2;
    uint8_t reattach = 0;

    /* emcore_get_version */
    struct emcore_dev_info dev_info;

    /* emcore_get_malloc_pool_bounds */
    struct emcore_malloc_pool_bounds bounds;

    /* emcore_readmem */
    /*
    void *buf;
    uint16_t buf_size;
    uint32_t read_addr;
    */
    
    /* emcore_readi2c */
    /* uint8_t i2cdata; */

    /* emcore_dir_open */
    //uint32_t dir_handle;

    /* emcore_dir_close_all */
    //uint32_t count;

    res = usb_init();

    if (res == LIBUSB_SUCCESS) {
        res = usb_find(EMCORE_USB_VID, EMCORE_USB_PID, &reattach);
    }

    if (res == LIBUSB_SUCCESS) {
        res = emcorefs_init();
    }

    do {
        if (res != EMCORE_SUCCESS) {
            break;
        }

        res = emcore_get_version(&dev_info);

        if (res != EMCORE_SUCCESS) {
            break;
        }

        printf("Connected to %c%c%c%c running %s v%d.%d.%d r%d\n",
            dev_info.hw_type & 0xFF, (dev_info.hw_type >> 8) & 0xFF, (dev_info.hw_type >> 16) & 0xFF, dev_info.hw_type >> 24,
            (dev_info.sw_type == 2 ? "emCORE" : "UNKNOWN"),
            dev_info.major, dev_info.minor, dev_info.patch, dev_info.svn_revision
        );
        
        res = emcore_get_malloc_pool_bounds(&bounds);

        if (res != EMCORE_SUCCESS) {
            break;
        }

        printf("Malloc pool bounds: 0x%08x - 0x%08x\n", bounds.lower, bounds.upper);
/*
        read_addr = 0x09000000;
        buf_size = 0x1000;
        buf = malloc(buf_size);
        
        printf("Reading 0x%08x bytes from 0x%08x\n", buf_size, read_addr);

        res = emcore_read(buf, read_addr, buf_size);

        if (res != EMCORE_SUCCESS) {
            free(buf);
            break;
        }

#ifdef DEBUG
        //dump_packet(buf, buf_size);

#endif

        printf("Writing 0x%08x bytes to 0x%08x\n", buf_size, read_addr);

        res = emcore_write(buf, read_addr, buf_size);

        if (res != EMCORE_SUCCESS) {
            free(buf);
            break;
        }

        free(buf);
*/
        /*
        printf("Reading 1 byte from I2C\n");

        res = emcore_readi2c(&i2cdata, 0, 0xe6, 0x29, 1);

        if (res != EMCORE_SUCCESS) {
            break;
        }

#ifdef DEBUG
        dump_packet(&i2cdata, 1);

#endif
        */
        /* nano2g - turns on/off the backlight */
        /*
        i2cdata = 1;

        printf("Writing 1 byte to I2C\n");

        res = emcore_writei2c(&i2cdata, 0, 0xe6, 0x29, 1);

        if (res != EMCORE_SUCCESS) {
            break;
        }

        sleep(1);

        i2cdata = 0;

        printf("Writing 1 byte to I2C\n");

        res = emcore_writei2c(&i2cdata, 0, 0xe6, 0x29, 1);

        if (res != EMCORE_SUCCESS) {
            break;
        }
        */
        /*
        // short filenames without malloc()
        res = emcore_file_open(&dir_handle, "/README", 0);

        if (res != EMCORE_SUCCESS) {
            break;
        }

        printf("Opened file handle: 0x%08x\n", dir_handle);

        res = emcore_file_close(dir_handle);

        if (res != EMCORE_SUCCESS) {
            break;
        }

        printf("Closed file handle: 0x%08x\n", dir_handle);

        // long filenames with malloc()
        res = emcore_file_open(&dir_handle, "/aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa.png", 0);

        if (res != EMCORE_SUCCESS) {
            break;
        }

        printf("Opened file handle: 0x%08x\n", dir_handle);

        res = emcore_file_close(dir_handle);

        if (res != EMCORE_SUCCESS) {
            break;
        }

        printf("Closed file handle: 0x%08x\n", dir_handle);
        */
        /*
        res = emcore_dir_open(&dir_handle, "/");

        if (res != EMCORE_SUCCESS) {
            break;
        }

        printf("Opened dir handle: 0x%08x\n", dir_handle);

        res = emcore_ls(dir_handle);

        if (res != EMCORE_SUCCESS) {
            break;
        }

        printf("Listed dir handle: 0x%08x\n", dir_handle);

        res = emcore_dir_close(dir_handle);

        if (res != EMCORE_SUCCESS) {
            break;
        }

        printf("Closed dir handle 0x%08x\n", dir_handle);

        res = emcore_dir_close_all(&count);

        if (res != EMCORE_SUCCESS) {
            break;
        }

        printf("Closed %d dir handles\n", count);
        */
        
        /* powers off the device - graceful
        res = emcore_poweroff(1);

        if (res != EMCORE_SUCCESS) {
            break;
        }

        printf("Device powered off successfully!\n");
        */
    }
    while (0);

    if (res != EMCORE_SUCCESS) {
        print_error(res);
    }

    res2 = usb_destroy(reattach);

    if (res2 != LIBUSB_SUCCESS) {
        print_error(res2);
    }

    if (res < 0) {
        res = -res;
    }

    return res;
}
