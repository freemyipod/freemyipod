#include "global.h"
#include "soc/s5l87xx/i2c.h"
#include "soc/s5l87xx/clockgate.h"
#include "soc/s5l87xx/regs.h"
#include "protocol/i2c/i2c.h"
#include "sys/util.h"


#ifdef SOC_S5L8702
#define I2C_WAIT() while (IIC10(bus))
#else
#define I2C_WAIT()
#endif


static enum i2c_result s5l87xx_i2c_init(const struct i2c_driver_instance* instance)
{
#ifdef SOC_S5L8701
    PCON10 = 5;
#endif
    return I2C_RESULT_OK;
}

static void s5l87xx_i2c_send_byte(int bus, uint8_t byte)
{
    I2C_WAIT();
    IICDS(bus) = byte;
    I2C_WAIT();
    IICCON(bus) = 0xb7;
    I2C_WAIT();
    while (!(IICCON(bus) & 0x10));
}

static uint8_t s5l87xx_i2c_recv_byte(int bus, bool ack)
{
    I2C_WAIT();
    IICCON(bus) = ack ? 0xb7 : 0x37;
    I2C_WAIT();
    while (!(IICCON(bus) & 0x10));
    return IICDS(bus);
}

static void s5l87xx_i2c_send_start(int bus, int addr, bool tx)
{
    uint8_t byte;
    if (addr > 0x77) byte = (0x78 | (addr >> 8));
    else byte = addr;
    I2C_WAIT();
    IICDS(bus) = (byte << 1) | !!tx;
    I2C_WAIT();
    IICSTAT(bus) = 0xf0;
    I2C_WAIT();
    IICCON(bus) = 0xb7;
    I2C_WAIT();
    while (!(IICCON(bus) & 0x10));
    if (tx && addr > 0x77) s5l87xx_i2c_send_byte(bus, addr);
}

static void s5l87xx_i2c_send_stop(int bus)
{
    I2C_WAIT();
    IICSTAT(bus) = 0x90;
    I2C_WAIT();
    IICCON(bus) = 0xb7;
    I2C_WAIT();
    while (IICSTAT(bus) & (1 << 5));
}

static enum i2c_result s5l87xx_i2c_txn(const struct i2c_driver_instance* instance, const struct i2c_transaction* txn)
{
    const struct s5l87xx_i2c_driver_config* config = (const struct s5l87xx_i2c_driver_config*)instance->driver_config;
    int bus = config->index;
    clockgate_enable(CLOCKGATE_I2C(bus), true);
    IICCON(bus) = 0xb7;
    IICSTAT(bus) = 0x10;
    bool tx = true;
    int index;
    for (index = 0; index < txn->transfercount; index++)
    {
        if (txn->transfers[index].type == I2C_TRANSFER_TYPE_TX) tx = true;
        else if (txn->transfers[index].type == I2C_TRANSFER_TYPE_RX) tx = false;
        uint8_t* buf = txn->transfers[index].rxbuf;
        int len = txn->transfers[index].len;
        if (!index && !tx) s5l87xx_i2c_send_start(bus, txn->address, true);
        if (!tx && !len) continue;
        if (txn->transfers[index].type != I2C_TRANSFER_TYPE_CONT)
            s5l87xx_i2c_send_start(bus, txn->address, tx);
        while (len--)
        {
            if (tx) s5l87xx_i2c_send_byte(bus, *buf++);
            else *buf++ = s5l87xx_i2c_recv_byte(bus, len);
        }
    }
    s5l87xx_i2c_send_stop(bus);
    IICSTAT(bus) = 0;
    clockgate_enable(CLOCKGATE_I2C(bus), false);
    return I2C_RESULT_OK;
}

const struct i2c_driver s5l87xx_i2c_driver =
{
    .init = s5l87xx_i2c_init,
    .txn = s5l87xx_i2c_txn,
};
