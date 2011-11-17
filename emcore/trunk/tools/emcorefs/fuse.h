//
//
//    Copyright 2011 user890104
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


#ifndef __FUSE_H__
#define __FUSE_H__

#include "global.h"

int emcorefs_init(void);
int emcorefs_getattr(const char* path, struct stat* stbuf);
int emcorefs_opendir(const char* path, struct fuse_file_info* fi);
int emcorefs_readdir(const char* path, void* buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info* fi);
int emcorefs_releasedir(const char* path, struct fuse_file_info* fi);
int emcorefs_open(const char* path, struct fuse_file_info* fi);
int emcorefs_read(const char* path, char* buf, uint32_t size, off_t offset, struct fuse_file_info* fi);
int emcorefs_write(const char* path, const char* buf, uint32_t size, off_t offset, struct fuse_file_info* fi);
int emcorefs_release(const char* path, struct fuse_file_info* fi);
int emcorefs_mkdir(const char* path, mode_t mode);
int emcorefs_rmdir(const char* path);
int emcorefs_create(const char* path, mode_t mode, struct fuse_file_info* fi);
int emcorefs_mknod(const char* path, mode_t mode, dev_t dev);
int emcorefs_unlink(const char* path);
int emcorefs_rename(const char* path, const char* new_path);
int emcorefs_truncate(const char* path, off_t size);
int emcorefs_ftruncate(const char* path, off_t size, struct fuse_file_info* fi);

#endif /* __FUSE_H__ */
