"""Nano5g monitor commands. Device definitions live in emcore/usb/usbdebug.c."""
from enum import IntEnum

class Cmd(IntEnum):
    GETINFO = 1
    READ_MEM = 4
    WRITE_MEM = 5
    READ_CONSOLE = 10
    WRITE_CONSOLE = 11
    GETPROCINFO = 15
    FREEZE = 16
    SUSPEND = 17
    KILL = 18
    CREATETHREAD = 19
    FLUSHCACHE = 20
    WEDGE_FIX = 68
    RESET = 104
    HEARTBEAT_ASYNC = 200
    NAND_STATUS = 120
    NAND_STEP = 121
    NAND_READ_ROM = 122
    NAND_READ_PAGE = 123
    NAND_FTL_OPEN = 124
    NAND_FTL_READ = 125
    NAND_BLOCK_READ = 127
    NAND_WRITE_CAPS = 128
    NAND_WRITE_OPEN = 129
    NAND_BLOCK_WRITE = 130
    NAND_FLUSH = 131
    MODULE_INFO = 132
    MODULE_RESERVE = 133
    MODULE_ACTIVATE = 134
