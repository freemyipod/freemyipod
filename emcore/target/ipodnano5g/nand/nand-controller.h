/* Copyright (C) 2009 Michael Sparmann
 * Copyright (C) 2026 Viktor Krapivin
 * SPDX-License-Identifier: GPL-2.0-or-later
 * Shared S5L87xx controller implementation. Caller serializes all requests. */
/* True once us microseconds have passed since start (USEC_TIMER wraps; the
 * unsigned difference handles that) */
static _Bool nand_elapsed(uint32_t start, uint32_t us)
{
    return (uint32_t)(USEC_TIMER - start) >= us;
}

/* Wait until (reg & mask) == value, at most POLL_TIMEOUT_US */
static int fmss_poll(unsigned int reg, uint32_t mask, uint32_t value)
{
    uint32_t start = USEC_TIMER;
    while ((FMSS_RD(reg) & mask) != value)
        if (nand_elapsed(start, POLL_TIMEOUT_US))
            return NAND_ERR_TIMEOUT;
    return 0;
}

/* Stop the sequencer and reset its buffers, FMC and ECC unit. */

static int fmss_reset(void)
{
    uint32_t start, status;

    FMSS_WR(FMSS_CTRL, FMSS_CTRL_STOP);
    start = USEC_TIMER;
    while ((status = FMSS_RD(FMSS_STATUS)) != 0)
    {
        if (status == 4)  /* FMSS_STATUS 4 = a program is running */
        {
            /* busy: abort the program and wait for idle or aborted */
            FMSS_WR(FMSS_CTRL, FMSS_CTRL_ABORT);
            start = USEC_TIMER;
            while ((status = FMSS_RD(FMSS_STATUS)) != 0 && status != 3)  /* 0 idle, 3 aborted */
                if (nand_elapsed(start, POLL_TIMEOUT_US))
                    return NAND_ERR_TIMEOUT;
            break;
        }
        if (nand_elapsed(start, POLL_TIMEOUT_US))
            return NAND_ERR_TIMEOUT;
    }

    if (FMSS_RD(FMSS_BUFCFG) != FMSS_BUFCFG_BYPASS)
    {
        FMSS_WR(FMSS_BUFRST, 1);
        if (fmss_poll(FMSS_BUFRST_OK, 0xffffffff, 1))
            return NAND_ERR_TIMEOUT;
        FMSS_WR(FMSS_BUFRST, 0);
    }

    FMSS_WR(FMSS_CTRL, FMSS_CTRL_FMCRESET);
    FMSS_WR(FMC_CTRL1, FMCTRL1_RESET);
    if (fmss_poll(FMC_CTRL1, FMCTRL1_IDLE, FMCTRL1_IDLE))
        return NAND_ERR_TIMEOUT;
    FMSS_WR(FMC_CTRL1, FMCTRL1_RELEASE);
    FMSS_WR(ECC_RESET, 1);
    return 0;
}

/* Clear done/fault/pause flags and wait for acknowledgement. */

static int fmss_clear_irq(void)
{
    FMSS_WR(FMSS_IRQ, FMSS_IRQ_MASK);
    return fmss_poll(FMSS_IRQ, FMSS_IRQ_MASK, 0);
}

/* Reset buffers and start the program at FMSS_PROG. */

static int fmss_start(void)
{
    FMSS_WR(FMSS_IRQ, FMSS_IRQ_ALL);
    FMSS_WR(FMSS_BUFCFG, 0);
    FMSS_WR(FMSS_BUFRST, 1);
    if (fmss_poll(FMSS_BUFRST_OK, 0xffffffff, 1))
        return NAND_ERR_TIMEOUT;
    FMSS_WR(FMSS_BUFRST, 0);
    FMSS_WR(FMSS_CTRL, FMSS_CTRL_START);
    return 0;
}

/* Wait for completion; reset the sequencer on fault or timeout. */
/* A pause advances FMSS_PC by one instruction before resuming. */
static int fmss_wait(void)
{
    uint32_t start = USEC_TIMER, irq;

    do
    {
        if (nand_elapsed(start, PROG_TIMEOUT_US))
        {
            fmss_reset();
            return NAND_ERR_TIMEOUT;
        }
        irq = FMSS_RD(FMSS_IRQ) & FMSS_IRQ_MASK;
        if (irq & FMSS_IRQ_PAUSE)
        {
            FMSS_WR(FMSS_PC, FMSS_RD(FMSS_PC) + 8);  /* skip the 8-byte pausing instruction */
            FMSS_WR(FMSS_IRQ, FMSS_IRQ_PAUSE);
            irq &= ~FMSS_IRQ_PAUSE;
            FMSS_WR(FMSS_CTRL, FMSS_RD(FMSS_CTRL) | FMSS_CTRL_RESUME);
        }
    } while (!(irq & FMSS_IRQ_DONE));

    if (irq != FMSS_IRQ_DONE && (irq & FMSS_IRQ_FAULT))
        fmss_reset();
    if (fmss_clear_irq())
        return NAND_ERR_TIMEOUT;
    return irq == FMSS_IRQ_DONE ? 0 : NAND_ERR_TIMEOUT;
}
