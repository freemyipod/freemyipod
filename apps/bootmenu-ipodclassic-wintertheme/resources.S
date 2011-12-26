//
//
//    Copyright 2011 TheSeven, user890104
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
.global icons_png
.global icons_png_size
.global rockbox_png
.global rockbox_png_size

background_png_size:
.word background_png_end - background_png

icons_png_size:
.word icons_png_end - icons_png

rockbox_png_size:
.word rockbox_png_end - rockbox_png


background_png:
.incbin "images/background.png"
background_png_end:

icons_png:
.incbin "images/icons.png"
icons_png_end:

rockbox_png:
.incbin "images/rockbox.png"
rockbox_png_end:

.align 2
