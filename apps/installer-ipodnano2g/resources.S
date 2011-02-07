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


background_png_size:
.word background_png_end - background_png

darkener_png_size:
.word darkener_png_end - darkener_png

disclaimer_png_size:
.word disclaimer_png_end - disclaimer_png

actions_png_size:
.word actions_png_end - actions_png


flashscript:

.byte 1
.byte 1
.byte 0
.byte 1
.ascii "diskmode"

.byte 2
.byte 1
.byte 0
.byte 1
.ascii "diagmode"

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
.word f_umsboot_ipodnano2g_ucl
.word f_umsboot_ipodnano2g_ucl_end - f_umsboot_ipodnano2g_ucl
.ascii "umsboot "

.byte 0
.byte 4
.byte 8
.byte 1
.word f_emcoreldr_ipodnano2g_dfu
.word f_emcoreldr_ipodnano2g_dfu_end - f_emcoreldr_ipodnano2g_dfu

.byte 0
.byte 4
.byte 10
.byte 1
.word f_emcore_ipodnano2g_ucl
.word f_emcore_ipodnano2g_ucl_end - f_emcore_ipodnano2g_ucl

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
.word f_bootmenu_ipodnano2g_emcoreapp
.word f_bootmenu_ipodnano2g_emcoreapp_end - f_bootmenu_ipodnano2g_emcoreapp
.ascii "bootmenu"

.byte 0
.byte 0
.byte 0
.byte 1
.word f_background_png
.word f_background_png_end - f_background_png
.ascii "backgrnd"

.byte 0
.byte 0
.byte 0
.byte 1
.word f_icons_png
.word f_icons_png_end - f_icons_png
.ascii "iconset "

.byte 0
.byte 0
.byte 0
.byte 1
.word f_rockbox_png
.word f_rockbox_png_end - f_rockbox_png
.ascii "rbxlogo "

.byte 0
.byte 0
.byte 0
.byte 1
.word f_crapple_png
.word f_crapple_png_end - f_crapple_png
.ascii "crapple "

.word 0


firstinstcost:
.word 0
firstinstscript:
.word 0

commoncost:
.word 11
commonscript:
.word 1
.word boot_dirname
.word 1
.word 2
.word ofw_filename
.word 0xffffffff
.word 0
.word 10
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

boot_dirname:
.ascii "/.boot\0"

ofw_filename:
.ascii "/.boot/appleos.bin\0"

.align 4
f_background_png:
.incbin "flashfiles/background.png"
f_background_png_end:

.align 4
f_icons_png:
.incbin "flashfiles/icons.png"
f_icons_png_end:

.align 4
f_rockbox_png:
.incbin "flashfiles/rockbox.png"
f_rockbox_png_end:

.align 4
f_crapple_png:
.incbin "flashfiles/crapple.png"
f_crapple_png_end:

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
f_bootmenu_ipodnano2g_emcoreapp:
.incbin "flashfiles/bootmenu-ipodnano2g.emcoreapp"
f_bootmenu_ipodnano2g_emcoreapp_end:

.align 4
f_umsboot_ipodnano2g_ucl:
.incbin "flashfiles/umsboot-ipodnano2g.ucl"
f_umsboot_ipodnano2g_ucl_end:

.align 4
f_emcore_ipodnano2g_ucl:
.incbin "flashfiles/emcore-ipodnano2g.ucl"
f_emcore_ipodnano2g_ucl_end:

.align 4
f_emcoreldr_ipodnano2g_dfu:
.incbin "flashfiles/emcoreldr-ipodnano2g.dfu"
f_emcoreldr_ipodnano2g_dfu_end:

.align 2