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


#include "global.h"
#include "storage.h"


void shutdown(bool shutdownhw)
{
    DEBUGF("Shutting down...");
#ifdef HAVE_USB
    usb_exit();
#endif
#ifdef HAVE_STORAGE_FLUSH
    storage_flush();
#endif
    if (shutdownhw)
    {
#ifdef HAVE_BACKLIGHT
        backlight_set_fade(0);
        backlight_on(false);
#endif
#ifdef HAVE_LCD_SHUTDOWN
        lcd_shutdown();
#endif
    }
}
