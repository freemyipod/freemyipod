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

int emcore_cout(const void* data, uint32_t length);
int emcore_cin(void* data, uint32_t length);
int emcore_dout(const void* data, uint32_t length);
int emcore_din(void* data, uint32_t length);

int emcore_monitor_command(const void* out, void* in, uint32_t send_length, uint32_t receive_length);

int emcore_get_version(struct emcore_dev_info* dev_info);
int emcore_get_packet_info(struct emcore_usb_endpoints_max_packet_size* max_packet_size);
int emcore_get_user_mem_range(struct emcore_user_mem_range* mem_range);
int emcore_reset(uint8_t graceful);
int emcore_poweroff(uint8_t graceful);
int emcore_readmem(void* data, uint32_t addr, uint32_t size);
int emcore_writemem(const void* data, uint32_t addr, uint32_t size);
int emcore_readdma(void* data, uint32_t addr, uint32_t size);
int emcore_writedma(const void* data, uint32_t addr, uint32_t size);
int emcore_readi2c(void* data, uint8_t bus, uint8_t slave, uint8_t addr, uint8_t size);
int emcore_writei2c(const void* data, uint8_t bus, uint8_t slave, uint8_t addr, uint8_t size);
int emcore_file_open(uint32_t* handle, const char* pathname, int flags);
int emcore_file_size(uint32_t* size, uint32_t handle);
int emcore_file_read(uint32_t* nread, uint32_t handle, uint32_t addr, uint32_t size);
int emcore_file_write(uint32_t* nwrite, uint32_t handle, uint32_t addr, uint32_t size);
int emcore_file_seek(uint32_t handle, uint32_t offset, uint32_t whence);
int emcore_file_truncate(uint32_t handle, uint32_t length);
int emcore_file_sync(uint32_t handle);
int emcore_file_close(uint32_t handle);
int emcore_file_close_all(uint32_t* count);
int emcore_file_kill_all(uint32_t volume);
int emcore_file_unlink(const char* name);
int emcore_file_rename(const char* path, const char* newpath);
int emcore_dir_open(uint32_t* handle, const char* name);
int emcore_dir_read(struct emcore_dir_entry* entry, uint32_t handle);
int emcore_dir_close(uint32_t handle);
int emcore_dir_close_all(uint32_t* count);
int emcore_dir_create(const char* name);
int emcore_dir_remove(const char* name);
int emcore_errno(uint32_t* emcore_errno_value);
int emcore_malloc(uint32_t* ptr, uint32_t size);
int emcore_memalign(uint32_t* ptr, uint32_t align, uint32_t size);
int emcore_realloc(uint32_t* new_ptr, uint32_t ptr, uint32_t size);
int emcore_reownalloc(uint32_t ptr, uint32_t owner);
int emcore_free(uint32_t ptr);
int emcore_free_all(void);

int emcore_read(void* data, uint32_t addr, uint32_t size);
int emcore_write(const void* data, uint32_t addr, uint32_t size);
int emcore_ls(uint32_t handle);
int emcore_test(void);

#endif /* __EMCORE_H__ */
