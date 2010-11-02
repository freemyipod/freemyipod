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
#include "clickwheel.h"
#include "button.h"
#include "thread.h"
#include "timer.h"
#include "s5l8701.h"
#include "contextswitch.h"


static struct wakeup clickwheel_wakeup IBSS_ATTR;
static volatile uint32_t clickwheel_packet IBSS_ATTR;
static uint32_t clickwheel_stack[0x100];
static bool oldtouched IBSS_ATTR;
static int oldpos IBSS_ATTR;
static int oldbuttons IBSS_ATTR;
static uint32_t lastpacket IBSS_ATTR;
static int packets IBSS_ATTR;
static int collect IBSS_ATTR;
static int lastdiff IBSS_ATTR;


void clickwheel_thread(void) ICODE_ATTR;
void clickwheel_thread()
{
    int i;
    while (true)
    {
        wakeup_wait(&clickwheel_wakeup, TIMEOUT_BLOCK);
        DEBUGF("Got clickwheel packet");
        uint32_t mode = enter_critical_section();
        uint32_t data = clickwheel_packet;
        leave_critical_section(mode);
        DEBUGF("Acquired clickwheel packet: %08X", data);
        if ((data & 0x800000FF) == 0x8000001A)
        {
            int newbuttons = (data >> 8) & 0x1f;
            int newpos = (data >> 16) & 0xff;
            bool newtouched = (data & 0x40000000) ? true : false;

            DEBUGF("This is a change packet, button state: %02X, position: %02d, touched: %d",
                   newbuttons, newpos, newtouched);
            int buttonschanged = oldbuttons ^ newbuttons;
            DEBUGF("Changed buttons: %02X", buttonschanged);
            for (i = 0; i < 5; i++)
                if ((buttonschanged >> i) & 1)
                {
                    if ((oldbuttons >> i) & 1) button_send_event(BUTTON_RELEASE, i, 0);
                    else button_send_event(BUTTON_PRESS, i, 0);
                }

            if (newtouched)
            {
                if (!oldtouched) button_send_event(WHEEL_TOUCH, 0, newpos);
                button_send_event(WHEEL_POSITION, 0, newpos);
                int distance = newpos - oldpos;
                DEBUGF("Time since last packet: %d microseconds", USEC_TIMER - lastpacket);
                if (TIMEOUT_EXPIRED(lastpacket, 200000))
                {
                    DEBUGF("Resetting accel due to timeout");
                    packets = 10;
                }
                else if (lastdiff * distance < 0)
                {
                    DEBUGF("Resetting accel due to direction change");
                    packets = 10;
                }
                else packets++;
                lastdiff = distance;
                if (packets > 200) packets = 200;
                if (distance < -48) distance += 96;
                else if (distance > 48) distance -= 96;
                DEBUGF("Wheel moved %d units without accel", distance);
                DEBUGF("Wheel moved %d units with accel", distance * packets);
                button_send_event(WHEEL_MOVED, 0, distance);
                collect += distance * packets;
                enum button_event e = collect > 0 ? WHEEL_FORWARD : WHEEL_BACKWARD;
                int data = (collect > 0 ? collect : -collect) / 128;
                if (data) button_send_event(e, 0, data);
                collect %= 128;
                DEBUGF("Wheel moved %d steps (%d left)", data, collect);
            }
            else if (oldtouched)
            {
                DEBUGF("Wheel was untouched");
                button_send_event(WHEEL_POSITION, 0, newpos);
                button_send_event(WHEEL_UNTOUCH, 0, newpos);
                collect = 0;
                packets = 0;
                lastdiff = 0;
            }

            oldbuttons = newbuttons;
            oldpos = newpos;
            oldtouched = newtouched;
            lastpacket = USEC_TIMER;
        }
        else if ((data & 0x8000FFFF) == 0x8000023A)
        {
            if (data & 0x1F0000) oldbuttons = (data >> 16) & 0x1F;
            DEBUGF("This is an init packet, button state: %02X", oldbuttons);
        }
    }
}


void clickwheel_init()
{
    wakeup_init(&clickwheel_wakeup);
    oldtouched = false;
    oldbuttons = 0;
    lastpacket = 0;
    collect = 0;
    lastdiff = 0;
    INTMSK |= 1 << IRQ_WHEEL;
    PWRCON(1) &= ~1;
    PCON15 = (PCON15 & ~0xFFFF0000) | 0x22220000;
    PUNK15 = 0xF0;
    WHEEL08 = 0x3A980;
    WHEEL00 = 0x280000;
    WHEEL10 = 3;
    PCON10 = (PCON10 & ~0xFF0) | 0x10;
    PDAT10 |= 2;
    WHEELTX = 0x8000023A;
    WHEEL04 |= 1;
    PDAT10 &= ~2;
    thread_create("Clickwheel dispatcher", clickwheel_thread, clickwheel_stack,
                  sizeof(clickwheel_stack), OS_THREAD, 200, true);
}

void INT_WHEEL(void) ICODE_ATTR;
void INT_WHEEL()
{
    uint32_t events = WHEELINT;
    if (events & 4) WHEELINT = 4;
    if (events & 2) WHEELINT = 2;
    if (events & 1)
    {
        clickwheel_packet = WHEELRX;
        wakeup_signal(&clickwheel_wakeup);
        WHEELINT = 1;
    }
}

uint32_t clickwheel_get_state()
{
    return (oldtouched << 15) | (oldpos << 8) | oldbuttons;
}
