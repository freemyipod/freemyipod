/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Alan Korr
 * Copyright (C) 2008 by Frank Gevaerts
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
#ifndef __STORAGE_H__
#define __STORAGE_H__

#include "global.h"
#include "mv.h"

#define STORAGE_GET_INFO

#if (CONFIG_STORAGE & STORAGE_SD)
#include "storage_sd.h"
#endif
#if (CONFIG_STORAGE & STORAGE_MMC)
#include "storage_mmc.h"
#endif
#if (CONFIG_STORAGE & STORAGE_ATA)
#include "storage_ata.h"
#endif
#if (CONFIG_STORAGE & STORAGE_NAND)
#include "storage_nand.h"
#endif
#if (CONFIG_STORAGE & STORAGE_RAMDISK)
#include "storage_ramdisk.h"
#endif

struct storage_info
{
    unsigned int sector_size;
    unsigned int num_sectors;
    char *vendor;
    char *product;
    char *revision;
    void *driverinfo;
};

#if !defined(CONFIG_STORAGE_MULTI)
/* storage_spindown, storage_sleep and storage_spin are passed as
 * pointers, which doesn't work with argument-macros.
 */
    #if (CONFIG_STORAGE & STORAGE_ATA)
        #define STORAGE_FUNCTION(NAME) (ata_## NAME)
    #elif (CONFIG_STORAGE & STORAGE_SD)
        #define STORAGE_FUNCTION(NAME) (sd_## NAME)
     #elif (CONFIG_STORAGE & STORAGE_MMC)
        #define STORAGE_FUNCTION(NAME) (mmc_## NAME)
    #elif (CONFIG_STORAGE & STORAGE_NAND)
        #define STORAGE_FUNCTION(NAME) (nand_## NAME)
    #elif (CONFIG_STORAGE & STORAGE_RAMDISK)
        #define STORAGE_FUNCTION(NAME) (ramdisk_## NAME)
    #else
        //#error No storage driver!
    #endif
#endif

void storage_enable(bool on);
void storage_sleep(void);
void storage_sleepnow(void);
bool storage_disk_is_active(void);
int storage_soft_reset(void);
int storage_init(void);
int storage_flush(void);
void storage_spin(void);
void storage_spindown(int seconds);
long storage_last_disk_activity(void);
int storage_num_drives(void);
#ifdef HAVE_HOTSWAP
bool storage_removable(int drive);
bool storage_present(int drive);
#endif

int storage_read_sectors(IF_MD2(int drive,) unsigned long start, int count, void* buf);
int storage_read_sectors_md(int drive, unsigned long start, int count, void* buf);
int storage_write_sectors(IF_MD2(int drive,) unsigned long start, int count, const void* buf);
int storage_write_sectors_md(int drive, unsigned long start, int count, const void* buf);
#ifdef STORAGE_GET_INFO
void storage_get_info(int drive, struct storage_info *info);
#endif
#endif
