#!/usr/bin/env python
#
#
#    Copyright 2010 TheSeven, benedikt93, Farthen
#
#
#    This file is part of emBIOS.
#
#    emBIOS is free software: you can redistribute it and/or
#    modify it under the terms of the GNU General Public License as
#    published by the Free Software Foundation, either version 2 of the
#    License, or (at your option) any later version.
#
#    emBIOS is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
#    See the GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License
#    along with emBIOS.  If not, see <http://www.gnu.org/licenses/>.
#
#

thread_state = (
    "THREAD_FREE",
    "THREAD_SUSPENDED",
    "THREAD_READY",
    "THREAD_RUNNING",
    "THREAD_BLOCKED",
    "THREAD_DEFUNCT",
    "THREAD_DEFUNCT_ACK"
)

thread_block = (
    "THREAD_NOT_BLOCKED",
    "THREAD_BLOCK_SLEEP",
    "THREAD_BLOCK_MUTEX",
    "THREAD_BLOCK_WAKEUP",
    "THREAD_DEFUNCT_STKOV",
    "THREAD_DEFUNCT_PANIC"
)

thread_type = (
    "USER_THREAD",
    "OS_THREAD",
    "CORE_THREAD"
)

hwtypes = {
    0: "invalid",
    0x47324e49: "iPod nano 2g",
    0x47334e49: "iPod nano 3g",
    0x47344e49: "iPod nano 4g",
    0x4c435049: "iPod classic"
}

swtypes = {
    0: "invalid",
    1: "emBIOS Debugger"
}

responsecodes = {
    0: "invalid",
    1: "ok",
    2: "unsupported",
    3: "busy"
}