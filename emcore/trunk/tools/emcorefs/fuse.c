//
//
//    Copyright 2013 user890104
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

#include "global.h"

#include "fuse.h"
#include "util.h"
#include "cache.h"
#include "emcore.h"

int32_t emcorefs_init(void) {
    int32_t res;
    uint32_t count;

    res = emcore_file_close_all(&count);

    if (res != EMCORE_SUCCESS) {
        return res;
    }

    res = emcore_dir_close_all(&count);

    return res;
}

int32_t emcorefs_getattr(const char *path, struct stat *stbuf) {
    int32_t res = 0;
    struct emcore_dir_entry *entry = NULL, curr_entry;
    uint32_t dir_handle;
    char *parent, *filename;

    memset(stbuf, 0, sizeof(*stbuf));

#ifdef DEBUG2
    if (strcmp(path, "/__cache_dump") == 0) {
        cache_dump();
    }
#endif
    entry = cache_get(path);

    if (!entry) {
        parent = strdup(path);
        dirname(parent);
        filename = basename((char *) path);

        res = emcore_dir_open(&dir_handle, parent);

        if (res == EMCORE_SUCCESS) {
            while (1) {
                res = emcore_dir_read(&curr_entry, dir_handle);

                if (res == EMCORE_ERROR_NO_MORE_ENTRIES) {
                    break;
                }

                if (res != EMCORE_SUCCESS) {
                    break;
                }
                
                cache_insert(parent, &curr_entry);

                if (strcmp(filename, curr_entry.name) == 0) {
                    entry = malloc(sizeof(*entry));

                    memcpy(entry, &curr_entry, sizeof(curr_entry));

                    break;
                }
            };

            emcore_dir_close(dir_handle);
        }

        free(parent);
    }

    if (!entry) {
        if (res == EMCORE_ERROR_NO_MORE_ENTRIES) {
            return -ENOENT;
        }

        return -EIO;
    }
    else {
        stbuf->st_uid = getuid();
        stbuf->st_gid = getgid();
        stbuf->st_mtime = fat_time_to_unix_ts(entry->wrttime, entry->wrtdate);

        if (entry->attributes & 0x10) {
            stbuf->st_mode = S_IFDIR | 0755;
            stbuf->st_nlink = 2;
            stbuf->st_size = 0x1000;
        }
        else {
            stbuf->st_mode = S_IFREG | 0644;
            stbuf->st_nlink = 1;
            stbuf->st_size = entry->size;
        }
    }

    return 0;
}

int32_t emcorefs_opendir(const char *path, struct fuse_file_info *fi) {
    int32_t res;
    uint32_t handle;

    res = emcore_dir_open(&handle, path);

    if (res != EMCORE_SUCCESS) {
        return -EIO;
    }
    
    fi->fh = handle;

    return 0;
}

int32_t emcorefs_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi) {
    int32_t res;
    struct emcore_dir_entry entry;
    (void) offset;
    (void) fi;

    if (strcmp(path, "/") == 0) {
        filler(buf, ".", NULL, 0);
        filler(buf, "..", NULL, 0);
    }

    while (1) {
        res = emcore_dir_read(&entry, fi->fh);

        if (res == EMCORE_ERROR_NO_MORE_ENTRIES) {
            break;
        }

        if (res != EMCORE_SUCCESS) {
            return -EIO;
        }

        cache_insert(path, &entry);

        filler(buf, entry.name, NULL, 0);
    }

    return 0;
}

int32_t emcorefs_releasedir(const char *path, struct fuse_file_info *fi) {
    int32_t res;
    uint32_t emcore_errno_value;
    (void)path;

    res = emcore_dir_close(fi->fh);

    if (res == EMCORE_ERROR_IO) {
        res = emcore_errno(&emcore_errno_value);

        if (res != EMCORE_SUCCESS) {
            return -EIO;
        }

        if (emcore_errno_value == EMCORE_SUCCESS) {
            return -emcore_errno_value;
        }
    }
    
    if (res != EMCORE_SUCCESS) {
        return -EIO;
    }
    
    return 0;
}

int32_t emcorefs_open(const char *path, struct fuse_file_info *fi) {
    int32_t res;
    uint32_t handle, emcore_errno_value;
    
#ifdef DEBUG
    fprintf(stderr, "FILE OPEN: [%s], 0x%08x\n", path, fi->flags);
#endif
    
    res = emcore_file_open(&handle, path, fi->flags);
    
    if (res == EMCORE_ERROR_IO) {
        res = emcore_errno(&emcore_errno_value);

        if (res != EMCORE_SUCCESS) {
            return -EIO;
        }

        if (emcore_errno_value != EMCORE_SUCCESS) {
            return -emcore_errno_value;
        }
    }
    
    if (res != EMCORE_SUCCESS) {
        return -EIO;
    }
    
    fi->fh = handle;
    
    return 0;
}

int32_t emcorefs_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
#ifdef DEBUG2
    fprintf(stderr, "FUSE_READ: path=[%s] size=[%d] offset=[%jd] fi->flags=[%d]\n", path, size, offset, fi->flags);
#else
    (void)path;
#endif
    int32_t res;
    uint32_t emcore_errno_value, addr, nread = size;

    if (!fi->fh) {
        return -EIO;
    }
    
    res = emcore_malloc(&addr, size);
    
    if (res != EMCORE_SUCCESS) {
        return -EIO;
    }
    
    do {
        if (offset) {
            res = emcore_file_seek(fi->fh, offset, SEEK_SET);
            
            if (res == EMCORE_ERROR_IO) {
                res = emcore_errno(&emcore_errno_value);

                if (res != EMCORE_SUCCESS) {
                    nread = -EIO;
                    break;
                }

                if (emcore_errno_value != EMCORE_SUCCESS) {
                    nread = -emcore_errno_value;
                    break;
                }
            }
        
            if (res != EMCORE_SUCCESS) {
                nread = -EIO;
                break;
            }
        }
        
        res = emcore_file_read(&nread, fi->fh, addr, size);
        
        if (res == EMCORE_ERROR_IO) {
            res = emcore_errno(&emcore_errno_value);

            if (res != EMCORE_SUCCESS) {
                nread = -EIO;
                break;
            }

            if (emcore_errno_value != EMCORE_SUCCESS) {
                nread = -emcore_errno_value;
                break;
            }
        }
    
        if (res != EMCORE_SUCCESS) {
            nread = -EIO;
            break;
        }
        
        res = emcore_read(buf, addr, nread);
        
        if (res != EMCORE_SUCCESS) {
            nread = -EIO;
        }
    }
    while(0);
    
    res = emcore_free(addr);
    
    if (res != EMCORE_SUCCESS) {
        return -EIO;
    }
    
    return nread;
}

int32_t emcorefs_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
#ifdef DEBUG2
    fprintf(stderr, "FUSE_WRITE: path=[%s] size=[%d] offset=[%jd] fi->flags=[%d]\n", path, size, offset, fi->flags);
#else
    (void)path;
#endif
    int32_t res;
    uint32_t emcore_errno_value, addr, nwrite = size;

    if (!fi->fh) {
        return -EIO;
    }
    
    res = emcore_malloc(&addr, size);
    
    if (res != EMCORE_SUCCESS) {
        return -EIO;
    }
    
    do {
        if (offset) {
            res = emcore_file_seek(fi->fh, offset, SEEK_SET);
            
            if (res == EMCORE_ERROR_IO) {
                res = emcore_errno(&emcore_errno_value);

                if (res != EMCORE_SUCCESS) {
                    nwrite = -EIO;
                    break;
                }

                if (emcore_errno_value != EMCORE_SUCCESS) {
                    nwrite = -emcore_errno_value;
                    break;
                }
            }
        
            if (res != EMCORE_SUCCESS) {
                nwrite = -EIO;
                break;
            }
        }
        
        res = emcore_write(buf, addr, nwrite);
        
        if (res != EMCORE_SUCCESS) {
            nwrite = -EIO;
            break;
        }
        
        res = emcore_file_write(&nwrite, fi->fh, addr, size);
        
        if (res == EMCORE_ERROR_IO) {
            res = emcore_errno(&emcore_errno_value);

            if (res != EMCORE_SUCCESS) {
                nwrite = -EIO;
                break;
            }

            if (emcore_errno_value != EMCORE_SUCCESS) {
                nwrite = -emcore_errno_value;
                break;
            }
        }
    
        if (res != EMCORE_SUCCESS) {
            nwrite = -EIO;
        }
    }
    while(0);
    
    res = emcore_free(addr);
    
    if (res != EMCORE_SUCCESS) {
        return -EIO;
    }
    
    cache_remove(path);
    
    return nwrite;
}

int32_t emcorefs_release(const char *path, struct fuse_file_info *fi) {
    int32_t res;
    uint32_t emcore_errno_value;
    (void)path;

    if (!fi->fh) {
        return -EIO;
    }
    
    res = emcore_file_close(fi->fh);
    
    if (EMCORE_ERROR_IO == res) {
        res = emcore_errno(&emcore_errno_value);

        if (res != EMCORE_SUCCESS) {
            return -EIO;
        }

        if (emcore_errno_value != EMCORE_SUCCESS) {
            return -emcore_errno_value;
        }
    }
    
    if (res != EMCORE_SUCCESS) {
        return -EIO;
    }
    
    return 0;
}

int32_t emcorefs_mkdir(const char *path, mode_t mode) {
    (void)mode;
    int32_t res;
    uint32_t emcore_errno_value;

    res = emcore_dir_create(path);
    
    if (res == EMCORE_ERROR_IO) {
        res = emcore_errno(&emcore_errno_value);

        if (res != EMCORE_SUCCESS) {
            return -EIO;
        }

        if (emcore_errno_value != EMCORE_SUCCESS) {
            return -emcore_errno_value;
        }
    }
    
    if (res != EMCORE_SUCCESS) {
        return -EIO;
    }
    
    return 0;
}

int32_t emcorefs_rmdir(const char *path) {
    int32_t res;
    uint32_t emcore_errno_value;

    res = emcore_dir_remove(path);
    
    if (res == EMCORE_ERROR_IO) {
        res = emcore_errno(&emcore_errno_value);

        if (res != EMCORE_SUCCESS) {
            return -EIO;
        }

        if (emcore_errno_value != EMCORE_SUCCESS) {
            return -emcore_errno_value;
        }
    }
    
    if (res != EMCORE_SUCCESS) {
        return -EIO;
    }
    
    cache_remove(path);
    
    return 0;
}

int32_t emcorefs_create(const char *path, mode_t mode, struct fuse_file_info *fi) {
    (void)mode;
    return emcorefs_open(path, fi);
}

int32_t emcorefs_mknod(const char *path, mode_t mode, dev_t dev) {
    (void)dev;
    int32_t res;
    struct fuse_file_info fi;
    
    fi.flags = O_WRONLY | O_CREAT | O_TRUNC;
    
    res = emcorefs_create(path, mode, &fi);
    
    if (res) {
        return res;
    }
    
    return emcorefs_release(path, &fi);
}

int32_t emcorefs_unlink(const char *path)
{
    int32_t res;
    uint32_t emcore_errno_value;

    res = emcore_file_unlink(path);
    
    if (res == EMCORE_ERROR_IO) {
        res = emcore_errno(&emcore_errno_value);

        if (res != EMCORE_SUCCESS) {
            return -EIO;
        }

        if (emcore_errno_value != EMCORE_SUCCESS) {
            return -emcore_errno_value;
        }
    }
    
    if (res != EMCORE_SUCCESS) {
        return -EIO;
    }
    
    cache_remove(path);
    
    return 0;
}

int32_t emcorefs_rename(const char *path, const char *new_path) {
    int32_t res;
    uint32_t emcore_errno_value;

    res = emcore_file_rename(path, new_path);

    if (res == EMCORE_ERROR_IO) {
        res = emcore_errno(&emcore_errno_value);

        if (res != EMCORE_SUCCESS) {
            return -EIO;
        }

        if (emcore_errno_value != EMCORE_SUCCESS) {
            return -emcore_errno_value;
        }
    }
    
    if (res != EMCORE_SUCCESS) {
        return -EIO;
    }
    
    cache_remove(path);
    
    return 0;
}

int32_t emcorefs_truncate(const char *path, off_t size) {
    int32_t res;
    struct fuse_file_info fi;
    
    res = emcorefs_open(path, &fi);
    
    if (res) {
        return res;
    }
    
    res = emcorefs_ftruncate(path, size, &fi);
    
    if (res) {
        return res;
    }
    
    return emcorefs_release(path, &fi);
}

int32_t emcorefs_ftruncate(const char *path, off_t size, struct fuse_file_info *fi) {
    int32_t res;
    uint32_t emcore_errno_value;
    (void)path;

    if (!fi->fh) {
        return -EIO;
    }
    
    res = emcore_file_truncate(fi->fh, size);
    
    if (res == EMCORE_ERROR_IO) {
        res = emcore_errno(&emcore_errno_value);

        if (res != EMCORE_SUCCESS) {
            return -EIO;
        }

        if (emcore_errno_value != EMCORE_SUCCESS) {
            return -emcore_errno_value;
        }
    }
    
    if (res != EMCORE_SUCCESS) {
        return -EIO;
    }
    
    cache_remove(path);
    
    return 0;
}
