#include "global.h"
#include "protocol/i2c/i2c.h"

enum i2c_result i2c_init(const struct i2c_driver_instance* instance)
{
    if (!instance) return I2C_RESULT_BAD_OBJECT;
    return instance->driver->init(instance);
}

enum i2c_result i2c_txn(const struct i2c_driver_instance* instance, const struct i2c_transaction* transaction)
{
    if (!instance) return I2C_RESULT_BAD_OBJECT;
    return instance->driver->txn(instance, transaction);
}

enum i2c_result i2c_read_regs(const struct i2c_driver_instance* instance, int address, int reg, void* buf, int len)
{
    if (!instance) return I2C_RESULT_BAD_OBJECT;
    if (reg < 0 || reg > 255) return I2C_RESULT_INVALID_ARGUMENT;
    struct __attribute__((packed,aligned(4)))
    {
        struct i2c_transaction info;
        struct i2c_transfer transfers[2];
    } txn =
    {
        .info.address = address,
        .info.transfercount = 2,
        .transfers =
        {
            { .type = I2C_TRANSFER_TYPE_TX, .len = 1, .txbuf = &reg },
            { .type = I2C_TRANSFER_TYPE_RX, .len = len, .txbuf = buf },
        },
    };
    return i2c_txn(instance, &txn.info);
}

enum i2c_result i2c_write_regs(const struct i2c_driver_instance* instance, int address, int reg, const void* buf, int len)
{
    if (!instance) return I2C_RESULT_BAD_OBJECT;
    if (reg < 0 || reg > 255) return I2C_RESULT_INVALID_ARGUMENT;
    struct __attribute__((packed,aligned(4)))
    {
        struct i2c_transaction info;
        struct i2c_transfer transfers[2];
    } txn =
    {
        .info.address = address,
        .info.transfercount = 2,
        .transfers =
        {
            { .type = I2C_TRANSFER_TYPE_TX, .len = 1, .txbuf = &reg },
            { .type = I2C_TRANSFER_TYPE_CONT, .len = len, .txbuf = buf },
        },
    };
    return i2c_txn(instance, &txn.info);
}
