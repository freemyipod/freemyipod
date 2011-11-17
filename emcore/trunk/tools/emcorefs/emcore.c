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


#include "global.h"

#include "emcore.h"
#include "util.h"
#include "usb.h"


struct emcore_usb_endpoints_addr emcore_usb_eps_addr;
struct emcore_usb_endpoints_max_packet_size emcore_usb_eps_mps;

int emcore_cout(const void* data, const uint32_t length)
{
    return usb_bulk_transfer(emcore_usb_eps_addr.cout, (void*)data, length);
}

int emcore_cin(void* data, const uint32_t length)
{
    return usb_bulk_transfer(emcore_usb_eps_addr.cin, data, length);
}

int emcore_dout(const void* data, const uint32_t length)
{
    return usb_bulk_transfer(emcore_usb_eps_addr.dout, (void*)data, length);
}

int emcore_din(void* data, const uint32_t length)
{
    return usb_bulk_transfer(emcore_usb_eps_addr.din, data, length);
}

int emcore_monitor_command(const void* out, void* in,
    const uint32_t send_length, const uint32_t receive_length)
{
    int res;
    uint32_t status;

#ifdef DEBUG_USB_PACKETS
    fprintf(stderr, "--------------------------------------------\n");
    fprintf(stderr, "Sending %d bytes...\n", send_length);

    dump_packet(out, send_length);

#endif
    res = emcore_cout(out, send_length);

    if (LIBUSB_SUCCESS != res)
    {
        return res;
    }

    if (in && receive_length)
    {
#ifdef DEBUG_USB_PACKETS
        fprintf(stderr, "Receiving %d bytes...\n", receive_length);

#endif
        res = emcore_cin(in, receive_length);

        if (LIBUSB_SUCCESS != res)
        {
            return res;
        }

#ifdef DEBUG_USB_PACKETS
        dump_packet(in, receive_length);

#endif
        status = *((int *)(in));
    }
    else
    {
        status = EMCORE_SUCCESS;
    }

#ifdef DEBUG_USB_PACKETS
    fprintf(stderr, "--------------------------------------------\n");

#endif
    switch (status)
    {
        case 0:
            return EMCORE_ERROR_INVALID;
        break;
        case 1:
            return EMCORE_SUCCESS;
        break;
        case 2:
            return EMCORE_ERROR_NOT_SUPPORTED;
        break;
        case 3:
            return EMCORE_ERROR_BUSY;
        break;
        default:
            return EMCORE_ERROR_INVALID;
        break;
    }
}

int emcore_get_version(struct emcore_dev_info* dev_info)
{
    int res;
    uint32_t out[4] = { 1, 0, 0, 0 }, in[4];

    res = emcore_monitor_command(out, in, 16, 16);

    if (EMCORE_SUCCESS != res)
    {
        return res;
    }

    memcpy(dev_info, &in[1], sizeof(*dev_info));

    return EMCORE_SUCCESS;
}

int emcore_get_packet_info(struct emcore_usb_endpoints_max_packet_size* max_packet_size)
{
    int res;
    uint32_t out[4] = { 1, 1, 0, 0 }, in[4];

    res = emcore_monitor_command(out, in, 16, 16);

    if (EMCORE_SUCCESS != res)
    {
        return res;
    }

    memcpy(max_packet_size, &in[1], sizeof(*max_packet_size));

    return EMCORE_SUCCESS;
}

int emcore_get_user_mem_range(struct emcore_user_mem_range* mem_range)
{
    int res;
    uint32_t out[4] = { 1, 2, 0, 0 }, in[4];

    res = emcore_monitor_command(out, in, 16, 16);

    if (EMCORE_SUCCESS != res)
    {
        return res;
    }

    memcpy(mem_range, &in[1], sizeof(*mem_range));

    return EMCORE_SUCCESS;
}

int emcore_reset(const uint8_t graceful)
{
    int res;
    uint32_t out[4] = { 2, 0xdeadbeef, 0, 0 }, in[4];

    out[1] = graceful;

    res = emcore_monitor_command(out, in, 16, graceful ? 16 : 0);

    if (EMCORE_SUCCESS != res)
    {
        return res;
    }

    return EMCORE_SUCCESS;
}

int emcore_poweroff(const uint8_t graceful)
{
    int res;
    uint32_t out[4] = { 3, 0xdeadbeef, 0, 0 }, in[4];

    out[1] = graceful;

    res = emcore_monitor_command(out, in, 16, graceful ? 16 : 0);

    if (EMCORE_SUCCESS != res)
    {
        return res;
    }

    return EMCORE_SUCCESS;
}

int emcore_readmem(void* data, const uint32_t addr, const uint32_t size)
{
    int res;
    uint32_t data_length, out[4] = { 4, 0xdeadbeef, 0xdeadbeef, 0 };
    void* in;

    out[1] = addr;
    out[2] = size;

    if (size + EMCORE_HEADER_SIZE > emcore_usb_eps_mps.cin)
    {
        return EMCORE_ERROR_OVERFLOW;
    }

    data_length = size + EMCORE_HEADER_SIZE;
    in = malloc(data_length);

    res = emcore_monitor_command(out, in, 16, data_length);

    if (EMCORE_SUCCESS != res)
    {
        return res;
    }

    memcpy(data, in + EMCORE_HEADER_SIZE, size);

    return EMCORE_SUCCESS;
}

int emcore_writemem(const void* data, const uint32_t addr, const uint32_t size)
{
    int res;
    uint32_t data_length, in[4], *out;

    if (size + EMCORE_HEADER_SIZE > emcore_usb_eps_mps.cout)
    {
        return EMCORE_ERROR_OVERFLOW;
    }

    data_length = size + EMCORE_HEADER_SIZE;
    out = malloc(data_length);

    *(out) = 5;
    *(out + 1) = addr;
    *(out + 2) = size;
    *(out + 3) = 0;
    memcpy(out + 4, data, size);

    res = emcore_monitor_command(out, in, data_length, 16);

    if (EMCORE_SUCCESS != res)
    {
        return res;
    }

    return EMCORE_SUCCESS;
}

int emcore_readdma(void* data, const uint32_t addr, const uint32_t size)
{
    int res;
    uint32_t out[4] = { 6, 0xdeadbeef, 0xdeadbeef, 0 }, in[4];

    out[1] = addr;
    out[2] = size;

    if (size > emcore_usb_eps_mps.din)
    {
        return EMCORE_ERROR_OVERFLOW;
    }

    res = emcore_monitor_command(out, in, 16, 16);

    if (EMCORE_SUCCESS != res)
    {
        return res;
    }

    res = emcore_din(data, size);

    return EMCORE_SUCCESS;
}

int emcore_writedma(const void* data, const uint32_t addr, const uint32_t size)
{
    int res;
    uint32_t out[4] = { 7, 0xdeadbeef, 0xdeadbeef, 0 }, in[4];

    out[1] = addr;
    out[2] = size;

    if (size > emcore_usb_eps_mps.dout)
    {
        return EMCORE_ERROR_OVERFLOW;
    }

    res = emcore_monitor_command(out, in, 16, 16);

    if (EMCORE_SUCCESS != res)
    {
        return res;
    }

    res = emcore_dout(data, size);

    return EMCORE_SUCCESS;
}

int emcore_readi2c(void* data, const uint8_t bus, const uint8_t slave, const uint8_t addr, const uint8_t size)
{
    int res;
    uint32_t data_length, out[4] = { 8, 0xdeadbeef, 0, 0 };
    void* in;

    out[1] = bus | (slave << 8) | (addr << 16) | (size << 24);

    if (size + EMCORE_HEADER_SIZE > emcore_usb_eps_mps.cin)
    {
        return EMCORE_ERROR_OVERFLOW;
    }

    data_length = 16 + size;
    in = malloc(data_length);

    res = emcore_monitor_command(out, in, 16, data_length);

    if (EMCORE_SUCCESS != res)
    {
        return res;
    }

    memcpy(data, in + 16, size);

    return EMCORE_SUCCESS;
}

int emcore_writei2c(const void* data, const uint8_t bus, const uint8_t slave, const uint8_t addr, const uint8_t size)
{
    int res;
    uint32_t data_length, in[4], *out;

    if (size + EMCORE_HEADER_SIZE > emcore_usb_eps_mps.cout)
    {
        return EMCORE_ERROR_OVERFLOW;
    }

    data_length = 16 + size;
    out = calloc(data_length, 1);

    *(out) = 9; // bytes 0-3
    *(out + 4) = bus;
    *(out + 5) = slave;
    *(out + 6) = addr;
    *(out + 7) = size;

    memcpy(out + 16, data, size); // bytes 16+

    res = emcore_monitor_command(out, in, data_length, 16);

    if (EMCORE_SUCCESS != res)
    {
        return res;
    }

    return EMCORE_SUCCESS;
}

int emcore_file_open(uint32_t* handle, const char* path, const int flags)
{
    int res;
    uint32_t str_length, data_length, in[4], *out;
    int flags_emcore = 0;
    
    *handle = 0;
    
    str_length = strlen(path);
    data_length = str_length + 1 + EMCORE_HEADER_SIZE;
    
    if (data_length > emcore_usb_eps_mps.cout)
    {
        return EMCORE_ERROR_OVERFLOW;
    }

    out = calloc(sizeof(char), data_length);

    *(out) = 30;
    
    /*
    #define O_RDONLY 0
    #define O_WRONLY 1
    #define O_RDWR   2
    #define O_CREAT  4
    #define O_APPEND 8
    #define O_TRUNC  0x10
    */
    
    if (flags & O_WRONLY) {
        flags_emcore |= 0x01;
    }
    
    if (flags & O_RDWR) {
        flags_emcore |= 0x02;
    }
    
    if (flags & O_CREAT) {
        flags_emcore |= 0x04;
    }
    
    if (flags & O_APPEND) {
        flags_emcore |= 0x08;
    }
    
    if (flags & O_TRUNC) {
        flags_emcore |= 0x10;
    }
    
    *(out + 1) = flags_emcore;
    
    strncpy(((char*)(out + 4)), path, str_length);

    res = emcore_monitor_command(out, in, data_length, 16);

    if (EMCORE_SUCCESS != res)
    {
        return res;
    }
    
    if (in[1] > 0x80000000)
    {
        return EMCORE_ERROR_IO;
    }
    
    *handle = in[1];

    return EMCORE_SUCCESS;
}

int emcore_file_size(uint32_t* size, const uint32_t handle)
{
    int res;
    uint32_t out[4] = { 31, 0xdeadbeef, 0, 0 }, in[4];
    
    out[1] = handle;

    res = emcore_monitor_command(out, in, 16, 16);

    if (EMCORE_SUCCESS != res)
    {
        return res;
    }
    
    if (in[1] > 0x80000000)
    {
        return EMCORE_ERROR_IO;
    }
    
    *size = in[1];
    
    return EMCORE_SUCCESS;
}

int emcore_file_read(uint32_t* nread, const uint32_t handle, const uint32_t addr, const uint32_t size)
{
    int res;
    uint32_t out[4] = { 32, 0xdeadbeef, 0xdeadbeef, 0xdeadbeef }, in[4];
    
    out[1] = handle;
    out[2] = addr;
    out[3] = size;

    res = emcore_monitor_command(out, in, 16, 16);

    if (EMCORE_SUCCESS != res)
    {
        return res;
    }
    
    if (in[1] > 0x80000000)
    {
        return EMCORE_ERROR_IO;
    }
    
    *nread = in[1];
    
    return EMCORE_SUCCESS;
}

int emcore_file_write(uint32_t* nwrite, const uint32_t handle, const uint32_t addr, const uint32_t size)
{
    int res;
    uint32_t out[4] = { 33, 0xdeadbeef, 0xdeadbeef, 0xdeadbeef }, in[4];
    
    out[1] = handle;
    out[2] = addr;
    out[3] = size;

    res = emcore_monitor_command(out, in, 16, 16);

    if (EMCORE_SUCCESS != res)
    {
        return res;
    }
    
    if (in[1] > 0x80000000)
    {
        return EMCORE_ERROR_IO;
    }
    
    *nwrite = in[1];
    
    return EMCORE_SUCCESS;
}

int emcore_file_seek(const uint32_t handle, const uint32_t offset, const uint32_t whence)
{
    int res;
    uint32_t out[4] = { 34, 0xdeadbeef, 0xdeadbeef, 0xdeadbeef }, in[4];
    
    out[1] = handle;
    out[2] = offset;
    out[3] = whence;

    res = emcore_monitor_command(out, in, 16, 16);

    if (EMCORE_SUCCESS != res)
    {
        return res;
    }
    
    if (in[1] > 0x80000000)
    {
        return EMCORE_ERROR_IO;
    }
    
    return EMCORE_SUCCESS;
}

int emcore_file_truncate(const uint32_t handle, const uint32_t length)
{
    int res;
    uint32_t out[4] = { 35, 0xdeadbeef, 0xdeadbeef, 0 }, in[4];
    
    out[1] = handle;
    out[2] = length;

    res = emcore_monitor_command(out, in, 16, 16);

    if (EMCORE_SUCCESS != res)
    {
        return res;
    }
    
    if (in[1] > 0x80000000)
    {
        return EMCORE_ERROR_IO;
    }
    
    return EMCORE_SUCCESS;
}

int emcore_file_sync(const uint32_t handle)
{
    int res;
    uint32_t out[4] = { 36, 0xdeadbeef, 0, 0 }, in[4];

    out[1] = handle;

    res = emcore_monitor_command(out, in, 16, 16);

    if (EMCORE_SUCCESS != res)
    {
        return res;
    }
    
    if (in[1] > 0x80000000)
    {
        return EMCORE_ERROR_IO;
    }
    
    return EMCORE_SUCCESS;
}

int emcore_file_close(const uint32_t handle)
{
    int res;
    uint32_t out[4] = { 37, 0xdeadbeef, 0, 0 }, in[4];

    out[1] = handle;

    res = emcore_monitor_command(out, in, 16, 16);

    if (EMCORE_SUCCESS != res)
    {
        return res;
    }
    
    if (in[1] > 0x80000000)
    {
        return EMCORE_ERROR_IO;
    }
    
    return EMCORE_SUCCESS;
}

int emcore_file_close_all(uint32_t* count)
{
    int res;
    uint32_t out[4] = { 38, 0, 0, 0 }, in[4];

    res = emcore_monitor_command(out, in, 16, 16);

    if (EMCORE_SUCCESS != res)
    {
        return res;
    }

    if (in[1] > 0x80000000)
    {
        return EMCORE_ERROR_IO;
    }
    
    *count = in[1];

    return EMCORE_SUCCESS;
}

int emcore_file_kill_all(const uint32_t volume)
{
    int res;
    uint32_t out[4] = { 39, 0xdeadbeef, 0, 0 }, in[4];

    out[1] = volume;

    res = emcore_monitor_command(out, in, 16, 16);

    if (EMCORE_SUCCESS != res)
    {
        return res;
    }
    
    if (in[1] > 0x80000000)
    {
        return EMCORE_ERROR_IO;
    }
    
    return EMCORE_SUCCESS;
}

int emcore_file_unlink(const char* name)
{
    int res;
    uint32_t str_length, data_length, in[4], *out;
    
    str_length = strlen(name);
    data_length = str_length + 1 + EMCORE_HEADER_SIZE;
    
    if (data_length > emcore_usb_eps_mps.cout)
    {
        return EMCORE_ERROR_OVERFLOW;
    }

    out = calloc(sizeof(char), data_length);

    *(out) = 40;
    
    strncpy(((char*)(out + 4)), name, str_length);

    res = emcore_monitor_command(out, in, data_length, 16);

    if (EMCORE_SUCCESS != res)
    {
        return res;
    }
    
    if (in[1] > 0x80000000)
    {
        return EMCORE_ERROR_IO;
    }
    
    return EMCORE_SUCCESS;
}

int emcore_file_rename(const char* path, const char* newpath)
{
    int res;
    uint32_t str_length, data_length, in[4], *out;
    
    if (strlen(path) > 247)
    {
        return EMCORE_ERROR_OVERFLOW;
    }
    
    str_length = MIN(247, strlen(newpath));
    data_length = str_length + 1 + 248 + EMCORE_HEADER_SIZE;
    
    if (data_length > emcore_usb_eps_mps.cout)
    {
        return EMCORE_ERROR_OVERFLOW;
    }

    out = calloc(sizeof(char), data_length);

    *(out) = 41;
    
    strncpy(((char*)(out + 4)), path, strlen(path));
    strncpy(((char*)(out) + 248), newpath, str_length);

    res = emcore_monitor_command(out, in, data_length, 16);

    if (EMCORE_SUCCESS != res)
    {
        return res;
    }
    
    if (in[1] > 0x80000000)
    {
        return EMCORE_ERROR_IO;
    }
    
    return EMCORE_SUCCESS;
}

int emcore_dir_open(uint32_t* handle, const char* name)
{
    int res;
    size_t name_length;
    uint32_t data_length, in[4], *out;

    name_length = strlen(name);
    data_length = name_length + 1 + EMCORE_HEADER_SIZE;
    out = calloc(data_length, 1);

    *out = 42;
    strncpy(((char*)(out + 4)), name, name_length + 1);

    res = emcore_monitor_command(out, in, data_length, 16);

    if (EMCORE_SUCCESS != res)
    {
        return res;
    }

    *handle = in[1];

    return EMCORE_SUCCESS;
}

int emcore_dir_read(struct emcore_dir_entry* entry, const uint32_t handle)
{
    int res;
    uint32_t maxpath, ptr, dirent_size, emcore_errno_value, filename_buf_len,
        out[4] = { 43, 0xdeadbeef, 0, 0 }, in[4];
    void *buf;

    memset(entry, 0, sizeof(*entry));

    out[1] = handle;

    res = emcore_monitor_command(out, in, 16, 16);

    if (EMCORE_SUCCESS != res)
    {
        return res;
    }

    ptr = in[3];

    if (0 == ptr)
    {
        res = emcore_errno(&emcore_errno_value);

        if (EMCORE_SUCCESS != res)
        {
            return res;
        }
        
        if (EMCORE_SUCCESS != emcore_errno_value && 2 != emcore_errno_value)
        {
            return EMCORE_ERROR_IO;
        }

        return EMCORE_ERROR_NO_MORE_ENTRIES;
    }

    if (1 != in[1]) // version
    {
        return EMCORE_ERROR_NOT_IMPLEMENTED;
    }

    maxpath = in[2];

    dirent_size = maxpath + 16;

    buf = malloc(dirent_size);

    res = emcore_read(buf, ptr, dirent_size);

    if (EMCORE_SUCCESS != res)
    {
        return res;
    }

    filename_buf_len = strlen((char*)buf) + 1;

    entry->name = malloc(filename_buf_len);
    strncpy(entry->name, buf, filename_buf_len);
    memcpy(&entry->attributes, buf + maxpath, 16);

#ifdef DEBUG_DIR_ENTRIES
    fprintf(stderr, "Read directory entry: %s\n", entry->name);
    fprintf(stderr, "Attributes: 0x%08x\n", entry->attributes);
    fprintf(stderr, "Size: %d\n", entry->size);
    fprintf(stderr, "Start cluster: %d\n", entry->startcluster);
    fprintf(stderr, "Last written date: 0x%04x\n", entry->wrtdate);
    fprintf(stderr, "Last written time: 0x%04x\n", entry->wrttime);
    fprintf(stderr, "Last written TS: %lu\n",
        (unsigned long) fat_time_to_unix_ts(entry->wrttime, entry->wrtdate));
#endif
    return EMCORE_SUCCESS;
}

int emcore_dir_close(const uint32_t handle)
{
    int res;
    uint32_t out[4] = { 44, 0xdeadbeef, 0, 0 }, in[4];

    out[1] = handle;

    res = emcore_monitor_command(out, in, 16, 16);

    if (EMCORE_SUCCESS != res)
    {
        return res;
    }
    
    if (in[1] > 0x80000000)
    {
        return EMCORE_ERROR_IO;
    }
    
    return EMCORE_SUCCESS;
}

int emcore_dir_close_all(uint32_t* count)
{
    int res;
    uint32_t out[4] = { 45, 0, 0, 0 }, in[4];

    res = emcore_monitor_command(out, in, 16, 16);

    if (EMCORE_SUCCESS != res)
    {
        return res;
    }
    
    if (in[1] > 0x80000000)
    {
        return EMCORE_ERROR_IO;
    }
    
    *count = in[1];
    
    return EMCORE_SUCCESS;
}

int emcore_dir_create(const char* name)
{
    int res;
    uint32_t str_length, data_length, in[4], *out;
    
    str_length = strlen(name);
    data_length = str_length + 1 + EMCORE_HEADER_SIZE;
    
    if (data_length > emcore_usb_eps_mps.cout)
    {
        return EMCORE_ERROR_OVERFLOW;
    }

    out = calloc(sizeof(char), data_length);

    *(out) = 47;
    
    strncpy(((char*)(out + 4)), name, str_length);

    res = emcore_monitor_command(out, in, data_length, 16);

    if (EMCORE_SUCCESS != res)
    {
        return res;
    }
    
    if (in[1] > 0x80000000)
    {
        return EMCORE_ERROR_IO;
    }
    
    return EMCORE_SUCCESS;
}

int emcore_dir_remove(const char* name)
{
    int res;
    uint32_t str_length, data_length, in[4], *out;
    
    str_length = strlen(name);
    data_length = str_length + 1 + EMCORE_HEADER_SIZE;
    
    if (data_length > emcore_usb_eps_mps.cout)
    {
        return EMCORE_ERROR_OVERFLOW;
    }

    out = calloc(sizeof(char), data_length);

    *(out) = 48;
    
    strncpy(((char*)(out + 4)), name, str_length);

    res = emcore_monitor_command(out, in, data_length, 16);

    if (EMCORE_SUCCESS != res)
    {
        return res;
    }
    
    if (in[1] > 0x80000000)
    {
        return EMCORE_ERROR_IO;
    }
    
    return EMCORE_SUCCESS;
}

int emcore_errno(uint32_t* emcore_errno_value)
{
    int res;
    uint32_t out[4] = { 49, 0, 0, 0 }, in[4];

    res = emcore_monitor_command(out, in, 16, 16);

    if (EMCORE_SUCCESS != res)
    {
        return res;
    }

    *emcore_errno_value = in[1];

    return EMCORE_SUCCESS;
}

int emcore_malloc(uint32_t* ptr, const uint32_t size)
{
    int res;
    uint32_t out[4] = { 52, 0xdeadbeef, 0, 0 }, in[4];

    out[1] = size;

    res = emcore_monitor_command(out, in, 16, 16);

    if (EMCORE_SUCCESS != res)
    {
        return res;
    }

    *ptr = in[1];

    return EMCORE_SUCCESS;
}

int emcore_memalign(uint32_t* ptr, const uint32_t align, const uint32_t size)
{
    int res;
    uint32_t out[4] = { 53, 0xdeadbeef, 0xdeadbeef, 0 }, in[4];

    out[1] = align;
    out[2] = size;

    res = emcore_monitor_command(out, in, 16, 16);

    if (EMCORE_SUCCESS != res)
    {
        return res;
    }

    *ptr = in[1];

    return EMCORE_SUCCESS;
}

int emcore_realloc(uint32_t* new_ptr, const uint32_t ptr, const uint32_t size)
{
    int res;
    uint32_t out[4] = { 54, 0xdeadbeef, 0xdeadbeef, 0 }, in[4];

    out[1] = ptr;
    out[2] = size;

    res = emcore_monitor_command(out, in, 16, 16);

    if (EMCORE_SUCCESS != res)
    {
        return res;
    }

    *new_ptr = in[1];

    return EMCORE_SUCCESS;
}

int emcore_reownalloc(const uint32_t ptr, const uint32_t owner)
{
    uint32_t out[4] = { 55, 0xdeadbeef, 0xdeadbeef, 0 }, in[4];

    out[1] = ptr;
    out[2] = owner;

    return emcore_monitor_command(out, in, 16, 16);
}

int emcore_free(const uint32_t ptr)
{
    uint32_t out[4] = { 56, 0xdeadbeef, 0, 0 }, in[4];

    out[1] = ptr;

    return emcore_monitor_command(out, in, 16, 16);
}

int emcore_free_all(void)
{
    uint32_t out[4] = { 57, 0, 0, 0 }, in[4];

    return emcore_monitor_command(out, in, 16, 16);
}

int emcore_read(void* data, const uint32_t addr, const uint32_t size)
{
    int res;
    struct alignsizes* sizes;
    uint32_t cin_maxsize, readsize, curraddr;

    cin_maxsize = emcore_usb_eps_mps.cin - EMCORE_HEADER_SIZE;
    sizes = malloc(sizeof(*sizes));

    alignsplit(sizes, addr, size, cin_maxsize, 16);
#ifdef DEBUG_ALIGN_SPLIT
    fprintf(stderr, "Downloading %d bytes from 0x%08x, split as (%d/%d/%d)\n",
        size, addr, sizes->head, sizes->body, sizes->tail);

#endif
    curraddr = addr;

    if (sizes->head > 0)
    {
        res = emcore_readmem(data, curraddr, sizes->head);

        if (EMCORE_SUCCESS != res)
        {
            return res;
        }

        data += sizes->head;
        curraddr += sizes->head;
    }

    while (sizes->body > 0)
    {
        if (sizes->body >= cin_maxsize * 2)
        {
            readsize = MIN(sizes->body, emcore_usb_eps_mps.din);
            res = emcore_readdma(data, curraddr, readsize);
        }
        else
        {
            readsize = MIN(sizes->body, cin_maxsize);
            res = emcore_readmem(data, curraddr, readsize);
        }

        if (EMCORE_SUCCESS != res)
        {
            return res;
        }

        data += readsize;
        curraddr += readsize;
        sizes->body -= readsize;
    }

    if (sizes->tail > 0)
    {
        res = emcore_readmem(data, curraddr, sizes->tail);

        if (EMCORE_SUCCESS != res)
        {
            return res;
        }

        data += sizes->tail;
    }

    return EMCORE_SUCCESS;
}

int emcore_write(const void* data, const uint32_t addr, const uint32_t size)
{
    int res;
    struct alignsizes* sizes;
    uint32_t cout_maxsize, writesize, curraddr;

    cout_maxsize = emcore_usb_eps_mps.cout - 16;
    sizes = malloc(sizeof(*sizes));

    alignsplit(sizes, addr, size, cout_maxsize, 16);
#ifdef DEBUG_ALIGN_SPLIT
    fprintf(stderr, "Uploading %d bytes from 0x%08x, split as (%d/%d/%d)\n",
        size, addr, sizes->head, sizes->body, sizes->tail);

#endif
    curraddr = addr;

    if (sizes->head > 0)
    {
        res = emcore_writemem(data, curraddr, sizes->head);

        if (EMCORE_SUCCESS != res)
        {
            return res;
        }

        data += sizes->head;
        curraddr += sizes->head;
    }

    while (sizes->body > 0)
    {
        if (sizes->body >= 2 * cout_maxsize)
        {
            writesize = MIN(sizes->body, emcore_usb_eps_mps.dout);
            res = emcore_writedma(data, curraddr, writesize);
        }
        else
        {
            writesize = MIN(sizes->body, cout_maxsize);
            res = emcore_writemem(data, curraddr, writesize);
        }

        if (EMCORE_SUCCESS != res)
        {
            return res;
        }

        data += writesize;
        curraddr += writesize;
        sizes->body -= writesize;
    }

    if (sizes->tail > 0)
    {
        res = emcore_writemem(data, curraddr, sizes->tail);

        if (EMCORE_SUCCESS != res)
        {
            return res;
        }

        data += sizes->tail;
    }

    return EMCORE_SUCCESS;
}

int emcore_ls(const uint32_t handle)
{
    int res = 0;
    struct emcore_dir_entry entry;

    while (1)
    {
        res = emcore_dir_read(&entry, handle);

        if (EMCORE_ERROR_NO_MORE_ENTRIES == res)
        {
            res = EMCORE_SUCCESS;
            break;
        }

        if (EMCORE_SUCCESS != res)
        {
            return res;
        }

#ifdef DEBUG_DIR_ENTRIES
        fprintf(stderr, "Read directory entry:\n");
        fprintf(stderr, "Name: %s\n", entry.name);
        fprintf(stderr, "Attributes: 0x%08x\n", entry.attributes);
        fprintf(stderr, "Size: %d\n", entry.size);
        fprintf(stderr, "Start cluster: %d\n", entry.startcluster);
        fprintf(stderr, "Last written date: 0x%04x\n", entry.wrtdate);
        fprintf(stderr, "Last written time: 0x%04x\n", entry.wrttime);
        fprintf(stderr, "Last written TS: %lu\n",
            (unsigned long) fat_time_to_unix_ts(entry.wrttime, entry.wrtdate));
#endif
        if (entry.attributes & 0x10)
        {
            printf("     [DIR]");
        }
        else
        {
            printf("%10d", entry.size);
        }

        printf(" %s\n", entry.name);
    }

    return res;
}

int emcore_test(void)
{
    int res;

    /* emcore_get_version */
    struct emcore_dev_info dev_info;
    char *hw_type;

    /* emcore_get_packet_info */
    struct emcore_usb_endpoints_max_packet_size max_packet_size;

    /* emcore_get_user_mem_range */
    struct emcore_user_mem_range mem_range;

    /* emcore_readmem */
    void* buf;
    uint16_t buf_size;
    uint32_t read_addr;

    /* emcore_readi2c */
    /* uint8_t i2cdata; */

    /* emcore_dir_open */
    uint32_t dir_handle;

    /* emcore_dir_close_all */
    uint32_t count;

    res = emcore_get_version(&dev_info);

    if (EMCORE_SUCCESS != res)
    {
        return res;
    }

    hw_type = malloc(5);

    strncpy(hw_type, ((char*)&dev_info.hw_type), 4);

    printf("Connected to %4s running %s v%d.%d.%d r%d\n",
        hw_type, (1 == dev_info.sw_type ? "emBIOS" : (2 == dev_info.sw_type ? "emCORE" : "UNKNOWN")),
        dev_info.major, dev_info.minor, dev_info.patch, dev_info.svn_revision
    );

    free(hw_type);

    res = emcore_get_packet_info(&max_packet_size);

    if (EMCORE_SUCCESS != res)
    {
        return res;
    }

    printf("COUT max pckt: %d, CIN max pckt: %d, DOUT max pckt: %d, DIN max pckt: %d\n",
        max_packet_size.cout, max_packet_size.cin, max_packet_size.dout, max_packet_size.din
    );

    res = emcore_get_user_mem_range(&mem_range);

    if (EMCORE_SUCCESS != res)
    {
        return res;
    }

    printf("User mem range: 0x%08x - 0x%08x\n", mem_range.lower, mem_range.upper);

    read_addr = 0x09000000;
    buf_size = 0x1000;
    buf = malloc(buf_size);

    printf("Reading 0x%08x bytes from 0x%08x\n", buf_size, read_addr);

    res = emcore_read(buf, read_addr, buf_size);

    if (EMCORE_SUCCESS != res)
    {
        return res;
    }

#ifdef DEBUG
    dump_packet(buf, buf_size);

#endif
    printf("Writing 0x%08x bytes to 0x%08x\n", buf_size, read_addr);

    res = emcore_write(buf, read_addr, buf_size);

    if (EMCORE_SUCCESS != res)
    {
        return res;
    }

    free(buf);

    /*
    printf("Reading 1 byte from I2C\n");

    res = emcore_readi2c(&i2cdata, 0, 0xe6, 0x29, 1);

    if (EMCORE_SUCCESS != res)
    {
        return res;
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

    if (EMCORE_SUCCESS != res)
    {
        return res;
    }

    sleep(1);

    i2cdata = 0;

    printf("Writing 1 byte to I2C\n");

    res = emcore_writei2c(&i2cdata, 0, 0xe6, 0x29, 1);

    if (EMCORE_SUCCESS != res)
    {
        return res;
    }
    */
    res = emcore_dir_open(&dir_handle, "/");

    if (EMCORE_SUCCESS != res)
    {
        return res;
    }

    printf("Opened dir handle: 0x%08x\n", dir_handle);

    res = emcore_ls(dir_handle);

    if (EMCORE_SUCCESS != res)
    {
        return res;
    }

    printf("Listed dir handle: 0x%08x\n", dir_handle);

    res = emcore_dir_close(dir_handle);

    if (EMCORE_SUCCESS != res)
    {
        return res;
    }

    printf("Closed dir handle 0x%08x\n", dir_handle);

    res = emcore_dir_close_all(&count);

    if (EMCORE_SUCCESS != res)
    {
        return res;
    }

    printf("Closed %d dir handles\n", count);
    /* powers off the device - graceful
    res = emcore_poweroff(1);

    if (EMCORE_SUCCESS != res)
    {
        return res;
    }

    printf("Device powered off successfully!\n");
    */
    return EMCORE_SUCCESS;
}
