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


#include "emcoreapp.h"
#include "main.h"
#include "settings.h"


struct snowflake
{
    int x;
    int y;
    uint8_t opacity[4];
};


static struct snowflake snow_flakes[300];
static int snow_velocity_x;
static int snow_velocity_y;


void snow_init()
{
    int i;
    snow_velocity_x = (rand() % 11) - 5;
    snow_velocity_y = rand() % 21;
    for (i = 0; i < ARRAYLEN(snow_flakes); i++)
    {
        snow_flakes[i].x = rand() % (320 << 4);
        snow_flakes[i].y = rand() % (240 << 4);
        snow_flakes[i].opacity[0] = rand() & 0xff;
        snow_flakes[i].opacity[1] = rand() & 0xff;
        snow_flakes[i].opacity[2] = rand() & 0xff;
        snow_flakes[i].opacity[3] = rand() & 0xff;
    }
}

void render_snow()
{
    int i;
    if (!settings.snow) return;
    snow_velocity_x += (rand() % 3) - 1;
    if (snow_velocity_x > 2) snow_velocity_x = 2;
    if (snow_velocity_x < -2) snow_velocity_x = -2;
    snow_velocity_y += (rand() % 3) - 1;
    if (snow_velocity_y > 20) snow_velocity_y = 20;
    if (snow_velocity_y < 15) snow_velocity_y = 15;
    for (i = 0; i < ARRAYLEN(snow_flakes) * settings.snow / SETTINGS_SNOW_MAX; i++)
    {
        snow_flakes[i].x += snow_velocity_x + (rand() % 9) - 4;
        if (snow_flakes[i].x < 0) snow_flakes[i].x += (320 << 4);
        if (snow_flakes[i].x >= (320 << 4)) snow_flakes[i].x -= (320 << 4);
        snow_flakes[i].y += snow_velocity_y + (rand() % 9) - 4;
        if (snow_flakes[i].y < 0) snow_flakes[i].y += (240 << 4);
        if (snow_flakes[i].y >= (240 << 4)) snow_flakes[i].y -= (240 << 4);
        int x = snow_flakes[i].x >> 4;
        int y = snow_flakes[i].y >> 4;
        int x2 = x < 319 ? x + 1 : 0;
        int y2 = y < 239 ? y + 1 : 0;
        int o0 = (snow_flakes[i].opacity[0] << 24) | 0xffffff;
        int o1 = (snow_flakes[i].opacity[1] << 24) | 0xffffff;
        int o2 = (snow_flakes[i].opacity[2] << 24) | 0xffffff;
        int o3 = (snow_flakes[i].opacity[3] << 24) | 0xffffff;
        ui->blendcolor(1, 1, o0, framebuf, x, y, 320, framebuf, x, y, 320);
        ui->blendcolor(1, 1, o1, framebuf, x2, y, 320, framebuf, x2, y, 320);
        ui->blendcolor(1, 1, o2, framebuf, x, y2, 320, framebuf, x, y2, 320);
        ui->blendcolor(1, 1, o3, framebuf, x2, y2, 320, framebuf, x2, y2, 320);
    }
}
