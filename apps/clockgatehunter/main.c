#include "emcoreapp.h"


struct wakeup eventwakeup;
int pos = 64;
int state[65];

void handler(void* user, enum button_event eventtype, int which, int value)
{
    bool action = false;
    switch (eventtype)
    {
    case BUTTON_PRESS:
        switch (which)
        {
        case 0:
            state[pos] = !state[pos];
            action = true;
            break;
        case 1:
            pos++;
            action = true;
            break;
        case 2:
            pos--;
            action = true;
            break;
        }
        break;
    case WHEEL_FORWARD:
        pos++;
        action = true;
        break;
    case WHEEL_BACKWARD:
        pos--;
        action = true;
        break;
    }
    while (pos < 0) pos += 65;
    while (pos > 65) pos -= 65;
    if (action) wakeup_signal(&eventwakeup);
}

static void renderline(void* framebuf, int width, int fontwidth, int fontheight, int line)
{
    int i;
    for (i = 0; i < 16; i++)
        renderchar(framebuf, 2 + fontwidth * (i + i / 4), 2 + fontheight * (2 + line), width,
                    pos == i + line * 16 ? 0xffffffff : 0xff000000,
                    pos == i + line * 16 ? 0xff000000 : 0xffffffff, 
                    *("01X" + state[i + line * 16]));
}

static void main()
{
    int i, j;
    char buf[9];
    for (i = 0; i < 64; i++) state[i] = clockgate_get_state(i);
    state[i] = false;
    uint32_t orig[2] = {0xffffffff, 0xffffffff};
    uint32_t now[2];
    for (i = 0; i < 2; i++)
        for (j = 0; j < 32; j++)
            if (state[i * 32 + j])
                orig[i] &= ~(1 << j);
    cprintf(3, "Initial state: %08X %08X\n", orig[0], orig[1]);
    int fontwidth = get_font_width();
    int fontheight = get_font_height();
    int width = 19 * fontwidth + 4;
    int height = 6 * fontheight + 4;
    int xoffs = (lcd_get_width() - width) / 2;
    int yoffs = (lcd_get_height() - height) / 2;
    int framebufsize = width * height * 3;
    void* framebuf = malloc(framebufsize);
    if (!framebuf) panicf(PANIC_KILLTHREAD, "Could not allocate framebuffer!");
    memset(framebuf, 0xff, framebufsize);
    memset(framebuf, 0, width * 3);
    memset(framebuf + (height - 1) * width * 3, 0, width * 3);
    for (i = 0; i < height; i++)
    {
        char* ptr = (char*)framebuf + (i * width - 1) * 3;
        for (j = 0; j < 6; j++) *ptr++ = 0;
    }
    wakeup_init(&eventwakeup);
    wakeup_signal(&eventwakeup);
    struct button_hook_entry* hook = button_register_handler(handler, NULL);
    if (!hook) panicf(PANIC_KILLTHREAD, "Could not register button hook!");
    while (true)
    {
        wakeup_wait(&eventwakeup, TIMEOUT_BLOCK);
        now[0] = 0xffffffff;
        now[1] = 0xffffffff;
        for (i = 0; i < 2; i++)
            for (j = 0; j < 32; j++)
            {
                bool oldstate = state[i * 32 + j];
                clockgate_enable(i * 32 + j, oldstate);
                state[i * 32 + j] = clockgate_get_state(i * 32 + j);
                if (state[i * 32 + j] != oldstate) state[i * 32 + j] = 2;
                if (state[i * 32 + j]) now[i] &= ~(1 << j);
            }
        for (i = 0; i < 4; i++) renderline(framebuf, width, fontwidth, fontheight, i);
        renderchar(framebuf, 2 + fontwidth * 18, 2, width, pos == 64 ? 0xffffffff : 0xff000000,
                   pos == 64 ? 0xff000000 : 0xffffffff, 'X');
        snprintf(buf, sizeof(buf), "%08X", orig[0]);
        rendertext(framebuf, 2, 2, width, 0xff000000, 0xffffffff, buf);
        snprintf(buf, sizeof(buf), "%08X", orig[1]);
        rendertext(framebuf, 2 + 9 * fontwidth, 2, width, 0xff000000, 0xffffffff, buf);
        snprintf(buf, sizeof(buf), "%08X", now[0]);
        rendertext(framebuf, 2, 2 + fontheight, width, 0xff000000, 0xffffffff, buf);
        snprintf(buf, sizeof(buf), "%08X", now[1]);
        rendertext(framebuf, 2 + 9 * fontwidth, 2 + fontheight, width,
                   0xff000000, 0xffffffff, buf);
        displaylcd(xoffs, yoffs, width, height, framebuf, 0, 0, width);
        if (state[64]) break;
    }
    button_unregister_handler(hook);
    cprintf(3, "Final state: %08X %08X\n", now[0], now[1]);
}


EMCORE_APP_HEADER("Clock gate hunter", main, 127)
