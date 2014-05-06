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
} cur_cmd;


static struct
{
    unsigned char sense_key;
    unsigned char information;
    unsigned char asc;
    unsigned char ascq;
} cur_sense_data;


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
    tb.csw.signature = 0x53425355;
    tb.csw.tag = cur_cmd.tag;
    tb.csw.data_residue = 0;
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
    length = size;
    usb_receive(data, size);
    state = RECEIVING_BLOCKS;
}


static void send_block_data(void* data, int size)
{
    length = size;
    usb_transmit(data, size);
    state = SENDING_BLOCKS;
}


static void send_command_result(void* data, int size)
{
    length = size;
    usb_transmit(data, size);
    state = SENDING_RESULT;
}


static void send_command_failed_result()
{
    usb_transmit(NULL, 0);
    state = SENDING_FAILED_RESULT;
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
        usb_stall();
        return;
    }
    cur_cmd.tag = cbw->tag;
    cur_cmd.lun = cbw->lun;
    cur_cmd.cur_cmd = cbw->command_block[0];

    switch (cbw->command_block[0])
    {
        case SCSI_TEST_UNIT_READY:
            if (!ums_ejected) send_csw(0);
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
            if (ums_ejected)
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
            if (ums_ejected)
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
                    send_command_failed_result();
                    cur_sense_data.sense_key = SENSE_ILLEGAL_REQUEST;
                    cur_sense_data.asc = ASC_INVALID_FIELD_IN_CBD;
                    cur_sense_data.ascq = 0;
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
            if (!ums_ejected)
            {
                tb.capacity_data.block_count = swap32(storage_info.num_sectors - 1);
                tb.capacity_data.block_size = swap32(storage_info.sector_size);
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
            if (ums_ejected)
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

            if ((cur_cmd.sector + cur_cmd.count) > storage_info.num_sectors)
            {
                send_csw(1);
                cur_sense_data.sense_key = SENSE_ILLEGAL_REQUEST;
                cur_sense_data.asc = ASC_LBA_OUT_OF_RANGE;
                cur_sense_data.ascq = 0;
            }
            else if (!cur_cmd.count) send_csw(0);
            else send_and_read_next();
            break;

        case SCSI_WRITE_10:
            if (ums_ejected)
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

            if ((cur_cmd.sector + cur_cmd.count) > storage_info.num_sectors)
            {
                send_csw(1);
                cur_sense_data.sense_key = SENSE_ILLEGAL_REQUEST;
                cur_sense_data.asc = ASC_LBA_OUT_OF_RANGE;
                cur_sense_data.ascq = 0;
            }
            else if (!cur_cmd.count) send_csw(0);
            else
                receive_block_data(umsbuf[1][!writebuf_current],
                                   MIN(UMS_BUFSIZE, cur_cmd.count * storage_info.sector_size));
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
            send_csw(1);
            cur_sense_data.sense_key = SENSE_MEDIUM_ERROR;
            cur_sense_data.asc = ASC_WRITE_ERROR;
            cur_sense_data.ascq = 0;
        }
        send_csw(0);
        write_rc = 0;
    }
    else receive_block_data(umsbuf[1][!writebuf_current], MIN(UMS_BUFSIZE, remaining));
}


void ums_xfer_complete(bool in, int bytesleft)
{
    length -= bytesleft;
    switch (state)
    {
        case RECEIVING_BLOCKS:
            if (length != storage_info.sector_size * cur_cmd.count && length != UMS_BUFSIZE) break;
            update_readcache();
            if (!writebuf_busy) writebuf_push();
            else writebuf_overrun = true;
            enqueue_async();
            break;
        case WAITING_FOR_CSW_COMPLETION:
            state = WAITING_FOR_COMMAND;
            listen();
            break;
        case WAITING_FOR_COMMAND:
            invalidate_dcache(&cmdbuf, sizeof(cmdbuf));  // Who pulls this into a cache line!?
            handle_scsi(&cmdbuf.cbw);
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
        {
            send_csw(1);
            cur_sense_data.sense_key = SENSE_MEDIUM_ERROR;
            cur_sense_data.asc = ASC_READ_ERROR;
            cur_sense_data.ascq = 0;
        }
        else send_and_read_next();
    }
}

