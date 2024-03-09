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
#ifndef __STORAGE_ATA_H__
#define __STORAGE_ATA_H__

#include "global.h"
#include "mv.h" /* for HAVE_MULTIDRIVE or not */

struct storage_info;

void ata_spindown(int seconds);
void ata_sleep(void);
void ata_sleepnow(void);
bool ata_disk_is_active(void);
int  ata_soft_reset(void);
int  ata_init(void);
void ata_close(void);
int  ata_read_sectors(IF_MD2(int drive,) unsigned long start, int count, void* buf);
int  ata_write_sectors(IF_MD2(int drive,) unsigned long start, int count, const void* buf);
#ifdef HAVE_STORAGE_FLUSH
int  ata_flush(void);
#endif
void ata_spin(void);

#ifdef STORAGE_GET_INFO
void ata_get_info(IF_MD2(int drive,) struct storage_info *info);
#endif

long ata_last_disk_activity(void);

#ifdef CONFIG_STORAGE_MULTI
int ata_num_drives(int first_drive);
#endif

#endif