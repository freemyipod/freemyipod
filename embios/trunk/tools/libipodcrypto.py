#!/usr/bin/env python
#
#
#    Copyright 2010 TheSeven
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


import sys
import struct
import time
import libembios
from libembios import Error
import libembiosdata


def nano2gcryptdfu(data):
    data = data.ljust((len(data) + 0x3f) & ~0x3f, "\0")
    header = "87011.0\0\0\x08\0\0" + struct.pack("<I", len(data))
    embios = libembios.Embios()
    embios.write(0x08000000, header.ljust(0x800, "\0") + data)
    embios.lib.dev.timeout = 20000
    embios.hmac_sha1(0x08000800, len(data), 0x08000010)
    embios.hmac_sha1(0x08000000, 0x40, 0x08000040)
    embios.aesencrypt(0x08000000, len(data) + 0x800, 1)
    return embios.read(0x08000000, len(data) + 0x800)


def nano2gdecryptdfu(data):
    embios = libembios.Embios()
    embios.write(0x08000000, data)
    embios.lib.dev.timeout = 20000
    embios.aesdecrypt(0x08000000, len(data), 1)
    return embios.read(0x08000800, len(data) - 0x800)


def nano2gcryptfirmware(data):
    data = data.ljust((len(data) + 0x3f) & ~0x3f, "\0")
    header = "\0\0\0\0\x02\0\0\0\x01\0\0\0\x40\0\0\0\0\0\0\0" + struct.pack("<I", len(data))
    embios = libembios.Embios()
    embios.write(0x08000000, header.ljust(0x800, "\0") + data)
    embios.lib.dev.timeout = 20000
    embios.hmac_sha1(0x08000800, len(data), 0x0800001c)
    embios.hmac_sha1(0x08000000, 0x200, 0x080001d4)
    embios.aesencrypt(0x08000800, len(data), 1)
    return embios.read(0x08000000, len(data) + 0x800)


def nano2gdecryptfirmware(data):
    embios = libembios.Embios()
    embios.write(0x08000000, data)
    embios.lib.dev.timeout = 20000
    embios.aesdecrypt(0x08000800, len(data) - 0x800, 1)
    return embios.read(0x08000800, len(data) - 0x800)


def nano2gcryptdfufile(infile, outfile):
    print(outfile)
    infile = open(infile, "rb")
    outfile = open(outfile, "wb")
    outfile.write(nano2gcryptdfu(infile.read()))
    infile.close()
    outfile.close()


def nano2gdecryptdfufile(infile, outfile):
    infile = open(infile, "rb")
    outfile = open(outfile, "wb")
    outfile.write(nano2gdecryptdfu(infile.read()))
    infile.close()
    outfile.close()


def nano2gcryptfirmwarefile(infile, outfile):
    infile = open(infile, "rb")
    outfile = open(outfile, "wb")
    outfile.write(nano2gcryptfirmware(infile.read()))
    infile.close()
    outfile.close()


def nano2gdecryptfirmwarefile(infile, outfile):
    infile = open(infile, "rb")
    outfile = open(outfile, "wb")
    outfile.write(nano2gdecryptfirmware(infile.read()))
    infile.close()
    outfile.close()
