#!/usr/bin/env python
#
#
#    Copyright 2010 TheSeven
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

import struct

def encode(addr, option):
    if option == None: return ""
    addr = addr + 16
    data1 = encode(addr, option[2])
    if len(data1) == 0: addr1 = 0
    else: addr1 = addr
    addr = addr + len(data1)
    data2 = encode(addr, option[3])
    if len(data2) == 0: addr2 = 0
    else: addr2 = addr
    addr = addr + len(data2)
    if type(option[1]).__name__ == "str":
        data = option[1] + '\0'
        data = data.ljust((len(data) + 3) & ~3, '\0')
    else:
        data = ""
        addr = option[1]
    return struct.pack("<IIII", addr1, addr2, option[0], addr) + data1 + data2 + data
    

def configure(binary, options):
    fileaddr =  binary.index("emCOboot")
    version = struct.unpack("<I", binary[fileaddr + 8 : fileaddr + 12])[0]
    if version != 1: raise ValueError("Unknown boot configuration data version")
    memaddr = struct.unpack("<I", binary[fileaddr + 12 : fileaddr + 16])[0]
    data = encode(memaddr + 24, options)
    if len(data) == 0: addr = 0
    else: addr = memaddr + 24
    return binary[:fileaddr + 16] + struct.pack("<II", len(data) + 24, addr) + data
    