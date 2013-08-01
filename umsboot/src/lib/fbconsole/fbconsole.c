#include "global.h"
#include "lib/fbconsole/fbconsole.h"
#include "interface/console/console.h"
#include "interface/framebuffer/framebuffer.h"
#include "interface/textrenderer/textrenderer.h"
#include "sys/util.h"


static const uint32_t fbconsole_color[][8] =
{
    {0x000000, 0xaa0000, 0x00aa00, 0xaa5500, 0x0000aa, 0xaa00aa, 0x00aaaa, 0xaaaaaa},
    {0x555555, 0xff5555, 0x55ff55, 0xffff55, 0x5555ff, 0xff55ff, 0x55ffff, 0xffffff},
};

static void fbconsole_dirty(const struct console_instance* instance, int x, int y, int w, int h)
{
    struct fbconsole_state* state = (struct fbconsole_state*)instance->driver_state;
    if (!state->dirty)
    {
        state->dirty = true;
        state->dirty_xs = x;
        state->dirty_xe = x + w;
        state->dirty_ys = y;
        state->dirty_ye = y + h;
        return;
    }
    if (x < state->dirty_xs) state->dirty_xs = x;
    if (x + w > state->dirty_xe) state->dirty_xe = x + w;
    if (y < state->dirty_ys) state->dirty_ys = y;
    if (y + h > state->dirty_ye) state->dirty_ye = y + h;
}

static void fbconsole_renderchar(const struct console_instance* instance, int x, int y, char c)
{
    struct fbconsole_config* data = (struct fbconsole_config*)instance->driver_config;
    struct fbconsole_state* state = (struct fbconsole_state*)instance->driver_state;
    framebuffer_fill(data->fb, data->x + x * state->cwidth, data->y + y * state->cheight, state->cwidth,
                     state->cheight, data->fb->format, state->bgcolor, FRAMEBUFFER_CONVERSION_QUALITY_CLIP);
    textrenderer_render_char(data->renderer, data->fb, data->x + x * state->cwidth,
                             data->y + y * state->cheight, state->fgcolor, c);
    fbconsole_dirty(instance, x, y, 1, 1);
}

static void fbconsole_erase_range(const struct console_instance* instance, int x, int y, int w, int h)
{
    struct fbconsole_config* data = (struct fbconsole_config*)instance->driver_config;
    struct fbconsole_state* state = (struct fbconsole_state*)instance->driver_state;
    framebuffer_fill(data->fb, data->x + x * state->cwidth, data->y + y * state->cheight, w * state->cwidth,
                     h * state->cheight, data->fb->format, state->bgcolor, FRAMEBUFFER_CONVERSION_QUALITY_CLIP);
    fbconsole_dirty(instance, x, y, w, h);
}

static void fbconsole_clear(const struct console_instance* instance)
{
    struct fbconsole_config* data = (struct fbconsole_config*)instance->driver_config;
    fbconsole_erase_range(instance, 0, 0, data->w, data->h);
}

static void fbconsole_scroll(const struct console_instance* instance, int lines)
{
    struct fbconsole_config* data = (struct fbconsole_config*)instance->driver_config;
    struct fbconsole_state* state = (struct fbconsole_state*)instance->driver_state;
    if (lines >= data->h || -lines >= data->h) return fbconsole_clear(instance);
    if (lines > 0)
    {
        framebuffer_blit(data->fb, data->x, data->y + lines * state->cheight, data->fb, data->x, data->y,
                         data->w * state->cwidth, (data->h - lines) * state->cheight, FRAMEBUFFER_CONVERSION_QUALITY_CLIP);
        framebuffer_fill(data->fb, data->x, data->y + (data->h - lines) * state->cheight, data->w * state->cwidth,
                         state->cheight * lines, data->fb->format, state->bgcolor, FRAMEBUFFER_CONVERSION_QUALITY_CLIP);
    }
    else
    {
        int y;
        for (y = 0; y < data->h + lines; y++)
            framebuffer_blit(data->fb, data->x, data->y + y * state->cheight,
                             data->fb, data->x, data->y + (y - lines) * state->cheight,
                             data->w * state->cwidth, state->cheight, FRAMEBUFFER_CONVERSION_QUALITY_CLIP);
        framebuffer_fill(data->fb, data->x, data->y, data->w * state->cwidth, state->cheight * -lines,
                         data->fb->format, state->bgcolor, FRAMEBUFFER_CONVERSION_QUALITY_CLIP);
    }
    fbconsole_dirty(instance, 0, 0, data->w, data->h);
}

static void fbconsole_process_sgr(const struct console_instance* instance, int sgr)
{
    struct fbconsole_config* data = (struct fbconsole_config*)instance->driver_config;
    struct fbconsole_state* state = (struct fbconsole_state*)instance->driver_state;
    switch (sgr)
    {
    case 0:
        state->bgcolcode = 0;
        state->fgcolcode = 7;
        state->intensity = 0;
        state->inverse = 0;
        break;
    case 1:
        state->intensity = 1;
        break;
    case 7:
        state->inverse = 1;
        break;
    case 21:
        state->intensity = 0;
        break;
    case 27:
        state->inverse = 0;
        break;
    case 30 ... 37:
        state->fgcolcode = sgr - 30;
        break;
    case 40 ... 47:
        state->bgcolcode = sgr - 40;
        break;
    default:
        break;
    }
    state->fgcolor = framebuffer_color_from_rgb888_be(data->fb->format, fbconsole_color[state->intensity][state->fgcolcode]);
    state->bgcolor = framebuffer_color_from_rgb888_be(data->fb->format, fbconsole_color[0][state->bgcolcode]);
    if (state->inverse)
    {
        uint32_t tmp = state->fgcolor;
        state->fgcolor = state->bgcolor;
        state->bgcolor = tmp;
    }
}

static void fbconsole_init(const struct console_instance* instance)
{
    struct fbconsole_config* data = (struct fbconsole_config*)instance->driver_config;
    struct fbconsole_state* state = (struct fbconsole_state*)instance->driver_state;
    textrenderer_init(data->renderer);
    state->cwidth = textrenderer_get_max_width(data->renderer);
    state->cheight = textrenderer_get_line_height(data->renderer);
    state->cpos_x = 0;
    state->cpos_y = 0;
    state->dirty = false;
    state->meta = false;
    fbconsole_process_sgr(instance, 0);
    fbconsole_clear(instance);
}

static int fbconsole_get_width(const struct console_instance* instance)
{
    struct fbconsole_config* data = (struct fbconsole_config*)instance->driver_config;
    return data->w;
}

static int fbconsole_get_height(const struct console_instance* instance)
{
    struct fbconsole_config* data = (struct fbconsole_config*)instance->driver_config;
    return data->h;
}

static void fbconsole_putc(const struct console_instance* instance, char c)
{
    struct fbconsole_config* data = (struct fbconsole_config*)instance->driver_config;
    struct fbconsole_state* state = (struct fbconsole_state*)instance->driver_state;

    if (c == 0x1b)
    {
        state->escape = true;
        state->meta = false;
        return;
    }

    if (state->meta)
        switch (c)
        {
        case '0' ... '9':
            if (state->secondarg)
            {
                state->arg2_present = true;
                state->meta_arg2 = state->meta_arg2 * 10 + c - '0';
            }
            else
            {
                state->arg1_present = true;
                state->meta_arg1 = state->meta_arg1 * 10 + c - '0';
            }
            return;
        case ';':
            if (state->secondarg) state->badmeta = true;
            else state->secondarg = true;
            return;
        case 'A' ... 'Z':
        case 'a' ... 'z':
            state->meta = false;
            if (state->badmeta) return;
            if (!state->arg1_present) state->meta_arg1 = 1;
            if (!state->arg2_present) state->meta_arg2 = 1;
            switch (c)
            {
            case 'A':
                state->cpos_y = MAX(0, state->cpos_y - state->meta_arg1 + 1);
                break;
            case 'B':
                state->cpos_y = MIN(data->h, state->cpos_y + state->meta_arg1 - 1);
                break;
            case 'C':
                state->cpos_x = MAX(0, state->cpos_x - state->meta_arg1 + 1);
                break;
            case 'D':
                state->cpos_x = MIN(data->w, state->cpos_x + state->meta_arg1 - 1);
                break;
            case 'E':
                state->cpos_x = 0;
                state->cpos_y = MIN(data->h, state->cpos_y + state->meta_arg1 - 1);
                break;
            case 'F':
                state->cpos_x = 0;
                state->cpos_y = MAX(0, state->cpos_y - state->meta_arg1 + 1);
                break;
            case 'G':
                state->cpos_x = MAX(0, MIN(data->w, state->meta_arg1 - 1));
                break;
            case 'H':
            case 'f':
                state->cpos_x = MAX(0, MIN(data->w, state->meta_arg2 - 1));
                state->cpos_y = MAX(0, MIN(data->h, state->meta_arg1 - 1));
                break;
            case 'J':
                if (!state->arg1_present) state->meta_arg1 = 0;
                switch (state->meta_arg1)
                {
                case 0:
                    if (state->cpos_y < data->h - 1)
                        fbconsole_erase_range(instance, 0, state->cpos_y + 1, data->w, data->h - state->cpos_y - 1);
                    if (state->cpos_x < data->w)
                        fbconsole_erase_range(instance, state->cpos_x, state->cpos_y, data->w - state->cpos_x, 1);
                    break;
                case 1:
                    if (state->cpos_y) fbconsole_erase_range(instance, 0, 0, data->w, state->cpos_y);
                    if (state->cpos_x) fbconsole_erase_range(instance, 0, state->cpos_y, state->cpos_x, 1);
                    break;
                case 2:
                    fbconsole_clear(instance);
                    break;
                default:
                    break;
                }
                break;
            case 'K':
                if (!state->arg1_present) state->meta_arg1 = 0;
                switch (state->meta_arg1)
                {
                case 0:
                    if (state->cpos_x < data->w)
                        fbconsole_erase_range(instance, state->cpos_x, state->cpos_y, data->w - state->cpos_x, 1);
                    break;
                case 1:
                    if (state->cpos_x) fbconsole_erase_range(instance, 0, state->cpos_y, state->cpos_x, 1);
                    break;
                case 2:
                    if (state->cpos_y < data->h)fbconsole_erase_range(instance, 0, state->cpos_y, data->w, 1);
                    break;
                default:
                    break;
                }
                break;
            case 'S':
                fbconsole_scroll(instance, state->meta_arg1);
                break;
            case 'T':
                fbconsole_scroll(instance, -state->meta_arg1);
                break;
            case 'm':
                if (!state->arg1_present) state->meta_arg1 = 0;
                fbconsole_process_sgr(instance, state->meta_arg1);
                if (state->arg2_present) fbconsole_process_sgr(instance, state->meta_arg2);
                break;
            default:
                break;
            }
            return;
        default:
            state->meta = false;
            break;
        }

    if (state->escape && c == '[')
    {
        state->escape = false;
        state->meta = true;
        state->badmeta = false;
        state->secondarg = false;
        state->arg1_present = false;
        state->arg2_present = false;
        state->meta_arg1 = 0;
        state->meta_arg2 = 0;
        return;
    }
    state->escape = false;

    switch (c)
    {
    case 0x08:  // backspace
        if (!state->cpos_x) break;
        state->cpos_x--;
        fbconsole_renderchar(instance, state->cpos_x, state->cpos_y, ' ');
        break;
    case 0x0a:  // line feed
        state->cpos_x = 0;
        state->cpos_y++;
        break;
    case 0x0c:  // form feed
        state->cpos_x = 0;
        state->cpos_y = 0;
        fbconsole_clear(instance);
        break;
    case 0x0d:  // carriage return
        state->cpos_x = 0;
        break;
    default:
        if (state->cpos_x >= data->w)
        {
            state->cpos_x = 0;
            state->cpos_y++;
        }
        if (state->cpos_y >= data->h)
        {
            fbconsole_scroll(instance, state->cpos_y - data->h + 1);
            state->cpos_y = data->h - 1;
        }
        fbconsole_renderchar(instance, state->cpos_x++, state->cpos_y, c);
        break;
    }
}

static void fbconsole_puts(const struct console_instance* instance, const char* str)
{
    while (*str) fbconsole_putc(instance, *str++);
}

static void fbconsole_write(const struct console_instance* instance, const char* buf, int len)
{
    while (len--) fbconsole_putc(instance, *buf++);
}

static void fbconsole_flush(const struct console_instance* instance)
{
    struct fbconsole_config* data = (struct fbconsole_config*)instance->driver_config;
    struct fbconsole_state* state = (struct fbconsole_state*)instance->driver_state;
    if (!state->dirty) return;
    framebuffer_update(data->fb, data->x + state->dirty_xs * state->cwidth, data->y + state->dirty_ys * state->cheight,
                       (state->dirty_xe - state->dirty_xs) * state->cwidth, (state->dirty_ye - state->dirty_ys) * state->cheight);
    state->dirty = false;
}


const struct console_driver fbconsole_driver =
{
    .init = fbconsole_init,
    .get_width = fbconsole_get_width,
    .get_height = fbconsole_get_height,
    .putc = fbconsole_putc,
    .puts = fbconsole_puts,
    .write = fbconsole_write,
    .flush = fbconsole_flush,
};
