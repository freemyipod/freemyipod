#include "embiosapp.h"


void main();
EMBIOS_APP_HEADER("iLoader GUI thread", 0x1000, main, 127)


uint16_t lcdbuffer[240 * 320];
uint16_t backdrop[240 * 320];

uint32_t config[0x4000];
uint32_t nordir[0x200];

struct mutex eventmtx;
struct wakeup eventwakeup;
volatile int button;
volatile int increment;


void handler(enum button_event eventtype, int which, int value)
{
    mutex_lock(&eventmtx, TIMEOUT_BLOCK);
    if (eventtype == BUTTON_PRESS) button |= 1 << which;
    if (eventtype == WHEEL_FORWARD) increment += value;
    else if (eventtype == WHEEL_BACKWARD) increment -= value;
    mutex_unlock(&eventmtx);
    wakeup_signal(&eventwakeup);
}
    
void fallbackcfg()
{
    uint32_t i;
    memset(config, 0, sizeof(config));
    bootflash_readraw(nordir, 0, sizeof(nordir));

    for (i = 0; i < 0x20; i++) config[i] = 0x88;  // Boot vectors
    memcpy(&config[0x20], "\x84\0\0\0"  // 0x0080: Exception vector
                          "\x0f\0\0\0"  // 0x0084: Power off (in case of an error)
                          "\x0a\0\0\0"  // 0x0088: Fill screen (entry point)
                              "\0\0\0\0"  // 0x008c: x
                              "\0\0\0\0"  // 0x0090: y
                              "\0\0\0\0"  // 0x0094: width (will be filled in later)
                              "\0\0\0\0"  // 0x0098: height (will be filled in later)
                              "\0\0\0\0"  // 0x009c: color (black)
                          "\x0b\0\0\0"  // 0x00a0: Text: "iLoader - no config file!"
                              "\0\0\0\0"      // 0x00a4: x
                              "\0\0\0\0"      // 0x00a8: y
                              "\xff\xff\0\0"  // 0x00ac: fgcolor (white)
                              "\0\0\0\0"      // 0x00b0: bgcolor (black)
                              "\xf0\0\0\0"    // 0x00b4: *text
                          "\x0e\0\0\0"  // 0x00b8: Backlight: On, brightness 21.5%, fade 32
                              "\x01\0\0\0"  // 0x00bc: state
                              "\x37\0\0\0"  // 0x00c0: brightness
                              "\x20\0\0\0"  // 0x00c4: fade
                          "\x09\0\0\0"  // 0x00c8: Menu
                              "\x20\x01\0\0"      // 0x00cc: *entries
                              "\0\0\0\0"          // 0x00d0: selected (first)
                              "\xff\xff\xff\xff"  // 0x00d4: *addrright (next entry)
                              "\xfe\xff\xff\xff"  // 0x00d8: *addrleft (previous entry)
                              "\xff\xff\xff\xff"  // 0x00dc: *addrplay (next entry)
                              "\xfe\xff\xff\xff"  // 0x00e0: *addrmenu (previous entry)
                              "\x80\xc3\xc9\x01"  // 0x00e4: timeout (30 seconds)
                          "\x0f\0\0\0"  // 0x00e8: Power off (in case of a timeout)
                          "\x01\0\0\0"  // 0x00ec: Terminate (for 0x0140)
                          "iLoader - no config file!\0"  // 0x00f0: String data for 0x00a0
                          "power off\0"  // 0x010a: String data for 0x0114
                          "exit iLdr\0\0\0"  // 0x0114: String data for 0x0114
                          // 0x0120: Menu data for 0x00c8     <*text> <x> <y> <fgcolor> <bgcolor> <fgactive> <bgactive> <*addr>
                              "\x0a\x01\0\0"  // 0x0120: *text ("power off")
                              "\0\0\0\0"      // 0x0124: x
                              "\0\0\0\0"      // 0x0128: y (will be filled in later)
                              "\xff\xff\0\0"  // 0x012c: fgcolor (white)
                              "\0\0\0\0"      // 0x0130: bgcolor (black)
                              "\xff\xe0\0\0"  // 0x0134: fgactive (yellow)
                              "\0\x1f\0\0"    // 0x0138: bgactive (blue)
                              "\x84\0\0\0"    // 0x013c: *addr (power off)
                          // Next entry
                              "\x14\x01\0\0"  // 0x0140: *text ("exit iLdr")
                              "\0\0\0\0"      // 0x0144: x
                              "\0\0\0\0"      // 0x0148: y (will be filled in later)
                              "\xff\xff\0\0"  // 0x014c: fgcolor (white)
                              "\0\0\0\0"      // 0x0150: bgcolor (black)
                              "\xff\xe0\0\0"  // 0x0154: fgactive (yellow)
                              "\0\x1f\0\0"    // 0x0158: bgactive (blue)
                              "\xec\0\0", 224);  // 0x015c: *addr (terminate)
    // The last line has only 3 bytes because the string will be terminated by a null byte anyway

    int dw = lcd_get_width();
    int dh = lcd_get_height();
    int fw = get_font_width();
    int fh = get_font_height();
    config[0x25] = dw;  // width and height for 0x0088
    config[0x26] = dh;
    config[0x4a] = fh;  // font height for 0x0120
    config[0x52] = fh * 2;  // font height for 0x0140
    uint32_t found = 0;
    uint32_t x = 0;
    uint32_t y = fh * 2;
    for (i = 0; i < 0x400; i += 4)
        if (nordir[i] != 0x00000000 && nordir[i] != 0xffffffff)
        {
            y += fh;
            if (y + fh - 1 > dh)
            {
                y = 2 * fh;
                x += 10 * fw;
                if (x + 9 * fw - 1 > dw) break;
            }
            memcpy(&((uint8_t*)config)[0x3564 + 9 * found], &nordir[i], 8);
            ((uint8_t*)config)[0x3564 + 9 * found + 8] = 0;
            config[0x58 + found * 8] = 0x3564 + 9 * found;
            config[0x59 + found * 8] = x;
            config[0x5a + found * 8] = y;
            config[0x5b + found * 8] = 0xffff;
            config[0x5c + found * 8] = 0x0000;
            config[0x5d + found * 8] = 0xe0ff;
            config[0x5e + found * 8] = 0x1f00;
            config[0x5f + found * 8] = 0x2164 + found * 20;
            config[0x859 + found * 5] = 0x03;
            config[0x85a + found * 5] = 0x08000000;
            config[0x85b + found * 5] = 0x3564 + 9 * found;
            config[0x85c + found * 5] = 0x13;
            config[0x85d + found * 5] = 0x08000000;
            found++;
        }
    config[0x58 + found * 8] = 0;
}

void main(void)
{
    uint32_t i;
    uint32_t size;
    uint32_t errhandler;
    uint32_t pc = clickwheel_get_state() & 0x1f;
    char* filename;
    int width = lcd_get_width();
    int height = lcd_get_height();

    mutex_init(&eventmtx);
    wakeup_init(&eventwakeup);
    button_register_handler(handler);

    int fd = file_open("/iLoader/iLoader.cfg", O_RDONLY);
    if (fd >= 0)
    {
        size = filesize(fd);
        if (size > 0 && read(fd, config, size) == size)
            goto configfound;
    }
    size = bootflash_filesize("ildrcfg ");
    if (size > 0)
    {
        if (bootflash_attributes("ildrcfg ") & 0x800)
        {
            if (bootflash_is_memmapped)
            {
                if (!ucl_decompress(bootflash_getaddr("ildrcfg "), size, config, &size))
                    goto configfound;
            }
            else if (bootflash_read("ildrcfg ", backdrop, 0, size) == size)
                if (!ucl_decompress(backdrop, size, config, &size))
                    goto configfound;
        }
        else if (bootflash_read("ildrcfg ", config, 0, size) == size)
            goto configfound;
    }
    fallbackcfg();
configfound:
    pc = config[pc] >> 2;
    errhandler = config[0x21] >> 2;

    while (1)
    {
        if (pc >= 0x4000)
        {
            fallbackcfg();
            pc = 0x22;
        }

        uint32_t oldpc = pc;

        switch (config[pc])
        {
            case 0:
                pc++;
                break;

            case 0x1:
                cputs(1, "iLoader terminated on user's behalf\n");
                return;

            case 0x3:
                filename = &((char*)config)[config[pc + 2]];
                size = bootflash_filesize(filename);
                if (size < 0) pc = errhandler;
                else if (bootflash_attributes(filename) & 0x800)
                {
                    if (bootflash_is_memmapped)
                    {
                        if (ucl_decompress(filename, size, (void*)config[pc + 1], &size))
                            pc = errhandler;
                        else pc += 3;
                    }
                    else if (bootflash_read(filename, (void*)(0x09e00000 - size), 0, size) == size)
                    {
                        if (ucl_decompress((void*)(0x09e00000 - size), size,
                                           (void*)config[pc + 1], &size))
                            pc = errhandler;
                        else pc += 3;
                    }
                    else pc = errhandler;
                }
                else if (bootflash_read(filename, (void*)config[pc + 1], 0, size) != size)
                    pc = errhandler;
                else pc += 3;
                break;

            case 0x4:
                fd = file_open(&((char*)config)[config[pc + 2]], O_RDONLY);
                if (fd < 0) pc = errhandler;
                else
                {
                    size = filesize(fd);
                    if (size < 0) pc = errhandler;
                    else if (read(fd, (void*)config[pc + 1], size) != size) pc = errhandler;
                    else pc += 3;
                }
                break;

            case 0x5:
                pc = config[pc + 2] >> 2;
                memcpy(config, (void*)config[oldpc + 1], sizeof(config));
                break;

            case 0x6:
                errhandler = config[pc + 1] >> 2;
                pc += 2;
                break;

            case 0x7:
                pc = config[pc + 1] >> 2;
                break;

            case 0x8:
                {
                    uint32_t timestamp = USEC_TIMER;
                    bool done = false;
                    while (!done)
                    {
                        if (config[pc + 6] && TIMEOUT_EXPIRED(timestamp, config[pc + 6]))
                        {
                            pc += 7;
                            break;
                        }
                        long left = (long)config[pc + 6] - ((long)USEC_TIMER - (long)timestamp);
                        if (left > 0) wakeup_wait(&eventwakeup, left);
                        mutex_lock(&eventmtx, TIMEOUT_BLOCK);
                        int buttons = button;
                        button = 0;
                        mutex_unlock(&eventmtx);
                        for (i = 0; i < 5; i++)
                            if (buttons & (1 << i))
                            {
                                pc = config[pc + i + 1] >> 2;
                                done = true;
                                break;
                            }
                    }
                }
                break;

            case 0x9:
                {
                    uint32_t timestamp = USEC_TIMER;
                    uint32_t timeout = config[pc + 7];
                    memcpy(backdrop, lcdbuffer, height * width * 2);
                    uint32_t ci = (config[pc + 1] >> 2) + 8 * config[pc + 2];
                    bool done = false;
                    while (!done)
                    {
                        memcpy(lcdbuffer, backdrop, height * width * 2);
                        uint32_t di = (config[pc + 1] >> 2);
                        while (config[di])
                        {
                            rendertext(&lcdbuffer[config[di + 1] + width * config[di + 2]],
                                       config[di + (di == ci ? 5 : 3)],
                                       config[di + (di == ci ? 6 : 4)],
                                       &((uint8_t*)config)[config[di]], width);
                            di += 8;
                        }
                        displaylcd(0, width - 1, 0, height - 1, lcdbuffer, 0);

                        bool changed = false;
                        while (!changed)
                        {
                            if (timeout && TIMEOUT_EXPIRED(timestamp, timeout))
                            {
                                pc += 8;
                                done = true;
                                break;
                            }
                            long left = (long)timeout - ((long)USEC_TIMER - (long)timestamp);
                            if (left > 0) wakeup_wait(&eventwakeup, left);
                            else if (!timeout) wakeup_wait(&eventwakeup, TIMEOUT_BLOCK);
                            mutex_lock(&eventmtx, TIMEOUT_BLOCK);
                            int buttons = button;
                            button = 0;
                            int incr = increment;
                            increment = 0;
                            mutex_unlock(&eventmtx);
                            if ((buttons & 1) && config[ci + 7])
                            {
                                pc = config[ci + 7] >> 2;
                                done = true;
                                break;
                            }
                            for (i = 1; i < 5; i++)
                                if (buttons & (1 << i))
                                {
                                    if (config[pc + i + 2] == 0xfffffffe)
                                    {
                                        if (ci > (config[pc + 1] >> 2)) ci -= 8;
                                        changed = true;
                                    }
                                    else if (config[pc + i + 2] == 0xffffffff)
                                    {
                                        if (config[ci + 8]) ci += 8;
                                        changed = true;
                                    }
                                    else if (config[pc + i + 2])
                                    {
                                        pc = config[pc + i + 2] >> 2;
                                        done = true;
                                        break;
                                    }
                                }
                            while (incr < 0 && ci > (config[pc + 1] >> 2))
                            {
                                ci -= 8;
                                incr++;
                                changed = true;
                            }
                            while (incr > 0 && config[ci + 8])
                            {
                                ci += 8;
                                incr--;
                                changed = true;
                            }
                        }
                        if (changed) timeout = 0;
                    }
                }
                break;

            case 0xa:
                renderfillrect(lcdbuffer, config[pc + 1] , config[pc + 2],
                               config[pc + 3], config[pc + 4], config[pc + 5], width);
                pc += 6;
                break;

            case 0xb:
                rendertext(&lcdbuffer[config[pc + 1] + width * config[pc + 2]],
                           config[pc + 3], config[pc + 4],
                           &((char*)config)[config[pc + 5]], width);
                pc += 6;
                break;

            case 0xc:
                renderbmp(&lcdbuffer[config[pc + 1] + width * config[pc + 2]],
                          (void*)config[pc + 3], width);
                pc += 4;
                break;

            case 0xd:
                displaylcd(0, width - 1, 0, height - 1, lcdbuffer, 0);
                pc++;
                break;

            case 0xe:
                backlight_set_fade(config[pc + 3]);
                backlight_set_brightness(config[pc + 2]);
                backlight_on(config[pc + 1]);
                pc += 4;
                break;

            case 0xf:
                shutdown(true);
                power_off();

            case 0x10:
                {
                    uint8_t* image = (uint8_t*)config[pc + 1];
                    uint32_t checksum = image[3] | (image[2] << 8)
                                      | (image[1] << 16) | (image[0] << 24);
                    uint32_t mysum = 62;
                    if (*((uint32_t*)&image[4]) != get_platform_id()) pc = errhandler;
                    else
                    {
                        for (i = 0; i < size - 8; i++)
                        {
                            image[i] = image[i + 8];
                            mysum += image[i + 8];
                        }
                        if (checksum != mysum) pc = errhandler;
                        else pc = pc + 2;
                    }
                }
                break;

            case 0x11:
                sleep(config[pc + 1]);
                pc += 2;
                break;

            case 0x12:
                clockgate_enable(config[pc + 1], config[pc + 2]);
                pc += 3;
                break;

            case 0x13:
                shutdown(false);
                execfirmware((void*)config[pc + 1]);

            case 0x14:
                if (ucl_decompress((void*)config[pc + 1], size, (void*)config[pc + 2], &size))
                    pc = errhandler;
                else pc += 3;
                break;

            default:
                fallbackcfg();
                pc = 0x22;
        }
    }
}
