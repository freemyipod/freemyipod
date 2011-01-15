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


import sys
import os
import struct
import time
import hashlib
import libemcore
from libemcore import Error
import libemcoredata


def s5l8701cryptdfu(data):
    data = data.ljust((len(data) + 0x3f) & ~0x3f, "\0")
    header = "87011.0\0\0\x08\0\0" + struct.pack("<I", len(data))
    emcore = libemcore.Emcore()
    emcore.write(0x08000000, header.ljust(0x800, "\0") + data)
    emcore.hmac_sha1(0x08000800, len(data), 0x08000010)
    emcore.hmac_sha1(0x08000000, 0x40, 0x08000040)
    emcore.aesencrypt(0x08000000, len(data) + 0x800, 1)
    return emcore.read(0x08000000, len(data) + 0x800)


def s5l8701decryptdfu(data):
    emcore = libemcore.Emcore()
    emcore.write(0x08000000, data)
    emcore.aesdecrypt(0x08000000, len(data), 1)
    return emcore.read(0x08000800, len(data) - 0x800)


def s5l8701cryptfirmware(data):
    data = data.ljust((len(data) + 0x3f) & ~0x3f, "\0")
    header = "\0\0\0\0\x02\0\0\0\x01\0\0\0\x40\0\0\0\0\0\0\0" + struct.pack("<I", len(data))
    emcore = libemcore.Emcore()
    emcore.write(0x08000000, header.ljust(0x800, "\0") + data)
    emcore.hmac_sha1(0x08000800, len(data), 0x0800001c)
    emcore.hmac_sha1(0x08000000, 0x200, 0x080001d4)
    emcore.aesencrypt(0x08000800, len(data), 1)
    return emcore.read(0x08000000, len(data) + 0x800)


def s5l8701decryptfirmware(data):
    emcore = libemcore.Emcore()
    emcore.write(0x08000000, data)
    emcore.aesdecrypt(0x08000800, len(data) - 0x800, 1)
    return emcore.read(0x08000800, len(data) - 0x800)


def s5l8702cryptnor(data):
    data = data.ljust((len(data) + 0xf) & ~0xf, "\0")
    header = "87021.0\x01\0\0\0\0" + struct.pack("<I", len(data)) + hashlib.sha1(data).digest()[:0x10]
    emcore = libemcore.Emcore()
    emcore.write(0x08000000, header.ljust(0x800, "\0") + data)
    emcore.aesencrypt(0x08000800, len(data), 2)
    emcore.aesencrypt(0x08000010, 0x10, 2)
    emcore.write(0x08000040, hashlib.sha1(emcore.read(0x08000000, 0x40)).digest()[:0x10])
    emcore.aesencrypt(0x08000040, 0x10, 2)
    return emcore.read(0x08000000, len(data) + 0x800)


def s5l8702decryptnor(data):
    emcore = libemcore.Emcore()
    emcore.write(0x08000000, data[0x800:])
    emcore.aesdecrypt(0x08000000, len(data) - 0x800, 1)
    return emcore.read(0x08000000, len(data) - 0x800)


def s5l8702genpwnage(data):
    cert = open(os.path.dirname(__file__) + "/libipodcrypto/s5l8702pwnage.cer", "rb").read()
    data = data.ljust(max(0x840, (len(data) + 0xf) & ~0xf), "\0")
    header = ("87021.0\x03\0\0\0\0" + struct.pack("<IIII", len(data) - 0x830, len(data) - 0x4f6, len(data) - 0x7b0, 0x2ba)).ljust(0x40, "\0")
    emcore = libemcore.Emcore()
    emcore.write(0x08000000, header + hashlib.sha1(header).digest()[:0x10])
    emcore.aesencrypt(0x08000040, 0x10, 1)
    return emcore.read(0x08000000, 0x50) + data + cert.ljust((len(cert) + 0xf) & ~0xf, "\0")


def s5l8701cryptdfufile(infile, outfile):
    infile = open(infile, "rb")
    outfile = open(outfile, "wb")
    outfile.write(s5l8701cryptdfu(infile.read()))
    infile.close()
    outfile.close()


def s5l8701decryptdfufile(infile, outfile):
    infile = open(infile, "rb")
    outfile = open(outfile, "wb")
    outfile.write(s5l8701decryptdfu(infile.read()))
    infile.close()
    outfile.close()


def s5l8701cryptfirmwarefile(infile, outfile):
    infile = open(infile, "rb")
    outfile = open(outfile, "wb")
    outfile.write(s5l8701cryptfirmware(infile.read()))
    infile.close()
    outfile.close()


def s5l8701decryptfirmwarefile(infile, outfile):
    infile = open(infile, "rb")
    outfile = open(outfile, "wb")
    outfile.write(s5l8701decryptfirmware(infile.read()))
    infile.close()
    outfile.close()


def s5l8702cryptnorfile(infile, outfile):
    infile = open(infile, "rb")
    outfile = open(outfile, "wb")
    outfile.write(s5l8702cryptnor(infile.read()))
    infile.close()
    outfile.close()


def s5l8702decryptnorfile(infile, outfile):
    infile = open(infile, "rb")
    outfile = open(outfile, "wb")
    outfile.write(s5l8702decryptnor(infile.read()))
    infile.close()
    outfile.close()


def s5l8702genpwnagefile(infile, outfile):
    infile = open(infile, "rb")
    outfile = open(outfile, "wb")
    outfile.write(s5l8702genpwnage(infile.read()))
    infile.close()
    outfile.close()
