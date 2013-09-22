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

struct fuse_operations emcorefs_oper = {
    .getattr    = emcorefs_getattr,
    
    .opendir    = emcorefs_opendir,
    .readdir    = emcorefs_readdir,
    .releasedir = emcorefs_releasedir,

    .open       = emcorefs_open,
    .read       = emcorefs_read,
    .write      = emcorefs_write,
    .release    = emcorefs_release,

    .mkdir      = emcorefs_mkdir,
    .rmdir      = emcorefs_rmdir,

    .create     = emcorefs_create,
    .mknod      = emcorefs_mknod,
    .unlink     = emcorefs_unlink,
    .rename     = emcorefs_rename,
    .truncate   = emcorefs_truncate,
    
    .ftruncate  = emcorefs_ftruncate,
};

int main(int argc, char **argv) {
    int32_t res, res2;
    uint8_t reattach = 0;

    res = usb_init();

    if (res == LIBUSB_SUCCESS) {
        res = usb_find(EMCORE_USB_VID, EMCORE_USB_PID, &reattach);
    }

    if (res == LIBUSB_SUCCESS) {
        res = emcorefs_init();
    }

    if (res == EMCORE_SUCCESS) {
#ifdef TEST_ONLY
        /* gcc complains about unused vars */
        (void)(argc);
        (void)(argv);

        res = emcore_test();
#else
        cache_init();

        res = fuse_main(argc, argv, &emcorefs_oper, NULL);

        cache_destroy();
#endif
    }

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
