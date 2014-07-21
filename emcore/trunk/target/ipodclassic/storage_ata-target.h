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


#ifndef __STORAGE_ATA_TARGET_H__
#define __STORAGE_ATA_TARGET_H__

#include "../global.h"


struct __attribute__((packed)) ata_raw_cmd_t
{
    uint8_t lba48;
    uint8_t transfer;
    uint8_t send;
    uint8_t dma;
    uint32_t delay;
    void* buffer;
    uint32_t size;
    uint32_t blksize;
    uint16_t feature;
    uint16_t count;
    uint16_t lba_low;
    uint16_t lba_mid;
    uint16_t lba_high;
    uint8_t device;
    uint8_t command;
    uint8_t result_valid;
    uint8_t reserved[3];
};

struct ata_target_driverinfo
{
    void (*srst_after_error)(bool enable);
    void (*set_retries)(int retries);
    int (*bbt_translate)(uint64_t sector, uint32_t count, uint64_t* phys, uint32_t* physcount);
    int (*bbt_reload)();
    void (*bbt_disable)();
    int (*soft_reset)();
    int (*hard_reset)();
    int (*read_taskfile)(struct ata_raw_cmd_t* cmd);
    int (*raw_cmd)(struct ata_raw_cmd_t* cmd);
};


extern uint16_t ata_identify_data[0x100];
extern uint64_t ata_total_sectors;
extern struct mutex ata_mutex;

extern void ata_set_retries(int retries);
extern void ata_srst_after_error(bool enable);
extern int ata_lock_exclusive(int timeout);
extern void ata_unlock_exclusive();
extern int ata_soft_reset();
extern int ata_hard_reset();
extern int ata_read_taskfile(struct ata_raw_cmd_t* cmd);
extern int ata_raw_cmd(struct ata_raw_cmd_t* cmd);

#ifdef ATA_HAVE_BBT
extern uint16_t (*ata_bbt)[0x20];
extern uint64_t ata_virtual_sectors;

int ata_bbt_translate(uint64_t sector, uint32_t count, uint64_t* phys, uint32_t* physcount);
int ata_bbt_reload();
void ata_bbt_disable();
int ata_rw_sectors_internal(uint64_t sector, uint32_t count, void* buffer, bool write);
#endif


#endif
