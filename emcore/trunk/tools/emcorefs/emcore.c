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

#include "emcore.h"
#include "util.h"
#include "usb.h"

int32_t emcore_send(const void *data, uint32_t length) {
    return usb_control_transfer(0x41, (void *)data, length);
}

int32_t emcore_receive(void *data, uint32_t length) {
    return usb_control_transfer(0xc1, data, length);
}

int32_t emcore_monitor_command(const void *out, void *in,
    uint32_t send_length, uint32_t receive_length) {
    int32_t res;
    uint32_t status;

#ifdef DEBUG_USB_PACKETS
    fprintf(stderr, "--------------------------------------------\n");
    fprintf(stderr, "Sending %d bytes...\n", send_length);

    dump_packet(out, send_length);

#endif
    res = emcore_send(out, send_length);

    if (res != LIBUSB_SUCCESS) {
        return res;
    }

    if (in && receive_length) {
#ifdef DEBUG_USB_PACKETS
        fprintf(stderr, "Receiving %d bytes...\n", receive_length);

#endif
        res = emcore_receive(in, receive_length);

        if (res != LIBUSB_SUCCESS) {
            return res;
        }

#ifdef DEBUG_USB_PACKETS
        dump_packet(in, receive_length);

#endif
        status = *((int32_t *)(in));
    }
    else {
        status = EMCORE_SUCCESS;
    }

#ifdef DEBUG_USB_PACKETS
    fprintf(stderr, "--------------------------------------------\n");

#endif
    switch (status) {
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

int32_t emcore_get_version(struct emcore_dev_info *dev_info) {
    int32_t res;
    uint32_t out[4] = { 1, 0, 0, 0 }, in[4];

    res = emcore_monitor_command(out, in, 16, 16);

    if (res != EMCORE_SUCCESS) {
        return res;
    }

    memcpy(dev_info, &in[1], sizeof(*dev_info));

    return EMCORE_SUCCESS;
}

int32_t emcore_get_malloc_pool_bounds(struct emcore_malloc_pool_bounds *bounds) {
    int32_t res;
    uint32_t out[4] = { 1, 1, 0, 0 }, in[4];

    res = emcore_monitor_command(out, in, 16, 16);

    if (res != EMCORE_SUCCESS) {
        return res;
    }

    memcpy(bounds, &in[1], sizeof(*bounds));

    return EMCORE_SUCCESS;
}

int32_t emcore_reset(uint8_t graceful) {
    int32_t res;
    uint32_t out[4] = { 2, 0xdeadbeef, 0, 0 };

    out[1] = graceful;

    res = emcore_monitor_command(out, NULL, 16, 0);

    if (res != EMCORE_SUCCESS) {
        return res;
    }

    return EMCORE_SUCCESS;
}

int32_t emcore_poweroff(uint8_t graceful) {
    int32_t res;
    uint32_t out[4] = { 3, 0xdeadbeef, 0, 0 };

    out[1] = graceful;

    res = emcore_monitor_command(out, NULL, 16, 0);

    if (res != EMCORE_SUCCESS) {
        return res;
    }

    return EMCORE_SUCCESS;
}

int32_t emcore_read(void *data, uint32_t addr, uint32_t size) {
    int32_t res;
    uint32_t readsize, out[4] = { 4, 0xdeadbeef, 0xdeadbeef, 0 };
    void *in;
    
    in = malloc(EMCORE_HEADER_SIZE + 0x0F00);
    
    while (size) {
        readsize = MIN(size, 0x0F00);
        
        out[1] = addr;
        out[2] = readsize;
        
        res = emcore_monitor_command(out, in, 16, readsize + EMCORE_HEADER_SIZE);
        
        if (res != EMCORE_SUCCESS) {
            free(in);
            return res;
        }
        
        memcpy(data, in + EMCORE_HEADER_SIZE, readsize);
        
        data += readsize;
        addr += readsize;
        size -= readsize;
    }

    free(in);
    
    return EMCORE_SUCCESS;
}

int32_t emcore_write(const void *data, uint32_t addr, uint32_t size) {
    int32_t res;
    uint32_t writesize, in[4], *out;
    
    out = calloc(sizeof(char), EMCORE_HEADER_SIZE + 0x0F00);

    *(out) = 5;
    
    while (size) {
        writesize = MIN(size, 0x0F00);
        
        memcpy(out + 4, data, writesize);

        *(out + 1) = addr;
        *(out + 2) = writesize;
        
        res = emcore_monitor_command(out, in, writesize + EMCORE_HEADER_SIZE, 16);

        if (res != EMCORE_SUCCESS) {
            free(out);
            return res;
        }
        
        data += writesize;
        addr += writesize;
        size -= writesize;
    }
    
    free(out);

    return EMCORE_SUCCESS;
}

int32_t emcore_readi2c(void *data, uint8_t bus, uint8_t slave, uint8_t addr, uint8_t size) {
    int32_t res;
    uint32_t data_length, out[4] = { 8, 0xdeadbeef, 0, 0 };
    void *in;

    if (!size) {
        return EMCORE_SUCCESS;
    }

    out[1] = bus | (slave << 8) | (addr << 16) | (size << 24);

    data_length = EMCORE_HEADER_SIZE + size;
    in = malloc(data_length);

    res = emcore_monitor_command(out, in, 16, data_length);

    if (res != EMCORE_SUCCESS) {
        free(in);

        return res;
    }

    memcpy(data, in + EMCORE_HEADER_SIZE, size);

    free(in);

    return EMCORE_SUCCESS;
}

int32_t emcore_writei2c(const void *data, uint8_t bus, uint8_t slave, uint8_t addr, uint8_t size) {
    int32_t res;
    uint32_t data_length, in[4], *out;

    if (!size) {
        return EMCORE_SUCCESS;
    }

    if (size > 48) {
        return EMCORE_ERROR_OVERFLOW;
    }
    
    data_length = size + EMCORE_HEADER_SIZE;
    out = calloc(data_length, 1);

    *(out) = 9;
    out[1] = bus | (slave << 8) | (addr << 16) | (size << 24);

    memcpy(out + 4, data, size);

    res = emcore_monitor_command(out, in, data_length, 16);

    if (res != EMCORE_SUCCESS) {
        return res;
    }

    return EMCORE_SUCCESS;
}

int32_t emcore_file_open(uint32_t *handle, const char *path, uint32_t flags) {
    int32_t res;
    uint32_t str_length, data_length, in[4], *out;
    int32_t flags_emcore = 0;
    
    *handle = 0;
    
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
    
    str_length = strlen(path) + 1;
    
    if (str_length > 48) {
        uint32_t buf = 0;
        
        res = emcore_malloc(&buf, str_length);
        
        if (res != EMCORE_SUCCESS) {
            return res;
        }
        
        res = emcore_write(path, buf, str_length);
        
        if (res != EMCORE_SUCCESS) {
            return res;
        }
        
        out = calloc(sizeof(char), EMCORE_HEADER_SIZE);

        *(out) = 30;
        *(out + 1) = flags_emcore;
        *(out + 3) = buf;
        
        res = emcore_monitor_command(out, in, EMCORE_HEADER_SIZE, 16);
    }
    else {
        data_length = EMCORE_HEADER_SIZE + str_length;
        
        out = calloc(sizeof(char), data_length);

        *(out) = 30;
        *(out + 1) = flags_emcore;
        
        strncpy(((char *)(out + 4)), path, str_length - 1);
        
        res = emcore_monitor_command(out, in, data_length, 16);
    }

    if (res != EMCORE_SUCCESS) {
        return res;
    }
    
    if (in[1] > 0x80000000) {
        return EMCORE_ERROR_IO;
    }
    
    *handle = in[1];

    return EMCORE_SUCCESS;
}

int32_t emcore_file_size(uint32_t *size, uint32_t handle) {
    int32_t res;
    uint32_t out[4] = { 31, 0xdeadbeef, 0, 0 }, in[4];
    
    out[1] = handle;

    res = emcore_monitor_command(out, in, 16, 16);

    if (res != EMCORE_SUCCESS) {
        return res;
    }
    
    if (in[1] > 0x80000000) {
        return EMCORE_ERROR_IO;
    }
    
    *size = in[1];
    
    return EMCORE_SUCCESS;
}

int32_t emcore_file_read(uint32_t *nread, uint32_t handle, uint32_t addr, uint32_t size) {
    int32_t res;
    uint32_t out[4] = { 32, 0xdeadbeef, 0xdeadbeef, 0xdeadbeef }, in[4];
    
    out[1] = handle;
    out[2] = addr;
    out[3] = size;

    res = emcore_monitor_command(out, in, 16, 16);

    if (res != EMCORE_SUCCESS) {
        return res;
    }
    
    if (in[1] > 0x80000000) {
        return EMCORE_ERROR_IO;
    }
    
    *nread = in[1];
    
    return EMCORE_SUCCESS;
}

int32_t emcore_file_write(uint32_t *nwrite, uint32_t handle, uint32_t addr, uint32_t size) {
    int32_t res;
    uint32_t out[4] = { 33, 0xdeadbeef, 0xdeadbeef, 0xdeadbeef }, in[4];
    
    out[1] = handle;
    out[2] = addr;
    out[3] = size;

    res = emcore_monitor_command(out, in, 16, 16);

    if (res != EMCORE_SUCCESS) {
        return res;
    }
    
    if (in[1] > 0x80000000) {
        return EMCORE_ERROR_IO;
    }
    
    *nwrite = in[1];
    
    return EMCORE_SUCCESS;
}

int32_t emcore_file_seek(uint32_t handle, uint32_t offset, uint32_t whence) {
    int32_t res;
    uint32_t out[4] = { 34, 0xdeadbeef, 0xdeadbeef, 0xdeadbeef }, in[4];
    
    out[1] = handle;
    out[2] = offset;
    out[3] = whence;

    res = emcore_monitor_command(out, in, 16, 16);

    if (res != EMCORE_SUCCESS) {
        return res;
    }
    
    if (in[1] > 0x80000000) {
        return EMCORE_ERROR_IO;
    }
    
    return EMCORE_SUCCESS;
}

int32_t emcore_file_truncate(uint32_t handle, uint32_t length) {
    int32_t res;
    uint32_t out[4] = { 35, 0xdeadbeef, 0xdeadbeef, 0 }, in[4];
    
    out[1] = handle;
    out[2] = length;

    res = emcore_monitor_command(out, in, 16, 16);

    if (res != EMCORE_SUCCESS) {
        return res;
    }
    
    if (in[1] > 0x80000000) {
        return EMCORE_ERROR_IO;
    }
    
    return EMCORE_SUCCESS;
}

int32_t emcore_file_sync(uint32_t handle) {
    int32_t res;
    uint32_t out[4] = { 36, 0xdeadbeef, 0, 0 }, in[4];

    out[1] = handle;

    res = emcore_monitor_command(out, in, 16, 16);

    if (res != EMCORE_SUCCESS) {
        return res;
    }
    
    if (in[1] > 0x80000000) {
        return EMCORE_ERROR_IO;
    }
    
    return EMCORE_SUCCESS;
}

int32_t emcore_file_close(uint32_t handle) {
    int32_t res;
    uint32_t out[4] = { 37, 0xdeadbeef, 0, 0 }, in[4];

    out[1] = handle;

    res = emcore_monitor_command(out, in, 16, 16);

    if (res != EMCORE_SUCCESS) {
        return res;
    }
    
    if (in[1] > 0x80000000) {
        return EMCORE_ERROR_IO;
    }
    
    return EMCORE_SUCCESS;
}

int32_t emcore_file_close_all(uint32_t *count) {
    int32_t res;
    uint32_t out[4] = { 38, 0, 0, 0 }, in[4];

    res = emcore_monitor_command(out, in, 16, 16);

    if (res != EMCORE_SUCCESS) {
        return res;
    }

    if (in[1] > 0x80000000) {
        return EMCORE_ERROR_IO;
    }
    
    *count = in[1];

    return EMCORE_SUCCESS;
}

int32_t emcore_file_kill_all(uint32_t volume) {
    int32_t res;
    uint32_t out[4] = { 39, 0xdeadbeef, 0, 0 }, in[4];

    out[1] = volume;

    res = emcore_monitor_command(out, in, 16, 16);

    if (res != EMCORE_SUCCESS) {
        return res;
    }
    
    if (in[1] > 0x80000000) {
        return EMCORE_ERROR_IO;
    }
    
    return EMCORE_SUCCESS;
}

int32_t emcore_file_unlink(const char *path) {
    int32_t res;
    uint32_t str_length, data_length, in[4], *out;
    
    str_length = strlen(path) + 1;
    
    if (str_length > 48) {
        uint32_t buf;
        
        res = emcore_malloc(&buf, str_length);
        
        if (res != EMCORE_SUCCESS) {
            return res;
        }
        
        res = emcore_write(path, buf, str_length);
        
        if (res != EMCORE_SUCCESS) {
            return res;
        }
        
        out = calloc(sizeof(char), EMCORE_HEADER_SIZE);

        *(out) = 40;
        *(out + 3) = buf;
        
        res = emcore_monitor_command(out, in, EMCORE_HEADER_SIZE, 16);
    }
    else {
        data_length = EMCORE_HEADER_SIZE + str_length;
        
        out = calloc(sizeof(char), data_length);

        *(out) = 40;
        
        strncpy(((char *)(out + 4)), path, str_length - 1);
        
        res = emcore_monitor_command(out, in, data_length, 16);
    }

    if (res != EMCORE_SUCCESS) {
        return res;
    }
    
    if (in[1] > 0x80000000) {
        return EMCORE_ERROR_IO;
    }
    
    return EMCORE_SUCCESS;
}

int32_t emcore_file_rename(const char *path, const char *newpath) {
    // TODO: currently, calling file_rename crashes the emCORE kernel
    return EMCORE_ERROR_NOT_IMPLEMENTED;
    
    int32_t res;
    uint32_t in[4], out[4] = { 41, 0, 0xdeadbeef, 0xdeadbeef }, buf;
    void *data;
    size_t obytes, nbytes;
    
    obytes = strlen(path) + 1;
    nbytes = strlen(newpath) + 1;
    
    res = emcore_malloc(&buf, obytes + nbytes);
    
    if (res != EMCORE_SUCCESS) {
        return res;
    }
    
    data = calloc(sizeof(char), obytes + nbytes);
    
    strcpy(data, path);
    strcpy(data + obytes, newpath);
    
    res = emcore_write(data, buf, obytes + nbytes);
    
    free(data);
    
    if (res != EMCORE_SUCCESS) {
        return res;
    }

    out[2] = buf;
    out[3] = buf + obytes;
    
    res = emcore_monitor_command(out, in, EMCORE_HEADER_SIZE, 16);

    if (res != EMCORE_SUCCESS) {
        return res;
    }
    
    res = emcore_free(buf);

    if (res != EMCORE_SUCCESS) {
        return res;
    }
    
    if (in[1] > 0x80000000) {
        return EMCORE_ERROR_IO;
    }
    
    return EMCORE_SUCCESS;
}

int32_t emcore_dir_open(uint32_t *handle, const char *name) {
    int32_t res;
    uint32_t str_length, data_length, in[4], *out;
    
    str_length = strlen(name) + 1;
    
    if (str_length > 48) {
        uint32_t buf = 0;
        
        res = emcore_malloc(&buf, str_length);
        
        if (res != EMCORE_SUCCESS) {
            return res;
        }
        
        res = emcore_write(name, buf, str_length);
        
        if (res != EMCORE_SUCCESS) {
            return res;
        }
        
        out = calloc(sizeof(char), EMCORE_HEADER_SIZE);

        *(out) = 42;
        *(out + 3) = buf;
        
        res = emcore_monitor_command(out, in, EMCORE_HEADER_SIZE, 16);
    }
    else {
        data_length = EMCORE_HEADER_SIZE + str_length;
        
        out = calloc(sizeof(char), data_length);

        *(out) = 42;
        
        strncpy(((char *)(out + 4)), name, str_length - 1);
        
        res = emcore_monitor_command(out, in, data_length, 16);
    }

    if (res != EMCORE_SUCCESS) {
        return res;
    }
    
    if (in[1] > 0x80000000) {
        return EMCORE_ERROR_IO;
    }
    
    *handle = in[1];

    return EMCORE_SUCCESS;
}

int32_t emcore_dir_read(struct emcore_dir_entry *entry, uint32_t handle) {
    int32_t res;
    uint32_t maxpath, ptr, dirent_size, emcore_errno_value, filename_buf_len,
        out[4] = { 43, 0xdeadbeef, 0, 0 }, in[4];
    void *buf;

    memset(entry, 0, sizeof(*entry));

    out[1] = handle;

    res = emcore_monitor_command(out, in, 16, 16);

    if (res != EMCORE_SUCCESS) {
        return res;
    }

    ptr = in[3];

    if (!ptr) {
        res = emcore_errno(&emcore_errno_value);

        if (res != EMCORE_SUCCESS) {
            return res;
        }
        
        if (emcore_errno_value != EMCORE_SUCCESS && emcore_errno_value != 2) {
            return EMCORE_ERROR_IO;
        }

        return EMCORE_ERROR_NO_MORE_ENTRIES;
    }

    if (in[1] != 1) { // version
        return EMCORE_ERROR_NOT_IMPLEMENTED;
    }

    maxpath = in[2];

    dirent_size = maxpath + 16;

    buf = malloc(dirent_size);

    res = emcore_read(buf, ptr, dirent_size);

    if (res != EMCORE_SUCCESS) {
        free(buf);
        return res;
    }

    filename_buf_len = strlen((char *)buf) + 1;

    entry->name = malloc(filename_buf_len);
    strncpy(entry->name, buf, filename_buf_len);
    memcpy(&entry->attributes, buf + maxpath, 16);
    
    free(buf);

#ifdef DEBUG_DIR_ENTRIES
    fprintf(stderr, "Read directory entry: %s\n", entry->name);
    fprintf(stderr, "Attributes: 0x%08x\n", entry->attributes);
    fprintf(stderr, "Size: %d\n", entry->size);
    fprintf(stderr, "Start cluster: %d\n", entry->startcluster);
    fprintf(stderr, "Last written date: 0x%04x\n", entry->wrtdate);
    fprintf(stderr, "Last written time: 0x%04x\n", entry->wrttime);
    fprintf(stderr, "Last written TS: %d\n",
        fat_time_to_unix_ts(entry->wrttime, entry->wrtdate));
#endif
    return EMCORE_SUCCESS;
}

int32_t emcore_dir_close(uint32_t handle) {
    int32_t res;
    uint32_t out[4] = { 44, 0xdeadbeef, 0, 0 }, in[4];

    out[1] = handle;

    res = emcore_monitor_command(out, in, 16, 16);

    if (res != EMCORE_SUCCESS) {
        return res;
    }
    
    if (in[1] > 0x80000000) {
        return EMCORE_ERROR_IO;
    }
    
    return EMCORE_SUCCESS;
}

int32_t emcore_dir_close_all(uint32_t *count) {
    int32_t res;
    uint32_t out[4] = { 45, 0, 0, 0 }, in[4];

    res = emcore_monitor_command(out, in, 16, 16);

    if (res != EMCORE_SUCCESS) {
        return res;
    }
    
    if (in[1] > 0x80000000) {
        return EMCORE_ERROR_IO;
    }
    
    *count = in[1];
    
    return EMCORE_SUCCESS;
}

int32_t emcore_dir_create(const char *name) {
    int32_t res;
    uint32_t str_length, data_length, in[4], *out;
    
    str_length = strlen(name) + 1;
    
    if (str_length > 48) {
        uint32_t buf = 0;
        
        res = emcore_malloc(&buf, str_length);
        
        if (res != EMCORE_SUCCESS) {
            return res;
        }
        
        res = emcore_write(name, buf, str_length);
        
        if (res != EMCORE_SUCCESS) {
            return res;
        }
        
        out = calloc(sizeof(char), EMCORE_HEADER_SIZE);

        *(out) = 47;
        *(out + 3) = buf;
        
        res = emcore_monitor_command(out, in, EMCORE_HEADER_SIZE, 16);
    }
    else {
        data_length = EMCORE_HEADER_SIZE + str_length;
        
        out = calloc(sizeof(char), data_length);

        *(out) = 47;
        
        strncpy(((char *)(out + 4)), name, str_length - 1);
        
        res = emcore_monitor_command(out, in, data_length, 16);
    }

    if (res != EMCORE_SUCCESS) {
        return res;
    }
    
    if (in[1] > 0x80000000) {
        return EMCORE_ERROR_IO;
    }
    
    return EMCORE_SUCCESS;
}

int32_t emcore_dir_remove(const char *name) {
    int32_t res;
    uint32_t str_length, data_length, in[4], *out;
    
    str_length = strlen(name) + 1;
    
    if (str_length > 48) {
        uint32_t buf = 0;
        
        res = emcore_malloc(&buf, str_length);
        
        if (res != EMCORE_SUCCESS) {
            return res;
        }
        
        res = emcore_write(name, buf, str_length);
        
        if (res != EMCORE_SUCCESS) {
            return res;
        }
        
        out = calloc(sizeof(char), EMCORE_HEADER_SIZE);

        *(out) = 48;
        *(out + 3) = buf;
        
        res = emcore_monitor_command(out, in, EMCORE_HEADER_SIZE, 16);
    }
    else {
        data_length = EMCORE_HEADER_SIZE + str_length;
        
        out = calloc(sizeof(char), data_length);

        *(out) = 48;
        
        strncpy(((char *)(out + 4)), name, str_length - 1);
        
        res = emcore_monitor_command(out, in, data_length, 16);
    }

    if (res != EMCORE_SUCCESS) {
        return res;
    }
    
    if (in[1] > 0x80000000) {
        return EMCORE_ERROR_IO;
    }
    
    return EMCORE_SUCCESS;
}

int32_t emcore_errno(uint32_t *emcore_errno_value) {
    int32_t res;
    uint32_t out[4] = { 49, 0, 0, 0 }, in[4];

    res = emcore_monitor_command(out, in, 16, 16);

    if (res != EMCORE_SUCCESS) {
        return res;
    }

    *emcore_errno_value = in[1];

    return EMCORE_SUCCESS;
}

int32_t emcore_malloc(uint32_t *ptr, uint32_t size) {
    int32_t res;
    uint32_t out[4] = { 52, 0xdeadbeef, 0, 0 }, in[4];

    out[1] = size;

    res = emcore_monitor_command(out, in, 16, 16);

    if (res != EMCORE_SUCCESS) {
        return res;
    }

    *ptr = in[1];

    return EMCORE_SUCCESS;
}

int32_t emcore_memalign(uint32_t *ptr, uint32_t align, uint32_t size) {
    int32_t res;
    uint32_t out[4] = { 53, 0xdeadbeef, 0xdeadbeef, 0 }, in[4];

    out[1] = align;
    out[2] = size;

    res = emcore_monitor_command(out, in, 16, 16);

    if (res != EMCORE_SUCCESS) {
        return res;
    }

    *ptr = in[1];

    return EMCORE_SUCCESS;
}

int32_t emcore_realloc(uint32_t *new_ptr, uint32_t ptr, uint32_t size) {
    int32_t res;
    uint32_t out[4] = { 54, 0xdeadbeef, 0xdeadbeef, 0 }, in[4];

    out[1] = ptr;
    out[2] = size;

    res = emcore_monitor_command(out, in, 16, 16);

    if (res != EMCORE_SUCCESS) {
        return res;
    }

    *new_ptr = in[1];

    return EMCORE_SUCCESS;
}

int32_t emcore_reownalloc(uint32_t ptr, uint32_t owner) {
    uint32_t out[4] = { 55, 0xdeadbeef, 0xdeadbeef, 0 }, in[4];

    out[1] = ptr;
    out[2] = owner;

    return emcore_monitor_command(out, in, 16, 16);
}

int32_t emcore_free(uint32_t ptr) {
    uint32_t out[4] = { 56, 0xdeadbeef, 0, 0 }, in[4];

    out[1] = ptr;

    return emcore_monitor_command(out, in, 16, 16);
}

int32_t emcore_free_all(void) {
    uint32_t out[4] = { 57, 0, 0, 0 }, in[4];

    return emcore_monitor_command(out, in, 16, 16);
}

int32_t emcore_ls(uint32_t handle) {
    uint32_t res = 0;
    struct emcore_dir_entry entry;

    while (1) {
        res = emcore_dir_read(&entry, handle);

        if (res == EMCORE_ERROR_NO_MORE_ENTRIES) {
            res = EMCORE_SUCCESS;
            break;
        }

        if (res != EMCORE_SUCCESS) {
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
        fprintf(stderr, "Last written TS: %d\n",
            fat_time_to_unix_ts(entry.wrttime, entry.wrtdate));
#endif
        if (entry.attributes & 0x10) {
            printf("     [DIR]");
        }
        else {
            printf("%10d", entry.size);
        }

        printf(" %s\n", entry.name);
    }

    return res;
}
