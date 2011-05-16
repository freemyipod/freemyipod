#!/usr/bin/env python
#
#
#    Copyright 2010 TheSeven, benedikt93, Farthen
#
#
#    This file is part of emCORE.
#
#    emCORE is free software: you can redistribute it and/or
#    modify it under the terms of the GNU General Public License as
#    published by the Free Software Foundation, either version 2 of the
#    License, or (at your option) any later version.
#
#    emCORE is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
#    See the GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License
#    along with emCORE.  If not, see <http://www.gnu.org/licenses/>.
#
#

"""
    Contains emCORE data structures, enums and dicts
"""

from ctypes import *
from misc import ExtendedCStruct, c_enum


class thread_type(c_enum):
    _fields_ = ["USER_THREAD",
                "OS_THREAD",
                "CORE_THREAD",
               ]

class thread_state(c_enum):
    _fields_ = ["THREAD_FREE",
                "THREAD_SUSPENDED",
                "THREAD_READY",
                "THREAD_RUNNING",
                "THREAD_BLOCKED",
                "THREAD_DEFUNCT",
                "THREAD_DEFUNCT_ACK",
               ]

class thread_block(c_enum):
    _fields_ = ["THREAD_NOT_BLOCKED",
                "THREAD_BLOCK_SLEEP",
                "THREAD_BLOCK_MUTEX",
                "THREAD_BLOCK_WAKEUP",
                "THREAD_DEFUNCT_STKOV",
                "THREAD_DEFUNCT_PANIC"
               ]

class responsecode(c_enum):
    _fields_ = ["INVALID",
                "OK",
                "UNSUPPORTED",
                "BUSY"
               ]

class scheduler_thread(ExtendedCStruct):
    _fields_ = [("regs", c_uint32 * 16),
                ("cpsr", c_uint32),
                ("state", c_uint32),
                ("name", c_uint32),
                ("cputime_current", c_uint32),
                ("cputime_total", c_uint64),
                ("startusec", c_uint32),
                ("thread_next", c_uint32),
                ("queue_next", c_uint32),
                ("owned_mutexes", c_uint32),
                ("timeout", c_uint32),
                ("blocked_since", c_uint32),
                ("blocked_by", c_uint32),
                ("stack", c_uint32),
                ("err_no", c_int32),
                ("block_type", thread_block),
                ("thread_type", thread_type),
                ("priority", c_uint8),
                ("cpuload", c_uint8),
               ]

class mutex(ExtendedCStruct):
    _fields_ = [("owner", c_uint32),
                ("waiters", c_uint32),
                ("count", c_int32),
                ]

class wakeup(ExtendedCStruct):
    _fields_ = [("waiter", c_uint32),
                ("signalled", c_uint32),
               ]


swtypes = {
    0: "invalid",
    1: "emBIOS Debugger",
    2: "emCORE Debugger"
}

hwtypes = {
    0:          ("invalid",     "invalid"),
    0x47324e49: ("ipodnano2g",  "iPod nano 2g"),
    0x47334e49: ("ipodnano3g",  "iPod nano 3g"),
    0x47344e49: ("ipodnano4g",  "iPod nano 4g"),
    0x4c435049: ("ipodclassic", "iPod classic"),
}