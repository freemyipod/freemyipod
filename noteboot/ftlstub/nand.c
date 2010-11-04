//
//
//    Copyright 2009 TheSeven
//
//
//    This file is part of the Linux4Nano toolkit.
//
//    TheSeven's iBugger is free software: you can redistribute it and/or
//    modify it under the terms of the GNU General Public License as
//    published by the Free Software Foundation, either version 2 of the
//    License, or (at your option) any later version.
//
//    TheSeven's iBugger is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//    See the GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License along
//    with the Linux4Nano toolkit.  If not, see <http://www.gnu.org/licenses/>.
//
//


#include <toolkit.h>
#include <util.h>
#include <timer.h>
#include <nand.h>

#define NAND_CMD_READ       0x00
#define NAND_CMD_PROGCNFRM  0x10
#define NAND_CMD_READ2      0x30
#define NAND_CMD_BLOCKERASE 0x60
#define NAND_CMD_GET_STATUS 0x70
#define NAND_CMD_PROGRAM    0x80
#define NAND_CMD_ERASECNFRM 0xD0
#define NAND_CMD_RESET      0xFF

#define NAND_STATUS_READY   0x40

#define NAND_DEVICEINFOTABLE_ENTRIES 33

static const struct nand_device_info_type nand_deviceinfotable[] =
{
    {0x1580F1EC, 1024, 968, 6, 2, 1, 0},
    {0x1580DAEC, 2048, 1936, 6, 2, 1, 0},
    {0x15C1DAEC, 2048, 1936, 6, 2, 1, 0},
    {0x1510DCEC, 4096, 3872, 6, 2, 1, 0},
    {0x95C1DCEC, 4096, 3872, 6, 2, 1, 0},
    {0x2514DCEC, 2048, 1936, 7, 2, 1, 0},
    {0x2514D3EC, 4096, 3872, 7, 2, 1, 0},
    {0x2555D3EC, 4096, 3872, 7, 2, 1, 0},
    {0x2555D5EC, 8192, 7744, 7, 2, 1, 0},
    {0x2585D3AD, 4096, 3872, 7, 3, 2, 0},
    {0x9580DCAD, 4096, 3872, 6, 3, 2, 0},
    {0xA514D3AD, 4096, 3872, 7, 3, 2, 0},
    {0xA550D3AD, 4096, 3872, 7, 3, 2, 0},
    {0xA560D5AD, 4096, 3872, 7, 3, 2, 0},
    {0xA555D5AD, 8192, 7744, 7, 3, 2, 0},
    {0xA585D598, 8320, 7744, 7, 3, 1, 0},
    {0xA584D398, 4160, 3872, 7, 3, 1, 0},
    {0x95D1D32C, 8192, 7744, 6, 2, 1, 0},
    {0x1580DC2C, 4096, 3872, 6, 2, 1, 0},
    {0x15C1D32C, 8192, 7744, 6, 2, 1, 0},
    {0x9590DC2C, 4096, 3872, 6, 2, 1, 0},
    {0xA594D32C, 4096, 3872, 7, 2, 1, 0},
    {0x2584DC2C, 2048, 1936, 7, 2, 1, 0},
    {0xA5D5D52C, 8192, 7744, 7, 3, 2, 0},
    {0x95D1D389, 8192, 7744, 6, 2, 1, 0},
    {0x1580DC89, 4096, 3872, 6, 2, 1, 0},
    {0x15C1D389, 8192, 7744, 6, 2, 1, 0},
    {0x9590DC89, 4096, 3872, 6, 2, 1, 0},
    {0xA594D389, 4096, 3872, 7, 2, 1, 0},
    {0x2584DC89, 2048, 1936, 7, 2, 1, 0},
    {0xA5D5D589, 8192, 7744, 7, 2, 1, 0},
    {0xA514D320, 4096, 3872, 7, 2, 1, 0},
    {0xA555D520, 8192, 3872, 7, 2, 1, 0}
};

static uint8_t nand_tunk1;
static uint8_t nand_twp;
static int nand_type[4];

static uint8_t nand_ctrl[0x200] __attribute__((aligned(16)));
static uint8_t nand_ecc[0x30] __attribute__((aligned(16)));


static uint32_t nand_wait_rbbdone(void)
{
    uint32_t timeout = USEC_TIMER + 20000;
    while (!(FMCSTAT & FMCSTAT_RBBDONE))
        if (TIME_AFTER(USEC_TIMER, timeout)) return 1;
    FMCSTAT = FMCSTAT_RBBDONE;
    return 0;
}

static void nand_wait_cmddone(void)
{
    while ((FMCSTAT & FMCSTAT_CMDDONE) == 0);
    FMCSTAT = FMCSTAT_CMDDONE;
}

static void nand_wait_addrdone(void)
{
    while ((FMCSTAT & FMCSTAT_ADDRDONE) == 0);
    FMCSTAT = FMCSTAT_ADDRDONE;
}

static void nand_wait_chip_ready(uint32_t bank)
{
    while ((FMCSTAT & (FMCSTAT_BANK0READY << bank)) == 0);
    FMCSTAT = (FMCSTAT_BANK0READY << bank);
}

static void nand_set_fmctrl0(uint32_t bank, uint32_t flags)
{
    FMCTRL0 = (nand_tunk1 << 16) | (nand_twp << 12)
            | (1 << 11) | 1 | (1 << (bank + 1)) | flags;
}

static uint32_t nand_send_cmd(uint32_t cmd)
{
    FMCMD = cmd;
    return nand_wait_rbbdone();
}

static void nand_send_address(uint32_t page, uint32_t offset)
{
    FMANUM = 4;
    FMADDR0 = (page << 16) | offset;
    FMADDR1 = (page >> 16) & 0xFF;
    FMCTRL1 = FMCTRL1_DOTRANSADDR;
    nand_wait_cmddone();
}

uint32_t nand_reset(uint32_t bank)
{
    nand_set_fmctrl0(bank, 0);
    if (nand_send_cmd(NAND_CMD_RESET)) return 1;
    nand_wait_chip_ready(bank);
    FMCTRL1 = FMCTRL1_CLEARRFIFO | FMCTRL1_CLEARWFIFO;
    sleep(1000);
    return 0;
}

static uint32_t nand_wait_status_ready(uint32_t bank)
{
    nand_set_fmctrl0(bank, 0);
    if ((FMCSTAT & (FMCSTAT_BANK0READY << bank)))
        FMCSTAT = (FMCSTAT_BANK0READY << bank);
    FMCTRL1 = FMCTRL1_CLEARRFIFO;
    nand_send_cmd(NAND_CMD_GET_STATUS);
    while (1)
    {
        FMDNUM = 0;
        FMCTRL1 = FMCTRL1_DOREADDATA;
        nand_wait_addrdone();
        if (FMFIFO & NAND_STATUS_READY) break;
        FMCTRL1 = FMCTRL1_CLEARRFIFO;
    }
    FMCTRL1 = FMCTRL1_CLEARRFIFO;
    nand_send_cmd(NAND_CMD_READ);
}

static void nand_transfer_data(uint32_t bank, uint32_t direction, void* buffer, uint32_t size)
{
    nand_set_fmctrl0(bank, FMCTRL0_ENABLEDMA);
    FMDNUM = size - 1;
    FMCTRL1 = FMCTRL1_DOREADDATA << direction;
    DMACON3 = (2 << DMACON_DEVICE_SHIFT)
            | (direction << DMACON_DIRECTION_SHIFT)
            | (2 << DMACON_DATA_SIZE_SHIFT)
            | (3 << DMACON_BURST_LEN_SHIFT);
    while ((DMAALLST & DMAALLST_CHAN3_MASK))
        DMACOM3 = DMACOM_CLEARBOTHDONE;
    DMABASE3 = (uint32_t)buffer;
    DMATCNT3 = (size >> 4) - 1;
    DMACOM3 = 4;
    while (DMAALLST & DMAALLST_DMABUSY3);
    nand_wait_addrdone();
    if (!direction) FMCTRL1 = FMCTRL1_CLEARRFIFO | FMCTRL1_CLEARWFIFO;
    else FMCTRL1 = FMCTRL1_CLEARRFIFO;
}

static uint32_t ecc_decode(uint32_t size, void* databuffer, void* sparebuffer)
{
    ECC_INT_CLR = 1;
    SRCPND = INTMSK_ECC;
    ECC_UNK1 = size;
    ECC_DATA_PTR = (uint32_t)databuffer;
    ECC_SPARE_PTR = (uint32_t)sparebuffer;
    ECC_CTRL = ECCCTRL_STARTDECODING;
    while (!(SRCPND & INTMSK_ECC));
    ECC_INT_CLR = 1;
    SRCPND = INTMSK_ECC;
    return ECC_RESULT;
}

uint32_t nand_check_empty(uint8_t* buffer)
{
    uint32_t i, count;
    count = 0;
    for (i = 0; i < 0x40; i++) if (buffer[i] != 0xFF) count++;
    if (count < 2) return 1;
    return 0;
}

uint32_t nand_get_chip_type(uint32_t bank)
{
    uint32_t result;
    if (nand_reset(bank)) return 0xFFFFFFFF;
    if (nand_send_cmd(0x90)) return 0xFFFFFFFF;
    FMANUM = 0;
    FMADDR0 = 0;
    FMCTRL1 = FMCTRL1_DOTRANSADDR;
    nand_wait_cmddone();
    FMDNUM = 4;
    FMCTRL1 = FMCTRL1_DOREADDATA;
    nand_wait_addrdone();
    result = FMFIFO;
    FMCTRL1 = FMCTRL1_CLEARRFIFO;
    return result;
}

uint32_t nand_read_page(uint32_t bank, uint32_t page, void* databuffer,
                        void* sparebuffer, uint32_t checkempty)
{
    uint32_t rc, eccresult;
    uint8_t* data = (uint8_t*)databuffer;
    uint8_t* spare = (uint8_t*)sparebuffer;
    nand_set_fmctrl0(bank, FMCTRL0_ENABLEDMA);
    nand_send_cmd(NAND_CMD_READ);
    nand_send_address(page, 0);
    nand_send_cmd(NAND_CMD_READ2);
    nand_wait_status_ready(bank);
    nand_transfer_data(bank, 0, data, 0x800);
    nand_transfer_data(bank, 0, spare, 0x40);
    memcpy(nand_ecc, &spare[0xC], 0x28);
    rc = (ecc_decode(3, data, nand_ecc) & 0xF) << 4;
    memset(nand_ctrl, 0xFF, 0x200);
    memcpy(nand_ctrl, spare, 0xC);
    memcpy(nand_ecc, &spare[0x34], 0xC);
    eccresult = ecc_decode(0, nand_ctrl, nand_ecc);
    rc |= (eccresult & 0xF) << 8;
    if (eccresult & 1) memset(spare, 0xFF, 0xC);
    else memcpy(spare, nand_ctrl, 0xC);
    if (checkempty) rc |= nand_check_empty(spare) << 1;
    return rc;
}

const struct nand_device_info_type* nand_get_device_type(uint32_t bank)
{
    if (nand_type[bank] < 0)
        return (struct nand_device_info_type*)0;
    return &nand_deviceinfotable[nand_type[bank]];
}

uint32_t nand_init()
{
    uint32_t type;
    uint32_t i, j;
    PWRCONEXT &= ~0x40;
    PWRCON &= ~0x100000;
    PCON2 = 0x33333333;
    PDAT2 = 0;
    PCON3 = 0x11113333;
    PDAT3 = 0;
    PCON4 = 0x33333333;
    PDAT4 = 0;
    PCON5 = (PCON5 & ~0xF) | 3;
    PUNK5 = 1;
    sleep(10000);
    for (i = 0; i < 4; i++) nand_type[i] = -1;
    for (i = 0; i < 4; i++)
    {
        nand_tunk1 = 7;
        nand_twp = 7;
        type = nand_get_chip_type(i);
        if (type >= 0xFFFFFFF0)
        {
            nand_type[i] = (int)type;
            continue;
        }
        for (j = 0; ; j++)
        {
            if (j == NAND_DEVICEINFOTABLE_ENTRIES) break;
            else if (nand_deviceinfotable[j].id == type)
            {
                nand_type[i] = j;
                break;
            }
        }
    }
    nand_tunk1 = nand_deviceinfotable[nand_type[0]].tunk1;
    nand_twp = nand_deviceinfotable[nand_type[0]].twp;
    return 0;
}
