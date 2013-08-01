#include "global.h"
#include "interface/textrenderer/textrenderer.h"
#include "sys/util.h"

void textrenderer_init(const struct textrenderer_instance* instance)
{
    instance->driver->init(instance);
}

int textrenderer_get_line_height(const struct textrenderer_instance* instance)
{
    return instance->driver->get_line_height(instance);
}

int textrenderer_get_max_width(const struct textrenderer_instance* instance)
{
    return instance->driver->get_max_width(instance);
}

int textrenderer_render_char(const struct textrenderer_instance* instance,
                             const struct framebuffer_instance* fb, int x, int y, uint32_t color, char c)
{
    return instance->driver->render_char(instance, fb, x, y, color, c);
}
