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

#ifndef __EMCORE_H__
#define __EMCORE_H__

#include "global.h"

#define EMCORE_USB_VID 0xFFFF
#define EMCORE_USB_PID 0xE000

#define EMCORE_USB_INTERFACE_CLASS 0xFF
#define EMCORE_USB_INTERFACE_SUB_CLASS 0x00
#define EMCORE_USB_INTERFACE_PROTOCOL 0x00

#define EMCORE_HEADER_SIZE 0x10

struct emcore_dev_info {
    uint32_t svn_revision;
    uint8_t major, minor, patch, sw_type;
    uint32_t hw_type;
} __attribute__ ((packed));

struct emcore_malloc_pool_bounds {
    uint32_t lower;
    uint32_t upper;
} __attribute__ ((packed));

struct emcore_dir_entry {
    char *name;
    uint32_t attributes;
    uint32_t size;
    uint32_t startcluster;
    uint16_t wrtdate;
    uint16_t wrttime;
} __attribute__ ((packed));

enum emcore_error {
    EMCORE_SUCCESS,
    EMCORE_ERROR_INVALID,
    EMCORE_ERROR_NOT_SUPPORTED,
    EMCORE_ERROR_BUSY,
    EMCORE_ERROR_NO_DEVICE,
    EMCORE_ERROR_INCOMPLETE,
    EMCORE_ERROR_OVERFLOW,
    EMCORE_ERROR_NOT_IMPLEMENTED,
    EMCORE_ERROR_NO_MORE_ENTRIES,
    EMCORE_ERROR_IO,
};

int32_t emcore_send(const void *data, uint32_t length);
int32_t emcore_receive(void *data, uint32_t length);

int32_t emcore_monitor_command(const void *out, void *in, uint32_t send_length, uint32_t receive_length);

int32_t emcore_get_version(struct emcore_dev_info *dev_info);
int32_t emcore_get_malloc_pool_bounds(struct emcore_malloc_pool_bounds *bounds);
int32_t emcore_reset(uint8_t graceful);
int32_t emcore_poweroff(uint8_t graceful);
int32_t emcore_read(void *data, uint32_t addr, uint32_t size);
int32_t emcore_write(const void *data, uint32_t addr, uint32_t size);
int32_t emcore_readi2c(void *data, uint8_t bus, uint8_t slave, uint8_t addr, uint8_t size);
int32_t emcore_writei2c(const void *data, uint8_t bus, uint8_t slave, uint8_t addr, uint8_t size);
int32_t emcore_file_open(uint32_t *handle, const char *path, uint32_t flags);
int32_t emcore_file_size(uint32_t *size, uint32_t handle);
int32_t emcore_file_read(uint32_t *nread, uint32_t handle, uint32_t addr, uint32_t size);
int32_t emcore_file_write(uint32_t *nwrite, uint32_t handle, uint32_t addr, uint32_t size);
int32_t emcore_file_seek(uint32_t handle, uint32_t offset, uint32_t whence);
int32_t emcore_file_truncate(uint32_t handle, uint32_t length);
int32_t emcore_file_sync(uint32_t handle);
int32_t emcore_file_close(uint32_t handle);
int32_t emcore_file_close_all(uint32_t *count);
int32_t emcore_file_kill_all(uint32_t volume);
int32_t emcore_file_unlink(const char *path);
int32_t emcore_file_rename(const char *path, const char *newpath);
int32_t emcore_dir_open(uint32_t *handle, const char *name);
int32_t emcore_dir_read(struct emcore_dir_entry *entry, uint32_t handle);
int32_t emcore_dir_close(uint32_t handle);
int32_t emcore_dir_close_all(uint32_t *count);
int32_t emcore_dir_create(const char *name);
int32_t emcore_dir_remove(const char *name);
int32_t emcore_errno(uint32_t *emcore_errno_value);
int32_t emcore_malloc(uint32_t *ptr, uint32_t size);
int32_t emcore_memalign(uint32_t *ptr, uint32_t align, uint32_t size);
int32_t emcore_realloc(uint32_t *new_ptr, uint32_t ptr, uint32_t size);
int32_t emcore_reownalloc(uint32_t ptr, uint32_t owner);
int32_t emcore_free(uint32_t ptr);
int32_t emcore_free_all(void);

int32_t emcore_ls(uint32_t handle);
int32_t emcore_test(void);

#endif /* __EMCORE_H__ */
