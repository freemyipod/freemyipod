/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2026 Viktor Krapivin
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

/* The NAND worker serializes these operations. Logical sectors are 4096 bytes. */
#ifndef __FTL_TARGET_H__
#define __FTL_TARGET_H__

#include "libc/include/stdint.h"

/* Mount after nand_phys_init(). Load the newest context or rebuild the map
 * without writing to flash. Return 0 or a negative NAND_ERR_* value. */
int ftl_init(void);

/* Read logical sectors into buffer. Unwritten sectors read as 0xff.
 * Return 0, -1 for an unmounted FTL or invalid range, -2 for bad data,
 * or -3 for a controller timeout. */
int ftl_read(uint32_t sector, uint32_t count, void *buffer);

/* Write sectors or publish a checkpoint. A failed mutation blocks further
 * writes and checkpoints until remount. */
int ftl_write(uint32_t sector, uint32_t count, const void *buffer);
int ftl_flush(void);

/* Logical sector count (0 before mounting) and mount-recovery diagnostics. */
uint32_t ftl_num_sectors(void);
_Bool ftl_was_restored(void);

#endif /* __FTL_TARGET_H__ */
