#include "emcoreapp.h"
#include "libui.h"


#define TILE_W 5
#define TILE_H 5


enum state
{
    STATE_FREE,
    STATE_SNAKE,
    STATE_FOOD,
    STATE_OUTOFBOUNDS,
    NUM_STATES
};

struct pos
{
    uint8_t x;
    uint8_t y;
};


uint32_t colors[NUM_STATES] =
{
    [STATE_FREE] = 0,
    [STATE_SNAKE] = 0xff000000,
    [STATE_FOOD] = 0xffff0000
};


struct libui_api* ui;
int dirx;
int diry;
int offsetx;
int offsety;
int tilesx;
int tilesy;
int tilecount;
void* framebuf;
int framebufx;
int framebufy;
int framebufsize;
uint8_t* field;
struct pos* snake;
int snakehead;
int snaketail;
int snakelength;
int xrandshift;
int yrandshift;


void buttonhandler(void* user, enum button_event eventtype, int which, int value)
{
    if (eventtype == BUTTON_PRESS)
        switch (which)
        {
        case 1:
            dirx = 1;
            diry = 0;
            break;
        case 2:
            dirx = -1;
            diry = 0;
            break;
        case 3:
            dirx = 0;
            diry = 1;
            break;
        case 4:
            dirx = 0;
            diry = -1;
            break;
        }
}

void setstate(int x, int y, enum state s)
{
    if (x < 0 || x >= tilesx || y < 0 || y >= tilesy) return;
    field[y * tilesx + x] = (uint8_t)s;
}

enum state getstate(int x, int y)
{
    if (x < 0 || x >= tilesx || y < 0 || y >= tilesy) return STATE_OUTOFBOUNDS;
    return (enum state)field[y * tilesx + x];
}

enum state extendsnake()
{
    int newx = snake[snakehead].x + dirx;
    int newy = snake[snakehead].y + diry;
    enum state s = getstate(newx, newy);
    if (s != STATE_FREE && s != STATE_FOOD) return s;
    snakehead++;
    setstate(newx, newy, STATE_SNAKE);
    if (snakehead >= tilecount) snakehead = 0;
    snake[snakehead].x = newx;
    snake[snakehead].y = newy;
    snakelength++;
    return s;
}

void shrinksnake()
{
    setstate(snake[snaketail].x, snake[snaketail].y, STATE_FREE);
    snaketail++;
    if (snaketail >= tilecount) snaketail = 0;
    snakelength--;
}

void placefood()
{
    int x;
    int y;
    enum state s = STATE_OUTOFBOUNDS;
    while (s != STATE_FREE)
    {
        x = rand() >> xrandshift;
        y = rand() >> yrandshift;
        s = getstate(x, y);
    }
    setstate(x, y, STATE_FOOD);
}

void draw()
{
    int x, y;
    memset(framebuf, 0xff, framebufsize);
    for (y = 0; y < tilesy; y++)
        for (x = 0; x < tilesx; x++)
        {
            uint32_t color = colors[getstate(x, y)];
            if (color) ui->fill(TILE_W, TILE_H, color, framebuf, x * TILE_W, y * TILE_H, framebufx);
        }
    displaylcd(offsetx, offsety, framebufx, framebufy, framebuf, 0, 0, framebufx);
}


static void main()
{
    int i;

    struct emcorelib_header* libui = get_library(LIBUI_IDENTIFIER, LIBUI_API_VERSION, LIBSOURCE_BOOTFLASH, "libui   ");
    if (!libui) panicf(PANIC_KILLTHREAD, "Could not load user interface library!");
    ui = (struct libui_api*)libui->api;

    int width = lcd_get_width();
    int height = lcd_get_height();
    tilesx = width / TILE_W;
    tilesy = height / TILE_H;
    tilecount = tilesx * tilesy;
    framebufx = tilesx * TILE_W;
    framebufy = tilesy * TILE_H;
    framebufsize = framebufx * framebufy * 3;
    offsetx = (width - framebufx) / 2;
    offsety = (height - framebufy) / 2;
    xrandshift = __emcore_syscall->__clzsi2(tilesx - 1) - 1;
    yrandshift = __emcore_syscall->__clzsi2(tilesy - 1) - 1;
    framebuf = malloc(framebufsize);
    field = malloc(tilecount * sizeof(typeof(*field)));
    snake = malloc(tilecount * sizeof(typeof(*snake)));
    if (!framebuf || !field || !snake) panicf(PANIC_KILLTHREAD, "Out of memory!");
    struct button_hook_entry* hook = button_register_handler(buttonhandler, NULL);
    if (!hook) panicf(PANIC_KILLTHREAD, "Could not register button hook!");

    memset(field, STATE_FREE, tilesx * tilesy);
    bool finished = false;
    dirx = 1;
    diry = 0;
    snakehead = 0;
    snaketail = 0;
    snakelength = 0;
    snake[0].x = 10;
    snake[0].y = 10;
    for (i = 0; i < 10; i++) placefood();
    long steptime = 500000;
    long lasttime = USEC_TIMER - steptime;

    while (!finished)
    {
        long time = USEC_TIMER;
        lasttime += steptime;
        if (time >= lasttime) lasttime = time;
        else sleep(lasttime - time);
        bool food = false;
        switch (extendsnake())
        {
        case STATE_SNAKE:
        case STATE_OUTOFBOUNDS:
            finished = true;
            break;
        case STATE_FOOD:
            food = true;
        }
        if (food) placefood();
        else if (snakelength > 5) shrinksnake();
        if (steptime > 50000) steptime -= 100;
        draw();
    }

    cprintf(1, "Score: %d\n", snakelength);

    button_unregister_handler(hook);
    free(snake);
    free(field);
    free(framebuf);

    release_library(libui);
    library_unload(libui);
}


EMCORE_APP_HEADER("Snake", main, 127)
