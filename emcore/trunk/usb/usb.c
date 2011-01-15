//
//
//    Copyright 2010 TheSeven
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
#include "panic.h"
#include "usb.h"
#include "usb_ch9.h"
#include "usbdrv.h"
#include "thread.h"
#include "console.h"
#include "util.h"
#include "contextswitch.h"
#include "power.h"
#include "mmu.h"
#include "shutdown.h"
#include "execimage.h"
#ifdef HAVE_I2C
#include "i2c.h"
#endif
#ifdef HAVE_BOOTFLASH
#include "bootflash.h"
#endif
#ifdef HAVE_HWKEYAES
#include "hwkeyaes.h"
#endif
#ifdef HAVE_HMACSHA1
#include "hmacsha1.h"
#endif
#ifdef USB_HAVE_TARGET_SPECIFIC_REQUESTS
#include "usbtarget.h"
#endif
#ifdef HAVE_STORAGE
#include "storage.h"
#include "disk.h"
#include "file.h"
#include "dir.h"
#include "libc/include/errno.h"
#endif


static uint8_t ctrlresp[2] CACHEALIGN_ATTR;
static uint32_t dbgrecvbuf[0x80] CACHEALIGN_ATTR;
static uint32_t dbgsendbuf[0x80] CACHEALIGN_ATTR;
static uint32_t dbgasyncsendbuf[0x80] CACHEALIGN_ATTR;
static char dbgendpoints[4] IBSS_ATTR;

enum dbgaction_t
{
    DBGACTION_IDLE = 0,
    DBGACTION_I2CSEND,
    DBGACTION_I2CRECV,
    DBGACTION_RESET,
    DBGACTION_POWEROFF,
    DBGACTION_CWRITE,
    DBGACTION_CREAD,
    DBGACTION_CFLUSH,
    DBGACTION_EXECIMAGE,
    DBGACTION_EXECFIRMWARE,
    DBGACTION_READBOOTFLASH,
    DBGACTION_WRITEBOOTFLASH,
    DBGACTION_HWKEYAES,
    DBGACTION_HMACSHA1,
    DBGACTION_TARGETSPECIFIC,
    DBGACTION_STORAGE
};

static uint32_t dbgstack[0x200] STACK_ATTR;
struct wakeup dbgwakeup IBSS_ATTR;
extern struct scheduler_thread* scheduler_threads;
extern struct scheduler_thread* current_thread;
static enum dbgaction_t dbgaction IBSS_ATTR;
static int dbgi2cbus;
static int dbgi2cslave;
static int dbgactionaddr;
static int dbgactionoffset;
static int dbgactionlength;
static int dbgactionconsoles;
static int dbgactiontype;
static char dbgconsendbuf[4096];
static char dbgconrecvbuf[1024];
static int dbgconsendreadidx IBSS_ATTR;
static int dbgconsendwriteidx IBSS_ATTR;
static int dbgconrecvreadidx IBSS_ATTR;
static int dbgconrecvwriteidx IBSS_ATTR;
static struct wakeup dbgconsendwakeup IBSS_ATTR;
static struct wakeup dbgconrecvwakeup IBSS_ATTR;
static bool dbgconsoleattached IBSS_ATTR;

static const char dbgconoverflowstr[] = "\n\n[overflowed]\n\n";

extern int _initstart;   // These aren't ints at all, but gcc complains about void types being
extern int _sdramstart;  // used here, and we only need the address, so just make it happy...


static struct usb_device_descriptor CACHEALIGN_ATTR device_descriptor =
{
    .bLength            = sizeof(struct usb_device_descriptor),
    .bDescriptorType    = USB_DT_DEVICE,
    .bcdUSB             = 0x0200,
    .bDeviceClass       = USB_CLASS_VENDOR_SPEC,
    .bDeviceSubClass    = 0xff,
    .bDeviceProtocol    = 0xff,
    .bMaxPacketSize0    = 64,
    .idVendor           = 0xffff,
    .idProduct          = 0xe000,
    .bcdDevice          = 0x0001,
    .iManufacturer      = 1,
    .iProduct           = 2,
    .iSerialNumber      = 0,
    .bNumConfigurations = 1
};

static struct usb_config_bundle
{
    struct usb_config_descriptor config_descriptor;
    struct usb_interface_descriptor interface_descriptor;
    struct usb_endpoint_descriptor endpoint1_descriptor;
    struct usb_endpoint_descriptor endpoint2_descriptor;
    struct usb_endpoint_descriptor endpoint3_descriptor;
    struct usb_endpoint_descriptor endpoint4_descriptor;
} __attribute__((packed)) CACHEALIGN_ATTR config_bundle = 
{
    .config_descriptor =
    {
        .bLength             = sizeof(struct usb_config_descriptor),
        .bDescriptorType     = USB_DT_CONFIG,
        .wTotalLength        = sizeof(struct usb_config_descriptor)
                             + sizeof(struct usb_interface_descriptor)
                             + sizeof(struct usb_endpoint_descriptor) * 4,
        .bNumInterfaces      = 1,
        .bConfigurationValue = 1,
        .iConfiguration      = 0,
        .bmAttributes        = USB_CONFIG_ATT_ONE,
        .bMaxPower           = 250
    },
    .interface_descriptor =
    {
        .bLength             = sizeof(struct usb_interface_descriptor),
        .bDescriptorType     = USB_DT_INTERFACE,
        .bInterfaceNumber    = 0,
        .bAlternateSetting   = 0,
        .bNumEndpoints       = 4,
        .bInterfaceClass     = USB_CLASS_VENDOR_SPEC,
        .bInterfaceSubClass  = 0xff,
        .bInterfaceProtocol  = 0xff,
        .iInterface          = 0
    },
    .endpoint1_descriptor =
    {
        .bLength             = sizeof(struct usb_endpoint_descriptor),
        .bDescriptorType     = USB_DT_ENDPOINT,
        .bEndpointAddress    = 0,
        .bmAttributes        = USB_ENDPOINT_XFER_BULK,
        .wMaxPacketSize      = 0,
        .bInterval           = 1
    },
    .endpoint2_descriptor =
    {
        .bLength             = sizeof(struct usb_endpoint_descriptor),
        .bDescriptorType     = USB_DT_ENDPOINT,
        .bEndpointAddress    = 0,
        .bmAttributes        = USB_ENDPOINT_XFER_BULK,
        .wMaxPacketSize      = 0,
        .bInterval           = 1
    },
    .endpoint3_descriptor =
    {
        .bLength             = sizeof(struct usb_endpoint_descriptor),
        .bDescriptorType     = USB_DT_ENDPOINT,
        .bEndpointAddress    = 0,
        .bmAttributes        = USB_ENDPOINT_XFER_BULK,
        .wMaxPacketSize      = 0,
        .bInterval           = 1
    },
    .endpoint4_descriptor =
    {
        .bLength             = sizeof(struct usb_endpoint_descriptor),
        .bDescriptorType     = USB_DT_ENDPOINT,
        .bEndpointAddress    = 0,
        .bmAttributes        = USB_ENDPOINT_XFER_BULK,
        .wMaxPacketSize      = 0,
        .bInterval           = 1
    }
};

static struct usb_string_descriptor CACHEALIGN_ATTR string_devicename =
{
    32,
    USB_DT_STRING,
    {'e', 'm', 'B', 'I', 'O', 'S', ' ', 'D', 'e', 'b', 'u', 'g', 'g', 'e', 'r'}
};

static const struct usb_string_descriptor CACHEALIGN_ATTR lang_descriptor =
{
    4,
    USB_DT_STRING,
    {0x0409}
};


void usb_setup_dbg_listener()
{
    usb_drv_recv(dbgendpoints[0], dbgrecvbuf, usb_drv_port_speed() ? 512 : 64);
}

void usb_handle_control_request(struct usb_ctrlrequest* req)
{
    const void* addr;
    int size = -1;
    switch (req->bRequest)
    {
    case USB_REQ_GET_STATUS:
        if (req->bRequestType == USB_DIR_IN) ctrlresp[0] = 1;
        else ctrlresp[0] = 0;
        ctrlresp[1] = 0;
        addr = ctrlresp;
        size = 2;
        break;
    case USB_REQ_CLEAR_FEATURE:
        if (req->bRequestType == USB_RECIP_ENDPOINT && req->wValue == USB_ENDPOINT_HALT)
            usb_drv_stall(req->wIndex & 0xf, false, req->wIndex >> 7);
        size = 0;
        break;
    case USB_REQ_SET_FEATURE:
        size = 0;
        break;
    case USB_REQ_SET_ADDRESS:
        size = 0;
        usb_drv_cancel_all_transfers();
        usb_drv_set_address(req->wValue);
        usb_setup_dbg_listener();
        break;
    case USB_REQ_GET_DESCRIPTOR:
        switch (req->wValue >> 8)
        {
        case USB_DT_DEVICE:
            addr = &device_descriptor;
            size = sizeof(device_descriptor);
            break;
        case USB_DT_OTHER_SPEED_CONFIG:
        case USB_DT_CONFIG:
            if ((req->wValue >> 8) == USB_DT_CONFIG)
            {
                int maxpacket = usb_drv_port_speed() ? 512 : 64;
                config_bundle.endpoint1_descriptor.wMaxPacketSize = maxpacket;
                config_bundle.endpoint2_descriptor.wMaxPacketSize = maxpacket;
                config_bundle.endpoint3_descriptor.wMaxPacketSize = maxpacket;
                config_bundle.endpoint4_descriptor.wMaxPacketSize = maxpacket;
                config_bundle.config_descriptor.bDescriptorType = USB_DT_CONFIG;
            }
            else
            {
                int maxpacket = usb_drv_port_speed() ? 64 : 512;
                config_bundle.endpoint1_descriptor.wMaxPacketSize = maxpacket;
                config_bundle.endpoint2_descriptor.wMaxPacketSize = maxpacket;
                config_bundle.endpoint3_descriptor.wMaxPacketSize = maxpacket;
                config_bundle.endpoint4_descriptor.wMaxPacketSize = maxpacket;
                config_bundle.config_descriptor.bDescriptorType = USB_DT_OTHER_SPEED_CONFIG;
            }
            addr = &config_bundle;
            size = sizeof(config_bundle);
            break;
        case USB_DT_STRING:
            switch (req->wValue & 0xff)
            {
            case 0:
                addr = &lang_descriptor;
                size = lang_descriptor.bLength;
                break;
            case 1:
                string_devicename.bLength = 14;
                addr = &string_devicename;
                size = string_devicename.bLength;
                break;
            case 2:
                string_devicename.bLength = 32;
                addr = &string_devicename;
                size = string_devicename.bLength;
                break;
            }
            break;
        }
        break;
    case USB_REQ_GET_CONFIGURATION:
        ctrlresp[0] = 1;
        addr = ctrlresp;
        size = 1;
        break;
    case USB_REQ_SET_CONFIGURATION:
        usb_drv_cancel_all_transfers();
        usb_setup_dbg_listener();
        size = 0;
        break;
    }
    if (!size) usb_drv_send_nonblocking(0, NULL, 0);
    else if (size == -1)
    {
        usb_drv_stall(0, true, true);
        usb_drv_stall(0, true, false);
    }
    else
    {
        usb_drv_recv(0, NULL, 0);
        usb_drv_send_nonblocking(0, addr, size > req->wLength ? req->wLength : size);
    }
}

bool set_dbgaction(enum dbgaction_t action, int addsize)
{
    if (dbgaction != DBGACTION_IDLE)
    {
        dbgsendbuf[0] = 3;
        usb_drv_send_nonblocking(dbgendpoints[1], dbgsendbuf, 16 + addsize);
        return true;
    }
    dbgaction = action;
    wakeup_signal(&dbgwakeup);
    return false;
}

void reset() __attribute__((noreturn));

void usb_handle_transfer_complete(int endpoint, int dir, int status, int length)
{
    void* addr = dbgsendbuf;
    int size = 0;
    if (endpoint == dbgendpoints[0])
    {
#ifdef USB_HAVE_TARGET_SPECIFIC_REQUESTS
        if (dbgrecvbuf[0] >= 0xffff0000)
        {
            if (!set_dbgaction(DBGACTION_TARGETSPECIFIC, 0))
                memcpy(dbgasyncsendbuf, dbgrecvbuf, sizeof(dbgasyncsendbuf));
            usb_setup_dbg_listener();
            return;
        }
#endif
        switch (dbgrecvbuf[0])
        {
        case 1:  // GET INFO
            dbgsendbuf[0] = 1;
            size = 16;
            switch (dbgrecvbuf[1])
            {
            case 0:  // GET VERSION INFO
                dbgsendbuf[1] = VERSION_SVN_INT;
                dbgsendbuf[2] = VERSION_MAJOR | (VERSION_MINOR << 8)
                              | (VERSION_PATCH << 16) | (1 << 24);
                dbgsendbuf[3] = PLATFORM_ID;
                break;
            case 1:  // GET PACKET SIZE INFO
                dbgsendbuf[1] = 0x02000200;
                dbgsendbuf[2] = usb_drv_get_max_out_size();
                dbgsendbuf[3] = usb_drv_get_max_in_size();
                break;
            case 2:  // GET USER MEMORY INFO
                dbgsendbuf[1] = (uint32_t)&_initstart;
                dbgsendbuf[2] = (uint32_t)&_sdramstart;
                break;
            default:
                dbgsendbuf[0] = 2;
            }
            break;
        case 2:  // RESET
            if (dbgrecvbuf[1])
            {
                if (set_dbgaction(DBGACTION_RESET, 0)) break;
                dbgsendbuf[0] = 1;
                size = 16;
            }
            else reset();
            break;
        case 3:  // POWER OFF
            if (set_dbgaction(DBGACTION_POWEROFF, 0)) break;
            dbgactiontype = dbgrecvbuf[1];
            dbgsendbuf[0] = 1;
            size = 16;
            break;
        case 4:  // READ MEMORY
            dbgsendbuf[0] = 1;
            memcpy(&dbgsendbuf[4], (const void*)dbgrecvbuf[1], dbgrecvbuf[2]);
            size = dbgrecvbuf[2] + 16;
            break;
        case 5:  // WRITE MEMORY
            dbgsendbuf[0] = 1;
            memcpy((void*)dbgrecvbuf[1], &dbgrecvbuf[4], dbgrecvbuf[2]);
            size = 16;
            break;
        case 6:  // READ DMA
            dbgsendbuf[0] = 1;
            usb_drv_send_nonblocking(dbgendpoints[1], dbgsendbuf, 16);
            usb_drv_send_nonblocking(dbgendpoints[3], (const void*)dbgrecvbuf[1], dbgrecvbuf[2]);
            break;
        case 7:  // WRITE DMA
            dbgsendbuf[0] = 1;
            size = 16;
            usb_drv_recv(dbgendpoints[2], (void*)dbgrecvbuf[1], dbgrecvbuf[2]);
            break;
#ifdef HAVE_I2C
        case 8:  // READ I2C
            if (set_dbgaction(DBGACTION_I2CRECV, dbgrecvbuf[1] >> 24)) break;
            dbgi2cbus = dbgrecvbuf[1] & 0xff;
            dbgi2cslave = (dbgrecvbuf[1] >> 8) & 0xff;
            dbgactionaddr = (dbgrecvbuf[1] >> 16) & 0xff;
            dbgactionlength = dbgrecvbuf[1] >> 24;
            if (!dbgactionlength) dbgactionlength = 256;
            break;
        case 9:  // WRITE I2C
            if (set_dbgaction(DBGACTION_I2CSEND, 0)) break;
            dbgi2cbus = dbgrecvbuf[1] & 0xff;
            dbgi2cslave = (dbgrecvbuf[1] >> 8) & 0xff;
            dbgactionaddr = (dbgrecvbuf[1] >> 16) & 0xff;
            dbgactionlength = dbgrecvbuf[1] >> 24;
            if (!dbgactionlength) dbgactionlength = 256;
            memcpy(dbgasyncsendbuf, &dbgrecvbuf[4], dbgactionlength);
            break;
#endif
        case 10:  // READ CONSOLE
            dbgconsoleattached = true;
            int bytes = dbgconsendwriteidx - dbgconsendreadidx;
            int used = 0;
            if (bytes)
            {
                if (bytes < 0) bytes += sizeof(dbgconsendbuf);
                used = bytes;
                if (bytes > dbgrecvbuf[1]) bytes = dbgrecvbuf[1];
                int readbytes = bytes;
                char* outptr = (char*)&dbgsendbuf[4];
                if (dbgconsendreadidx + bytes >= sizeof(dbgconsendbuf))
                {
                    readbytes = sizeof(dbgconsendbuf) - dbgconsendreadidx;
                    memcpy(outptr, &dbgconsendbuf[dbgconsendreadidx], readbytes);
                    dbgconsendreadidx = 0;
                    outptr = &outptr[readbytes];
                    readbytes = bytes - readbytes;
                }
                if (readbytes) memcpy(outptr, &dbgconsendbuf[dbgconsendreadidx], readbytes);
                dbgconsendreadidx += readbytes;
                wakeup_signal(&dbgconsendwakeup);
            }
            dbgsendbuf[0] = 1;
            dbgsendbuf[1] = bytes;
            dbgsendbuf[2] = sizeof(dbgconsendbuf);
            dbgsendbuf[3] = used - bytes;
            size = 16 + dbgrecvbuf[1];
            break;
        case 11:  // WRITE CONSOLE
            bytes = dbgconrecvreadidx - dbgconrecvwriteidx - 1;
            if (bytes < 0) bytes += sizeof(dbgconrecvbuf);
            if (bytes)
            {
                if (bytes > dbgrecvbuf[1]) bytes = dbgrecvbuf[1];
                int writebytes = bytes;
                char* readptr = (char*)&dbgrecvbuf[4];
                if (dbgconrecvwriteidx + bytes >= sizeof(dbgconrecvbuf))
                {
                    writebytes = sizeof(dbgconrecvbuf) - dbgconrecvwriteidx;
                    memcpy(&dbgconrecvbuf[dbgconrecvwriteidx], readptr, writebytes);
                    dbgconrecvwriteidx = 0;
                    readptr = &readptr[writebytes];
                    writebytes = bytes - writebytes;
                }
                if (writebytes) memcpy(&dbgconrecvbuf[dbgconrecvwriteidx], readptr, writebytes);
                dbgconrecvwriteidx += writebytes;
                wakeup_signal(&dbgconrecvwakeup);
            }
            dbgsendbuf[0] = 1;
            dbgsendbuf[1] = bytes;
            dbgsendbuf[2] = sizeof(dbgconrecvbuf);
            dbgsendbuf[3] = dbgconrecvreadidx - dbgconrecvwriteidx - 1;
            size = 16;
            break;
        case 12:  // CWRITE
            if (set_dbgaction(DBGACTION_CWRITE, 0)) break;
            dbgactionconsoles = dbgrecvbuf[1];
            dbgactionlength = dbgrecvbuf[2];
            memcpy(dbgasyncsendbuf, &dbgrecvbuf[4], dbgactionlength);
            break;
        case 13:  // CREAD
            if (set_dbgaction(DBGACTION_CREAD, dbgrecvbuf[2])) break;
            dbgactionconsoles = dbgrecvbuf[1];
            dbgactionlength = dbgrecvbuf[2];
            break;
        case 14:  // CFLUSH
            if (set_dbgaction(DBGACTION_CFLUSH, 0)) break;
            dbgactionconsoles = dbgrecvbuf[1];
            break;
        case 15:  // GET PROCESS INFO
            dbgsendbuf[0] = 1;
            dbgsendbuf[1] = SCHEDULER_THREAD_INFO_VERSION;
            dbgsendbuf[2] = MAX_THREADS * sizeof(struct scheduler_thread);
            memcpy(&dbgsendbuf[4], (void*)(((uint32_t)&scheduler_threads) + dbgrecvbuf[1]),
                   dbgrecvbuf[2]);
            size = dbgrecvbuf[2] + 16;
            break;
        case 16:  // FREEZE SCHEDULER
            dbgsendbuf[1] = scheduler_freeze(dbgrecvbuf[1]);
            scheduler_switch(-1);
            dbgsendbuf[0] = 1;
            size = 16;
            break;
        case 17:  // SUSPEND THREAD
            if (dbgrecvbuf[1])
            {
                if (thread_suspend(dbgrecvbuf[2]) == -4) dbgsendbuf[1] = 1;
                else dbgsendbuf[1] = 0;
            }
            else
            {
                if (thread_resume(dbgrecvbuf[2]) == -5) dbgsendbuf[1] = 0;
                else dbgsendbuf[1] = 1;
            }
            dbgsendbuf[0] = 1;
            size = 16;
            break;
        case 18:  // KILL THREAD
            thread_terminate(dbgrecvbuf[1]);
            dbgsendbuf[0] = 1;
            size = 16;
            break;
        case 19:  // KILL THREAD
            dbgsendbuf[0] = 1;
            dbgsendbuf[1] = thread_create((const char*)dbgsendbuf[1], (const void*)dbgsendbuf[2],
                                          (char*)dbgsendbuf[3], dbgsendbuf[4], dbgsendbuf[5],
                                          dbgsendbuf[6], dbgsendbuf[7]);
            size = 16;
            break;
        case 20:  // FLUSH CACHE
            clean_dcache();
            invalidate_icache();
            dbgsendbuf[0] = 1;
            size = 16;
            break;
        case 21:  // EXECIMAGE
            if (set_dbgaction(DBGACTION_EXECIMAGE, 0)) break;
            dbgactionaddr = dbgrecvbuf[1];
            break;
#ifdef HAVE_BOOTFLASH
        case 22:  // READ BOOT FLASH
            if (set_dbgaction(DBGACTION_READBOOTFLASH, 0)) break;
            dbgactionaddr = dbgrecvbuf[1];
            dbgactionoffset = dbgrecvbuf[2];
            dbgactionlength = dbgrecvbuf[3];
            break;
        case 23:  // WRITE BOOT FLASH
            if (set_dbgaction(DBGACTION_WRITEBOOTFLASH, 0)) break;
            dbgactionaddr = dbgrecvbuf[1];
            dbgactionoffset = dbgrecvbuf[2];
            dbgactionlength = dbgrecvbuf[3];
            break;
#endif
        case 24:  // EXECFIRMWARE
            if (set_dbgaction(DBGACTION_EXECFIRMWARE, 0)) break;
            dbgactionaddr = dbgrecvbuf[1];
            break;
#ifdef HAVE_HWKEYAES
        case 25:  // HWKEYAES
            if (set_dbgaction(DBGACTION_HWKEYAES, 0)) break;
            dbgactiontype = ((uint8_t*)dbgrecvbuf)[4];
            dbgactionoffset = ((uint16_t*)dbgrecvbuf)[3];
            dbgactionaddr = dbgrecvbuf[2];
            dbgactionlength = dbgrecvbuf[3];
            break;
#endif
#ifdef HAVE_HMACSHA1
        case 26:  // HMACSHA1
            if (set_dbgaction(DBGACTION_HMACSHA1, 0)) break;
            dbgactionaddr = dbgrecvbuf[1];
            dbgactionlength = dbgrecvbuf[2];
            dbgactionoffset = dbgrecvbuf[3];
            break;
#endif
#ifdef HAVE_STORAGE
        case 27:  // STORAGE_GET_INFO
        case 28:  // STORAGE_READ_SECTORS_MD
        case 29:  // STORAGE_WRITE_SECTORS_MD
        case 30:  // FILE_OPEN
        case 31:  // FILESIZE
        case 32:  // READ
        case 33:  // WRITE
        case 34:  // LSEEK
        case 35:  // FTRUNCATE
        case 36:  // FSYNC
        case 37:  // CLOSE
        case 38:  // CLOSE_MONITOR_FILES
        case 39:  // RELEASE_FILES
        case 40:  // REMOVE
        case 41:  // RENAME
        case 42:  // OPENDIR
        case 43:  // READDIR
        case 44:  // CLOSEDIR
        case 45:  // CLOSE_MONITOR_DIRS
        case 46:  // RELEASE_DIRS
        case 47:  // MKDIR
        case 48:  // RMDIR
        case 49:  // ERRNO
#ifdef HAVE_HOTSWAP
        case 50:  // DISK_MOUNT
        case 51:  // DISK_UNMOUNT
#endif
            if (!set_dbgaction(DBGACTION_STORAGE, 0))
                memcpy(dbgasyncsendbuf, dbgrecvbuf, sizeof(dbgasyncsendbuf));
            break;
#endif
        default:
            dbgsendbuf[0] = 2;
            size = 16;
        }
        usb_setup_dbg_listener();
        if (size) usb_drv_send_nonblocking(dbgendpoints[1], addr, size);
    }
}

void usb_handle_bus_reset(void)
{
    dbgendpoints[0] = usb_drv_request_endpoint(USB_ENDPOINT_XFER_BULK, USB_DIR_OUT);
    dbgendpoints[1] = usb_drv_request_endpoint(USB_ENDPOINT_XFER_BULK, USB_DIR_IN);
    dbgendpoints[2] = usb_drv_request_endpoint(USB_ENDPOINT_XFER_BULK, USB_DIR_OUT);
    dbgendpoints[3] = usb_drv_request_endpoint(USB_ENDPOINT_XFER_BULK, USB_DIR_IN);
    config_bundle.endpoint1_descriptor.bEndpointAddress = dbgendpoints[0];
    config_bundle.endpoint2_descriptor.bEndpointAddress = dbgendpoints[1];
    config_bundle.endpoint3_descriptor.bEndpointAddress = dbgendpoints[2];
    config_bundle.endpoint4_descriptor.bEndpointAddress = dbgendpoints[3];
    usb_setup_dbg_listener();
}

void dbgthread(void)
{
    int i;
    int t;
    while (1)
    {
        wakeup_wait(&dbgwakeup, TIMEOUT_BLOCK);
        for (i = 0; i < MAX_THREADS; i++)
            if (scheduler_threads[i].state == THREAD_DEFUNCT)
            {
                if (scheduler_threads[i].block_type == THREAD_DEFUNCT_STKOV)
                {
                    if (scheduler_threads[i].name)
                        cprintf(1, "\n*PANIC*\nStack overflow! (%s)\n",
                                scheduler_threads[i].name);
                    else cprintf(1, "\n*PANIC*\nStack overflow! (ID %d)\n", i);
                }
                scheduler_threads[i].state = THREAD_DEFUNCT_ACK;
            }
        if (dbgaction != DBGACTION_IDLE)
        {
            switch (dbgaction)
            {
#ifdef HAVE_I2C
            case DBGACTION_I2CSEND:
                i2c_send(dbgi2cbus, dbgi2cslave, dbgactionaddr,
                         (uint8_t*)dbgasyncsendbuf, dbgactionlength);
                dbgasyncsendbuf[0] = 1;
                usb_drv_send_nonblocking(dbgendpoints[1], dbgasyncsendbuf, 16);
                break;
            case DBGACTION_I2CRECV:
                i2c_recv(dbgi2cbus, dbgi2cslave, dbgactionaddr,
                         (uint8_t*)(&dbgasyncsendbuf[4]), dbgactionlength);
                dbgasyncsendbuf[0] = 1;
                usb_drv_send_nonblocking(dbgendpoints[1], dbgasyncsendbuf, 16 + dbgactionlength);
                break;
#endif
            case DBGACTION_POWEROFF:
                if (dbgactiontype) shutdown(true);
                power_off();
                break;
            case DBGACTION_RESET:
                shutdown(true);
                reset();
                break;
            case DBGACTION_CWRITE:
                cwrite(dbgactionconsoles, (const char*)dbgasyncsendbuf, dbgactionlength);
                dbgasyncsendbuf[0] = 1;
                usb_drv_send_nonblocking(dbgendpoints[1], dbgasyncsendbuf, 16);
                break;
            case DBGACTION_CREAD:
                dbgasyncsendbuf[0] = 1;
                dbgasyncsendbuf[1] = cread(dbgactionconsoles, (char*)&dbgasyncsendbuf[4],
                                           dbgactionlength, 0);
                usb_drv_send_nonblocking(dbgendpoints[1], dbgasyncsendbuf, 16 + dbgactionlength);
                break;
            case DBGACTION_CFLUSH:
                cflush(dbgactionconsoles);
                dbgasyncsendbuf[0] = 1;
                usb_drv_send_nonblocking(dbgendpoints[1], dbgasyncsendbuf, 16);
                break;
            case DBGACTION_EXECIMAGE:
                dbgasyncsendbuf[0] = 1;
                dbgasyncsendbuf[1] = execimage((void*)dbgactionaddr);
                usb_drv_send_nonblocking(dbgendpoints[1], dbgasyncsendbuf, 16);
                break;
            case DBGACTION_EXECFIRMWARE:
                shutdown(false);
                dbgasyncsendbuf[0] = 1;
                usb_drv_send_nonblocking(dbgendpoints[1], dbgasyncsendbuf, 16);
                execfirmware((void*)dbgactionaddr);
#ifdef HAVE_BOOTFLASH
            case DBGACTION_READBOOTFLASH:
                bootflash_readraw((void*)dbgactionaddr, dbgactionoffset, dbgactionlength);
                dbgasyncsendbuf[0] = 1;
                usb_drv_send_nonblocking(dbgendpoints[1], dbgasyncsendbuf, 16);
                break;
            case DBGACTION_WRITEBOOTFLASH:
                bootflash_writeraw((void*)dbgactionaddr, dbgactionoffset, dbgactionlength);
                dbgasyncsendbuf[0] = 1;
                usb_drv_send_nonblocking(dbgendpoints[1], dbgasyncsendbuf, 16);
                break;
#endif
#ifdef HAVE_HWKEYAES
            case DBGACTION_HWKEYAES:
                hwkeyaes((enum hwkeyaes_direction) dbgactiontype, dbgactionoffset,
                         (void*)dbgactionaddr, dbgactionlength);
                dbgasyncsendbuf[0] = 1;
                usb_drv_send_nonblocking(dbgendpoints[1], dbgasyncsendbuf, 16);
                break;
#endif
#ifdef HAVE_HMACSHA1
            case DBGACTION_HMACSHA1:
                hmacsha1((void*)dbgactionaddr, dbgactionlength, (void*)dbgactionoffset);
                dbgasyncsendbuf[0] = 1;
                usb_drv_send_nonblocking(dbgendpoints[1], dbgasyncsendbuf, 16);
                break;
#endif
#ifdef USB_HAVE_TARGET_SPECIFIC_REQUESTS
            case DBGACTION_TARGETSPECIFIC:
            {
                int size = usb_target_handle_request(dbgasyncsendbuf, sizeof(dbgasyncsendbuf));
                if (size) usb_drv_send_nonblocking(dbgendpoints[1], dbgasyncsendbuf, size);
                break;
            }
#endif
#ifdef HAVE_STORAGE
            case DBGACTION_STORAGE:
                switch(dbgasyncsendbuf[0])
                {
                case 27:  // STORAGE_GET_INFO
                    dbgasyncsendbuf[0] = 1;
                    storage_get_info(dbgasyncsendbuf[1],
                                     (struct storage_info*)&dbgasyncsendbuf[4]);
                    dbgasyncsendbuf[1] = 1;
                    usb_drv_send_nonblocking(dbgendpoints[1], dbgasyncsendbuf,
                                             16 + sizeof(struct storage_info));
                    break;
                case 28:  // STORAGE_READ_SECTORS_MD
                {
                    dbgasyncsendbuf[0] = 1;
                    int rc = storage_read_sectors_md(dbgasyncsendbuf[1],
                                                     dbgasyncsendbuf[2]
                                                   | (((uint64_t)(dbgasyncsendbuf[3]) << 32)),
                                                     dbgasyncsendbuf[4],
                                                     (void*)(dbgasyncsendbuf[5]));
                    dbgasyncsendbuf[1] = (uint32_t)rc;
                    usb_drv_send_nonblocking(dbgendpoints[1], dbgasyncsendbuf, 16);
                    break;
                }
                case 29:  // STORAGE_WRITE_SECTORS_MD
                {
                    dbgasyncsendbuf[0] = 1;
                    int rc = storage_write_sectors_md(dbgasyncsendbuf[1],
                                                      dbgasyncsendbuf[2]
                                                    | (((uint64_t)(dbgasyncsendbuf[3]) << 32)),
                                                      dbgasyncsendbuf[4],
                                                      (void*)(dbgasyncsendbuf[5]));
                    dbgasyncsendbuf[1] = (uint32_t)rc;
                    usb_drv_send_nonblocking(dbgendpoints[1], dbgasyncsendbuf, 16);
                    break;
                }
                case 30:  // FILE_OPEN
                {
                    dbgasyncsendbuf[0] = 1;
                    int fd = file_open((char*)(&dbgasyncsendbuf[4]), (int)(dbgasyncsendbuf[1]));
                    dbgasyncsendbuf[1] = (uint32_t)fd;
                    usb_drv_send_nonblocking(dbgendpoints[1], dbgasyncsendbuf, 16);
                    break;
                }
                case 31:  // FILESIZE
                    dbgasyncsendbuf[0] = 1;
                    dbgasyncsendbuf[1] = (uint32_t)filesize((int)(dbgasyncsendbuf[1]));
                    usb_drv_send_nonblocking(dbgendpoints[1], dbgasyncsendbuf, 16);
                    break;
                case 32:  // READ
                    dbgasyncsendbuf[0] = 1;
                    dbgasyncsendbuf[1] = (uint32_t)read((int)(dbgasyncsendbuf[1]),
                                                        (void*)(dbgasyncsendbuf[2]),
                                                        (size_t)(dbgasyncsendbuf[3]));
                    usb_drv_send_nonblocking(dbgendpoints[1], dbgasyncsendbuf, 16);
                    break;
                case 33:  // WRITE
                    dbgasyncsendbuf[0] = 1;
                    dbgasyncsendbuf[1] = (uint32_t)write((int)(dbgasyncsendbuf[1]),
                                                         (void*)(dbgasyncsendbuf[2]),
                                                         (size_t)(dbgasyncsendbuf[3]));
                    usb_drv_send_nonblocking(dbgendpoints[1], dbgasyncsendbuf, 16);
                    break;
                case 34:  // LSEEK
                    dbgasyncsendbuf[0] = 1;
                    dbgasyncsendbuf[1] = (uint32_t)lseek((int)(dbgasyncsendbuf[1]),
                                                         (off_t)(dbgasyncsendbuf[2]),
                                                         (int)(dbgasyncsendbuf[3]));
                    usb_drv_send_nonblocking(dbgendpoints[1], dbgasyncsendbuf, 16);
                    break;
                case 35:  // FTRUNCATE
                    dbgasyncsendbuf[0] = 1;
                    dbgasyncsendbuf[1] = (uint32_t)ftruncate((int)(dbgasyncsendbuf[1]),
                                                             (off_t)(dbgasyncsendbuf[2]));
                    usb_drv_send_nonblocking(dbgendpoints[1], dbgasyncsendbuf, 16);
                    break;
                case 36:  // FSYNC
                    dbgasyncsendbuf[0] = 1;
                    dbgasyncsendbuf[1] = (uint32_t)fsync((int)(dbgasyncsendbuf[1]));
                    usb_drv_send_nonblocking(dbgendpoints[1], dbgasyncsendbuf, 16);
                    break;
                case 37:  // CLOSE
                    dbgasyncsendbuf[0] = 1;
                    dbgasyncsendbuf[1] = (uint32_t)close((int)(dbgasyncsendbuf[1]));
                    usb_drv_send_nonblocking(dbgendpoints[1], dbgasyncsendbuf, 16);
                    break;
                case 38:  // CLOSE_MONITOR_FILES
                    dbgasyncsendbuf[0] = 1;
                    dbgasyncsendbuf[1] = (uint32_t)close_all_of_process(current_thread);
                    usb_drv_send_nonblocking(dbgendpoints[1], dbgasyncsendbuf, 16);
                    break;
                case 39:  // RELEASE_FILES
                    dbgasyncsendbuf[0] = 1;
                    dbgasyncsendbuf[1] = (uint32_t)release_files((int)(dbgasyncsendbuf[1]));
                    usb_drv_send_nonblocking(dbgendpoints[1], dbgasyncsendbuf, 16);
                    break;
                case 40:  // REMOVE
                    dbgasyncsendbuf[0] = 1;
                    dbgasyncsendbuf[1] = (uint32_t)remove((char*)(&dbgasyncsendbuf[4]));
                    usb_drv_send_nonblocking(dbgendpoints[1], dbgasyncsendbuf, 16);
                    break;
                case 41:  // RENAME
                    dbgasyncsendbuf[0] = 1;
                    dbgasyncsendbuf[1] = (uint32_t)rename((char*)(&dbgasyncsendbuf[4]),
                                                          (char*)(&dbgasyncsendbuf[66]));
                    usb_drv_send_nonblocking(dbgendpoints[1], dbgasyncsendbuf, 16);
                    break;
                case 42:  // OPENDIR
                    dbgasyncsendbuf[0] = 1;
                    dbgasyncsendbuf[1] = (uint32_t)opendir((char*)(&dbgasyncsendbuf[4]));
                    usb_drv_send_nonblocking(dbgendpoints[1], dbgasyncsendbuf, 16);
                    break;
                case 43:  // READDIR
                    dbgasyncsendbuf[0] = 1;
                    dbgasyncsendbuf[3] = (uint32_t)readdir((DIR*)(dbgasyncsendbuf[1]));
                    dbgasyncsendbuf[1] = 1;
                    dbgasyncsendbuf[2] = MAX_PATH;
                    usb_drv_send_nonblocking(dbgendpoints[1], dbgasyncsendbuf, 16);
                    break;
                case 44:  // CLOSEDIR
                    dbgasyncsendbuf[0] = 1;
                    dbgasyncsendbuf[1] = (uint32_t)closedir((DIR*)(dbgasyncsendbuf[1]));
                    usb_drv_send_nonblocking(dbgendpoints[1], dbgasyncsendbuf, 16);
                    break;
                case 45:  // CLOSE_MONITOR_DIRS
                    dbgasyncsendbuf[0] = 1;
                    dbgasyncsendbuf[1] = (uint32_t)closedir_all_of_process(current_thread);
                    usb_drv_send_nonblocking(dbgendpoints[1], dbgasyncsendbuf, 16);
                    break;
                case 46:  // RELEASE_DIRS
                    dbgasyncsendbuf[0] = 1;
                    dbgasyncsendbuf[1] = (uint32_t)release_dirs((int)(dbgasyncsendbuf[1]));
                    usb_drv_send_nonblocking(dbgendpoints[1], dbgasyncsendbuf, 16);
                    break;
                case 47:  // MKDIR
                    dbgasyncsendbuf[0] = 1;
                    dbgasyncsendbuf[1] = (uint32_t)mkdir((char*)(&dbgasyncsendbuf[4]));
                    usb_drv_send_nonblocking(dbgendpoints[1], dbgasyncsendbuf, 16);
                    break;
                case 48:  // RMDIR
                    dbgasyncsendbuf[0] = 1;
                    dbgasyncsendbuf[1] = (uint32_t)rmdir((char*)(&dbgasyncsendbuf[4]));
                    usb_drv_send_nonblocking(dbgendpoints[1], dbgasyncsendbuf, 16);
                    break;
                case 49:  // ERRNO
                    dbgasyncsendbuf[0] = 1;
                    dbgasyncsendbuf[1] = (uint32_t)errno;
                    usb_drv_send_nonblocking(dbgendpoints[1], dbgasyncsendbuf, 16);
                    break;
#ifdef HAVE_HOTSWAP
                case 50:  // DISK_MOUNT
                    dbgasyncsendbuf[0] = 1;
                    dbgasyncsendbuf[1] = (uint32_t)disk_mount((int)(dbgasyncsendbuf[1]));
                    usb_drv_send_nonblocking(dbgendpoints[1], dbgasyncsendbuf, 16);
                    break;
                case 51:  // DISK_UNMOUNT
                    dbgasyncsendbuf[0] = 1;
                    dbgasyncsendbuf[1] = (uint32_t)disk_unmount((int)(dbgasyncsendbuf[1]));
                    usb_drv_send_nonblocking(dbgendpoints[1], dbgasyncsendbuf, 16);
                    break;
#endif
                }
                break;
#endif
            }
            dbgaction = DBGACTION_IDLE;
        }
    }
}

void usb_init(void)
{
    dbgaction = DBGACTION_IDLE;
    wakeup_init(&dbgwakeup);
    dbgconsendreadidx = 0;
    dbgconsendwriteidx = 0;
    dbgconrecvreadidx = 0;
    dbgconrecvwriteidx = 0;
    wakeup_init(&dbgconsendwakeup);
    wakeup_init(&dbgconrecvwakeup);
    dbgconsoleattached = false;
    thread_create("monitor worker", dbgthread, dbgstack, sizeof(dbgstack), CORE_THREAD, 255, true);
    usb_drv_init();
}

int dbgconsole_getfree() ICODE_ATTR;
int dbgconsole_getfree()
{
    int free = dbgconsendreadidx - dbgconsendwriteidx - 1;
    if (free < 0) free += sizeof(dbgconsendbuf);
    return free;
}

int dbgconsole_makespace(int length) ICODE_ATTR;
int dbgconsole_makespace(int length)
{
    int free = dbgconsole_getfree();
    while (!free && dbgconsoleattached)
    {
        dbgconsoleattached = false;
        wakeup_wait(&dbgconsendwakeup, 2000000);
        free = dbgconsole_getfree();
    }
    if (free) return free > length ? length : free;
    if (length > sizeof(dbgconsendbuf) - 17) length = sizeof(dbgconsendbuf) - 17;
    uint32_t mode = enter_critical_section();
    dbgconsendreadidx += length;
    if (dbgconsendreadidx >= sizeof(dbgconsendbuf))
        dbgconsendreadidx -= sizeof(dbgconsendbuf);
    int offset = 0;
    int idx = dbgconsendreadidx;
    if (idx + 16 >= sizeof(dbgconsendbuf))
    {
        offset = sizeof(dbgconsendbuf) - dbgconsendreadidx;
        memcpy(&dbgconsendbuf[dbgconsendreadidx], dbgconoverflowstr, offset);
        idx = 0;
    }
    if (offset != 16) memcpy(&dbgconsendbuf[idx], &dbgconoverflowstr[offset], 16 - offset);
    leave_critical_section(mode);
    return length;
}

void dbgconsole_putc(char string)
{
    dbgconsole_makespace(1);
    dbgconsendbuf[dbgconsendwriteidx++] = string;
    if (dbgconsendwriteidx >= sizeof(dbgconsendbuf))
        dbgconsendwriteidx -= sizeof(dbgconsendbuf);
}

void dbgconsole_write(const char* string, size_t length)
{
    while (length)
    {
        int space = dbgconsole_makespace(length);
        if (dbgconsendwriteidx + space >= sizeof(dbgconsendbuf))
        {
            int bytes = sizeof(dbgconsendbuf) - dbgconsendwriteidx;
            memcpy(&dbgconsendbuf[dbgconsendwriteidx], string, bytes);
            dbgconsendwriteidx = 0;
            string = &string[bytes];
            space -= bytes;
            length -= bytes;
        }
        if (space) memcpy(&dbgconsendbuf[dbgconsendwriteidx], string, space);
        dbgconsendwriteidx += space;
        string = &string[space];
        length -= space;
    }
}

void dbgconsole_puts(const char* string)
{
    dbgconsole_write(string, strlen(string));
}

int dbgconsole_getavailable() ICODE_ATTR;
int dbgconsole_getavailable()
{
    int available = dbgconrecvwriteidx - dbgconrecvreadidx;
    if (available < 0) available += sizeof(dbgconrecvbuf);
    return available;
}

int dbgconsole_getc(int timeout)
{
    if (!dbgconsole_getavailable())
    {
        wakeup_wait(&dbgconrecvwakeup, TIMEOUT_NONE);
        if (!dbgconsole_getavailable())
        {
            wakeup_wait(&dbgconrecvwakeup, timeout);
            if (!dbgconsole_getavailable()) return -1;
        }
    }
    int byte = dbgconrecvbuf[dbgconrecvreadidx++];
    if (dbgconrecvreadidx >= sizeof(dbgconrecvbuf))
        dbgconrecvreadidx -= sizeof(dbgconrecvbuf);
    return byte;
}

int dbgconsole_read(char* buffer, size_t length, int timeout)
{
    if (!length) return 0;
    int available = dbgconsole_getavailable();
    if (!available)
    {
        wakeup_wait(&dbgconrecvwakeup, TIMEOUT_NONE);
        int available = dbgconsole_getavailable();
        if (!available)
        {
            wakeup_wait(&dbgconrecvwakeup, timeout);
            int available = dbgconsole_getavailable();
            if (!available) return 0;
        }
    }
    if (available > length) available = length;
    int left = available;
    if (dbgconrecvreadidx + available >= sizeof(dbgconrecvbuf))
    {
        int bytes = sizeof(dbgconrecvbuf) - dbgconrecvreadidx;
        memcpy(buffer, &dbgconrecvbuf[dbgconrecvreadidx], bytes);
        dbgconrecvreadidx = 0;
        buffer = &buffer[bytes];
        left -= bytes;
    }
    if (left) memcpy(buffer, &dbgconrecvbuf[dbgconrecvreadidx], left);
    dbgconrecvreadidx += left;
    return available;
}

void usb_exit(void)
{
    usb_drv_exit();
}
