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


#include "global.h"
#include "syscallapi.h"


struct emcore_syscall_table syscall_table ICONST_ATTR =
{
    .table_version = EMCORE_API_VERSION,
    .table_minversion = EMCORE_MIN_API_VERSION,
#ifdef ARM_ARCH
    .__clzsi2 = __clzsi2,
    .__aeabi_idivmod = __aeabi_idivmod,
    .__aeabi_uidivmod = __aeabi_uidivmod,
#endif
	.panic = panic,
    .panicf = panicf,
    .cprintf = cprintf,
    .cvprintf = cvprintf,
    .cputc = cputc,
    .cputs = cputs,
    .cwrite = cwrite,
    .cflush = cflush,
    .cgetc = cgetc,
    .cread = cread,
    .creada = creada,
    .format = format,
    .vuprintf = vuprintf,
    .shutdown = shutdown,
    .strcasecmp = strcasecmp,
    .strncasecmp = strncasecmp,
    .strcasestr = strcasestr,
    .strlcat = strlcat,
    .strlcpy = strlcpy,
    .mutex_init = mutex_init,
    .mutex_lock = mutex_lock,
    .mutex_unlock = mutex_unlock,
    .wakeup_init = wakeup_init,
    .wakeup_wait = wakeup_wait,
    .wakeup_signal = wakeup_signal,
    .sleep = sleep,
    .thread_create = thread_create,
    .thread_exit = thread_exit,
    .thread_suspend = thread_suspend,
    .thread_resume = thread_resume,
    .thread_terminate = thread_terminate,
    .__errno = __errno,
    .ucl_decompress = ucl_decompress,
    .read_native_timer = read_native_timer,
    .read_usec_timer = read_usec_timer,
    .i2c_send = i2c_send,
    .i2c_recv = i2c_recv,
    .i2c_sendbyte = i2c_sendbyte,
    .i2c_recvbyte = i2c_recvbyte,
    .interrupt_enable = interrupt_enable,
    .interrupt_set_handler = interrupt_set_handler,
    .int_timer_set_handler = int_timer_set_handler,
    .clean_dcache = clean_dcache,
    .invalidate_dcache = invalidate_dcache,
    .invalidate_icache = invalidate_icache,
    .power_off = power_off,
    .charging_state = charging_state,
    .atoi = atoi,
    .memchr = memchr,
    .memcmp = memcmp,
    .memcpy = memcpy,
    .memmove = memmove,
    .memset = memset,
    .qsort = qsort,
    .srand = srand,
    .rand = rand,
    .snprintf = snprintf,
    .vsnprintf = vsnprintf,
    .sscanf = sscanf,
    .strcat = strcat,
    .strchr = strchr,
    .strcmp = strcmp,
    .strcpy = strcpy,
    .strlen = strlen,
    .strncmp = strncmp,
    .strrchr = strrchr,
    .strstr = strstr,
    .strtok_r = strtok_r,
    .get_platform_id = get_platform_id,
    .tlsf_create = tlsf_create,
    .tlsf_destroy = tlsf_destroy,
    .tlsf_malloc = tlsf_malloc,
    .tlsf_memalign = tlsf_memalign,
    .tlsf_realloc = tlsf_realloc,
    .tlsf_free = tlsf_free,
    .tlsf_walk_heap = tlsf_walk_heap,
    .tlsf_check_heap = tlsf_check_heap,
    .tlsf_block_size = tlsf_block_size,
    .tlsf_overhead = tlsf_overhead,
    .execfirmware = execfirmware,
    .clockgate_enable = clockgate_enable,
    .yield = yield,
    .reset = reset,
    .execimage = execimage,
#ifdef HAVE_STORAGE
    .opendir = opendir,
    .closedir = closedir,
    .readdir = readdir,
    .mkdir = mkdir,
    .rmdir = rmdir,
    .ftruncate = ftruncate,
    .fsync = fsync,
    .close = close,
    .write = write,
    .read = read,
    .lseek = lseek,
    .remove = remove,
    .file_open = file_open,
    .rename = rename,
    .file_creat = file_creat,
    .filesize = filesize,
    .storage_read_sectors_md = storage_read_sectors_md,
    .storage_write_sectors_md = storage_write_sectors_md,
    .storage_get_info = storage_get_info,
#ifdef HAVE_HOTSWAP
    .disk_mount = disk_mount,
    .disk_unmount = disk_unmount,
#endif
#endif
#ifdef HAVE_LCD
    .lcdconsole_putc_noblit = lcdconsole_putc_noblit,
    .lcdconsole_puts_noblit = lcdconsole_puts_noblit,
    .lcdconsole_write_noblit = lcdconsole_write_noblit,
    .lcdconsole_update = lcdconsole_update,
    .lcdconsole_putc = lcdconsole_putc,
    .lcdconsole_puts = lcdconsole_puts,
    .lcdconsole_write = lcdconsole_write,
    .lcdconsole_get_current_x = lcdconsole_get_current_x,
    .lcdconsole_get_current_y = lcdconsole_get_current_y,
    .lcdconsole_get_lineend_x = lcdconsole_get_lineend_x,
    .lcdconsole_get_lineend_y = lcdconsole_get_lineend_y,
    .lcdconsole_progressbar = lcdconsole_progressbar,
    .progressbar_init = progressbar_init,
    .progressbar_setpos = progressbar_setpos,
    .displaylcd = displaylcd,
    .filllcd = filllcd,
    .displaylcd_native = displaylcd_native,
    .filllcd_native = filllcd_native,
    .displaylcd_sync = displaylcd_sync,
    .displaylcd_busy = displaylcd_busy,
    .displaylcd_safe = displaylcd_safe,
    .lcd_get_width = lcd_get_width,
    .lcd_get_height = lcd_get_height,
    .lcd_get_bytes_per_pixel = lcd_get_bytes_per_pixel,
    .lcd_translate_color = lcd_translate_color,
    .renderchar_native = renderchar_native,
    .renderchar = renderchar,
    .rendertext = rendertext,
    .get_font_width = get_font_width,
    .get_font_height = get_font_height,
#endif
#ifdef HAVE_BACKLIGHT
    .backlight_on = backlight_on,
    .backlight_set_fade = backlight_set_fade,
    .backlight_set_brightness = backlight_set_brightness,
#endif
#ifdef HAVE_BOOTFLASH
    .bootflash_filesize = bootflash_filesize,
    .bootflash_attributes = bootflash_attributes,
    .bootflash_getaddr = bootflash_getaddr,
    .bootflash_read = bootflash_read,
    .bootflash_readraw = bootflash_readraw,
    .bootflash_writeraw = bootflash_writeraw,
    .bootflash_getrawaddr = bootflash_getrawaddr,
    .bootflash_is_memmapped = bootflash_is_memmapped,
#endif
#ifdef HAVE_BUTTON
    .button_register_handler = button_register_handler,
    .button_unregister_handler = button_unregister_handler,
#endif
#ifdef HAVE_CLICKWHEEL
    .clickwheel_get_state = clickwheel_get_state,
#endif
#ifdef HAVE_HWKEYAES
    .hwkeyaes = hwkeyaes,
#endif
#ifdef HAVE_HMACSHA1
    .hmacsha1 = hmacsha1,
#endif
    .int_dma_set_handler = int_dma_set_handler,
    .thread_set_name = thread_set_name,
    .thread_set_priority = thread_set_priority,
    .malloc = malloc,
    .memalign = memalign,
    .realloc = realloc,
    .reownalloc = reownalloc,
    .free = free,
    .library_unload = library_unload,
    .get_library = get_library,
    .get_library_ext = get_library_ext,
    .release_library = release_library,
    .release_library_ext = release_library_ext,
#ifdef HAVE_STORAGE
    .fat_enable_flushing = fat_enable_flushing,
#endif
    .lcd_get_format = lcd_get_format,
    .crc32 = crc32,
    .clockgate_get_state = clockgate_get_state,
    .malloc_walk = malloc_walk,
#ifdef HAVE_STORAGE
    .storage_sleep = storage_sleep,
    .storage_disk_is_active = storage_disk_is_active,
    .storage_soft_reset = storage_soft_reset,
#ifdef HAVE_STORAGE_FLUSH
    .storage_flush = storage_flush,
#endif
    .storage_spin = storage_spin,
    .storage_spindown = storage_spindown,
    .storage_last_disk_activity = storage_last_disk_activity,
    .storage_num_drives = storage_num_drives,
#endif
    .read_battery_voltage = read_battery_voltage,
    .read_battery_current = read_battery_current,
    .read_battery_mwh_design = read_battery_mwh_design,
    .read_battery_mwh_full = read_battery_mwh_full,
    .read_battery_mwh_current = read_battery_mwh_current,
    .read_battery_mw = read_battery_mw,
    .read_input_mw = read_input_mw,
    .read_battery_state = read_battery_state,
    .tlsf_realign = tlsf_realign,
    .realign = realign
};
