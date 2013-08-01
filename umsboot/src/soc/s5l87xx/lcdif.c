#include "global.h"
#include "soc/s5l87xx/lcdif.h"
#include "interface/lcdif/lcdif.h"
#include "soc/s5l87xx/regs.h"


static void lcdif_init(const struct lcdif_instance* instance)
{
    LCDCON = LCDCON_INITVALUE;
    LCDPHTIME = 0;
}

static void lcdif_send_cmd(const struct lcdif_instance* instance, uint32_t cmd)
{
    while (LCDSTATUS & 0x10);
    LCDWCMD = cmd;
}

static void lcdif_send_data(const struct lcdif_instance* instance, uint32_t data)
{
    while (LCDSTATUS & 0x10);
    LCDWDATA = data;
}

static void lcdif_send_bulk(const struct lcdif_instance* instance, void* data, int words)
{
    uint16_t* in = (uint16_t*)data;
    while (words >= 4)
    {
        while (LCDSTATUS & 8);
        LCDWDATA = *in++;
        LCDWDATA = *in++;
        LCDWDATA = *in++;
        LCDWDATA = *in++;
        words -= 4;
    }
    while (LCDSTATUS & 8);
    while (words-- & 3) LCDWDATA = *in++;
}

static void lcdif_send_repeat(const struct lcdif_instance* instance, uint32_t data, int count)
{
    while (count >= 4)
    {
        while (LCDSTATUS & 8);
        LCDWDATA = data;
        LCDWDATA = data;
        LCDWDATA = data;
        LCDWDATA = data;
        count -= 4;
    }
    while (LCDSTATUS & 8);
    while (count-- & 3) LCDWDATA = data;
}

const struct lcdif_driver s5l87xx_lcdif_driver =
{
    .init = lcdif_init,
    .send_cmd = lcdif_send_cmd,
    .send_data = lcdif_send_data,
    .send_bulk = lcdif_send_bulk,
    .send_repeat = lcdif_send_repeat,
};
