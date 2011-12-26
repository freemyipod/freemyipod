//
//
//    Copyright 2011 TheSeven
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


.global background_png
.global background_png_size
.global darkener_png
.global darkener_png_size
.global disclaimer_png
.global disclaimer_png_size
.global actions_png
.global actions_png_size
.global flashscript
.global firstinstcost
.global firstinstscript
.global commoncost
.global commonscript
.global f_png_emcorelib
.global f_ui_emcorelib
.global f_mkfat32_emcorelib


background_png_size:
.word background_png_end - background_png

darkener_png_size:
.word darkener_png_end - darkener_png

disclaimer_png_size:
.word disclaimer_png_end - disclaimer_png

actions_png_size:
.word actions_png_end - actions_png


flashscript:

.byte 0
.byte 0
.byte 0
.byte 1
.word f_boot_emcorelib
.word f_boot_emcorelib_end - f_boot_emcorelib
.ascii "libboot "

.byte 0
.byte 0
.byte 0
.byte 1
.word f_png_emcorelib
.word f_png_emcorelib_end - f_png_emcorelib
.ascii "libpng  "

.byte 0
.byte 2
.byte 0
.byte 1
.word f_umsboot_ipodclassic_ucl
.word f_umsboot_ipodclassic_ucl_end - f_umsboot_ipodclassic_ucl
.ascii "umsboot "

.byte 0
.byte 12
.byte 8
.byte 1
.word f_emcoreldr_ipodclassic_bin
.word f_emcoreldr_ipodclassic_bin_end - f_emcoreldr_ipodclassic_bin

.byte 0
.byte 2
.byte 0
.byte 1
.word f_emcore_ipodclassic_ucl
.word f_emcore_ipodclassic_ucl_end - f_emcore_ipodclassic_ucl
.ascii "emcore  "

.byte 0
.byte 0
.byte 0
.byte 1
.word f_ui_emcorelib
.word f_ui_emcorelib_end - f_ui_emcorelib
.ascii "libui   "

.byte 0
.byte 0
.byte 0
.byte 1
.word f_mkfat32_emcorelib
.word f_mkfat32_emcorelib_end - f_mkfat32_emcorelib
.ascii "libmkf32"

.byte 0
.byte 0
.byte 0
.byte 1
.word f_bootmenu_ipodclassic_emcoreapp
.word f_bootmenu_ipodclassic_emcoreapp_end - f_bootmenu_ipodclassic_emcoreapp
.ascii "bootmenu"

.byte 0
.byte 2
.byte 0
.byte 1
.word f_rockbox_ipod_ucl
.word f_rockbox_ipod_ucl_end - f_rockbox_ipod_ucl
.ascii "rockbox "

.word 0


firstinstcost:
.word 0
commoncost:
.word 5

firstinstscript:
commonscript:
.word 1
.word apps_dirname
.word 1

.word 1
.word bootmenu_dirname
.word 1

.word 4
.word theme_emcoreapp_filename
.word f_theme_emcoreapp
.word f_theme_emcoreapp_end - f_theme_emcoreapp
.word 3

.word 0


background_png:
.incbin "oobe/background.png"
background_png_end:

darkener_png:
.incbin "oobe/darkener.png"
darkener_png_end:

disclaimer_png:
.incbin "oobe/disclaimer.png"
disclaimer_png_end:

actions_png:
.incbin "oobe/actions.png"
actions_png_end:

apps_dirname:
.ascii "/.apps\0"

bootmenu_dirname:
.ascii "/.apps/bootmenu\0"

theme_emcoreapp_filename:
.ascii "/.apps/bootmenu/theme.emcoreapp\0"

.align 4
f_boot_emcorelib:
.incbin "flashfiles/boot.emcorelib"
f_boot_emcorelib_end:

.align 4
f_png_emcorelib:
.incbin "flashfiles/png.emcorelib"
f_png_emcorelib_end:

.align 4
f_ui_emcorelib:
.incbin "flashfiles/ui.emcorelib"
f_ui_emcorelib_end:

.align 4
f_mkfat32_emcorelib:
.incbin "flashfiles/mkfat32.emcorelib"
f_mkfat32_emcorelib_end:

.align 4
f_bootmenu_ipodclassic_emcoreapp:
.incbin "flashfiles/bootmenu-ipodclassic.emcoreapp"
f_bootmenu_ipodclassic_emcoreapp_end:

.align 4
f_umsboot_ipodclassic_ucl:
.incbin "flashfiles/umsboot-ipodclassic.ucl"
f_umsboot_ipodclassic_ucl_end:

.align 4
f_rockbox_ipod_ucl:
.incbin "flashfiles/rockbox.ipod.ucl"
f_rockbox_ipod_ucl_end:

.align 4
f_emcore_ipodclassic_ucl:
.incbin "flashfiles/emcore-ipodclassic.ucl"
f_emcore_ipodclassic_ucl_end:

.align 4
f_emcoreldr_ipodclassic_bin:
.incbin "flashfiles/emcoreldr-ipodclassic.bin"
f_emcoreldr_ipodclassic_bin_end:

.align 4
f_theme_emcoreapp:
.incbin "fsfiles/.apps/bootmenu/theme.emcoreapp"
f_theme_emcoreapp_end:

.align 2