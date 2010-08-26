#!/usr/bin/env python
#
#
#    Copyright 2010 TheSeven
#
#
#    This file is part of TheSeven's iPod tools.
#
#    TheSeven's iBugger is free software: you can redistribute it and/or
#    modify it under the terms of the GNU General Public License as
#    published by the Free Software Foundation, either version 2 of the
#    License, or (at your option) any later version.
#
#    TheSeven's iBugger is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
#    See the GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License along
#    with TheSeven's iPod tools.  If not, see <http://www.gnu.org/licenses/>.
#
#


import sys
import struct

from optparse import *

parser = OptionParser("usage: %prog [options] <infile> <outfile>")
parser.add_option("--signature", metavar = "SIGN",
                  help = "The device signature. Must be 4 characters, e.g. \"nn2x\". (mandantory)")
parser.add_option("--targetid", type = "int", metavar = "ID",
                  help = "The numeric target ID. (mandantory)")
(options, args) = parser.parse_args()
if len(args) != 2: parser.error("incorrect number of arguments")
if not options.signature: parser.error("please specify a device signature")
if not options.targetid: parser.error("please specify numeric target id")
if len(options.signature) != 4: parser.error("device signature must be 4 characters")

file = open(args[0], "rb")
data = file.read()
file.close()

checksum = options.targetid
for i in range(len(data)):
  checksum = (checksum  + struct.unpack("B", data[i])[0]) & 0xffffffff

file = open(args[1], "wb")
file.write(struct.pack(">I", checksum) + options.signature + data)
file.close()
