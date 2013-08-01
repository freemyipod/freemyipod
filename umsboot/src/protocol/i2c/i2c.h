#ifndef __PROTOCOL_I2C_I2C_H__
#define __PROTOCOL_I2C_I2C_H__

#include "global.h"

#ifndef ASM_FILE
enum i2c_result
{
    I2C_RESULT_OK = 0,
    I2C_RESULT_BAD_OBJECT,
    I2C_RESULT_INVALID_STATE,
    I2C_RESULT_INVALID_ARGUMENT,
    I2C_RESULT_UNSUPPORTED,
    I2C_RESULT_UNKNOWN_ERROR,
    I2C_RESULT_NAK,
};

struct __attribute__((packed,aligned(4))) i2c_transaction
{
    uint16_t address;
    uint8_t reserved;
    uint8_t transfercount;
    struct __attribute__((packed,aligned(4))) i2c_transfer
    {
        enum __attribute__((packed)) i2c_transfer_type
        {
            I2C_TRANSFER_TYPE_TX,
            I2C_TRANSFER_TYPE_RX,
            I2C_TRANSFER_TYPE_CONT,
        } type;
        uint8_t reserved;
        uint16_t len;
        union __attribute__((packed,aligned(4)))
        {
            const void* txbuf;
            void* rxbuf;
        };
    } transfers[];
};

struct i2c_driver;

struct i2c_driver_instance
{
    const struct i2c_driver* driver;
    const void* driver_config;
    void* driver_state;
};

struct i2c_driver
{
    enum i2c_result (*init)(const struct i2c_driver_instance* instance);
    enum i2c_result (*txn)(const struct i2c_driver_instance* instance, const struct i2c_transaction* transaction);
};

extern enum i2c_result i2c_init(const struct i2c_driver_instance* instance);
extern enum i2c_result i2c_txn(const struct i2c_driver_instance* instance, const struct i2c_transaction* transaction);
extern enum i2c_result i2c_read_regs(const struct i2c_driver_instance* instance, int address, int reg, void* buf, int len);
extern enum i2c_result i2c_write_regs(const struct i2c_driver_instance* instance, int address, int reg, const void* buf, int len);
#endif

#endif
