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


#include "embiosapp.h"
#include "build/version.h"


void main();
EMBIOS_APP_HEADER("Uninstaller thread", 0x1000, main, 127)


uint16_t lcdbuffer[320 * 240];

struct wakeup eventwakeup;
volatile int button;

uint8_t norbuf[0x100000] __attribute__((aligned(16)));
#define norbufword ((uint32_t*)norbuf)

void handler(enum button_event eventtype, int which, int value)
{
    if (eventtype == BUTTON_PRESS) button |= 1 << which;
    wakeup_signal(&eventwakeup);
}

void main(void)
{
    uint32_t i, j, k;
    struct progressbar_state progressbar;

    wakeup_init(&eventwakeup);
    button_register_handler(handler);

    memset(lcdbuffer, 0xff, 320 * 240 * 2);
    memset(norbuf, 0xff, 0x100000);
    bootflash_readraw(norbuf, 0x1000, 0x1000);
    if (norbufword[0] != 0x53436667)
    {
        cputs(1, "Boot flash contents are damaged! (No SYSCFG found)\n\nPlease ask for help.\n");
        return;
    }

    rendertext(&lcdbuffer[320 * 10 + 10], 0, 0xffff, "Uninstaller v" VERSION " r" VERSION_SVN, 320);
    rendertext(&lcdbuffer[320 * 26 + 10], 0, 0xffff, "To restore your iPod to factory state,", 320);
    rendertext(&lcdbuffer[320 * 42 + 10], 0, 0xffff, "you need to first run this program and", 320);
    rendertext(&lcdbuffer[320 * 50 + 10], 0, 0xffff, "then restore your iPod using iTunes.", 320);
    rendertext(&lcdbuffer[320 * 74 + 10], 0, 0xffff, "Please note that after running this", 320);
    rendertext(&lcdbuffer[320 * 82 + 10], 0, 0xffff, "program, the screen of your iPod will", 320);
    rendertext(&lcdbuffer[320 * 90 + 10], 0, 0xffff, "power off, and it will appear to be", 320);
    rendertext(&lcdbuffer[320 * 98 + 10], 0, 0xffff, "completely dead until it is restored", 320);
    rendertext(&lcdbuffer[320 * 106 + 10], 0, 0xffff, "by iTunes. Even though the screen is", 320);
    rendertext(&lcdbuffer[320 * 114 + 10], 0, 0xffff, "dark, your iPod will still be powered", 320);
    rendertext(&lcdbuffer[320 * 122 + 10], 0, 0xffff, "on (and you can't power it off until", 320);
    rendertext(&lcdbuffer[320 * 130 + 10], 0, 0xffff, "you restore) after this step, so the", 320);
    rendertext(&lcdbuffer[320 * 138 + 10], 0, 0xffff, "battery will be discharging.", 320);
    rendertext(&lcdbuffer[320 * 154 + 10], 0, 0xffff, "Press skip forward to continue", 320);
    rendertext(&lcdbuffer[320 * 162 + 10], 0, 0xffff, "or skip backward to cancel.", 320);
    displaylcd(0, 319, 0, 239, lcdbuffer, 0);
    button = 0;
    while (true)
    {
        wakeup_wait(&eventwakeup, TIMEOUT_BLOCK);
        if (button == 2) break;
        else if (button == 4)
        {
            shutdown(false);
            reset();
        }
        button = 0;
    }
    rendertext(&lcdbuffer[320 * 178 + 10], 0, 0xffff, "Preparing...", 320);
    displaylcd(0, 319, 0, 239, lcdbuffer, 0);
    progressbar_init(&progressbar, 1, 318, 187, 194, 0, lcd_translate_color(0, 0xcf, 0xcf, 0xcf),
                     lcd_translate_color(0, 0, 0, 0xcf), 0, 256);
    for (i = 0; i < 256; i++)
    {
        bootflash_writeraw(&norbuf[i * 0x1000], i * 0x1000, 0x1000);
        progressbar_setpos(&progressbar, i, false);
    }
    rendertext(&lcdbuffer[320 * 194 + 10], 0, 0xffff, "Will enter DFU mode in 5 seconds...", 320);
    displaylcd(0, 319, 0, 239, lcdbuffer, 0);
    sleep(5000000);
    backlight_on(false);
    shutdown(true);
    reset();
}
