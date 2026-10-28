/***************************************************************************
 * Copyright (C) 2026
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * CPU-driven NAND page/spare transfers, independent of FMSS programs.
 * Derived from OSOS Thumb code, with register access supplied by the caller.
 ****************************************************************************/
#ifndef S5L_NAND_RAW_H
#define S5L_NAND_RAW_H
#include "libc/include/stdint.h"
#include "libc/include/string.h"

struct nand_raw_bus
{
    void *context;
    uint32_t (*read)(void *context, unsigned int offset);
    void (*write)(void *context, unsigned int offset, uint32_t value);
    int (*wait)(void *context, uint32_t status_mask);
};

/* OSOS mem:0x081e7c40: send one command and wait for its command event. */
static inline int nand_raw_command(const struct nand_raw_bus *bus, uint32_t command)
{
    bus->write(bus->context, 0x008, command);
    return bus->wait(bus->context, 2);
}

/* OSOS mem:0x08056350: column 0, three row bytes, five address cycles. */
static inline int nand_raw_address(const struct nand_raw_bus *bus, uint32_t row)
{
    bus->write(bus->context, 0x02c, 4);
    bus->write(bus->context, 0x00c, row << 16);
    bus->write(bus->context, 0x010, row >> 16);
    bus->write(bus->context, 0x004, 1);
    return bus->wait(bus->context, 4);
}

/* OSOS mem:0x0805614c: ready polling leaves the selected chip in status mode. */
static inline int nand_raw_ready(const struct nand_raw_bus *bus)
{
    (void)nand_raw_command(bus, 0x70);
    bus->write(bus->context, 0x048, 0x800000);
    bus->write(bus->context, 0x014, 1);
    bus->write(bus->context, 0x004, 0xea);
    int result = bus->wait(bus->context, 0x800000);
    bus->write(bus->context, 0x004, 0x20);
    return result;
}

/* OSOS mem:0x08056fd4 (write), 0x08064e30 (read). Sector buffer 0 holds
 * 512 bytes; buffer 4 is the 64-byte FIFO used for the spare-area tail. */
static inline void nand_raw_fifo(const struct nand_raw_bus *bus, uint32_t *words,
                                unsigned int buffer, unsigned int bytes, _Bool write)
{
    uint32_t control = bus->read(bus->context, 0);
    bus->write(bus->context, 0, control | 0x02000000);
    bus->write(bus->context, 0x014, 1u << (buffer + (write ? 0 : 8)));
    bus->write(bus->context, 0x004, write ? 0x2e0 : 0x1e0);
    for (unsigned int i = 0; i < bytes / 4; i++)
        if (write)
            bus->write(bus->context, 0x080, words[i]);
        else
            words[i] = bus->read(bus->context, 0x080);
    bus->write(bus->context, 0, bus->read(bus->context, 0) & ~0x02000000u);
}

/* OSOS mem:0x08053ad4 (read), 0x08053bd8 (write).
 * Transfers the entire physical data/spare stream without ECC interpretation.
 * Like OSOS's raw primitive, write completion waits for ready but does not
 * evaluate the NAND program-failure bit. The ECC program API checks that bit. */
static inline int nand_raw_transfer(const struct nand_raw_bus *bus, unsigned int ce,
                                   uint32_t timing, uint32_t row, uint8_t *data,
                                   unsigned int data_bytes, uint8_t *spare,
                                   unsigned int spare_bytes, _Bool write)
{
    uint32_t scratch[128] = {0};
    int result;
    bus->write(bus->context, 0x038, 7);
    bus->write(bus->context, 0, timing | (1u << ((ce & 7) + 1)) | 1);
    bus->write(bus->context, 0x048, 0x0ff00ffe);
    if (!write)
        (void)nand_raw_command(bus, 0); /* OSOS issues READ twice. */
    if ((result = nand_raw_command(bus, write ? 0x80 : 0)) != 0 ||
        (result = nand_raw_address(bus, row)) != 0)
        return result;
    if (!write && ((result = nand_raw_command(bus, 0x30)) != 0 ||
                   (result = nand_raw_ready(bus)) != 0 ||
                   (result = nand_raw_command(bus, 0)) != 0))
        return result;

    for (unsigned int part = 0; part < 2; part++)
    {
        uint8_t *buffer = part ? spare : data;
        unsigned int remaining = part ? spare_bytes : data_bytes;
        while (remaining != 0)
        {
            unsigned int bytes = remaining >= 512 ? 512 : remaining >= 64 ? 64 : remaining;
            _Bool sector = bytes == 512;
            if (write)
            {
                memcpy(scratch, buffer, bytes);
                nand_raw_fifo(bus, scratch, sector ? 0 : 4, sector ? 512 : 64, true);
            }
            bus->write(bus->context, 0x030, bytes - 1);
            if (sector)
                bus->write(bus->context, 0x018, 1);
            bus->write(bus->context, 0x014, (sector ? 1u : 0x10u) << (write ? 8 : 0));
            bus->write(bus->context, 0x004, (sector ? 0xe0u : 0x100f0u) | (write ? 4 : 2));
            if ((result = bus->wait(bus->context, 8)) != 0)
                return result;
            if (!write)
            {
                nand_raw_fifo(bus, scratch, sector ? 0 : 4, sector ? 512 : 64, false);
                memcpy(buffer, scratch, bytes);
            }
            buffer += bytes;
            remaining -= bytes;
        }
    }
    if (write)
    {
        (void)nand_raw_command(bus, 0x10);
        (void)nand_raw_ready(bus);
    }
    bus->write(bus->context, 0, 0);
    return 0;
}
#endif
