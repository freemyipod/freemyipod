//
//
//    Copyright 2010 TheSeven
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


#ifndef __SYSCALLWRAPPERS_H__
#define __SYSCALLWRAPPERS_H__


#define IN_APPLICATION_CODE
#include "syscallapi.h"


extern struct emcore_syscall_table* __emcore_syscall;


#define panic __emcore_syscall->panic
#define panicf __emcore_syscall->panicf
#define cprintf __emcore_syscall->cprintf
#define cvprintf __emcore_syscall->cvprintf
#define cputc __emcore_syscall->cputc
#define cputs __emcore_syscall->cputs
#define cwrite __emcore_syscall->cwrite
#define cflush __emcore_syscall->cflush
#define cgetc __emcore_syscall->cgetc
#define cread __emcore_syscall->cread
#define creada __emcore_syscall->creada
#define opendir __emcore_syscall->opendir
#define closedir __emcore_syscall->closedir
#define readdir __emcore_syscall->readdir
#define mkdir __emcore_syscall->mkdir
#define rmdir __emcore_syscall->rmdir
#define renderchar_native __emcore_syscall->renderchar_native
#define renderchar __emcore_syscall->renderchar
#define rendertext __emcore_syscall->rendertext
#define get_font_width __emcore_syscall->get_font_width
#define get_font_height __emcore_syscall->get_font_height
#define execimage __emcore_syscall->execimage
#define ftruncate __emcore_syscall->ftruncate
#define fsync __emcore_syscall->fsync
#define close __emcore_syscall->close
#define write __emcore_syscall->write
#define read __emcore_syscall->read
#define lseek __emcore_syscall->lseek
#define remove __emcore_syscall->remove
#define file_open __emcore_syscall->file_open
#define rename __emcore_syscall->rename
#define file_creat __emcore_syscall->file_creat
#define filesize __emcore_syscall->filesize
#define format __emcore_syscall->format
#define vuprintf __emcore_syscall->vuprintf
#define lcdconsole_putc_noblit __emcore_syscall->lcdconsole_putc_noblit
#define lcdconsole_puts_noblit __emcore_syscall->lcdconsole_puts_noblit
#define lcdconsole_write_noblit __emcore_syscall->lcdconsole_write_noblit
#define lcdconsole_update __emcore_syscall->lcdconsole_update
#define lcdconsole_putc __emcore_syscall->lcdconsole_putc
#define lcdconsole_puts __emcore_syscall->lcdconsole_puts
#define lcdconsole_write __emcore_syscall->lcdconsole_write
#define lcdconsole_get_current_x __emcore_syscall->lcdconsole_get_current_x
#define lcdconsole_get_current_y __emcore_syscall->lcdconsole_get_current_y
#define lcdconsole_get_lineend_x __emcore_syscall->lcdconsole_get_lineend_x
#define lcdconsole_get_lineend_y __emcore_syscall->lcdconsole_get_lineend_y
#define lcdconsole_progressbar __emcore_syscall->lcdconsole_progressbar
#define progressbar_init __emcore_syscall->progressbar_init
#define progressbar_setpos __emcore_syscall->progressbar_setpos
#define shutdown __emcore_syscall->shutdown
#define storage_read_sectors_md __emcore_syscall->storage_read_sectors_md
#define storage_write_sectors_md __emcore_syscall->storage_write_sectors_md
#define storage_get_info __emcore_syscall->storage_get_info
#define strcasecmp __emcore_syscall->strcasecmp
#define strncasecmp __emcore_syscall->strncasecmp
#define strcasestr __emcore_syscall->strcasestr
#define strlcat __emcore_syscall->strlcat
#define strlcpy __emcore_syscall->strlcpy
#define mutex_init __emcore_syscall->mutex_init
#define mutex_lock __emcore_syscall->mutex_lock
#define mutex_unlock __emcore_syscall->mutex_unlock
#define wakeup_init __emcore_syscall->wakeup_init
#define wakeup_wait __emcore_syscall->wakeup_wait
#define wakeup_signal __emcore_syscall->wakeup_signal
#define sleep __emcore_syscall->sleep
#define thread_create __emcore_syscall->thread_create
#define thread_exit __emcore_syscall->thread_exit
#define thread_suspend __emcore_syscall->thread_suspend
#define thread_resume __emcore_syscall->thread_resume
#define thread_terminate __emcore_syscall->thread_terminate
#define __errno __emcore_syscall->__errno
#define ucl_decompress __emcore_syscall->ucl_decompress
#define bootflash_filesize __emcore_syscall->bootflash_filesize
#define bootflash_attributes __emcore_syscall->bootflash_attributes
#define bootflash_getaddr __emcore_syscall->bootflash_getaddr
#define bootflash_read __emcore_syscall->bootflash_read
#define bootflash_readraw __emcore_syscall->bootflash_readraw
#define bootflash_writeraw __emcore_syscall->bootflash_writeraw
#define bootflash_getrawaddr __emcore_syscall->bootflash_getrawaddr
#define bootflash_is_memmapped __emcore_syscall->bootflash_is_memmapped
#define read_native_timer __emcore_syscall->read_native_timer
#define read_usec_timer __emcore_syscall->read_usec_timer
#define i2c_send __emcore_syscall->i2c_send
#define i2c_recv __emcore_syscall->i2c_recv
#define i2c_sendbyte __emcore_syscall->i2c_sendbyte
#define i2c_recvbyte __emcore_syscall->i2c_recvbyte
#define interrupt_enable __emcore_syscall->interrupt_enable
#define interrupt_set_handler __emcore_syscall->interrupt_set_handler
#define int_timer_set_handler __emcore_syscall->int_timer_set_handler
#define displaylcd __emcore_syscall->displaylcd
#define filllcd __emcore_syscall->filllcd
#define displaylcd_native __emcore_syscall->displaylcd_native
#define filllcd_native __emcore_syscall->filllcd_native
#define displaylcd_sync __emcore_syscall->displaylcd_sync
#define displaylcd_busy __emcore_syscall->displaylcd_busy
#define displaylcd_safe __emcore_syscall->displaylcd_safe
#define lcd_get_width __emcore_syscall->lcd_get_width
#define lcd_get_height __emcore_syscall->lcd_get_height
#define lcd_get_bytes_per_pixel __emcore_syscall->lcd_get_bytes_per_pixel
#define lcd_translate_color __emcore_syscall->lcd_translate_color
#define clean_dcache __emcore_syscall->clean_dcache
#define invalidate_dcache __emcore_syscall->invalidate_dcache
#define invalidate_icache __emcore_syscall->invalidate_icache
#define power_off __emcore_syscall->power_off
#define charging_state __emcore_syscall->charging_state
#define atoi __emcore_syscall->atoi
#define memchr __emcore_syscall->memchr
#define memcmp __emcore_syscall->memcmp
#define memcpy __emcore_syscall->memcpy
#define memmove __emcore_syscall->memmove
#define memset __emcore_syscall->memset
#define qsort __emcore_syscall->qsort
#define srand __emcore_syscall->srand
#define rand __emcore_syscall->rand
#define snprintf __emcore_syscall->snprintf
#define vsnprintf __emcore_syscall->vsnprintf
#define isspace __emcore_syscall->isspace
#define isdigit __emcore_syscall->isdigit
#define isxdigit __emcore_syscall->isxdigit
#define sscanf __emcore_syscall->sscanf
#define strcat __emcore_syscall->strcat
#define strchr __emcore_syscall->strchr
#define strcmp __emcore_syscall->strcmp
#define strcpy __emcore_syscall->strcpy
#define strlen __emcore_syscall->strlen
#define strncmp __emcore_syscall->strncmp
#define strrchr __emcore_syscall->strrchr
#define strstr __emcore_syscall->strstr
#define strtok_r __emcore_syscall->strtok_r
#define backlight_on __emcore_syscall->backlight_on
#define backlight_set_fade __emcore_syscall->backlight_set_fade
#define backlight_set_brightness __emcore_syscall->backlight_set_brightness
#define get_platform_id __emcore_syscall->get_platform_id
#define tlsf_create __emcore_syscall->tlsf_create
#define tlsf_destroy __emcore_syscall->tlsf_destroy
#define tlsf_malloc __emcore_syscall->tlsf_malloc
#define tlsf_memalign __emcore_syscall->tlsf_memalign
#define tlsf_realloc __emcore_syscall->tlsf_realloc
#define tlsf_free __emcore_syscall->tlsf_free
#define tlsf_walk_heap __emcore_syscall->tlsf_walk_heap
#define tlsf_check_heap __emcore_syscall->tlsf_check_heap
#define tlsf_block_size __emcore_syscall->tlsf_block_size
#define tlsf_overhead __emcore_syscall->tlsf_overhead
#define execfirmware __emcore_syscall->execfirmware
#define button_register_handler __emcore_syscall->button_register_handler
#define button_unregister_handler __emcore_syscall->button_unregister_handler
#define clickwheel_get_state __emcore_syscall->clickwheel_get_state
#define clockgate_enable __emcore_syscall->clockgate_enable
#define context_switch __emcore_syscall->context_switch
#define disk_mount __emcore_syscall->disk_mount
#define disk_unmount __emcore_syscall->disk_unmount
#define hwkeyaes __emcore_syscall->hwkeyaes
#define hmacsha1 __emcore_syscall->hmacsha1
#define reset __emcore_syscall->reset
#define int_dma_set_handler __emcore_syscall->int_dma_set_handler
#define thread_set_name __emcore_syscall->thread_set_name
#define thread_set_priority __emcore_syscall->thread_set_priority
#define malloc __emcore_syscall->malloc
#define memalign __emcore_syscall->memalign
#define realloc __emcore_syscall->realloc
#define reownalloc __emcore_syscall->reownalloc
#define free __emcore_syscall->free
#define library_unload __emcore_syscall->library_unload
#define get_library __emcore_syscall->get_library
#define get_library_ext __emcore_syscall->get_library_ext
#define release_library __emcore_syscall->release_library
#define release_library_ext __emcore_syscall->release_library_ext
#define fat_enable_flushing __emcore_syscall->fat_enable_flushing
#define lcd_get_format __emcore_syscall->lcd_get_format
#define crc32 __emcore_syscall->crc32
#define clockgate_get_state __emcore_syscall->clockgate_get_state
#define malloc_walk __emcore_syscall->malloc_walk


#endif
