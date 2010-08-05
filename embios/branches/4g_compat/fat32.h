//
//
//    Copyright 2010 TheSeven
//
//
//    This file is part of emBIOS.
//
//    emBIOS is free software: you can redistribute it and/or
//    modify it under the terms of the GNU General Public License as
//    published by the Free Software Foundation, either version 2 of the
//    License, or (at your option) any later version.
//
//    emBIOS is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//    See the GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License along
//    with emBIOS.  If not, see <http://www.gnu.org/licenses/>.
//
//


#ifndef __FAT32_H__
#define __FAT32_H__

#include "global.h"

uint32_t fat32_get_root();
uint32_t fat32_get_clusterchain(uint32_t clusterchain, uint32_t maxsize, void* buffer);
uint32_t fat32_get_direntry(uint32_t clusterchain, const char* filename, uint32_t* filesize);
uint32_t fat32_delete_clusterchain(uint32_t clusterchain);
uint32_t fat32_delete_direntry(uint32_t clusterchain, const char* filename);
uint32_t fat32_store_stream(void* buffer, uint32_t size);
uint32_t fat32_store_direntry(uint32_t dirchain, const char* filename,
                              uint32_t filechain, uint32_t filesize, uint32_t flags);
uint32_t fat32_create_dir(uint32_t parent, const char* dirname);
uint32_t fat32_read_file(const char* filename, uint32_t maxsize, void* buffer, uint32_t* filesize);
uint32_t fat32_resize_patchdirs(uint32_t clusterchain, uint32_t clustoffset);
uint32_t fat32_resize_fulldisk();
uint32_t fat32_get_partition_start();
uint32_t fat32_init();

#endif
