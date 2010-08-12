/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Daniel Stenberg
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

#ifndef __CONFIGMAGIC_H__
#define __CONFIGMAGIC_H__


/* symbolic names for multiple choice configurations: */

/* CONFIG_STORAGE (note these are combineable bit-flags) */
#define STORAGE_ATA     0x01
#define STORAGE_MMC     0x02
#define STORAGE_SD      0x04
#define STORAGE_NAND    0x08
#define STORAGE_RAMDISK 0x10


#include CONFIG_H
#include TARGET_H


#ifndef SCHEDULER_TICK
#define SCHEDULER_TICK 1048576
#endif

#ifndef SYSTEM_TICK
#define SYSTEM_TICK 10000
#endif

#ifndef MAX_THREADS
#define MAX_THREADS 32
#endif


/* Storage related config handling */

#if (CONFIG_STORAGE & (CONFIG_STORAGE - 1)) != 0
/* Multiple storage drivers */
#define CONFIG_STORAGE_MULTI
#endif

/* Explicit HAVE_MULTIVOLUME in the config file. Allow the maximum number */
#ifdef HAVE_MULTIVOLUME
#define NUM_VOLUMES_PER_DRIVE 4
#else
#define NUM_VOLUMES_PER_DRIVE 1
#endif
#if defined(CONFIG_STORAGE_MULTI) && !defined(HAVE_MULTIDRIVE)
#define HAVE_MULTIDRIVE
#endif

#if defined(HAVE_MULTIDRIVE) && !defined(HAVE_MULTIVOLUME)
#define HAVE_MULTIVOLUME
#endif

#if defined(HAVE_MULTIDRIVE) && !defined(NUM_DRIVES)
#error HAVE_MULTIDRIVE needs to have an explicit NUM_DRIVES
#endif

#ifndef NUM_DRIVES
#define NUM_DRIVES 1
#endif

#define NUM_VOLUMES (NUM_DRIVES * NUM_VOLUMES_PER_DRIVE)


#endif