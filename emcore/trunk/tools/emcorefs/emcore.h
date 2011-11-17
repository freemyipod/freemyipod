//
//
//    Copyright 2011 user890104
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

#define EMCORE_HEADER_SIZE 0x10


struct emcore_usb_endpoints_addr
{
    uint8_t cout;
    uint8_t cin;
    uint8_t dout;
    uint8_t din;
};

struct emcore_usb_endpoints_max_packet_size
{
    uint16_t cout;
    uint16_t cin;
    uint32_t dout;
    uint32_t din;
};

struct emcore_dev_info
{
    uint32_t svn_revision;
    uint8_t major, minor, patch, sw_type;
    uint32_t hw_type;
};

struct emcore_user_mem_range
{
    uint32_t lower;
    uint32_t upper;
};

struct emcore_dir_entry
{
    char* name;
    uint32_t attributes;
    uint32_t size;
    uint32_t startcluster;
    uint16_t wrtdate;
    uint16_t wrttime;
};

enum emcore_error
{
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

int emcore_cout(const void* data, const uint32_t length);
int emcore_cin(void* data, const uint32_t length);
int emcore_dout(const void* data, const uint32_t length);
int emcore_din(void* data, const uint32_t length);

int emcore_monitor_command(const void* out, void* in, const uint32_t send_length, const uint32_t receive_length);

int emcore_get_version(struct emcore_dev_info* dev_info);
int emcore_get_packet_info(struct emcore_usb_endpoints_max_packet_size* max_packet_size);
int emcore_get_user_mem_range(struct emcore_user_mem_range* mem_range);
int emcore_reset(const uint8_t graceful);
int emcore_poweroff(const uint8_t graceful);
int emcore_readmem(void* data, const uint32_t addr, const uint32_t size);
int emcore_writemem(const void* data, const uint32_t addr, const uint32_t size);
int emcore_readdma(void* data, const uint32_t addr, const uint32_t size);
int emcore_writedma(const void* data, const uint32_t addr, const uint32_t size);
int emcore_readi2c(void* data, const uint8_t bus, const uint8_t slave, const uint8_t addr, const uint8_t size);
int emcore_writei2c(const void* data, const uint8_t bus, const uint8_t slave, const uint8_t addr, const uint8_t size);
int emcore_file_open(uint32_t* handle, const char* pathname, const int flags);
int emcore_file_size(uint32_t* size, const uint32_t handle);
int emcore_file_read(uint32_t* nread, const uint32_t handle, const uint32_t addr, const uint32_t size);
int emcore_file_write(const uint32_t handle, const uint32_t addr, const uint32_t size);
int emcore_file_seek(const uint32_t handle, const uint32_t offset, const uint32_t whence);
int emcore_file_truncate(const uint32_t handle, const uint32_t length);
int emcore_file_sync(const uint32_t handle);
int emcore_file_close(const uint32_t handle);
int emcore_file_close_all(uint32_t* count);
int emcore_file_kill_all(const uint32_t volume);
int emcore_file_unlink(const char* name);
int emcore_file_rename(const char* path, const char* newpath);
int emcore_dir_open(uint32_t* handle, const char* name);
int emcore_dir_read(struct emcore_dir_entry* entry, const uint32_t handle);
int emcore_dir_close(const uint32_t handle);
int emcore_dir_close_all(uint32_t* count);
int emcore_dir_create(const char* name);
int emcore_dir_remove(const char* name);
int emcore_errno(uint32_t* emcore_errno_value);
int emcore_malloc(uint32_t* ptr, const uint32_t size);
int emcore_memalign(uint32_t* ptr, const uint32_t align, const uint32_t size);
int emcore_realloc(uint32_t* new_ptr, const uint32_t ptr, const uint32_t size);
int emcore_reownalloc(const uint32_t ptr, const uint32_t owner);
int emcore_free(const uint32_t ptr);
int emcore_free_all(void);

int emcore_read(void* data, const uint32_t addr, const uint32_t size);
int emcore_write(const void* data, const uint32_t addr, const uint32_t size);
int emcore_ls(const uint32_t handle);
int emcore_test(void);

#endif /* __EMCORE_H__ */
