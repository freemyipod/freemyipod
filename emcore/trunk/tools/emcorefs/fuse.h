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

int32_t emcorefs_init(void);
int32_t emcorefs_getattr(const char *path, struct stat *stbuf);
int32_t emcorefs_utimens(const char *path, const struct timespec tv[2]);
int32_t emcorefs_opendir(const char *path, struct fuse_file_info *fi);
int32_t emcorefs_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi);
int32_t emcorefs_releasedir(const char *path, struct fuse_file_info *fi);
int32_t emcorefs_open(const char *path, struct fuse_file_info *fi);
int32_t emcorefs_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi);
int32_t emcorefs_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi);
int32_t emcorefs_release(const char *path, struct fuse_file_info *fi);
int32_t emcorefs_mkdir(const char *path, mode_t mode);
int32_t emcorefs_rmdir(const char *path);
int32_t emcorefs_create(const char *path, mode_t mode, struct fuse_file_info *fi);
int32_t emcorefs_mknod(const char *path, mode_t mode, dev_t dev);
int32_t emcorefs_unlink(const char *path);
int32_t emcorefs_rename(const char *path, const char *new_path);
int32_t emcorefs_truncate(const char *path, off_t size);
int32_t emcorefs_ftruncate(const char *path, off_t size, struct fuse_file_info *fi);

#endif /* __FUSE_H__ */
