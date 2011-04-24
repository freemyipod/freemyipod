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


#include "emcorelib.h"
#include "libui.h"
#include "chooser.h"
#include "chooser_renderer_iconflow.h"


static int chooser_renderer_iconflow_init(struct chooser_data* data)
{
    const struct chooser_renderer_iconflow_params* params;
    params = (const struct chooser_renderer_iconflow_params*)(data->info->rendererparams);
    if (params->version != CHOOSER_RENDERER_ICONFLOW_PARAMS_VERSION) return -1;
    data->rendererdata = malloc(sizeof(struct chooser_renderer_iconflow_data));
    if (!data->rendererdata) return -2;
    struct chooser_renderer_iconflow_data* rdata;
    rdata = (struct chooser_renderer_iconflow_data*)(data->rendererdata);
    rdata->viewposition = params->startposition * data->info->actionhandler->stepsperitem(data);
    rdata->accumulator = 0;
    rdata->lastupdate = USEC_TIMER;
    return 0;
}

static void chooser_renderer_iconflow_geticondata(int x, int y, int w, int h, int siv,
                                                  int dist, const struct libui_surface* icon,
                                                  int* ix, int* iy, int* io)
{
    *ix = x + (w - icon->size.x) * (dist + siv / 2) / siv;
    *iy = y + (h - icon->size.y) - (h - icon->size.y) * dist * dist * 2 / (siv * siv);
    *io = MAX(0, 255 - 510 * ABS(dist) / siv);
}

static enum chooser_result chooser_renderer_iconflow_render(struct chooser_data* data)
{
    enum chooser_result rc = CHOOSER_RESULT_OK;
    const struct chooser_renderer_iconflow_params* params;
    params = (const struct chooser_renderer_iconflow_params*)(data->info->rendererparams);
    struct chooser_renderer_iconflow_data* rdata;
    rdata = (struct chooser_renderer_iconflow_data*)(data->rendererdata);
    long time = USEC_TIMER;
    long timediff = time - rdata->lastupdate;
    if (!timediff) timediff = 1;
    int distance;
    if (!rdata->lastupdate) distance = 0;
    else distance = data->position - rdata->viewposition;
    int factor = params->smoothness / timediff;
    if (!factor) factor = 1;
    rdata->accumulator += distance;
    distance = rdata->accumulator / factor;
    rdata->accumulator -= distance * factor;
    rdata->viewposition += distance;
    rdata->lastupdate = time;
    if (data->position - rdata->viewposition) rc = CHOOSER_RESULT_REDRAW;
    else rdata->lastupdate = 0;
    const struct chooser_item* selected = data->selected;
    if (params->copy_dest.buf.addr == params->fill_dest.loc.buf.addr
     && params->copy_dest.buf.stride == params->fill_dest.loc.buf.stride
     && params->copy_dest.pos.x == params->fill_dest.loc.pos.x
     && params->copy_dest.pos.y == params->fill_dest.loc.pos.y
     && params->copy_src.size.x == params->fill_dest.size.x
     && params->copy_src.size.y == params->fill_dest.size.y && params->copy_src.loc.buf.addr)
    {
        blendcolor(params->copy_src.size.x, params->copy_src.size.y, params->fill_color,
                   params->copy_dest.buf.addr, params->copy_dest.pos.x,
                   params->copy_dest.pos.y, params->copy_dest.buf.stride,
                   params->copy_src.loc.buf.addr, params->copy_src.loc.pos.x,
                   params->copy_src.loc.pos.y, params->copy_src.loc.buf.stride);
        if (params->bg_opacity && params->bg_dest.buf.addr && params->bg_src.loc.buf.addr)
            blenda(params->bg_src.size.x, params->bg_src.size.y, params->bg_opacity,
                   params->bg_dest.buf.addr, params->bg_dest.pos.x,
                   params->bg_dest.pos.y, params->bg_dest.buf.stride,
                   params->bg_dest.buf.addr, params->bg_dest.pos.x,
                   params->bg_dest.pos.y, params->bg_dest.buf.stride,
                   params->bg_src.loc.buf.addr, params->bg_src.loc.pos.x,
                   params->bg_src.loc.pos.y, params->bg_src.loc.buf.stride);
    }
    else if (params->copy_dest.buf.stride == params->bg_dest.buf.stride
     && params->bg_src.loc.buf.addr && params->copy_dest.buf.addr == params->bg_dest.buf.addr
     && params->copy_dest.pos.x == params->bg_dest.pos.x && !params->fill_dest.loc.buf.addr
     && params->copy_dest.pos.y == params->bg_dest.pos.y && params->copy_src.loc.buf.addr
     && params->copy_src.size.x == params->bg_src.size.x && params->copy_dest.buf.addr
     && params->copy_src.size.y == params->bg_src.size.y && params->bg_opacity)
    {
        blenda(params->copy_src.size.x, params->copy_src.size.y, params->bg_opacity,
               params->copy_dest.buf.addr, params->copy_dest.pos.x,
               params->copy_dest.pos.y, params->copy_dest.buf.stride,
               params->copy_src.loc.buf.addr, params->copy_src.loc.pos.x,
               params->copy_src.loc.pos.y, params->copy_src.loc.buf.stride,
               params->bg_src.loc.buf.addr, params->bg_src.loc.pos.x,
               params->bg_src.loc.pos.y, params->bg_src.loc.buf.stride);
    }
    else
    {
        if (params->copy_src.loc.buf.addr && params->copy_dest.buf.addr)
            blit(params->copy_src.size.x, params->copy_src.size.y, 3,
                 params->copy_dest.buf.addr, params->copy_dest.pos.x,
                 params->copy_dest.pos.y, params->copy_dest.buf.stride,
                 params->copy_src.loc.buf.addr, params->copy_src.loc.pos.x,
                 params->copy_src.loc.pos.y, params->copy_src.loc.buf.stride);
        if (params->fill_dest.loc.buf.addr)
            blendcolor(params->fill_dest.size.x, params->fill_dest.size.y, params->fill_color,
                       params->fill_dest.loc.buf.addr, params->fill_dest.loc.pos.x,
                       params->fill_dest.loc.pos.y, params->fill_dest.loc.buf.stride,
                       params->fill_dest.loc.buf.addr, params->fill_dest.loc.pos.x,
                       params->fill_dest.loc.pos.y, params->fill_dest.loc.buf.stride);
        if (params->bg_opacity && params->bg_src.loc.buf.addr && params->bg_dest.buf.addr)
            blenda(params->bg_src.size.x, params->bg_src.size.y, params->bg_opacity,
                   params->bg_dest.buf.addr, params->bg_dest.pos.x,
                   params->bg_dest.pos.y, params->bg_dest.buf.stride,
                   params->bg_dest.buf.addr, params->bg_dest.pos.x,
                   params->bg_dest.pos.y, params->bg_dest.buf.stride,
                   params->bg_src.loc.buf.addr, params->bg_src.loc.pos.x,
                   params->bg_src.loc.pos.y, params->bg_src.loc.buf.stride);
    }
    void* buf = params->viewport.loc.buf.addr;
    int stride = params->viewport.loc.buf.stride;
    int x = params->viewport.loc.pos.x;
    int y = params->viewport.loc.pos.y;
    int w = params->viewport.size.x;
    int h = params->viewport.size.y;
    int spi = data->info->actionhandler->stepsperitem(data);
    int dir = 1;
    int pos = spi / 2;
    int vpos = rdata->viewposition;
    int iiv = params->iconsinview;
    const struct chooser_renderer_iconflow_itemdata* iparams;
    const struct chooser_item* item = data->info->items;
    while (true)
    {
        if (item == selected && dir == 1)
        {
            dir = -1;
            item = &data->info->items[data->info->itemcount - 1];
            pos = spi * data->info->itemcount - spi / 2;
        }
        iparams = (const struct chooser_renderer_iconflow_itemdata*)(item->renderparams);
        const struct libui_surface* icon;
        if (item == selected) icon = &iparams->icon_selected;
        else icon = &iparams->icon;
        int ix, iy, io;
        int dist = pos - vpos;
        chooser_renderer_iconflow_geticondata(x, y, w, h, spi * iiv, dist, icon, &ix, &iy, &io);
        if (item == selected)
        {
            io = 255;
            if (iparams->text && iparams->text_color)
                rendertext(buf, params->text_pos.x - strlen(iparams->text) * get_font_width() / 2,
                           params->text_pos.y, stride, iparams->text_color, 0, iparams->text);
        }
        if (icon->loc.buf.addr && io && ix >= x && iy >= y
         && ix + icon->size.x <= x + w && iy + icon->size.y <= y + h)
            blenda(icon->size.x, icon->size.y, io, buf, ix, iy, stride, buf, ix, iy, stride,
                   icon->loc.buf.addr, icon->loc.pos.x, icon->loc.pos.y, icon->loc.buf.stride);
        if (item == selected) break;
        item += dir;
        pos += dir * spi;
    }
    if (params->preblit && params->preblit(data)) return rc;
    displaylcd(params->blit_dest.x, params->blit_dest.y, params->blit_src.size.x,
               params->blit_src.size.y, params->blit_src.loc.buf.addr, params->blit_src.loc.pos.x,
               params->blit_src.loc.pos.y, params->blit_src.loc.buf.stride);
    if (params->postblit) params->postblit(data);
    return rc;
}

static const struct chooser_item* chooser_renderer_iconflow_itematpixel(struct chooser_data* data,
                                                                        int x, int y)
{
    if (x < 0 || y < 0) return NULL;
    const struct chooser_renderer_iconflow_params* params;
    params = (const struct chooser_renderer_iconflow_params*)(data->info->rendererparams);
    if (x >= params->viewport.size.x || y >= params->viewport.size.y) return NULL;
    const struct chooser_item* selected = data->selected;
    struct chooser_renderer_iconflow_data* rdata;
    rdata = (struct chooser_renderer_iconflow_data*)(data->rendererdata);
    const struct chooser_item* result = NULL;
    int vx = params->viewport.loc.pos.x;
    int vy = params->viewport.loc.pos.y;
    int vw = params->viewport.size.x;
    int vh = params->viewport.size.y;
    int spi = data->info->actionhandler->stepsperitem(data);
    int dir = 1;
    int pos = spi / 2;
    int vpos = rdata->viewposition;
    int iiv = params->iconsinview;
    const struct chooser_renderer_iconflow_itemdata* iparams;
    const struct chooser_item* item = data->info->items;
    while (true)
    {
        if (item == selected && dir == 1)
        {
            dir = -1;
            item = &data->info->items[data->info->itemcount - 1];
            pos = spi * data->info->itemcount - spi / 2;
        }
        iparams = (const struct chooser_renderer_iconflow_itemdata*)(item->renderparams);
        const struct libui_surface* icon;
        if (item == selected) icon = &iparams->icon_selected;
        else icon = &iparams->icon;
        int ix, iy, o;
        int dist = pos - vpos;
        chooser_renderer_iconflow_geticondata(vx, vy, vw, vh, spi * iiv, dist, icon, &ix, &iy, &o);
        if (x >= ix && y >= ix && x < ix + icon->size.x && y < iy + icon->size.y) result = item;
        if (item == selected) break;
        item += dir;
        pos += dir * spi;
    }
    return result;
}

static void chooser_renderer_iconflow_destroy(struct chooser_data* data)
{
    free(data->rendererdata);
}


const struct chooser_renderer chooser_renderer_iconflow = 
{
    .version = CHOOSER_RENDERER_VERSION,
    .init = chooser_renderer_iconflow_init,
    .render = chooser_renderer_iconflow_render,
    .itematpixel = chooser_renderer_iconflow_itematpixel,
    .destroy = chooser_renderer_iconflow_destroy
};
