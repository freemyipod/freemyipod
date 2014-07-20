//
//
//    Copyright 2013 TheSeven
//    Copyright 2014 user890104
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


#include "emcoreapp.h"
#include "ums.h"
#include "scsi.h"
#include "usb.h"
#include "../../emcore/trunk/target/ipodclassic/storage_ata-target.h"


#define UMS_BUFSIZE 65536


static struct
{
    unsigned int sector;
    unsigned int count;
    unsigned int orig_count;
    unsigned int cur_cmd;
    unsigned int tag;
    unsigned int lun;
    unsigned int last_result;
    unsigned int bytes_pending;
    bool data_direction;
} cur_cmd;


static struct
{
    unsigned char sense_key;
    unsigned char information;
    unsigned char asc;
    unsigned char ascq;
} cur_sense_data;


static enum
{
    SAT_PENDING_NONE = 0,
    SAT_PENDING_SRST,
    SAT_PENDING_HRST,
    SAT_PENDING_CMD,
    SAT_PENDING_READ_CDB,
} sat_pending = SAT_PENDING_NONE;
static struct ata_raw_cmd_t sat_command;
static struct __attribute__((packed)) sat_response_information
{
    uint8_t descriptor_code;
    uint8_t additional_length;
    uint8_t extend;
    uint8_t error;
    uint8_t sector_count_h;
    uint8_t sector_count_l;
    uint8_t lba_low_h;
    uint8_t lba_low_l;
    uint8_t lba_mid_h;
    uint8_t lba_mid_l;
    uint8_t lba_high_h;
    uint8_t lba_high_l;
    uint8_t device;
    uint8_t status;
} sat_response_information;
static bool have_sat_response_information = false;
static bool sat_check;


struct __attribute__((packed)) command_block_wrapper
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


union __attribute__((aligned(32)))
{
    struct inquiry_data inquiry;
    struct capacity capacity_data;
    struct format_capacity format_capacity_data;
    struct sense_data_fixed sense_data_fixed;
    struct
    {
        struct sense_data_descr header;
        union
        {
            struct sat_response_information sat_response;
        } info;
    } sense_data_descr;
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
    RECEIVING_BLOCKS,
    WAITING_FOR_CSW_COMPLETION,
    RECEIVING_SAT_WRITE,
    PROCESSING,
} state = WAITING_FOR_COMMAND;

static uint32_t length;
static struct storage_info storage_info;
static uint8_t __attribute__((aligned(32))) umsbuf[2][2][UMS_BUFSIZE];
static union __attribute__((packed,aligned(32)))
{
    struct command_block_wrapper cbw;
    uint8_t dummy;
} cmdbuf;
static uint8_t readbuf_current;
static uint32_t readbuf_sector[2] = {0, 0};
static int8_t readbuf_count[2] = {0, 0};
static uint8_t read_blocked = false;
static uint8_t writebuf_current;
static uint32_t writebuf_sector;
static uint8_t writebuf_count;
static uint8_t writebuf_busy = false;
static uint8_t writebuf_overrun = false;
static uint32_t write_rc = 0;
static bool locked = false;


bool ums_ejected = false;


static void listen()
{
    usb_receive(&cmdbuf.cbw, sizeof(cmdbuf.cbw));
}


void ums_listen()
{
    state = WAITING_FOR_COMMAND;
    listen();
}


static void send_csw(int status)
{
    if (cur_cmd.bytes_pending)
    {
        if (cur_cmd.data_direction) usb_stall_in();
        else usb_stall_out();
    }

    tb.csw.signature = 0x53425355;
    tb.csw.tag = cur_cmd.tag;
    tb.csw.data_residue = cur_cmd.bytes_pending;
    tb.csw.status = status;

    state = WAITING_FOR_CSW_COMPLETION;
    usb_transmit(&tb.csw, sizeof(tb.csw));

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
    if (cur_cmd.data_direction) fail("diskmode: Attempting to receive data for IN command!");
    else if (size > cur_cmd.bytes_pending) fail("diskmode: Receive overrun!");
    else
    {
        cur_cmd.bytes_pending -= size;
        length = size;
        usb_receive(data, size);
        state = RECEIVING_BLOCKS;
    }
}


static void receive_sat_write(void* data, int size)
{
    if (cur_cmd.data_direction) fail("diskmode: Attempting to receive data for SAT IN command!");
    else if (size > cur_cmd.bytes_pending) fail("diskmode: Receive SAT overrun!");
    else
    {
        cur_cmd.bytes_pending -= size;
        length = size;
        usb_receive(data, size);
        state = RECEIVING_SAT_WRITE;
    }
}


static void send_block_data(void* data, int size)
{
    if (!cur_cmd.data_direction) fail("diskmode: Attempting to send data for OUT command!");
    else if (size > cur_cmd.bytes_pending) fail("diskmode: Send block overrun!");
    else
    {
        cur_cmd.bytes_pending -= size;
        length = size;
        usb_transmit(data, size);
        state = SENDING_BLOCKS;
    }
}


static void send_command_result(void* data, int size)
{
    if (!cur_cmd.data_direction) fail("diskmode: Attempting to send result for OUT command!");
    else if (size > cur_cmd.bytes_pending) fail("diskmode: Send result overrun!");
    else
    {
        cur_cmd.bytes_pending -= size;
        length = size;
        usb_transmit(data, size);
        state = SENDING_RESULT;
    }
}


static void send_command_failed_result(unsigned char key, unsigned char asc, unsigned char ascq)
{
    send_csw(1);
    cur_sense_data.sense_key = key;
    cur_sense_data.asc = asc;
    cur_sense_data.ascq = ascq;
}


static void send_and_read_next()
{
    int bufsize = UMS_BUFSIZE / storage_info.sector_size;
    int i;
    for (i = 0; i < 2; i++)
    {
        if (readbuf_sector[i] <= cur_cmd.sector && readbuf_sector[i] + bufsize > cur_cmd.sector)
        {
            if (readbuf_sector[i] + readbuf_count[i] <= cur_cmd.sector)
            {
                readbuf_current = i;
                read_blocked = true;
                enqueue_async();
                return;
            }
            int overlap = MIN(readbuf_sector[i] + readbuf_count[i] - cur_cmd.sector, cur_cmd.count);
            if (i == readbuf_current && readbuf_sector[i] + readbuf_count[i] < storage_info.num_sectors)
            {
                readbuf_current = !i;
                readbuf_sector[!i] = MAX(readbuf_sector[i] + readbuf_count[i], cur_cmd.sector + overlap);
                readbuf_count[!i] = 0;
                enqueue_async();
            }
            int offset = (cur_cmd.sector - readbuf_sector[i]) * storage_info.sector_size;
            send_block_data(umsbuf[0][i] + offset, overlap * storage_info.sector_size);
            cur_cmd.sector += overlap;
            cur_cmd.count -= overlap;
            return;
        }
    }
    readbuf_current ^= 1;
    readbuf_sector[readbuf_current] = cur_cmd.sector;
    readbuf_count[readbuf_current] = 0;
    read_blocked = true;
    enqueue_async();
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
    copy_padded(tb.inquiry.VendorId, storage_info.vendor, sizeof(tb.inquiry.VendorId));
    copy_padded(tb.inquiry.ProductId, storage_info.product, sizeof(tb.inquiry.ProductId));
    copy_padded(tb.inquiry.ProductRevisionLevel, storage_info.revision, sizeof(tb.inquiry.ProductRevisionLevel));

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
        usb_stall_in();
        usb_stall_out();
        return;
    }
    cur_cmd.tag = cbw->tag;
    cur_cmd.lun = cbw->lun;
    cur_cmd.cur_cmd = cbw->command_block[0];
    cur_cmd.bytes_pending = cbw->data_transfer_length;
    if (cbw->flags & 0x80) cur_cmd.data_direction = 1;
    else cur_cmd.data_direction = 0;

    switch (cbw->command_block[0])
    {
        case SCSI_TEST_UNIT_READY:
            if (!ums_ejected) send_csw(0);
            else send_command_failed_result(SENSE_NOT_READY, ASC_MEDIUM_NOT_PRESENT, 0);
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
            if (have_sat_response_information)
            {
                memset(&tb.sense_data_descr.header, 0, sizeof(tb.sense_data_descr.header));
                tb.sense_data_descr.header.ResponseCode = 0x72;
                tb.sense_data_descr.header.fei_sensekey = cur_sense_data.sense_key & 0x0f;
                tb.sense_data_descr.header.AdditionalSenseCode = cur_sense_data.asc;
                tb.sense_data_descr.header.AdditionalSenseCodeQualifier = cur_sense_data.ascq;
                tb.sense_data_descr.header.AdditionalSenseLength = sizeof(sat_response_information);
                memcpy(&tb.sense_data_descr.info.sat_response, &sat_response_information, sizeof(sat_response_information));
                send_command_result(&tb.sense_data_descr, MIN(sizeof(tb.sense_data_descr.header) + sizeof(sat_response_information), length));
            }
            else
            {
                tb.sense_data_fixed.ResponseCode = 0x70;
                tb.sense_data_fixed.Obsolete = 0;
                tb.sense_data_fixed.fei_sensekey = cur_sense_data.sense_key & 0x0f;
                tb.sense_data_fixed.Information = cur_sense_data.information;
                tb.sense_data_fixed.AdditionalSenseLength = 10;
                tb.sense_data_fixed.CommandSpecificInformation = 0;
                tb.sense_data_fixed.AdditionalSenseCode = cur_sense_data.asc;
                tb.sense_data_fixed.AdditionalSenseCodeQualifier = cur_sense_data.ascq;
                tb.sense_data_fixed.FieldReplaceableUnitCode = 0;
                tb.sense_data_fixed.SKSV = 0;
                tb.sense_data_fixed.SenseKeySpecific = 0;
                send_command_result(&tb.sense_data_fixed, MIN(sizeof(tb.sense_data_fixed), length));
            }
            break;
        }

        case SCSI_MODE_SENSE_10:
        {
            if (ums_ejected) 
            {
                send_command_failed_result(SENSE_NOT_READY, ASC_MEDIUM_NOT_PRESENT, 0);
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

                    tb.ms_data_10.block_descriptor.num_blocks[4] = (storage_info.num_sectors & 0xff000000) >> 24;
                    tb.ms_data_10.block_descriptor.num_blocks[5] = (storage_info.num_sectors & 0x00ff0000) >> 16;
                    tb.ms_data_10.block_descriptor.num_blocks[6] = (storage_info.num_sectors & 0x0000ff00) >> 8;
                    tb.ms_data_10.block_descriptor.num_blocks[7] = (storage_info.num_sectors & 0x000000ff);

                    tb.ms_data_10.block_descriptor.block_size[0] = (storage_info.sector_size & 0xff000000) >> 24;
                    tb.ms_data_10.block_descriptor.block_size[1] = (storage_info.sector_size & 0x00ff0000) >> 16;
                    tb.ms_data_10.block_descriptor.block_size[2] = (storage_info.sector_size & 0x0000ff00) >> 8;
                    tb.ms_data_10.block_descriptor.block_size[3] = (storage_info.sector_size & 0x000000ff);
                    send_command_result(&tb.ms_data_10, MIN(sizeof(tb.ms_data_10), length));
                    break;
                default:
                    send_command_failed_result(SENSE_ILLEGAL_REQUEST, ASC_INVALID_FIELD_IN_CBD, 0);
                    break;
            }
            break;
        }

        case SCSI_MODE_SENSE_6:
        {
            if (ums_ejected)
            {
                send_command_failed_result(SENSE_NOT_READY, ASC_MEDIUM_NOT_PRESENT, 0);
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
                    if (storage_info.num_sectors > 0xffffff)
                    {
                        tb.ms_data_6.block_descriptor.num_blocks[0] = 0xff;
                        tb.ms_data_6.block_descriptor.num_blocks[1] = 0xff;
                        tb.ms_data_6.block_descriptor.num_blocks[2] = 0xff;
                    }
                    else
                    {
                        tb.ms_data_6.block_descriptor.num_blocks[0] = (storage_info.num_sectors & 0xff0000) >> 16;
                        tb.ms_data_6.block_descriptor.num_blocks[1] = (storage_info.num_sectors & 0x00ff00) >> 8;
                        tb.ms_data_6.block_descriptor.num_blocks[2] = (storage_info.num_sectors & 0x0000ff);
                    }
                    tb.ms_data_6.block_descriptor.block_size[0] = (storage_info.sector_size & 0xff0000) >> 16;
                    tb.ms_data_6.block_descriptor.block_size[1] = (storage_info.sector_size & 0x00ff00) >> 8;
                    tb.ms_data_6.block_descriptor.block_size[2] = (storage_info.sector_size & 0x0000ff);
                    send_command_result(&tb.ms_data_6, MIN(sizeof(tb.ms_data_6), length));
                    break;
                default:
                    send_command_failed_result(SENSE_ILLEGAL_REQUEST, ASC_INVALID_FIELD_IN_CBD, 0);
                    break;
            }
            break;
        }

        case SCSI_START_STOP_UNIT:
            if ((cbw->command_block[4] & 0xf3) == 2) ums_ejected = true;
            send_csw(0);
            break;

        case SCSI_ALLOW_MEDIUM_REMOVAL:
            if ((cbw->command_block[4] & 0x03) == 0) locked = false;
            else locked = true;
            send_csw(0);
            break;

        case SCSI_READ_FORMAT_CAPACITY:
        {
            if (!ums_ejected)
            {
                tb.format_capacity_data.following_length = 0x08000000;
                tb.format_capacity_data.block_count = swap32(storage_info.num_sectors - 1);
                tb.format_capacity_data.block_size = swap32(storage_info.sector_size);
                tb.format_capacity_data.block_size |= swap32(SCSI_FORMAT_CAPACITY_FORMATTED_MEDIA);
                send_command_result(&tb.format_capacity_data, MIN(sizeof(tb.format_capacity_data), length));
           }
           else send_command_failed_result(SENSE_NOT_READY, ASC_MEDIUM_NOT_PRESENT, 0);
           break;
        }

        case SCSI_READ_CAPACITY:
        {
            if (!ums_ejected)
            {
                tb.capacity_data.block_count = swap32(storage_info.num_sectors - 1);
                tb.capacity_data.block_size = swap32(storage_info.sector_size);
                send_command_result(&tb.capacity_data, MIN(sizeof(tb.capacity_data), length));
            }
            else send_command_failed_result(SENSE_NOT_READY, ASC_MEDIUM_NOT_PRESENT, 0);
            break;
        }

        case SCSI_READ_10:
            if (ums_ejected)
            {
                send_command_failed_result(SENSE_NOT_READY, ASC_MEDIUM_NOT_PRESENT, 0);
                break;
            }
            cur_cmd.sector = (cbw->command_block[2] << 24 | cbw->command_block[3] << 16
                            | cbw->command_block[4] << 8 | cbw->command_block[5]);
            cur_cmd.count = (cbw->command_block[7] << 8 | cbw->command_block[8]);
            cur_cmd.orig_count = cur_cmd.count;

            if ((cur_cmd.sector + cur_cmd.count) > storage_info.num_sectors)
                send_command_failed_result(SENSE_ILLEGAL_REQUEST, ASC_LBA_OUT_OF_RANGE, 0);
            else if (!cur_cmd.count) send_csw(0);
            else send_and_read_next();
            break;

        case SCSI_WRITE_10:
            if (ums_ejected)
            {
                send_command_failed_result(SENSE_NOT_READY, ASC_MEDIUM_NOT_PRESENT, 0);
                break;
            }
            cur_cmd.sector = (cbw->command_block[2] << 24 | cbw->command_block[3] << 16
                            | cbw->command_block[4] << 8 | cbw->command_block[5]);
            cur_cmd.count = (cbw->command_block[7] << 8 | cbw->command_block[8]);
            cur_cmd.orig_count = cur_cmd.count;

            if ((cur_cmd.sector + cur_cmd.count) > storage_info.num_sectors)
                send_command_failed_result(SENSE_ILLEGAL_REQUEST, ASC_LBA_OUT_OF_RANGE, 0);
            else if (!cur_cmd.count) send_csw(0);
            else
                receive_block_data(umsbuf[1][!writebuf_current],
                                   MIN(UMS_BUFSIZE, cur_cmd.count * storage_info.sector_size));
            break;

        case SCSI_WRITE_BUFFER:
            break;
            
        case SCSI_ATA_PASSTHROUGH_12:
        case SCSI_ATA_PASSTHROUGH_16:
        {
            if (!storage_info.driverinfo || get_platform_id() != 0x4c435049)
            {
                send_command_failed_result(SENSE_ILLEGAL_REQUEST, ASC_INVALID_COMMAND, 0);
                break;
            }
            int cmd = cbw->command_block[0];
            int multi = cbw->command_block[1] >> 5;
            int protocol = (cbw->command_block[1] >> 1) & 0xf;
            int extend = 0;
            int offline = cbw->command_block[2] >> 6;
            int check = (cbw->command_block[2] >> 5) & 1;
            int type = (cbw->command_block[2] >> 4) & 1;
            int dir = (cbw->command_block[2] >> 3) & 1;
            int block = (cbw->command_block[2] >> 2) & 1;
            int len = cbw->command_block[2] & 3;
            int features = cbw->command_block[3];
            int count = cbw->command_block[4];
            int lbal = cbw->command_block[5];
            int lbam = cbw->command_block[6];
            int lbah = cbw->command_block[7];
            int device = cbw->command_block[8] & ~0x10;
            int command = cbw->command_block[9];
            int control = cbw->command_block[11];
            if (cmd == SCSI_ATA_PASSTHROUGH_16)
            {
                extend = cbw->command_block[1] & 1;
                features = (extend ? (cbw->command_block[3] << 8) : 0) | cbw->command_block[4];
                count = (extend ? (cbw->command_block[5] << 8) : 0) | cbw->command_block[6];
                lbal = (extend ? (cbw->command_block[7] << 8) : 0) | cbw->command_block[8];
                lbam = (extend ? (cbw->command_block[9] << 8) : 0) | cbw->command_block[10];
                lbah = (extend ? (cbw->command_block[11] << 8) : 0) | cbw->command_block[12];
                device = cbw->command_block[13] & ~0x10;
                command = cbw->command_block[14];
                control = cbw->command_block[15];
            }
            sat_command.lba48 = extend;
            sat_check = check;
            int delay = (2 << offline) - 2;
            int bytes = 0;
            switch (len)
            {
                case 1: bytes = features; break;
                case 2: bytes = count; break;
                case 3: bytes = length; break;
            }
            int blocks = 1;
            if (block)
            {
                blocks = bytes;
                bytes = type ? storage_info.sector_size : 512;
            }
            int invalid = blocks * bytes > UMS_BUFSIZE;
            int dma = 0;
            switch (protocol)
            {
                case SAT_HARD_RESET:
                    sat_pending = SAT_PENDING_HRST;
                    enqueue_async();
                    return;
                case SAT_SOFT_RESET:
                    sat_pending = SAT_PENDING_SRST;
                    enqueue_async();
                    return;
                case SAT_NON_DATA:
                    if (len) invalid = 1;
                    break;
                case SAT_PIO_DATA_IN:
                    if (!len || !dir) invalid = 1;
                    break;
                case SAT_PIO_DATA_OUT:
                    if (!len || dir) invalid = 1;
                    break;
                case SAT_UDMA_DATA_IN:
                    if (!len || !dir) invalid = 1;
                    dma = 1;
                    break;
                case SAT_UDMA_DATA_OUT:
                    if (!len || dir) invalid = 1;
                    dma = 1;
                    break;
                case SAT_RETURN_RESPONSE:
                    sat_pending = SAT_PENDING_READ_CDB;
                    enqueue_async();
                    return;
                //case SAT_NON_DATA_RESET:
                //case SAT_DMA:
                //case SAT_DMA_QUEUED:
                //case SAT_FPDMA:
                //case SAT_DIAGNOSTIC:
                default:
                    invalid = 1;
                    break;
            }
            if (invalid)
            {
                send_command_failed_result(SENSE_ILLEGAL_REQUEST, ASC_INVALID_FIELD_IN_CBD, 0);
                return;
            }
            readbuf_count[!readbuf_current] = 0;
            void* buffer = umsbuf[0][!readbuf_current];
            sat_command.transfer = !!len;
            sat_command.send = !dir;
            sat_command.dma = dma;
            sat_command.delay = delay * 1000000;
            sat_command.buffer = buffer;
            sat_command.size = blocks;
            sat_command.blksize = bytes;
            sat_command.feature = features;
            sat_command.count = count;
            sat_command.lba_low = lbal;
            sat_command.lba_mid = lbam;
            sat_command.lba_high = lbah;
            sat_command.device = device;
            sat_command.command = command;
            if (len && !dir) receive_sat_write(buffer, bytes * blocks);
            else
            {
                sat_pending = SAT_PENDING_CMD;
                enqueue_async();
            }
            break;
        }
        
        default:
            send_command_failed_result(SENSE_ILLEGAL_REQUEST, ASC_INVALID_COMMAND, 0);
            break;
    }
    have_sat_response_information = false;
}


void ums_init()
{
    storage_get_info(0, &storage_info);
    if (!storage_info.sector_size) panicf(PANIC_KILLTHREAD, "Sector size is zero!\n");
    if (!storage_info.num_sectors) panicf(PANIC_KILLTHREAD, "Number of sectors is zero!\n");
}


void update_readcache()
{
    int count = length / storage_info.sector_size;
    int i;
    for (i = 0; i < 2; i++)
        if ((readbuf_sector[i] <= cur_cmd.sector && readbuf_sector[i] + readbuf_count[i] > cur_cmd.sector)
         || (readbuf_sector[i] <= cur_cmd.sector + count
          && readbuf_sector[i] + readbuf_count[i] > cur_cmd.sector + count))
        {
            int roffset = MAX(0, (int)(cur_cmd.sector - readbuf_sector[i]));
            int woffset = MAX(0, (int)(readbuf_sector[i] - cur_cmd.sector));
            int len = MIN(readbuf_count[i] - roffset, count - woffset);
            roffset *= storage_info.sector_size;
            woffset *= storage_info.sector_size;
            len *= storage_info.sector_size;
            memcpy(umsbuf[0][i] + roffset, umsbuf[1][writebuf_current] + woffset, len);
        }
}


static void writebuf_push()
{
    int count = length / storage_info.sector_size;
    writebuf_current ^= 1;
    writebuf_sector = cur_cmd.sector;
    cur_cmd.sector += count;
    writebuf_count = count;
    cur_cmd.count -= count;
    writebuf_busy = true;
    int remaining = cur_cmd.count * storage_info.sector_size;
    if (!cur_cmd.count)
    {
        if (IS_ERR(write_rc))
        {
            send_command_failed_result(SENSE_MEDIUM_ERROR, ASC_WRITE_ERROR, 0);
            return;
        }
        send_csw(0);
        write_rc = 0;
    }
    else receive_block_data(umsbuf[1][!writebuf_current], MIN(UMS_BUFSIZE, remaining));
}


void ums_xfer_complete(bool in, int bytesleft)
{
    length -= bytesleft;
    if (in)
        switch (state)
        {
            case WAITING_FOR_CSW_COMPLETION:
                state = WAITING_FOR_COMMAND;
                listen();
                break;
            case SENDING_RESULT:
                send_csw(0);
                break;
            case SENDING_BLOCKS:
                if (!cur_cmd.count) send_csw(0);
                else send_and_read_next();
                break;
            default:
                fail("diskmode: Got IN completion in invalid state!");
        }
    else
        switch (state)
        {
            case RECEIVING_BLOCKS:
                if (length != storage_info.sector_size * cur_cmd.count && length != UMS_BUFSIZE) break;
                update_readcache();
                if (!writebuf_busy) writebuf_push();
                else writebuf_overrun = true;
                enqueue_async();
                break;
            case WAITING_FOR_COMMAND:
                state = PROCESSING;
                invalidate_dcache(&cmdbuf, sizeof(cmdbuf));  // Who pulls this into a cache line!?
                handle_scsi(&cmdbuf.cbw);
                break;
            case RECEIVING_SAT_WRITE:
                state = PROCESSING;
                sat_pending = SAT_PENDING_CMD;
                enqueue_async();
                break;
            default:
                fail("diskmode: Got OUT completion in invalid state!");
        }
}

void ums_handle_async()
{
    if (writebuf_overrun)
    {
        writebuf_overrun = false;
        writebuf_push();
    }
    if (writebuf_busy)
    {
        write_rc |= storage_write_sectors_md(0, writebuf_sector, writebuf_count, umsbuf[1][writebuf_current]);
        writebuf_busy = false;
    }

    int c = readbuf_current;        
    if (!readbuf_count[c])
    {
        int sector = readbuf_sector[c];
        int count = MIN(UMS_BUFSIZE / storage_info.sector_size, storage_info.num_sectors - sector);
        if (IS_ERR(storage_read_sectors_md(0, sector, count, umsbuf[0][c]))) count = -1;
        if (sector == readbuf_sector[c]) readbuf_count[c] = count;
        if (sector != readbuf_sector[c]) readbuf_count[c] = 0;  // Extremely ugly hack to neutralize race condition
    }
    if (read_blocked)
    {
        read_blocked = false;
        if (readbuf_count[readbuf_current] < 0)
            send_command_failed_result(SENSE_MEDIUM_ERROR, ASC_READ_ERROR, 0);
        else send_and_read_next();
    }
    
    struct ata_target_driverinfo* drv = storage_info.driverinfo;
    switch (sat_pending)
    {
        case SAT_PENDING_SRST:
        {
            sat_pending = SAT_PENDING_NONE;
            if (IS_ERR(drv->soft_reset()))
                send_command_failed_result(SENSE_HARDWARE_ERROR, ASC_UNSUCCESSFUL_SOFT_RESET, 0);
            else send_csw(0);
            break;
        }
        case SAT_PENDING_HRST:
        {
            sat_pending = SAT_PENDING_NONE;
            if (IS_ERR(drv->hard_reset()))
                send_command_failed_result(SENSE_HARDWARE_ERROR, ASC_UNSUCCESSFUL_SOFT_RESET, 0);
            else send_csw(0);
            break;
        }
        case SAT_PENDING_CMD:
        {
int cmd = sat_command.command;
            int datasize = sat_command.size * sat_command.blksize;
            void* buffer = sat_command.buffer;
            int rc = drv->raw_cmd(&sat_command);
            sat_response_information.descriptor_code = 9;
            sat_response_information.additional_length = 12;
            sat_response_information.extend = sat_command.lba48;
            sat_response_information.error = sat_command.feature;
            sat_response_information.sector_count_h = sat_command.count >> 8;
            sat_response_information.sector_count_l = sat_command.count & 0xff;
            sat_response_information.lba_low_h = sat_command.lba_low >> 8;
            sat_response_information.lba_low_l = sat_command.lba_low & 0xff;
            sat_response_information.lba_mid_h = sat_command.lba_mid >> 8;
            sat_response_information.lba_mid_l = sat_command.lba_mid & 0xff;
            sat_response_information.lba_high_h = sat_command.lba_high >> 8;
            sat_response_information.lba_high_l = sat_command.lba_high & 0xff;
            sat_response_information.device = sat_command.device;
            sat_response_information.status = sat_command.command;
            have_sat_response_information = true;
            sat_pending = SAT_PENDING_NONE;
            if (IS_ERR(rc) || (sat_command.command & 0x21))
            {
                if (sat_command.command & 0x20)
                    send_command_failed_result(SENSE_HARDWARE_ERROR, ASC_INTERNAL_TARGET_FAILURE, 0);
                else if ((sat_command.command & 0x01) && (sat_command.feature & 0x01))
                    send_command_failed_result(SENSE_MEDIUM_ERROR, ASC_ADDRESS_MARK_NOT_FOUND, 0);
                else if ((sat_command.command & 0x01) && (sat_command.feature & 0x02))
                    send_command_failed_result(SENSE_NOT_READY, ASC_MEDIUM_NOT_PRESENT, 0);
                else if ((sat_command.command & 0x01) && (sat_command.feature & 0x08))
                    send_command_failed_result(SENSE_UNIT_ATTENTION, ASC_OPERATOR_REQUEST, ASCQ_MEDIUM_REMOVAL_REQUEST);
                else if ((sat_command.command & 0x01) && (sat_command.feature & 0x10))
                    send_command_failed_result(SENSE_ILLEGAL_REQUEST, ASC_LBA_OUT_OF_RANGE, 0);
                else if ((sat_command.command & 0x01) && (sat_command.feature & 0x20))
                    send_command_failed_result(SENSE_UNIT_ATTENTION, ASC_MEDIUM_MAY_HAVE_CHANGED, 0);
                else if ((sat_command.command & 0x01) && (sat_command.feature & 0x40) && datasize && !sat_command.send)
                    send_command_failed_result(SENSE_MEDIUM_ERROR, ASC_READ_ERROR, 0);
                else if ((sat_command.command & 0x01) && (sat_command.feature & 0x40))
                    send_command_failed_result(SENSE_DATA_PROTECT, ASC_WRITE_PROTECTED, 0);
                else if ((sat_command.command & 0x01) && (sat_command.feature & 0x80))
                    send_command_failed_result(SENSE_ABORTED_COMMAND, ASC_SCSI_PARITY_ERROR, ASCQ_UICRC_ERROR_DETECTED);
                else if ((sat_command.command & 0x01) && (sat_command.feature & 0x04))
                    send_command_failed_result(SENSE_ABORTED_COMMAND, 0, 0);
                else send_command_failed_result(SENSE_HARDWARE_ERROR, ASC_INTERNAL_TARGET_FAILURE, 0);
            }
            else if (sat_check)
                send_command_failed_result(SENSE_SOFT_ERROR, ASC_RECOVERED_ERROR, ASCQ_SAT_INFO_AVAILABLE);
            else if (datasize) send_command_result(buffer, datasize);
            else send_csw(0);
            break;
        }
        case SAT_PENDING_READ_CDB:
        {
            int rc = drv->read_taskfile(&sat_command);
            sat_response_information.descriptor_code = 9;
            sat_response_information.additional_length = 12;
            sat_response_information.extend = sat_command.lba48;
            sat_response_information.error = sat_command.feature;
            sat_response_information.sector_count_h = sat_command.count >> 8;
            sat_response_information.sector_count_l = sat_command.count & 0xff;
            sat_response_information.lba_low_h = sat_command.lba_low >> 8;
            sat_response_information.lba_low_l = sat_command.lba_low & 0xff;
            sat_response_information.lba_mid_h = sat_command.lba_mid >> 8;
            sat_response_information.lba_mid_l = sat_command.lba_mid & 0xff;
            sat_response_information.lba_high_h = sat_command.lba_high >> 8;
            sat_response_information.lba_high_l = sat_command.lba_high & 0xff;
            sat_response_information.device = sat_command.device;
            sat_response_information.status = sat_command.command;
            sat_pending = SAT_PENDING_NONE;
            if (IS_ERR(rc)) send_command_failed_result(SENSE_HARDWARE_ERROR, ASC_INTERNAL_TARGET_FAILURE, 0);
            else send_command_result(&sat_response_information, sizeof(sat_response_information));
            break;
        }
    }
}

