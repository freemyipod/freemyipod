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

#include "util.h"


const char* libusb_error_messages[13] =
{
    "No error",                /* LIBUSB_SUCCESS = 0               */
    "Input/output error",      /* LIBUSB_ERROR_IO = -1             */
    "Invalid parameter",       /* LIBUSB_ERROR_INVALID_PARAM = -2  */
    "Access denied",           /* LIBUSB_ERROR_ACCESS = -3         */
    "No such device",          /* LIBUSB_ERROR_NO_DEVICE = -4      */
    "Entity not found",        /* LIBUSB_ERROR_NOT_FOUND = -5      */
    "Resource busy",           /* LIBUSB_ERROR_BUSY = -6           */
    "Operation timed out",     /* LIBUSB_ERROR_TIMEOUT = -7        */
    "Overflow",                /* LIBUSB_ERROR_OVERFLOW = -8       */
    "Pipe error",              /* LIBUSB_ERROR_PIPE = -9           */
    "System call interrupted", /* LIBUSB_ERROR_INTERRUPTED = -10   */
    "Insufficient memory",     /* LIBUSB_ERROR_NO_MEM = -11        */
    "Operation not supported", /* LIBUSB_ERROR_NOT_SUPPORTED = -12 */
};

const char* emcore_error_messages[10] =
{
    "No error",               /* EMCORE_SUCCESS = 0               */
    "Invalid command",        /* EMCORE_ERROR_INVALID = 1         */
    "Command not supported",  /* EMCORE_ERROR_NOT_SUPPORTED = 2   */
    "Device is busy",         /* EMCORE_ERROR_BUSY = 3            */
    "No such device",         /* EMCORE_ERROR_NO_DEVICE = 4       */
    "Incomplete transfer",    /* EMCORE_ERROR_INCOMPLETE = 5      */
    "Overflow",               /* EMCORE_ERROR_OVERFLOW = 6        */
    "Not implemented",        /* EMCORE_ERROR_NOT_IMPLEMENTED = 7 */
    "No more entries in dir", /* EMCORE_ERROR_NO_MORE_ENTRIES = 8 */
    "I/O error",              /* EMCORE_ERROR_IO = 9              */
};

void dump_packet(const void* data, size_t length)
{
    static size_t i;

    for (i = 0; i < length; ++i)
    {
        fprintf(stderr, "%02x ", *((uint8_t*)(data + i)));

        if (i % 4 == 3)
        {
            fprintf(stderr, " ");
        }

        if (i % 16 == 15 && i + 1 < length)
        {
            fprintf(stderr, "\n");
        }
    }

    fprintf(stderr, "\n");
}

void alignsplit(struct alignsizes* sizeptr, uint32_t addr,
    uint32_t size, uint32_t blksize, uint32_t align)
{
    uint32_t end, bodyaddr, tailaddr;

    if (size <= blksize)
    {
        sizeptr->head = size;
        sizeptr->body = 0;
        sizeptr->tail = 0;

        return;
    }

    end = addr + size;

    if (addr & (align -1))
    {
        bodyaddr = (addr + MIN(size, blksize)) & ~(align - 1);
    }
    else
    {
        bodyaddr = addr;
    }

    sizeptr->head = bodyaddr - addr;

    if ((size - sizeptr->head) & (align - 1))
    {
        tailaddr = ((end - MIN(end - bodyaddr, blksize) + align - 1) & ~(align - 1));
    }
    else
    {
        tailaddr = end;
    }

    sizeptr->tail = end - tailaddr;
    sizeptr->body = tailaddr - bodyaddr;

    return;
}

time_t fat_time_to_unix_ts(short wrttime, short wrtdate)
{
    struct tm result;
    /*
        Example time: 0x9365
        in memory:
        M M M S S S S S   H H H H H M M M
        as a number:
        H H H H H M M M   M M M S S S S S

        Example date: 0x3d37
        in memory:
        M M M D D D D D   Y Y Y Y Y Y Y M
        as a number:
        Y Y Y Y Y Y Y M   M M M D D D D D
    */

    result.tm_sec = ((wrttime & 0x1f) << 1);
    result.tm_min = (wrttime >> 5) & 0x3f;
    result.tm_hour = (wrttime >> 11) & 0x1f;
    result.tm_mday = wrtdate & 0x1f;
    result.tm_mon = ((wrtdate >> 5) & 0xf) - 1;
    result.tm_year = ((wrtdate >> 9) & 0x7f) + 80;

    return mktime(&result);
}

int32_t unix_ts_to_fat_time(time_t datetime)
{
    struct tm* tm_ptr;
    
    tm_ptr = localtime(&datetime);
    
    return (tm_ptr->tm_sec >> 1) |
           (tm_ptr->tm_min << 5) |
           (tm_ptr->tm_hour << 0xb) |
           (tm_ptr->tm_mday << 0x10) |
           ((tm_ptr->tm_mon + 1) << 0x15) |
           ((tm_ptr->tm_year - 80) << 0x19);
}

void print_error(int code)
{
    if (code > 0)
    {
        fprintf(stderr, "emcore error: %s\n", emcore_error_messages[code]);
    }
    else
    {
        fprintf(stderr, "libusb error: %s\n", (
            LIBUSB_ERROR_OTHER == code ?
                "Other error" :
                libusb_error_messages[-code]
        ));
    }
}
