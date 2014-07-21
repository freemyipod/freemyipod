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
#include "clickwheel.h"
#include "button.h"
#include "thread.h"
#include "timer.h"
#include "s5l8702.h"
#include "contextswitch.h"
#ifdef TARGET_ipodclassic
#include "fat.h"
#include "storage_ata.h"
#include "../ipodclassic/storage_ata-target.h"
#endif


static struct wakeup clickwheel_wakeup IBSS_ATTR;
static struct wakeup clickwheel_init_wakeup INITDATA_ATTR;
static volatile uint32_t clickwheel_packet IBSS_ATTR;
static struct scheduler_thread clickwheel_thread_handle;
static uint32_t clickwheel_stack[0x100] STACK_ATTR;
static bool wheel_initialized IBSS_ATTR;
static bool oldtouched IBSS_ATTR;
static int oldpos IBSS_ATTR;
static int oldbuttons IBSS_ATTR;
static int lastpacket IBSS_ATTR;
static int packets IBSS_ATTR;
static int collect IBSS_ATTR;
static int lastdiff IBSS_ATTR;


void clickwheel_thread(void* arg0, void* arg1, void* arg2, void* arg3) ICODE_ATTR;
void clickwheel_thread(void* arg0, void* arg1, void* arg2, void* arg3)
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
        int newbuttons = (data >> 8) & 0x1f;
        if ((data & 0x800000FF) == 0x8000001A)
        {
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
                int distance = 0;
                if (!oldtouched) button_send_event(WHEEL_TOUCH, 0, newpos);
                else distance = newpos - oldpos;
                button_send_event(WHEEL_POSITION, 0, newpos);
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
                button_send_event(WHEEL_MOVED, 0, distance);
                DEBUGF("Wheel moved %d units with accel", distance * packets);
                button_send_event(WHEEL_MOVED_ACCEL, 0, distance * packets);
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

            oldpos = newpos;
            oldtouched = newtouched;
            lastpacket = USEC_TIMER;
        }
        else if ((data & 0x8000FFFF) == 0x8000023A)
        {
            if (data & 0x1F0000) newbuttons = (data >> 16) & 0x1F;
            DEBUGF("This is an init packet, button state: %02X", newbuttons);
            if (!wheel_initialized)
            {
                wheel_initialized = true;
                wakeup_signal(&clickwheel_init_wakeup);
            }
        }

        #ifdef TARGET_ipodclassic
            if (newbuttons == 0x11 && oldbuttons != 0x11)
            {
                ata_lock_exclusive(TIMEOUT_BLOCK);
                flush_fat(true);
                ata_sleepnow();
            }
            else if (newbuttons != 0x11 && oldbuttons == 0x11)
                ata_unlock_exclusive();
#endif
                   
        oldbuttons = newbuttons;
    }
}


int clickwheel_init()
{
    wakeup_init(&clickwheel_wakeup);
    wakeup_init(&clickwheel_init_wakeup);
    wheel_initialized = false;
    oldtouched = false;
    oldbuttons = 0;
    lastpacket = 0;
    collect = 0;
    lastdiff = 0;
    PCON(14) = (PCON(14) & ~0xffff0000) | 0x22220000;
    PUNA(2) &= ~2;
    WHEELINT = 7;
    WHEEL10 = 1;
    interrupt_enable(IRQ_WHEEL, true);
    WHEEL08 = 0x20000;
    WHEEL00 = 0x380000;
    while (WHEEL0C & 4) yield();
    WHEELTX = 0x8000023A;
    WHEEL04 |= 1;
    thread_create(&clickwheel_thread_handle, "Clickwheel dispatcher", clickwheel_thread,
                  clickwheel_stack, sizeof(clickwheel_stack), OS_THREAD, 200, true,
                  NULL, NULL, NULL, NULL);
    wakeup_wait(&clickwheel_init_wakeup, 100000);
    if (!wheel_initialized) RET_ERR(0);
    return 0;
}

void INT_WHEEL(void) ICODE_ATTR;
void INT_WHEEL()
{
    uint32_t events = WHEELINT;
    if (events & 1)
    {
        clickwheel_packet = WHEELRX;
        wakeup_signal(&clickwheel_wakeup);
    }
    WHEELINT = events;
}

uint32_t clickwheel_get_state()
{
    return (oldtouched << 15) | (oldpos << 8) | oldbuttons;
}
