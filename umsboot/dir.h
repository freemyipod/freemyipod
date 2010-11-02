/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id: dir.h 13741 2007-06-30 02:08:27Z jethead71 $
 *
 * Copyright (C) 2002 by Björn Stenberg
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
#ifndef __DIR_H__
#define __DIR_H__

#include "global.h"
#include "thread.h"
#include "file.h"

#define ATTR_READ_ONLY   0x01
#define ATTR_HIDDEN      0x02
#define ATTR_SYSTEM      0x04
#define ATTR_VOLUME_ID   0x08
#define ATTR_DIRECTORY   0x10
#define ATTR_ARCHIVE     0x20
#define ATTR_VOLUME      0x40 /* this is a volume, not a real directory */

struct dirent {
    unsigned char d_name[MAX_PATH];
    int attribute;
    long size;
    long startcluster;
    unsigned short wrtdate; /*  Last write date */ 
    unsigned short wrttime; /*  Last write time */
};

#include "fat.h"

typedef struct {
    bool busy;
    long startcluster;
    struct fat_dir fatdir;
    struct dirent theent;
    struct scheduler_thread* process;
#ifdef HAVE_MULTIVOLUME
    int volumecounter; /* running counter for faked volume entries */
#endif
} DIR;

#ifdef HAVE_HOTSWAP
char *get_volume_name(int volume);
#endif

#ifdef HAVE_MULTIVOLUME
    int strip_volume(const char*, char*);
#endif

extern DIR* opendir(const char* name);
extern int closedir(DIR* dir);
extern int closedir_all_of_process(struct scheduler_thread* process);
extern int mkdir(const char* name);
extern int rmdir(const char* name);

extern struct dirent* readdir(DIR* dir);

extern int release_dirs(int volume);

#endif