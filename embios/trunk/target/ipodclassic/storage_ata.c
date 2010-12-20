/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007 Dave Chapman
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include "global.h"
#include "thread.h"
#include "disk.h"
#include "storage.h"
#include "storage_ata-target.h"
#include "timer.h"
#include "../ipodnano3g/s5l8702.h"

/** static, private data **/ 
uint16_t ata_identify_data[0x100];
bool ata_lba48;
bool ata_dma;
uint64_t ata_total_sectors;
static struct mutex ata_mutex;
static struct wakeup ata_wakeup;
static uint32_t ata_dma_flags;
static long ata_last_activity_value = -1;
static long ata_sleep_timeout = 20000000;
static uint32_t ata_stack[0x80];
static bool ata_powered;

#ifdef ATA_HAVE_BBT
#include "panic.h"
uint16_t ata_bbt[ATA_BBT_PAGES][0x20];
uint64_t ata_virtual_sectors;
uint32_t ata_last_offset;
uint64_t ata_last_phys;

void ata_bbt_read_sectors(uint32_t sector, uint32_t count, void* buffer)
{
    int rc = ata_rw_sectors_internal(sector, count, buffer, false);
    if (IS_ERR(rc))
        panicf(PANIC_KILLTHREAD, "ATA: Error %08X while reading BBT (sector %d, count %d)\n",
               rc, sector, count);
}
#endif


static uint16_t ata_read_cbr(uint32_t volatile* reg)
{
    while (!(ATA_PIO_READY & 2)) yield();
    volatile uint32_t dummy = *reg;
    while (!(ATA_PIO_READY & 1)) yield();
    return ATA_PIO_RDATA;
}

static void ata_write_cbr(uint32_t volatile* reg, uint16_t data)
{
    while (!(ATA_PIO_READY & 2)) yield();
    *reg = data;
}

static int ata_wait_for_not_bsy(long timeout)
{
    long startusec = USEC_TIMER;
    while (true)
    {
        uint8_t csd = ata_read_cbr(&ATA_PIO_CSD);
        if (!(csd & BIT(7))) return 0;
        if (TIMEOUT_EXPIRED(startusec, timeout)) RET_ERR(0);
    }
}

static int ata_wait_for_rdy(long timeout)
{
    long startusec = USEC_TIMER;
    PASS_RC(ata_wait_for_not_bsy(timeout), 1, 0);
    while (true)
    {
        uint8_t dad = ata_read_cbr(&ATA_PIO_DAD);
        if (dad & BIT(6)) return 0;
        if (TIMEOUT_EXPIRED(startusec, timeout)) RET_ERR(1);
    }
}

static int ata_wait_for_start_of_transfer(long timeout)
{
    long startusec = USEC_TIMER;
    PASS_RC(ata_wait_for_not_bsy(timeout), 2, 0);
    while (true)
    {
        uint8_t dad = ata_read_cbr(&ATA_PIO_DAD);
        if (dad & BIT(0)) RET_ERR(1);
        if ((dad & (BIT(7) | BIT(3))) == BIT(3)) return 0;
        if (TIMEOUT_EXPIRED(startusec, timeout)) RET_ERR(2);
    }
}

static int ata_wait_for_end_of_transfer(long timeout)
{
    PASS_RC(ata_wait_for_not_bsy(timeout), 2, 0);
    uint8_t dad = ata_read_cbr(&ATA_PIO_DAD);
    if (dad & BIT(0)) RET_ERR(1);
    if ((dad & (BIT(3) | BITRANGE(5, 7))) == BIT(6)) return 0;
    RET_ERR(2);
}    

int ata_identify(uint16_t* buf)
{
    int i;
    PASS_RC(ata_wait_for_not_bsy(10000000), 1, 0);
    ata_write_cbr(&ATA_PIO_DVR, 0);
    ata_write_cbr(&ATA_PIO_CSD, 0xec);
    PASS_RC(ata_wait_for_start_of_transfer(10000000), 1, 1);
    for (i = 0; i < 0x100; i++)
    {
        uint16_t word = ata_read_cbr(&ATA_PIO_DTR);
        buf[i] = (word >> 8) | (word << 8);
    }
}

void ata_set_active(void)
{
    ata_last_activity_value = USEC_TIMER;
}

int ata_set_feature(uint32_t feature, uint32_t param)
{
    PASS_RC(ata_wait_for_rdy(500000), 1, 0);
    ata_write_cbr(&ATA_PIO_DVR, 0);
    ata_write_cbr(&ATA_PIO_FED, 3);
    ata_write_cbr(&ATA_PIO_SCR, param);
    ata_write_cbr(&ATA_PIO_CSD, feature);
    PASS_RC(ata_wait_for_rdy(500000), 1, 1);
    return 0;
}

int ata_power_up()
{
    ata_set_active();
    if (ata_powered) return 0;
    i2c_sendbyte(0, 0xe6, 0x1b, 1);
    clockgate_enable(5, true);
    ATA_CFG = BIT(0);
    sleep(1000);
    ATA_CFG = 0;
    sleep(6000);
    ATA_SWRST = BIT(0);
    sleep(500);
    ATA_SWRST = 0;
    sleep(90000);
    ATA_CONTROL = BIT(0);
    sleep(200000);
    ATA_PIO_TIME = 0x191f7;
    ATA_PIO_LHR = 0;
    while (!(ATA_PIO_READY & BIT(1))) sleep(100);
    PASS_RC(ata_identify(ata_identify_data), 2, 0);
    uint32_t piotime = 0x11f3;
    uint32_t mdmatime = 0x1c175;
    uint32_t udmatime = 0x5071152;
    uint32_t param = 0;
    ata_dma_flags = 0;
    ata_lba48 = ata_identify_data[83] & BIT(10) ? true : false;
    if (ata_lba48)
        ata_total_sectors = ata_identify_data[100]
                          | (((uint64_t)ata_identify_data[101]) << 16)
                          | (((uint64_t)ata_identify_data[102]) << 32)
                          | (((uint64_t)ata_identify_data[103]) << 48);
    else ata_total_sectors = ata_identify_data[60] | (((uint32_t)ata_identify_data[61]) << 16);
    ata_total_sectors >>= 3;
    if (ata_identify_data[53] & BIT(1))
    {
        if (ata_identify_data[64] & BIT(1)) piotime = 0x2072;
        else if (ata_identify_data[64] & BIT(0)) piotime = 0x7083;
    }
    if (ata_identify_data[63] & BIT(2))
    {
        mdmatime = 0x5072;
        param = 0x22;
    }
    else if (ata_identify_data[63] & BIT(1))
    {
        mdmatime = 0x7083;
        param = 0x21;
    }
    if (ata_identify_data[63] & BITRANGE(0, 2))
    {
        ata_dma_flags = BIT(3) | BIT(10);
        param |= 0x20;
    }
    if (ata_identify_data[53] & BIT(2))
    {
        if (ata_identify_data[88] & BIT(4))
        {
            udmatime = 0x2010a52;
            param = 0x44;
        }
        else if (ata_identify_data[88] & BIT(3))
        {
            udmatime = 0x2020a52;
            param = 0x43;
        }
        else if (ata_identify_data[88] & BIT(2))
        {
            udmatime = 0x3030a52;
            param = 0x42;
        }
        else if (ata_identify_data[88] & BIT(1))
        {
            udmatime = 0x3050a52;
            param = 0x41;
        }
        if (ata_identify_data[88] & BITRANGE(0, 4))
        {
            ata_dma_flags = BIT(2) | BIT(3) | BIT(9) | BIT(10);
            param |= 0x40;
        }
    }
    ata_dma = param ? true : false;
    PASS_RC(ata_set_feature(0xef, param), 2, 1);
    if (ata_identify_data[82] & BIT(5)) PASS_RC(ata_set_feature(0x02, 0), 2, 2);
    if (ata_identify_data[82] & BIT(6)) PASS_RC(ata_set_feature(0x55, 0), 2, 3);
    ATA_PIO_TIME = piotime;
    ATA_MDMA_TIME = mdmatime;
    ATA_UDMA_TIME = udmatime;
    ata_powered = true;
    ata_set_active();
    return 0;
}

void ata_power_down()
{
    if (!ata_powered) return;
    ata_powered = false;
    ata_wait_for_rdy(1000000);
    ata_write_cbr(&ATA_PIO_DVR, 0);
    ata_write_cbr(&ATA_PIO_CSD, 0xe0);
    ata_wait_for_rdy(1000000);
    sleep(30000);
    ATA_CONTROL = 0;
    while (!(ATA_CONTROL & BIT(1))) yield();
    clockgate_enable(5, false);
    i2c_sendbyte(0, 0xe6, 0x1b, 0);
}

int ata_rw_chunk(uint64_t sector, uint32_t cnt, void* buffer, bool write)
{
    PASS_RC(ata_wait_for_rdy(100000), 2, 0);
    ata_write_cbr(&ATA_PIO_DVR, 0);
    if (ata_lba48)
    {
        ata_write_cbr(&ATA_PIO_SCR, cnt >> 5);
        ata_write_cbr(&ATA_PIO_SCR, (cnt << 3) & 0xff);
        ata_write_cbr(&ATA_PIO_LHR, (sector >> 37) & 0xff);
        ata_write_cbr(&ATA_PIO_LMR, (sector >> 29) & 0xff);
        ata_write_cbr(&ATA_PIO_LLR, (sector >> 21) & 0xff);
        ata_write_cbr(&ATA_PIO_LHR, (sector >> 13) & 0xff);
        ata_write_cbr(&ATA_PIO_LMR, (sector >> 5) & 0xff);
        ata_write_cbr(&ATA_PIO_LLR, (sector << 3) & 0xff);
        ata_write_cbr(&ATA_PIO_DVR, BIT(6));
        if (write) ata_write_cbr(&ATA_PIO_CSD, ata_dma ? 0x35 : 0x39);
        else ata_write_cbr(&ATA_PIO_CSD, ata_dma ? 0x25 : 0x29);
    }
    else
    {
        ata_write_cbr(&ATA_PIO_SCR, (cnt << 3) & 0xff);
        ata_write_cbr(&ATA_PIO_LHR, (sector >> 13) & 0xff);
        ata_write_cbr(&ATA_PIO_LMR, (sector >> 5) & 0xff);
        ata_write_cbr(&ATA_PIO_LLR, (sector << 3) & 0xff);
        ata_write_cbr(&ATA_PIO_DVR, BIT(6) | ((sector >> 21) & 0xf));
        if (write) ata_write_cbr(&ATA_PIO_CSD, ata_dma ? 0xca : 0x30);
        else ata_write_cbr(&ATA_PIO_CSD, ata_dma ? 0xc8 : 0xc4);
    }
    if (ata_dma)
    {
        PASS_RC(ata_wait_for_start_of_transfer(500000), 2, 1);
        if (write)
        {
            ATA_SBUF_START = buffer;
            ATA_SBUF_SIZE = SECTOR_SIZE * cnt;
            ATA_CFG |= BIT(4);
        }
        else
        {
            ATA_TBUF_START = buffer;
            ATA_TBUF_SIZE = SECTOR_SIZE * cnt;
            ATA_CFG &= ~BIT(4);
        }
        ATA_XFR_NUM = SECTOR_SIZE * cnt - 1;
        ATA_CFG |= ata_dma_flags;
        ATA_CFG &= ~(BIT(7) | BIT(8));
        wakeup_wait(&ata_wakeup, TIMEOUT_NONE);
        ATA_IRQ = BITRANGE(0, 4);
        ATA_IRQ_MASK = BIT(0);
        ATA_COMMAND = BIT(0);
        if (wakeup_wait(&ata_wakeup, 500000) == THREAD_TIMEOUT)
        {
            ATA_COMMAND = BIT(1);
            ATA_CFG &= ~(BITRANGE(2, 3) | BIT(12));
            RET_ERR(2);
        }
        ATA_COMMAND = BIT(1);
        ATA_CFG &= ~(BITRANGE(2, 3) | BIT(12));
    }
    else
    {
        cnt *= SECTOR_SIZE / 512;
        while (cnt--)
        {
            int i;
            PASS_RC(ata_wait_for_start_of_transfer(500000), 2, 1);
            if (write)
                for (i = 0; i < 256; i++)
                    ata_write_cbr(&ATA_PIO_DTR, ((uint16_t*)buffer)[i]);
            else
                for (i = 0; i < 256; i++)
                    ((uint16_t*)buffer)[i] = ata_read_cbr(&ATA_PIO_DTR);
            buffer += 512;
        }
    }
    PASS_RC(ata_wait_for_end_of_transfer(100000), 2, 3);
    return 0;
}

int ata_rw_sectors(uint64_t sector, uint32_t count, void* buffer, bool write)
{
#ifdef ATA_HAVE_BBT
    if (sector + count > ata_virtual_sectors) RET_ERR(0);
    while (count)
    {
        uint32_t offset;
        uint32_t l0idx = sector >> 15;
        uint32_t l0offs = sector & 0x7fff;
        uint32_t cnt = MIN(count, 0x8000 - l0offs);
        uint32_t l0data = ata_bbt[0][l0idx << 1];
        uint32_t base = ata_bbt[0][(l0idx << 1) | 1] << 12;
        if (l0data < 0x8000) offset = l0data + base;
        else
        {
            uint32_t l1idx = (sector >> 10) & 0x1f;
            uint32_t l1offs = sector & 0x3ff;
            cnt = MIN(count, 0x400 - l1offs);
            uint32_t l1data = ata_bbt[l0data & 0x7fff][l1idx];
            if (l1data < 0x8000) offset = l1data + base;
            else
            {
                uint32_t l2idx = (sector >> 5) & 0x1f;
                uint32_t l2offs = sector & 0x1f;
                cnt = MIN(count, 0x20 - l2offs);
                uint32_t l2data = ata_bbt[l1data & 0x7fff][l2idx];
                if (l2data < 0x8000) offset = l2data + base;
                else
                {
                    uint32_t l3idx = sector & 0x1f;
                    uint32_t l3data = ata_bbt[l2data & 0x7fff][l3idx];
                    for (cnt = 1; cnt < count && l3idx + cnt < 0x20; cnt++)
                        if (ata_bbt[l2data & 0x7fff][l3idx + cnt] != l3data)
                            break;
                    offset = l3data + base;
                }
            }
        }
        uint64_t phys = sector + offset;
        if (offset != ata_last_offset && phys - ata_last_phys < 64) ata_soft_reset();
        ata_last_offset = offset;
        ata_last_phys = phys + cnt;
        PASS_RC(ata_rw_sectors_internal(phys, cnt, buffer, write), 0, 0);
        buffer += cnt * SECTOR_SIZE;
        sector += cnt;
        count -= cnt;
    }
    return 0;
}

int ata_rw_sectors_internal(uint64_t sector, uint32_t count, void* buffer, bool write)
{
#endif
    if (sector + count > ata_total_sectors) RET_ERR(0);
    if (!ata_powered) ata_power_up();
    ata_set_active();
    if (ata_dma && write) clean_dcache();
    else if (ata_dma) invalidate_dcache();
    ATA_COMMAND = BIT(1);
    while (count)
    {
        uint32_t cnt = MIN(ata_lba48 ? 8192 : 32, count);
        int rc = -1;
        int tries = 3;
        while (tries-- && rc)
        {
            rc = ata_rw_chunk(sector, cnt, buffer, write);
            if (rc) ata_soft_reset();
        }
        if (rc)
        {
            void* buf = buffer;
            int sect;
            for (sect = sector; sect < sector + cnt; sect++)
            {
                rc = -1;
                tries = 3;
                while (tries-- && rc)
                {
                    rc = ata_rw_chunk(sect, 1, buf, write);
                    if (rc) ata_soft_reset();
                }
                if (rc) break;
                buf += SECTOR_SIZE;
            }
        }
        PASS_RC(rc, 1, 1);
        buffer += SECTOR_SIZE * cnt;
        sector += cnt;
        count -= cnt;
    }
    ata_set_active();
    return 0;
}

static void ata_thread(void)
{
    while (true)
    {
        mutex_lock(&ata_mutex, TIMEOUT_BLOCK);
        if (TIME_AFTER(USEC_TIMER, ata_last_activity_value + ata_sleep_timeout) && ata_powered)
            ata_power_down();
        mutex_unlock(&ata_mutex);
        sleep(1000000);
    }
}

/* API Functions */
int ata_soft_reset()
{
    mutex_lock(&ata_mutex, TIMEOUT_BLOCK);
    if (!ata_powered) ata_power_up();
    ata_set_active();
    ata_write_cbr(&ATA_PIO_DAD, BIT(1) | BIT(2));
    sleep(10);
    ata_write_cbr(&ATA_PIO_DAD, 0);
    PASS_RC_MTX(ata_wait_for_rdy(60000000), 0, 0, &ata_mutex);
    ata_set_active();
    mutex_unlock(&ata_mutex);
}

int ata_read_sectors(IF_MD2(int drive,) unsigned long start, int incount,
                     void* inbuf)
{
    mutex_lock(&ata_mutex, TIMEOUT_BLOCK);
    int rc = ata_rw_sectors(start, incount, inbuf, false);
    mutex_unlock(&ata_mutex);
    return rc;
}

int ata_write_sectors(IF_MD2(int drive,) unsigned long start, int count,
                      const void* outbuf)
{
    mutex_lock(&ata_mutex, TIMEOUT_BLOCK);
    int rc = ata_rw_sectors(start, count, (void*)((uint32_t)outbuf), true);
    mutex_unlock(&ata_mutex);
    return rc;
}

void ata_spindown(int seconds)
{
    ata_sleep_timeout = seconds * 1000000;
}

void ata_sleep(void)
{
    mutex_lock(&ata_mutex, TIMEOUT_BLOCK);
    ata_power_down();
    mutex_unlock(&ata_mutex);
}

void ata_sleepnow(void)
{
    ata_sleep();
}

void ata_close(void)
{
    ata_sleep();
}

void ata_spin(void)
{
    ata_power_up();
}

void ata_get_info(IF_MD2(int drive,) struct storage_info *info)
{
    (*info).sector_size = SECTOR_SIZE;
#ifdef ATA_HAVE_BBT
    (*info).num_sectors = ata_virtual_sectors;
#else
    (*info).num_sectors = ata_total_sectors;
#endif
    (*info).vendor = "Apple";
    (*info).product = "iPod Classic";
    (*info).revision = "1.0";
}

long ata_last_disk_activity(void)
{
    return ata_last_activity_value;
}

int ata_init(void)
{
    mutex_init(&ata_mutex);
    wakeup_init(&ata_wakeup);
    PCON(7) = 0x44444444;
    PCON(8) = 0x44444444;
    PCON(9) = 0x44444444;
    PCON(10) = (PCON(10) & ~0xffff) | 0x4444;
    ata_powered = false;
    ata_total_sectors = 0;
#ifdef ATA_HAVE_BBT
    mutex_lock(&ata_mutex, TIMEOUT_BLOCK);
    memset(ata_bbt, 0, sizeof(ata_bbt));
    ata_power_up();
    uint32_t* buf = (uint32_t*)(ata_bbt[ARRAYLEN(ata_bbt) - 64]);
    ata_bbt_read_sectors(0, 1, buf);
    if (!memcmp(buf, "emBIbbth", 8))
    {
        ata_virtual_sectors = (((uint64_t)buf[0x1fd]) << 32) | buf[0x1fc];
        uint32_t count = buf[0x1ff];
        if (count > (ATA_BBT_PAGES >> 6))
            panicf(PANIC_KILLTHREAD, "ATA: BBT too big! (%d pages, limit: %d)\n", count << 6, ATA_BBT_PAGES);
        uint32_t i;
        uint32_t cnt;
        for (i = 0; i < count; i += cnt)
        {
            uint32_t phys = buf[0x200 + i];
            for (cnt = 1; cnt < count; cnt++)
                if (buf[0x200 + i + cnt] != phys + cnt)
                    break;
            ata_bbt_read_sectors(phys, cnt, ata_bbt[i << 6]);
        }
    }
    else ata_virtual_sectors = ata_total_sectors;
    mutex_unlock(&ata_mutex);
#endif
    thread_create("ATA idle monitor", ata_thread, ata_stack,
                  sizeof(ata_stack), USER_THREAD, 1, true);
    return 0;
}

#ifdef CONFIG_STORAGE_MULTI
int ata_num_drives(int first_drive)
{
    /* We don't care which logical drive number(s) we have been assigned */
    (void)first_drive;
    
    return 1;
}
#endif

void INT_ATA()
{
    uint32_t ata_irq = ATA_IRQ;
    ATA_IRQ = ata_irq;
    if (ata_irq & ATA_IRQ_MASK) wakeup_signal(&ata_wakeup);
    ATA_IRQ_MASK = 0;
}
