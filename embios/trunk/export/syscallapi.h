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


#ifndef __SYSCALLAPI_H__
#define __SYSCALLAPI_H__


#include "../libc/include/assert.h"

#include "../global.h"
#include "../panic.h"
#include "../console.h"
#include "../disk.h"
#include "../dir.h"
#include "../file.h"
#include "../format.h"
#include "../drawing.h"
#include "../lcdconsole.h"
#include "../storage.h"
#include "../shutdown.h"
#include "../thread.h"
#include "../ucl.h"
#include "../bootflash.h"
#include "../timer.h"
#include "../i2c.h"
#include "../interrupt.h"
#include "../lcd.h"
#include "../mmu.h"
#include "../power.h"
#include "../execimage.h"
#include "../backlight.h"
#include "../syscall.h"
#include "../progressbar.h"
#include "../button.h"
#include "../clickwheel.h"
#include "../clockgates.h"
#include "../contextswitch.h"
#include "../hwkeyaes.h"
#include "../hmacsha1.h"
#include "../libc/include/string.h"
#include "../libc/include/stdlib.h"
#include "../libc/include/stdio.h"
#include "../libc/tlsf/tlsf.h"

/* increase this every time the api struct changes */
#define EMBIOS_API_VERSION 0

/* update this to latest version if a change to the api struct breaks
   backwards compatibility (and please take the opportunity to sort in any
   new function which are "waiting" at the end of the function table) */
#define EMBIOS_MIN_API_VERSION 0

/* NOTE: To support backwards compatibility, only add new functions at
         the end of the structure.  Every time you add a new function,
         remember to increase EMBIOS_API_VERSION. If you make changes to the
         existing APIs, also update EMBIOS_MIN_API_VERSION to current version */

struct embios_syscall_table
{
    uint32_t table_version;
    uint32_t table_minversion;
	typeof(panic) *panic;
	typeof(panicf) *panicf;
    typeof(cprintf) *cprintf;
    typeof(cvprintf) *cvprintf;
    typeof(cputc) *cputc;
    typeof(cputs) *cputs;
    typeof(cwrite) *cwrite;
    typeof(cflush) *cflush;
    typeof(cgetc) *cgetc;
    typeof(cread) *cread;
    typeof(creada) *creada;
    typeof(opendir) *opendir;
    typeof(closedir) *closedir;
    typeof(readdir) *readdir;
    typeof(mkdir) *mkdir;
    typeof(rmdir) *rmdir;
    typeof(renderbmp) *renderbmp;
    typeof(renderchar) *renderchar;
    typeof(rendertext) *rendertext;
    typeof(renderfillrect) *renderfillrect;
    typeof(get_font_width) *get_font_width;
    typeof(get_font_height) *get_font_height;
    typeof(execimage) *execimage;
    typeof(ftruncate) *ftruncate;
    typeof(fsync) *fsync;
    typeof(close) *close;
    typeof(write) *write;
    typeof(read) *read;
    typeof(lseek) *lseek;
    typeof(remove) *remove;
    typeof(file_open) *file_open;
    typeof(rename) *rename;
    typeof(file_creat) *file_creat;
    typeof(filesize) *filesize;
    typeof(format) *format;
    typeof(vuprintf) *vuprintf;
    typeof(lcdconsole_putc_noblit) *lcdconsole_putc_noblit;
    typeof(lcdconsole_puts_noblit) *lcdconsole_puts_noblit;
    typeof(lcdconsole_write_noblit) *lcdconsole_write_noblit;
    typeof(lcdconsole_update) *lcdconsole_update;
    typeof(lcdconsole_putc) *lcdconsole_putc;
    typeof(lcdconsole_puts) *lcdconsole_puts;
    typeof(lcdconsole_write) *lcdconsole_write;
    typeof(lcdconsole_get_current_x) *lcdconsole_get_current_x;
    typeof(lcdconsole_get_current_y) *lcdconsole_get_current_y;
    typeof(lcdconsole_get_lineend_x) *lcdconsole_get_lineend_x;
    typeof(lcdconsole_get_lineend_y) *lcdconsole_get_lineend_y;
    typeof(lcdconsole_progressbar) *lcdconsole_progressbar;
    typeof(progressbar_init) *progressbar_init;
    typeof(progressbar_setpos) *progressbar_setpos;
    typeof(shutdown) *shutdown;
    typeof(storage_read_sectors_md) *storage_read_sectors_md;
    typeof(storage_write_sectors_md) *storage_write_sectors_md;
    typeof(storage_get_info) *storage_get_info;
    typeof(strcasecmp) *strcasecmp;
    typeof(strncasecmp) *strncasecmp;
    typeof(strcasestr) *strcasestr;
    typeof(strlcat) *strlcat;
    typeof(strlcpy) *strlcpy;
    typeof(mutex_init) *mutex_init;
    typeof(mutex_lock) *mutex_lock;
    typeof(mutex_unlock) *mutex_unlock;
    typeof(wakeup_init) *wakeup_init;
    typeof(wakeup_wait) *wakeup_wait;
    typeof(wakeup_signal) *wakeup_signal;
    typeof(sleep) *sleep;
    typeof(thread_create) *thread_create;
    typeof(thread_exit) *thread_exit;
    typeof(thread_suspend) *thread_suspend;
    typeof(thread_resume) *thread_resume;
    typeof(thread_terminate) *thread_terminate;
    typeof(__errno) *__errno;
    typeof(ucl_decompress) *ucl_decompress;
    typeof(bootflash_filesize) *bootflash_filesize;
    typeof(bootflash_attributes) *bootflash_attributes;
    typeof(bootflash_getaddr) *bootflash_getaddr;
    typeof(bootflash_read) *bootflash_read;
    typeof(bootflash_readraw) *bootflash_readraw;
    typeof(bootflash_writeraw) *bootflash_writeraw;
    typeof(bootflash_getrawaddr) *bootflash_getrawaddr;
    typeof(bootflash_is_memmapped) *bootflash_is_memmapped;
    typeof(read_native_timer) *read_native_timer;
    typeof(read_usec_timer) *read_usec_timer;
    typeof(i2c_send) *i2c_send;
    typeof(i2c_recv) *i2c_recv;
    typeof(i2c_sendbyte) *i2c_sendbyte;
    typeof(i2c_recvbyte) *i2c_recvbyte;
    typeof(interrupt_enable) *interrupt_enable;
    typeof(interrupt_set_handler) *interrupt_set_handler;
    typeof(int_timer_set_handler) *int_timer_set_handler;
    typeof(displaylcd) *displaylcd;
    typeof(displaylcd_sync) *displaylcd_sync;
    typeof(displaylcd_busy) *displaylcd_busy;
    typeof(displaylcd_safe) *displaylcd_safe;
    typeof(lcd_get_width) *lcd_get_width;
    typeof(lcd_get_height) *lcd_get_height;
    typeof(lcd_get_bytes_per_pixel) *lcd_get_bytes_per_pixel;
    typeof(lcd_translate_color) *lcd_translate_color;
    typeof(clean_dcache) *clean_dcache;
    typeof(invalidate_dcache) *invalidate_dcache;
    typeof(invalidate_icache) *invalidate_icache;
    typeof(power_off) *power_off;
    typeof(charging_state) *charging_state;
    typeof(atoi) *atoi;
    typeof(memchr) *memchr;
    typeof(memcmp) *memcmp;
    typeof(memcpy) *memcpy;
    typeof(memmove) *memmove;
    typeof(memset) *memset;
    typeof(qsort) *qsort;
    typeof(srand) *srand;
    typeof(rand) *rand;
    typeof(snprintf) *snprintf;
    typeof(vsnprintf) *vsnprintf;
    typeof(sscanf) *sscanf;
    typeof(strcat) *strcat;
    typeof(strchr) *strchr;
    typeof(strcmp) *strcmp;
    typeof(strcpy) *strcpy;
    typeof(strlen) *strlen;
    typeof(strncmp) *strncmp;
    typeof(strrchr) *strrchr;
    typeof(strstr) *strstr;
    typeof(strtok_r) *strtok_r;
    typeof(backlight_on) *backlight_on;
    typeof(backlight_set_fade) *backlight_set_fade;
    typeof(backlight_set_brightness) *backlight_set_brightness;
    typeof(get_platform_id) *get_platform_id;
    typeof(tlsf_create) *tlsf_create;
    typeof(tlsf_destroy) *tlsf_destroy;
    typeof(tlsf_malloc) *tlsf_malloc;
    typeof(tlsf_memalign) *tlsf_memalign;
    typeof(tlsf_realloc) *tlsf_realloc;
    typeof(tlsf_free) *tlsf_free;
    typeof(tlsf_walk_heap) *tlsf_walk_heap;
    typeof(tlsf_check_heap) *tlsf_check_heap;
    typeof(tlsf_block_size) *tlsf_block_size;
    typeof(tlsf_overhead) *tlsf_overhead;
    typeof(execfirmware) *execfirmware;
    typeof(button_register_handler) *button_register_handler;
    typeof(button_unregister_handler) *button_unregister_handler;
    typeof(clickwheel_get_state) *clickwheel_get_state;
    typeof(clockgate_enable) *clockgate_enable;
    typeof(context_switch) *context_switch;
    typeof(disk_mount) *disk_mount;
    typeof(disk_unmount) *disk_unmount;
    typeof(hwkeyaes) *hwkeyaes;
    typeof(hmacsha1) *hmacsha1;
    typeof(reset) *reset;
};


#endif
