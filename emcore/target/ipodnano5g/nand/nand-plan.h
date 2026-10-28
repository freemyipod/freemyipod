/***************************************************************************
 * Copyright (C) 2026
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * S5L87xx NAND command planning. No MMIO or sequencer instructions live here.
 * OSOS addresses identify the routines whose behavior this code implements.
 * A request is planned completely before the controller can modify NAND.
 * Command families are chip capabilities, not generation switches. Keep cache,
 * multiplane and enhanced-status code shared: nano4g populations also need it.
 ****************************************************************************/
#ifndef S5L_NAND_PLAN_H
#define S5L_NAND_PLAN_H

#include <stddef.h>
#include "libc/include/stdint.h"
#include "libc/include/string.h"

struct nand_program_plan
{
    uint32_t *commands;       /* {command, physical row} pairs + zero terminator */
    uint32_t *dma;            /* one bus address for each 2048-byte chunk */
    uint8_t *metadata;        /* 12 bytes per page, in command order */
    unsigned int capacity;   /* pages allocated in all three arrays */
    unsigned int count;
    uint32_t status_row;     /* enhanced-status second-die row, or zero */
};

/* OSOS mem:0x0804a430 selects these eight descriptor families.
 * Implementations: default 0x081ddbdc; 10 0x08057480; 11 0x08057294;
 * 12 0x080572b0; 13 0x08057520; 14 0x08057546; 15 0x080574b6;
 * 16 0x08057582. The low byte is a physical CE mask, not a bank index. */
static inline uint32_t nand_program_command(uint32_t layout, unsigned int plane,
                                           uint8_t ce, _Bool busy, _Bool last)
{
    unsigned int mode = layout >> 16;
    uint32_t status = 0x70, setup = 0x80, confirm = 0x10;
    _Bool odd = (plane & 1) != 0;

    switch (mode)
    {
        case 0x10:
            setup = odd ? 0x81 : 0x80;
            confirm = odd ? 0x10 : 0x11;
            break;
        case 0x11:
            status = 0x78;
            break;
        case 0x12:
            confirm = odd ? 0x10 : 0x11;
            break;
        case 0x13:
            status = 0x78;
            confirm = last ? 0x10 : 0x15;
            break;
        case 0x14:
            status = 0x78;
            confirm = !odd ? 0x11 : last ? 0x10 : 0x15;
            break;
        case 0x15:
            status = 0x71;
            confirm = odd ? 0x10 : 0x11;
            break;
        case 0x16:
            status = 0x71;
            setup = odd ? 0x81 : 0x80;
            confirm = !odd ? 0x11 : last ? 0x10 : 0x15;
            break;
    }
    return (busy ? status << 24 : 0) | setup << 16 | confirm << 8 | ce;
}

/* Native helper: retain separate source and command indices when OSOS groups
 * pages by CE for multi-plane programming. Metadata follows the same permutation. */
static inline void nand_plan_page(struct nand_program_plan *plan,
                                  unsigned int output, unsigned int input,
                                  uint32_t command, uint32_t row,
                                  unsigned int page_bytes, uint32_t data,
                                  const uint8_t *metadata)
{
    unsigned int chunks = page_bytes / 2048;
    plan->commands[2 * output] = command;
    plan->commands[2 * output + 1] = row;
    for (unsigned int chunk = 0; chunk < chunks; chunk++)
        plan->dma[output * chunks + chunk] = data + input * page_bytes + chunk * 2048;
    memcpy(plan->metadata + output * 12, metadata + input * 12, 12);
}

/* OSOS mem:0x08044040: scattered pages use normal 80/10 programming,
 * irrespective of the multi-plane descriptor selected for sequential calls. */
static inline _Bool nand_plan_scattered(struct nand_program_plan *plan,
                                      const uint8_t *ce_map, unsigned int banks,
                                      unsigned int page_bytes, unsigned int count,
                                      const uint8_t *bank, const uint32_t *row,
                                      uint32_t data, const uint8_t *metadata)
{
    if (count == 0 || count > plan->capacity || banks == 0 || banks > 8 ||
        (page_bytes != 4096 && page_bytes != 8192))
        return false;
    for (unsigned int i = 0; i < count; i++)
        if (bank[i] >= banks || ce_map[bank[i]] >= 8)
            return false;

    uint8_t busy = 0;
    for (unsigned int i = 0; i < count; i++)
    {
        uint8_t ce = 1u << ce_map[bank[i]];
        nand_plan_page(plan, i, i, nand_program_command(0, 0, ce, (busy & ce) != 0, false),
                       row[i], page_bytes, data, metadata);
        busy |= ce;
    }
    plan->commands[count * 2] = 0;
    plan->count = count;
    plan->status_row = 0;
    return true;
}

/* OSOS mem:0x080440fc: input is plane-major within each stripe. Aligned,
 * correctly banked requests use CE-major command order and the selected
 * multi-plane/cache protocol. Other requests retain input order and 80/10.
 * The caller supplies disjoint input/output metadata buffers. */
static inline _Bool nand_plan_sequential(struct nand_program_plan *plan,
                                       const uint8_t *ce_map, unsigned int banks,
                                       unsigned int planes, unsigned int page_bytes,
                                       unsigned int blocks, unsigned int pages_per_block,
                                       uint32_t layout, unsigned int count,
                                       const uint32_t *row_base, uint32_t data,
                                       const uint8_t *metadata, _Bool aligned,
                                       _Bool correct_bank)
{
    unsigned int stripe = banks * planes;
    if (banks == 0 || banks > 8 || planes == 0 || planes > 2 || count == 0 ||
        count > plan->capacity || count % stripe != 0 ||
        (page_bytes != 4096 && page_bytes != 8192))
        return false;
    for (unsigned int bank = 0; bank < banks; bank++)
        if (ce_map[bank] >= 8)
            return false;

    _Bool grouped = aligned && correct_bank && layout != 0x00010001;
    uint8_t busy = 0;
    for (unsigned int start = 0; start < count; start += stripe)
        for (unsigned int slot = 0; slot < stripe; slot++)
        {
            unsigned int bank = grouped ? slot / planes : slot % banks;
            unsigned int plane = grouped ? slot % planes : slot / banks;
            unsigned int input = start + plane * banks + bank;
            uint8_t ce = 1u << ce_map[bank];
            uint32_t command = nand_program_command(grouped ? layout : 0, plane, ce,
                                                     (busy & ce) != 0,
                                                     start + stripe == count);
            nand_plan_page(plan, start + slot, input, command,
                           row_base[plane * banks + bank] + start / stripe,
                           page_bytes, data, metadata);
            busy |= ce;
        }
    unsigned int mode = layout >> 16;
    plan->status_row = grouped && mode >= 0x11 && mode <= 0x14
                       ? blocks * pages_per_block / 2 : 0;
    plan->commands[count * 2] = 0;
    plan->count = count;
    return true;
}
#endif
