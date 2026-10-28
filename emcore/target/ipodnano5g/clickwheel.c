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

/*
 * iPod nano 5G click wheel: the nano2g WHEEL controller at 0x3c200000, ported from
 * the upstream nano 2G click-wheel driver with nano 5G fw_disk values:
 *   init  (fw_disk wheel_init 0x080720dc): pins 0x57..0x5a = (10,7) (11,0) (11,1) (11,2) to function 2 through
 *          GPIOCMD 0x3cf00240, WHEEL00 &= ~0x4fffff, WHEEL08 = 0x3a980, WHEEL00 |= 0x100000, |= 0x200000
 *   send  (wheel_send 0x08072228): WHEEL00 &= ~0x200000, 1 ms, |= 0x200000, WHEELTX = (pkt >> 1) | 0x80000000,
 *          WHEEL04 |= 1, wait WHEEL0C b3 clear (bounded)
 *   xfer  (wheel_xfer 0x08072154): save WHEEL10 and write 0, WHEELINT = 1, send, poll <= 2 ms: INT b0 set and
 *          WHEEL0C b0 set -> data = WHEELRX (INT b0 set alone: INT = 1 and keep polling), INT = 1, restore WHEEL10
 *   query (wheel_read_buttons 0x080721ac): packet 0x8000023a, reply & 0x8000ffff == 0x8000023a, buttons = reply
 *          bits 16..20 (b0 select?, b2/b3/b4/b1 remapped by Apple to 8/0x10/4/2; emCORE keeps the raw bit order of
 *          the nano2g: bit i -> button i)
 * fw_disk polls the controller (no wheel IRQ is registered; the VIC line is unknown), so this
 * driver polls too: a thread sends the query packet every 20 ms. A nano2g-style change packet (0x8000001a, position in
 * bits 16..23, touch b30) is handled as on the nano2g if the controller ever returns one. PROVISIONAL: the query-reply
 * carries no wheel position (only buttons) until a position packet is observed on the device.
 */

#include "global.h"
#include "clickwheel.h"
#include "button.h"
#include "thread.h"
#include "timer.h"
#include "s5l8730.h"
#include "contextswitch.h"
#include "clockgates.h"


#define WHEEL_QUERY_PACKET 0x8000023a
#define WHEEL_POLL_US 20000

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

/* host-readable diagnostics */
uint32_t clickwheel_last_reply;
uint32_t clickwheel_replies;
uint32_t clickwheel_timeouts;

static void wheel_hw_init(void);


/* wheel_send 0x08072228 */
static void wheel_send(uint32_t packet)
{
    long start;
    WHEEL00 &= ~0x200000;
    udelay(1000);
    WHEEL00 |= 0x200000;
    WHEELTX = (packet >> 1) | 0x80000000;
    WHEEL04 |= 1;
    start = USEC_TIMER;
    while (WHEEL0C & 8) if (TIMEOUT_EXPIRED(start, 2000)) { clickwheel_timeouts++; break; }
}

/* wheel_xfer 0x08072154: returns the reply or 0 */
static uint32_t wheel_xfer(uint32_t packet)
{
    uint32_t saved10 = WHEEL10, data = 0;
    long start;
    WHEEL10 = 0;                 /* wheel_swap_ctrl10 0x0803f018 */
    WHEELINT = 1;
    WHEELINT = 1;
    wheel_send(packet);
    start = USEC_TIMER;
    while (1)
    {
        if (WHEELINT & 1)
        {
            if (WHEEL0C & 1) break;
            WHEELINT = 1;
        }
        if (TIMEOUT_EXPIRED(start, 2000)) { clickwheel_timeouts++; break; }
    }
    if (WHEEL0C & 1) data = WHEELRX;
    WHEELINT = 1;
    WHEEL10 = saved10;
    return data;
}

static void clickwheel_handle(uint32_t data)
{
    int i;
    if ((data & 0x800000FF) == 0x8000001A)
    {
        /* nano2g change packet */
        int newbuttons = (data >> 8) & 0x1f;
        int newpos = (data >> 16) & 0xff;
        bool newtouched = (data & 0x40000000) ? true : false;
        int buttonschanged = oldbuttons ^ newbuttons;
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
            if (TIMEOUT_EXPIRED(lastpacket, 200000)) packets = 10;
            else if (lastdiff * distance < 0) packets = 10;
            else packets++;
            lastdiff = distance;
            if (packets > 200) packets = 200;
            if (distance < -48) distance += 96;
            else if (distance > 48) distance -= 96;
            button_send_event(WHEEL_MOVED, 0, distance);
            button_send_event(WHEEL_MOVED_ACCEL, 0, distance * packets);
            collect += distance * packets;
            enum button_event e = collect > 0 ? WHEEL_FORWARD : WHEEL_BACKWARD;
            int steps = (collect > 0 ? collect : -collect) / 128;
            if (steps) button_send_event(e, 0, steps);
            collect %= 128;
        }
        else if (oldtouched)
        {
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
    else if ((data & 0x8000FFFF) == WHEEL_QUERY_PACKET)
    {
        /* query reply (fw_disk 0x080721ac): buttons in bits 16..20 */
        int newbuttons = (data >> 16) & 0x1f;
        int buttonschanged = oldbuttons ^ newbuttons;
        for (i = 0; i < 5; i++)
            if ((buttonschanged >> i) & 1)
            {
                if ((oldbuttons >> i) & 1) button_send_event(BUTTON_RELEASE, i, 0);
                else button_send_event(BUTTON_PRESS, i, 0);
            }
        oldbuttons = newbuttons;
        wheel_initialized = true;
    }
}

static void clickwheel_thread(void* arg0, void* arg1, void* arg2, void* arg3)
{
    while (true)
    {
        uint32_t data;
        data = wheel_xfer(WHEEL_QUERY_PACKET);
        if (data)
        {
            clickwheel_last_reply = data;
            clickwheel_replies++;
            clickwheel_handle(data);
        }
        sleep(WHEEL_POLL_US);
    }
}

static void wheel_hw_init(void)
{
    clockgate_enable(CLOCKGATE_WHEEL, true);             /* PWRCON1 b1 (Apple id 50 "TW") */
    GPIOCMD = GPIO_PIN(10, 7) | 2;                       /* wheel_init: pins 0x57..0x5a to function 2 */
    udelay(1000);
    GPIOCMD = GPIO_PIN(11, 0) | 2;
    GPIOCMD = GPIO_PIN(11, 1) | 2;
    GPIOCMD = GPIO_PIN(11, 2) | 2;
    WHEEL00 &= ~0x4fffff;
    WHEEL08 = 0x3a980;
    WHEEL00 |= 0x100000;
    WHEEL00 |= 0x200000;
}

int clickwheel_init()
{
    uint32_t data;
    int tries;
    wheel_initialized = false;
    oldtouched = false;
    oldbuttons = 0;
    lastpacket = 0;
    collect = 0;
    lastdiff = 0;
    wheel_hw_init();
    /* wheel_read_buttons: up to 5 queries until a matching reply */
    for (tries = 0; tries < 5 && !wheel_initialized; tries++)
    {
        data = wheel_xfer(WHEEL_QUERY_PACKET);
        if (data)
        {
            clickwheel_last_reply = data;
            clickwheel_replies++;
            clickwheel_handle(data);
        }
    }
    thread_create(&clickwheel_thread_handle, "Clickwheel poller", clickwheel_thread,
                  clickwheel_stack, sizeof(clickwheel_stack), OS_THREAD, 200, true,
                  NULL, NULL, NULL, NULL);
    if (!wheel_initialized) RET_ERR(0);
    return 0;
}

uint32_t clickwheel_get_state()
{
    return (oldtouched << 15) | (oldpos << 8) | oldbuttons;
}
