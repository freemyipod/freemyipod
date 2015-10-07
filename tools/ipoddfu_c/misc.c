//
//
//    Copyright 2011 user890104
//
//
//    This file is part of ipoddfu.
//
//    ipoddfu is free software: you can redistribute it and/or
//    modify it under the terms of the GNU General Public License as
//    published by the Free Software Foundation, either version 2 of the
//    License, or (at your option) any later version.
//
//    ipoddfu is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//    See the GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License along
//    with ipoddfu.  If not, see <http://www.gnu.org/licenses/>.
//
//


#include <stdio.h>

#include <libusb.h>

#include "misc.h"

const char *libusb_error_messages[13] =
{
    "No error",                // LIBUSB_SUCCESS = 0
    "Input/output error",      // LIBUSB_ERROR_IO = -1
    "Invalid parameter",       // LIBUSB_ERROR_INVALID_PARAM = -2
    "Access denied",           // LIBUSB_ERROR_ACCESS = -3
    "No such device",          // LIBUSB_ERROR_NO_DEVICE = -4
    "Entity not found",        // LIBUSB_ERROR_NOT_FOUND = -5
    "Resource busy",           // LIBUSB_ERROR_BUSY = -6
    "Operation timed out",     // LIBUSB_ERROR_TIMEOUT = -7
    "Overflow",                // LIBUSB_ERROR_OVERFLOW = -8
    "Pipe error",              // LIBUSB_ERROR_PIPE = -9
    "System call interrupted", // LIBUSB_ERROR_INTERRUPTED = -10
    "Insufficient memory",     // LIBUSB_ERROR_NO_MEM = -11
    "Operation not supported", // LIBUSB_ERROR_NOT_SUPPORTED = -12
};

long int fgetsize(FILE *fp)
{
    static long int pos, size;
    
    if (-1L == (pos = ftell(fp)))
    {
        perror("ftell");
        
        return -1L;
    }
    
    if(fseek(fp, 0L, SEEK_END))
    {
        perror("fseek");
        
        return -1L;
    }
    
    if (-1L == (size = ftell(fp)))
    {
        perror("ftell");
        
        return -1L;
    }
    
    if (fseek(fp, pos, SEEK_SET))
    {
        perror("fseek");
        
        return -1L;
    }
    
    return size;
}

void dump_packet(const unsigned char *data, const unsigned int length)
{
    static unsigned int i;
    
    for (i = 0; i < length; ++i)
    {
        printf("%02x ", data[i]);
        
        if (i % 4 == 3)
        {
            printf(" ");
        }
        
        if (i % 16 == 15 && i + 1 < length)
        {
            printf("\n");
        }
    }

    printf("\n");
}

void print_error(const int code)
{
    if (code > 0)
    {    
        fprintf(stderr, "error: code %d\n", code);
    }
    else
    {
        fprintf(stderr, "libusb error: %s (code %d)\n", (
            LIBUSB_ERROR_OTHER == code || code > 0 ?
                "unspecified" : libusb_error_messages[-code]
        ), code);
    }
}
