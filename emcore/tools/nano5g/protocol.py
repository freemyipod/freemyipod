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
