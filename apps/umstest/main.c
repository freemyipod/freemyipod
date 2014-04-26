//
//
//    Copyright 2013 TheSeven
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


#define LITTLE_ENDIAN


#include "emcoreapp.h"


#define SCSI_TEST_UNIT_READY        0x00
#define SCSI_INQUIRY                0x12
#define SCSI_MODE_SENSE_6           0x1a
#define SCSI_MODE_SENSE_10          0x5a
#define SCSI_REQUEST_SENSE          0x03
#define SCSI_ALLOW_MEDIUM_REMOVAL   0x1e
#define SCSI_READ_CAPACITY          0x25
#define SCSI_READ_FORMAT_CAPACITY   0x23
#define SCSI_READ_10                0x28
#define SCSI_WRITE_10               0x2a
#define SCSI_START_STOP_UNIT        0x1b
#define SCSI_REPORT_LUNS            0xa0
#define SCSI_WRITE_BUFFER           0x3b

#define SENSE_NOT_READY             0x02
#define SENSE_MEDIUM_ERROR          0x03
#define SENSE_ILLEGAL_REQUEST       0x05
#define SENSE_UNIT_ATTENTION        0x06

#define ASC_MEDIUM_NOT_PRESENT      0x3a
#define ASC_INVALID_FIELD_IN_CBD    0x24
#define ASC_LBA_OUT_OF_RANGE        0x21
#define ASC_WRITE_ERROR             0x0C
#define ASC_READ_ERROR              0x11
#define ASC_NOT_READY               0x04
#define ASC_INVALID_COMMAND         0x20

#define ASCQ_BECOMING_READY         0x01

#define DIRECT_ACCESS_DEVICE        0x00
#define DEVICE_REMOVABLE            0x80

#define SCSI_FORMAT_CAPACITY_FORMATTED_MEDIA 0x02000000


#define RAMDISK_SECTORSIZE 512
#define RAMDISK_SECTORS 2048


static const struct usb_devicedescriptor usb_devicedescriptor =
{
    .bLength = sizeof(struct usb_devicedescriptor),
    .bDescriptorType = USB_DESCRIPTOR_TYPE_DEVICE,
    .bcdUSB = 0x0200,
    .bDeviceClass = 0,
    .bDeviceSubClass = 0,
    .bDeviceProtocol = 0,
    .bMaxPacketSize0 = 64,
    .idVendor = 0xffff,
    .idProduct = 0xe001,
    .bcdDevice = 2,
    .iManufacturer = 1,
    .iProduct = 2,
    .iSerialNumber = 0,
    .bNumConfigurations = 1,
};

static struct __attribute__((packed)) _usb_config1_descriptors
{
    struct usb_configurationdescriptor c1;
    struct usb_interfacedescriptor c1_i0_a0;
    struct usb_endpointdescriptor c1_i0_a0_e1out;
    struct usb_endpointdescriptor c1_i0_a0_e1in;
} usb_config1_descriptors =
{
    .c1 =
    {
        .bLength = sizeof(struct usb_configurationdescriptor),
        .bDescriptorType = USB_DESCRIPTOR_TYPE_CONFIGURATION,
        .wTotalLength = sizeof(struct _usb_config1_descriptors),
        .bNumInterfaces = 1,
        .bConfigurationValue = 1,
        .iConfiguration = 0,
        .bmAttributes = { .buspowered = 1, .selfpowered = 1 },
        .bMaxPower = 100 / 2,
    },
    .c1_i0_a0 =
    {
        .bLength = sizeof(struct usb_interfacedescriptor),
        .bDescriptorType = USB_DESCRIPTOR_TYPE_INTERFACE,
        .bInterfaceNumber = 0,
        .bAlternateSetting = 0,
        .bNumEndpoints = 2,
        .bInterfaceClass = 0x08,
        .bInterfaceSubClass = 0x06,
        .bInterfaceProtocol = 0x50,
        .iInterface = 0,
    },
    .c1_i0_a0_e1out =
    {
        .bLength = sizeof(struct usb_endpointdescriptor),
        .bDescriptorType = USB_DESCRIPTOR_TYPE_ENDPOINT,
        .bEndpointAddress = { .number = 0, .direction = USB_ENDPOINT_DIRECTION_OUT },
        .bmAttributes = { .type = USB_ENDPOINT_ATTRIBUTE_TYPE_BULK },
        .wMaxPacketSize = 512,
        .bInterval = 1,
    },
    .c1_i0_a0_e1in =
    {
        .bLength = sizeof(struct usb_endpointdescriptor),
        .bDescriptorType = USB_DESCRIPTOR_TYPE_ENDPOINT,
        .bEndpointAddress = { .number = 0, .direction = USB_ENDPOINT_DIRECTION_IN },
        .bmAttributes = { .type = USB_ENDPOINT_ATTRIBUTE_TYPE_BULK },
        .wMaxPacketSize = 512,
        .bInterval = 1,
    },
};

static const struct usb_stringdescriptor usb_string_language =
{
    .bLength = sizeof(usb_string_language) + sizeof(*usb_string_language.wString),
    .bDescriptorType = USB_DESCRIPTOR_TYPE_STRING,
    .wString = { 0x0409 },
};

static const struct usb_stringdescriptor usb_string_vendor =
{
    .bLength = sizeof(usb_string_vendor) + sizeof(*usb_string_vendor.wString) * 14,
    .bDescriptorType = USB_DESCRIPTOR_TYPE_STRING,
    .wString = { 'f', 'r', 'e', 'e', 'm', 'y', 'i', 'p', 'o', 'd', '.', 'o', 'r', 'g' },
};

static const struct usb_stringdescriptor usb_string_product =
{
    .bLength = sizeof(usb_string_product) + sizeof(*usb_string_product.wString) * 28,
    .bDescriptorType = USB_DESCRIPTOR_TYPE_STRING,
    .wString = { 'e', 'm', 'C', 'O', 'R', 'E', ' ', 'U', 'S', 'B', ' ', 'm', 'a', 's', 's', ' ', 's', 't', 'o', 'r', 'a', 'g', 'e', ' ', 't', 'e', 's', 't' },
};

static const struct usb_stringdescriptor* usb_stringdescriptors[] =
{
    &usb_string_language,
    &usb_string_vendor,
    &usb_string_product,
};

struct __attribute__((packed,aligned(32))) command_block_wrapper
{
    unsigned int signature;
    unsigned int tag;
    unsigned int data_transfer_length;
    unsigned char flags;
    unsigned char lun;
    unsigned char command_length;
    unsigned char command_block[16];
};

struct __attribute__((packed)) command_status_wrapper
{
    unsigned int signature;
    unsigned int tag;
    unsigned int data_residue;
    unsigned char status;
};

struct __attribute__((packed)) inquiry_data
{
    unsigned char DeviceType;
    unsigned char DeviceTypeModifier;
    unsigned char Versions;
    unsigned char Format;
    unsigned char AdditionalLength;
    unsigned char Reserved[2];
    unsigned char Capability;
    char VendorId[8];
    char ProductId[16];
    char ProductRevisionLevel[4];
};

struct __attribute__((packed)) report_lun_data
{
    unsigned int lun_list_length;
    unsigned int reserved1;
    unsigned char luns[1][8];
};

struct __attribute__((packed)) sense_data
{
    unsigned char ResponseCode;
    unsigned char Obsolete;
    unsigned char fei_sensekey;
    unsigned int Information;
    unsigned char AdditionalSenseLength;
    unsigned int  CommandSpecificInformation;
    unsigned char AdditionalSenseCode;
    unsigned char AdditionalSenseCodeQualifier;
    unsigned char FieldReplaceableUnitCode;
    unsigned char SKSV;
    unsigned short SenseKeySpecific;
};

struct __attribute__((packed)) mode_sense_bdesc_longlba
{
    unsigned char num_blocks[8];
    unsigned char reserved[4];
    unsigned char block_size[4];
};

struct __attribute__((packed)) mode_sense_bdesc_shortlba
{
    unsigned char density_code;
    unsigned char num_blocks[3];
    unsigned char reserved;
    unsigned char block_size[3];
};

struct __attribute__((packed)) mode_sense_data_10
{
    unsigned short mode_data_length;
    unsigned char medium_type;
    unsigned char device_specific;
    unsigned char longlba;
    unsigned char reserved;
    unsigned short block_descriptor_length;
    struct mode_sense_bdesc_longlba block_descriptor;
};

struct __attribute__((packed)) mode_sense_data_6
{
    unsigned char mode_data_length;
    unsigned char medium_type;
    unsigned char device_specific;
    unsigned char block_descriptor_length;
    struct mode_sense_bdesc_shortlba block_descriptor;
};

struct __attribute__((packed)) capacity
{
    unsigned int block_count;
    unsigned int block_size;
};

struct __attribute__((packed)) format_capacity
{
    unsigned int following_length;
    unsigned int block_count;
    unsigned int block_size;
};

union __attribute__((aligned(32)))
{
    struct inquiry_data inquiry;
    struct capacity capacity_data;
    struct format_capacity format_capacity_data;
    struct sense_data sense_data;
    struct mode_sense_data_6 ms_data_6;
    struct mode_sense_data_10 ms_data_10;
    struct report_lun_data lun_data;
    struct command_status_wrapper csw;
} tb;

static enum
{
    WAITING_FOR_COMMAND,
    SENDING_BLOCKS,
    SENDING_RESULT,
    SENDING_FAILED_RESULT,
    RECEIVING_BLOCKS,
    WAITING_FOR_CSW_COMPLETION
} state = WAITING_FOR_COMMAND;

static struct
{
    unsigned int sector;
    unsigned int count;
    unsigned int orig_count;
    unsigned int cur_cmd;
    unsigned int tag;
    unsigned int lun;
    unsigned int last_result;
} cur_cmd;

static struct
{
    unsigned char sense_key;
    unsigned char information;
    unsigned char asc;
    unsigned char ascq;
} cur_sense_data;

static union usb_endpoint_number outep = { .number = 0, .direction = USB_ENDPOINT_DIRECTION_OUT };
static union usb_endpoint_number inep = { .number = 0, .direction = USB_ENDPOINT_DIRECTION_IN };

static struct command_block_wrapper cbw;
static int maxpacket = 512;
static int maxlen;
static int length;
static bool locked;
static const struct usb_instance* usb;
static volatile bool ejected = false;
static uint8_t __attribute__((aligned(32))) ramdisk[RAMDISK_SECTORS][RAMDISK_SECTORSIZE];

static void listen()
{
    usb_start_rx(usb, outep, &cbw, sizeof(cbw));
}

static void send_csw(int status)
{
    tb.csw.signature = 0x53425355;
    tb.csw.tag = cur_cmd.tag;
    tb.csw.data_residue = 0;
    tb.csw.status = status;

    state = WAITING_FOR_CSW_COMPLETION;
    usb_start_tx(usb, inep, &tb.csw, sizeof(tb.csw));

    if (!status)
    {
        cur_sense_data.sense_key = 0;
        cur_sense_data.information = 0;
        cur_sense_data.asc = 0;
        cur_sense_data.ascq = 0;
    }
}

static void receive_block_data(void* data, int size)
{
    length = size;
    usb_start_rx(usb, outep, data, size);
    state = RECEIVING_BLOCKS;
}

static void send_block_data(void* data, int size)
{
    length = size;
    usb_start_tx(usb, inep, data, size);
    state = SENDING_BLOCKS;
}

static void send_command_result(void* data, int size)
{
    length = size;
    usb_start_tx(usb, inep, data, size);
    state = SENDING_RESULT;
}

static void send_command_failed_result()
{
    usb_start_tx(usb, inep, NULL, 0);
    state = SENDING_FAILED_RESULT;
}

static void send_and_read_next()
{
    if (cur_cmd.last_result)
    {
        send_csw(1);
        cur_sense_data.sense_key = SENSE_MEDIUM_ERROR;
        cur_sense_data.asc = ASC_READ_ERROR;
        cur_sense_data.ascq = 0;
        return;
    }
    send_block_data(ramdisk[cur_cmd.sector], MIN(maxlen, cur_cmd.count * RAMDISK_SECTORSIZE));

    cur_cmd.sector += maxlen / RAMDISK_SECTORSIZE;
    cur_cmd.count -= MIN(cur_cmd.count, maxlen / RAMDISK_SECTORSIZE);
}

static void copy_padded(char* dest, char* src, int len)
{
   int i = 0;
   while (src[i] && i < len)
   {
      dest[i] = src[i];
      i++;
   }
   while(i < len) dest[i++] = ' ';
}

static void fill_inquiry()
{
    memset(&tb.inquiry, 0, sizeof(tb.inquiry));
    copy_padded(tb.inquiry.VendorId, "emCORE", sizeof(tb.inquiry.VendorId));
    copy_padded(tb.inquiry.ProductId, "UMS Test", sizeof(tb.inquiry.ProductId));
    copy_padded(tb.inquiry.ProductRevisionLevel, "0.1", sizeof(tb.inquiry.ProductRevisionLevel));

    tb.inquiry.DeviceType = DIRECT_ACCESS_DEVICE;
    tb.inquiry.AdditionalLength = 0x1f;
    tb.inquiry.Versions = 4;  // SPC-2
    tb.inquiry.Format = 2;  // SPC-2/3 inquiry format
    tb.inquiry.DeviceTypeModifier = DEVICE_REMOVABLE;
}

static void handle_scsi(struct command_block_wrapper* cbw)
{
    unsigned int length = cbw->data_transfer_length;

    if (cbw->signature != 0x43425355)
    {
        usb_set_stall(usb, outep, true);
        usb_set_stall(usb, inep, true);
        return;
    }
    cur_cmd.tag = cbw->tag;
    cur_cmd.lun = cbw->lun;
    cur_cmd.cur_cmd = cbw->command_block[0];

    switch (cbw->command_block[0])
    {
        case SCSI_TEST_UNIT_READY:
            if (!ejected) send_csw(0);
            else
            {
                send_csw(1);
                cur_sense_data.sense_key = SENSE_NOT_READY;
                cur_sense_data.asc = ASC_MEDIUM_NOT_PRESENT;
                cur_sense_data.ascq = 0;
            }
            break;

        case SCSI_REPORT_LUNS:
        {
            memset(&tb.lun_data, 0, sizeof(struct report_lun_data));
            tb.lun_data.lun_list_length = 0x08000000;
            send_command_result(&tb.lun_data, MIN(16, length));
            break;
        }

        case SCSI_INQUIRY:
            fill_inquiry();
            length = MIN(length, cbw->command_block[4]);
            send_command_result(&tb.inquiry, MIN(sizeof(tb.inquiry), length));
            break;

        case SCSI_REQUEST_SENSE:
        {
            tb.sense_data.ResponseCode = 0x70;
            tb.sense_data.Obsolete = 0;
            tb.sense_data.fei_sensekey = cur_sense_data.sense_key & 0x0f;
            tb.sense_data.Information = cur_sense_data.information;
            tb.sense_data.AdditionalSenseLength = 10;
            tb.sense_data.CommandSpecificInformation = 0;
            tb.sense_data.AdditionalSenseCode = cur_sense_data.asc;
            tb.sense_data.AdditionalSenseCodeQualifier = cur_sense_data.ascq;
            tb.sense_data.FieldReplaceableUnitCode = 0;
            tb.sense_data.SKSV = 0;
            tb.sense_data.SenseKeySpecific = 0;
            send_command_result(&tb.sense_data, MIN(sizeof(tb.sense_data), length));
            break;
        }

        case SCSI_MODE_SENSE_10:
        {
            if (ejected)
            {
                send_command_failed_result();
                cur_sense_data.sense_key = SENSE_NOT_READY;
                cur_sense_data.asc = ASC_MEDIUM_NOT_PRESENT;
                cur_sense_data.ascq = 0;
                break;
            }
            unsigned char page_code = cbw->command_block[2] & 0x3f;
            switch (page_code)
            {
                case 0x3f:
                    tb.ms_data_10.mode_data_length = htobe16(sizeof(tb.ms_data_10) - 2);
                    tb.ms_data_10.medium_type = 0;
                    tb.ms_data_10.device_specific = 0;
                    tb.ms_data_10.reserved = 0;
                    tb.ms_data_10.longlba = 1;
                    tb.ms_data_10.block_descriptor_length = htobe16(sizeof(tb.ms_data_10.block_descriptor));

                    memset(tb.ms_data_10.block_descriptor.reserved, 0, 4);
                    memset(tb.ms_data_10.block_descriptor.num_blocks, 0, 8);

                    tb.ms_data_10.block_descriptor.num_blocks[4] = (RAMDISK_SECTORS & 0xff000000) >> 24;
                    tb.ms_data_10.block_descriptor.num_blocks[5] = (RAMDISK_SECTORS & 0x00ff0000) >> 16;
                    tb.ms_data_10.block_descriptor.num_blocks[6] = (RAMDISK_SECTORS & 0x0000ff00) >> 8;
                    tb.ms_data_10.block_descriptor.num_blocks[7] = (RAMDISK_SECTORS & 0x000000ff);

                    tb.ms_data_10.block_descriptor.block_size[0] = (RAMDISK_SECTORSIZE & 0xff000000) >> 24;
                    tb.ms_data_10.block_descriptor.block_size[1] = (RAMDISK_SECTORSIZE & 0x00ff0000) >> 16;
                    tb.ms_data_10.block_descriptor.block_size[2] = (RAMDISK_SECTORSIZE & 0x0000ff00) >> 8;
                    tb.ms_data_10.block_descriptor.block_size[3] = (RAMDISK_SECTORSIZE & 0x000000ff);
                    send_command_result(&tb.ms_data_10, MIN(sizeof(tb.ms_data_10), length));
                    break;
                default:
                    send_command_failed_result();
                    cur_sense_data.sense_key = SENSE_ILLEGAL_REQUEST;
                    cur_sense_data.asc = ASC_INVALID_FIELD_IN_CBD;
                    cur_sense_data.ascq = 0;
                    break;
            }
            break;
        }

        case SCSI_MODE_SENSE_6:
        {
            if (ejected)
            {
                send_command_failed_result();
                cur_sense_data.sense_key = SENSE_NOT_READY;
                cur_sense_data.asc = ASC_MEDIUM_NOT_PRESENT;
                cur_sense_data.ascq = 0;
                break;
            }
            unsigned char page_code = cbw->command_block[2] & 0x3f;
            switch (page_code)
            {
                case 0x3f:
                    tb.ms_data_6.mode_data_length = sizeof(tb.ms_data_6) - 1;
                    tb.ms_data_6.medium_type = 0;
                    tb.ms_data_6.device_specific = 0;
                    tb.ms_data_6.block_descriptor_length = sizeof(tb.ms_data_6.block_descriptor);
                    tb.ms_data_6.block_descriptor.density_code = 0;
                    tb.ms_data_6.block_descriptor.reserved = 0;
                    if (RAMDISK_SECTORS > 0xffffff)
                    {
                        tb.ms_data_6.block_descriptor.num_blocks[0] = 0xff;
                        tb.ms_data_6.block_descriptor.num_blocks[1] = 0xff;
                        tb.ms_data_6.block_descriptor.num_blocks[2] = 0xff;
                    }
                    else
                    {
                        tb.ms_data_6.block_descriptor.num_blocks[0] = (RAMDISK_SECTORS & 0xff0000) >> 16;
                        tb.ms_data_6.block_descriptor.num_blocks[1] = (RAMDISK_SECTORS & 0x00ff00) >> 8;
                        tb.ms_data_6.block_descriptor.num_blocks[2] = (RAMDISK_SECTORS & 0x0000ff);
                    }
                    tb.ms_data_6.block_descriptor.block_size[0] = (RAMDISK_SECTORSIZE & 0xff0000) >> 16;
                    tb.ms_data_6.block_descriptor.block_size[1] = (RAMDISK_SECTORSIZE & 0x00ff00) >> 8;
                    tb.ms_data_6.block_descriptor.block_size[2] = (RAMDISK_SECTORSIZE & 0x0000ff);
                    send_command_result(&tb.ms_data_6, MIN(sizeof(tb.ms_data_6), length));
                    break;
                default:
                    send_command_failed_result();
                    cur_sense_data.sense_key = SENSE_ILLEGAL_REQUEST;
                    cur_sense_data.asc = ASC_INVALID_FIELD_IN_CBD;
                    cur_sense_data.ascq = 0;
                    break;
            }
            break;
        }

        case SCSI_START_STOP_UNIT:
            if ((cbw->command_block[4] & 0xf3) == 2) ejected = true;
            send_csw(0);
            break;

        case SCSI_ALLOW_MEDIUM_REMOVAL:
            if ((cbw->command_block[4] & 0x03) == 0) locked = false;
            else locked = true;
            send_csw(0);
            break;

        case SCSI_READ_FORMAT_CAPACITY:
        {
            if (!ejected)
            {
                tb.format_capacity_data.following_length = 0x08000000;
                tb.format_capacity_data.block_count = htobe32(RAMDISK_SECTORS - 1);
                tb.format_capacity_data.block_size = htobe32(RAMDISK_SECTORSIZE);
                tb.format_capacity_data.block_size |= htobe32(SCSI_FORMAT_CAPACITY_FORMATTED_MEDIA);
                send_command_result(&tb.format_capacity_data, MIN(sizeof(tb.format_capacity_data), length));
           }
           else
           {
               send_command_failed_result();
               cur_sense_data.sense_key = SENSE_NOT_READY;
               cur_sense_data.asc = ASC_MEDIUM_NOT_PRESENT;
               cur_sense_data.ascq = 0;
           }
           break;
        }

        case SCSI_READ_CAPACITY:
        {
            if (!ejected)
            {
                tb.capacity_data.block_count = htobe32(RAMDISK_SECTORS - 1);
                tb.capacity_data.block_size = htobe32(RAMDISK_SECTORSIZE);
                send_command_result(&tb.capacity_data, MIN(sizeof(tb.capacity_data), length));
            }
            else
            {
                send_command_failed_result();
                cur_sense_data.sense_key = SENSE_NOT_READY;
                cur_sense_data.asc = ASC_MEDIUM_NOT_PRESENT;
                cur_sense_data.ascq = 0;
            }
            break;
        }

        case SCSI_READ_10:
            if (ejected)
            {
                send_command_failed_result();
                cur_sense_data.sense_key = SENSE_NOT_READY;
                cur_sense_data.asc = ASC_MEDIUM_NOT_PRESENT;
                cur_sense_data.ascq = 0;
                break;
            }
            cur_cmd.sector = (cbw->command_block[2] << 24 | cbw->command_block[3] << 16
                            | cbw->command_block[4] << 8 | cbw->command_block[5]);
            cur_cmd.count = (cbw->command_block[7] << 8 | cbw->command_block[8]);
            cur_cmd.orig_count = cur_cmd.count;

            if ((cur_cmd.sector + cur_cmd.count) > RAMDISK_SECTORS)
            {
                send_csw(1);
                cur_sense_data.sense_key = SENSE_ILLEGAL_REQUEST;
                cur_sense_data.asc = ASC_LBA_OUT_OF_RANGE;
                cur_sense_data.ascq = 0;
            }
            else send_and_read_next();
            break;

        case SCSI_WRITE_10:
            if (ejected)
            {
                send_command_failed_result();
                cur_sense_data.sense_key = SENSE_NOT_READY;
                cur_sense_data.asc = ASC_MEDIUM_NOT_PRESENT;
                cur_sense_data.ascq = 0;
                break;
            }
            cur_cmd.sector = (cbw->command_block[2] << 24 | cbw->command_block[3] << 16
                            | cbw->command_block[4] << 8 | cbw->command_block[5]);
            cur_cmd.count = (cbw->command_block[7] << 8 | cbw->command_block[8]);
            cur_cmd.orig_count = cur_cmd.count;

            if ((cur_cmd.sector + cur_cmd.count) > RAMDISK_SECTORS)
            {
                send_csw(1);
                cur_sense_data.sense_key = SENSE_ILLEGAL_REQUEST;
                cur_sense_data.asc = ASC_LBA_OUT_OF_RANGE;
                cur_sense_data.ascq = 0;
            }
            else
                receive_block_data(ramdisk[cur_cmd.sector], MIN(maxlen, cur_cmd.count * RAMDISK_SECTORSIZE));
            break;

        case SCSI_WRITE_BUFFER:
            break;

        default:
            send_csw(1);
            cur_sense_data.sense_key = SENSE_ILLEGAL_REQUEST;
            cur_sense_data.asc = ASC_INVALID_COMMAND;
            cur_sense_data.ascq = 0;
            break;
    }
}

int handle_ctrl_request(const struct usb_instance* data, int interface, union usb_ep0_buffer* request, const void** response)
{
    int size = -1;
    switch (request->setup.bmRequestType.type)
    {
    case USB_SETUP_BMREQUESTTYPE_TYPE_CLASS:
        switch (request->setup.bRequest.raw)
        {
        case 0xfe:  // GET_MAX_LUN
            data->buffer->raw[0] = 0;
            size = 1;
            break;
        case 0xff:  // STORAGE_RESET
            size = 0;
            break;
        default: break;
        }
        break;
        default: break;
    }
    return size;
}

void handle_set_altsetting(const struct usb_instance* data, int interface, int altsetting)
{
    usb = data;
    state = WAITING_FOR_COMMAND;
    maxlen = maxpacket * MIN(usb_get_max_transfer_size(usb, outep), usb_get_max_transfer_size(usb, inep));
    usb_configure_ep(usb, outep, USB_ENDPOINT_TYPE_BULK, maxpacket);
    usb_configure_ep(usb, inep, USB_ENDPOINT_TYPE_BULK, maxpacket);
    listen();
}

void handle_unset_altsetting(const struct usb_instance* data, int interface, int altsetting)
{
    usb_unconfigure_ep(usb, outep);
    usb_unconfigure_ep(usb, inep);
}

int handle_ep_ctrl_request(const struct usb_instance* data, int interface, int endpoint, union usb_ep0_buffer* request, const void** response)
{
    int size = -1;
    switch (request->setup.bmRequestType.type)
    {
    case USB_SETUP_BMREQUESTTYPE_TYPE_STANDARD:
        switch (request->setup.bRequest.req)
        {
        case USB_SETUP_BREQUEST_CLEAR_FEATURE:
            if (request->setup.wLength || request->setup.wValue) break;
            listen();
            break;
        default: break;
        }
        break;
        default: break;
    }
    return size;
}

void handle_xfer_complete(const struct usb_instance* data, int ifnum, int epnum, int bytesleft)
{
    length -= bytesleft;
    switch (state)
    {
        case RECEIVING_BLOCKS:
            if (length != RAMDISK_SECTORSIZE * cur_cmd.count && length != maxlen) break;

            cur_cmd.sector += (maxlen / RAMDISK_SECTORSIZE);
            cur_cmd.count -= MIN(cur_cmd.count, maxlen / RAMDISK_SECTORSIZE);

            if (cur_cmd.count)
                receive_block_data(ramdisk[cur_cmd.sector], MIN(maxlen, cur_cmd.count * RAMDISK_SECTORSIZE));
            else send_csw(0);
            break;
        case WAITING_FOR_CSW_COMPLETION:
            state = WAITING_FOR_COMMAND;
            listen();
            break;
        case WAITING_FOR_COMMAND:
            handle_scsi(&cbw);
            break;
        case SENDING_RESULT:
            send_csw(0);
            break;
        case SENDING_FAILED_RESULT:
            send_csw(1);
            break;
        case SENDING_BLOCKS:
            if (!cur_cmd.count) send_csw(0);
            else send_and_read_next();
            break;
    }
}

void handle_timeout(const struct usb_instance* data, int interface, int endpoint, int bytesleft)
{
    usb_set_stall(usb, outep, true);
    usb_set_stall(usb, inep, true);
}

static void handle_bus_reset(const struct usb_instance* data, int configuration, int interface, int highspeed)
{
    maxpacket = highspeed ? 512 : 64;
    usb_config1_descriptors.c1_i0_a0_e1out.wMaxPacketSize = maxpacket;
    usb_config1_descriptors.c1_i0_a0_e1in.wMaxPacketSize = maxpacket;
}

static struct usb_endpoint usb_c1_i0_a0_ep1out =
{
    .number = { .number = 0, .direction = USB_ENDPOINT_DIRECTION_OUT },
    .ctrl_request = handle_ep_ctrl_request,
    .xfer_complete = handle_xfer_complete,
};

static struct usb_endpoint usb_c1_i0_a0_ep1in =
{
    .number = { .number = 0, .direction = USB_ENDPOINT_DIRECTION_IN },
    .ctrl_request = handle_ep_ctrl_request,
    .xfer_complete = handle_xfer_complete,
    .timeout = handle_timeout,
};

static const struct usb_altsetting usb_c1_i0_a0 =
{
    .set_altsetting = handle_set_altsetting,
    .unset_altsetting = handle_unset_altsetting,
    .endpoint_count = 2,
    .endpoints =
    {
        &usb_c1_i0_a0_ep1out,
        &usb_c1_i0_a0_ep1in,
    },
};

static const struct usb_interface usb_c1_i0 =
{
    .bus_reset = handle_bus_reset,
    .ctrl_request = handle_ctrl_request,
    .altsetting_count = 1,
    .altsettings =
    {
        &usb_c1_i0_a0,
    },
};

static const struct usb_configuration usb_c1 =
{
    .descriptor = &usb_config1_descriptors.c1,
    .set_configuration = NULL,
    .unset_configuration = NULL,
    .interface_count = 1,
    .interfaces =
    {
        &usb_c1_i0,
    },
};

static const struct usb_configuration* usb_configurations[] =
{
    &usb_c1,
};

static void main(int argc, const char** argv)
{
    int i;
    uint32_t eps = usbmanager_get_available_endpoints();
    int ep_out = 0;
    int ep_in = 0;
    for (i = 1; i < 16; i++)
        if (eps & (1 << i))
        {
            ep_out = i;
            break;
        }
    for (i = 1; i < 16; i++)
        if (eps & (1 << (16 + i)))
        {
            ep_in = i;
            break;
        }
    if (!ep_out || !ep_in) panicf(PANIC_KILLTHREAD, "Not enough USB endpoints available!\n");
    usb_config1_descriptors.c1_i0_a0_e1out.bEndpointAddress.number = ep_out;
    usb_config1_descriptors.c1_i0_a0_e1in.bEndpointAddress.number = ep_in;
    usb_c1_i0_a0_ep1out.number.number = ep_out;
    usb_c1_i0_a0_ep1in.number.number = ep_in;
    outep.number = ep_out;
    inep.number = ep_in;
    int rc = usbmanager_install_custom(&usb_devicedescriptor, ARRAYLEN(usb_configurations), usb_configurations,
                                       ARRAYLEN(usb_stringdescriptors), usb_stringdescriptors, true);
    if (IS_ERR(rc)) panicf(PANIC_KILLTHREAD, "Failed to register USB handler: %08X\n", rc);
    while (!ejected && usbmanager_get_connected()) sleep(200000);
    usbmanager_uninstall_custom();
}


EMCORE_APP_HEADER("USB mass storage test", main, 127)

