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
#include "chooser_renderer_list.h"


static void chooser_renderer_list_scroll_into_view(struct chooser_data* data,
                                                   const struct chooser_item* item)
{
    const struct chooser_renderer_list_params* params;
    params = (const struct chooser_renderer_list_params*)(data->info->rendererparams);
    struct chooser_renderer_list_data* rdata;
    rdata = (struct chooser_renderer_list_data*)(data->rendererdata);
    int available = params->viewport.size.y;
    if (item < rdata->top_item) rdata->top_item = item;
    const struct chooser_item* curr = rdata->top_item;
    const struct chooser_renderer_list_itemdata* iparams;
    while (available > 0)
    {
        if (curr >= &data->info->items[data->info->itemcount]) break;
        iparams = (const struct chooser_renderer_list_itemdata*)(curr->renderparams);
        if (available >= iparams->size.y)
        {
            available -= iparams->size.y;
            curr++;
        }
        else
        {
            break;
        }
    }
    rdata->bottom_item = --curr;
    while (curr < item && curr < &data->info->items[data->info->itemcount])
    {
        curr++;
        iparams = (const struct chooser_renderer_list_itemdata*)(curr->renderparams);
        available -= iparams->size.y;
        while (available < 0)
        {
            const struct chooser_item* top = rdata->top_item++;
            iparams = (const struct chooser_renderer_list_itemdata*)(top->renderparams);
            available += iparams->size.y;
        }
        rdata->bottom_item = curr;
    }
}

static int chooser_renderer_list_init(struct chooser_data* data)
{
    const struct chooser_renderer_list_params* params;
    params = (const struct chooser_renderer_list_params*)(data->info->rendererparams);
    if (params->version != CHOOSER_RENDERER_LIST_PARAMS_VERSION) return -1;
    data->rendererdata = malloc(sizeof(struct chooser_renderer_list_data));
    if (!data->rendererdata) return -2;
    struct chooser_renderer_list_data* rdata;
    rdata = (struct chooser_renderer_list_data*)(data->rendererdata);
    rdata->top_item = data->info->items;
    chooser_renderer_list_scroll_into_view(data, data->selected);
    return 0;
}

static enum chooser_result chooser_renderer_list_render(struct chooser_data* data)
{
    const struct chooser_renderer_list_params* params;
    params = (const struct chooser_renderer_list_params*)(data->info->rendererparams);
    const struct chooser_item* selected = data->selected;
    struct chooser_renderer_list_data* rdata;
    rdata = (struct chooser_renderer_list_data*)(data->rendererdata);
    chooser_renderer_list_scroll_into_view(data, selected);
    const struct chooser_item* item = rdata->top_item;
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
    const struct chooser_renderer_list_itemdata* iparams;
    while (item <= rdata->bottom_item)
    {
        iparams = (const struct chooser_renderer_list_itemdata*)(item->renderparams);
        uint32_t fill_color;
        uint32_t text_color;
        const struct libui_surface* icon;
        int icon_opacity;
        if (item == selected)
        {
            fill_color = iparams->fill_color_selected;
            text_color = iparams->text_color_selected;
            icon = &iparams->icon_selected;
            icon_opacity = iparams->icon_selected_opacity;
        }
        else
        {
            fill_color = iparams->fill_color;
            text_color = iparams->text_color;
            icon = &iparams->icon;
            icon_opacity = iparams->icon_opacity;
        }
        if (fill_color)
            blendcolor(iparams->fill_box.size.x, iparams->fill_box.size.y, fill_color,
                       buf, x + iparams->fill_box.pos.x, y + iparams->fill_box.pos.y, stride,
                       buf, x + iparams->fill_box.pos.x, y + iparams->fill_box.pos.y, stride);
        if (icon->loc.buf.addr && icon_opacity)
            blenda(icon->size.x, icon->size.y, icon_opacity,
                   buf, x + iparams->icon_pos.x, y + iparams->icon_pos.y, stride,
                   buf, x + iparams->icon_pos.x, y + iparams->icon_pos.y, stride);
        if (iparams->text && iparams->text_color)
            rendertext(buf, x + iparams->text_pos.x, y + iparams->text_pos.y,
                       stride, text_color, 0, iparams->text);
        if (iparams->render) iparams->render(data, item, item == selected, x, y);
        y += iparams->size.y;
        item++;
    }
    if (params->preblit && params->preblit(data)) return CHOOSER_RESULT_OK;
    displaylcd(params->blit_dest.x, params->blit_dest.y, params->blit_src.size.x,
               params->blit_src.size.y, params->blit_src.loc.buf.addr, params->blit_src.loc.pos.x,
               params->blit_src.loc.pos.y, params->blit_src.loc.buf.stride);
    if (params->postblit) params->postblit(data);
    return CHOOSER_RESULT_OK;
}

static const struct chooser_item* chooser_renderer_list_itematpixel(struct chooser_data* data,
                                                                    int x, int y)
{
    if (x < 0 || y < 0) return NULL;
    const struct chooser_renderer_list_params* params;
    params = (const struct chooser_renderer_list_params*)(data->info->rendererparams);
    struct chooser_renderer_list_data* rdata;
    rdata = (struct chooser_renderer_list_data*)(data->rendererdata);
    const struct chooser_item* item = rdata->top_item;
    const struct chooser_renderer_list_itemdata* iparams;
    while (true)
    {
        if (item >= &data->info->items[data->info->itemcount]) return NULL;
        iparams = (const struct chooser_renderer_list_itemdata*)(item->renderparams);
        if (y < iparams->size.y) break;
        y -= iparams->size.y;
        item++;
    }
    if (x < iparams->size.x) return item;
    return NULL;
}

static void chooser_renderer_list_destroy(struct chooser_data* data)
{
    free(data->rendererdata);
}

void chooser_renderer_list_render_attached_text(struct chooser_data* data,
                                                const struct chooser_item* item,
                                                bool selected, int x, int y, const char* text)
{
    struct chooser_renderer_list_params* rparams;
    rparams = (struct chooser_renderer_list_params*)data->info->rendererparams;
    struct chooser_renderer_list_itemdata* ritem;
    ritem = (struct chooser_renderer_list_itemdata*)item->renderparams;
    void* buf = rparams->viewport.loc.buf.addr;
    int stride = rparams->viewport.loc.buf.stride;
    x += ritem->size.x - ritem->text_pos.x - strlen(text) * get_font_width();
    y += ritem->text_pos.y;
    uint32_t color = selected ? ritem->text_color_selected : ritem->text_color;
    rendertext(buf, x, y, stride, color, 0, text);
}

void chooser_renderer_list_show_arrow_right(struct chooser_data* data,
                                            const struct chooser_item* item,
                                            bool selected, int x, int y)
{
    chooser_renderer_list_render_attached_text(data, item, selected, x, y, ">");
}

void chooser_renderer_list_show_arrow_left(struct chooser_data* data,
                                           const struct chooser_item* item,
                                           bool selected, int x, int y)
{
    chooser_renderer_list_render_attached_text(data, item, selected, x, y, "<");
}


const struct chooser_renderer chooser_renderer_list = 
{
    .version = CHOOSER_RENDERER_VERSION,
    .init = chooser_renderer_list_init,
    .render = chooser_renderer_list_render,
    .itematpixel = chooser_renderer_list_itematpixel,
    .destroy = chooser_renderer_list_destroy
};
