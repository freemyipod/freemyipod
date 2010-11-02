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
import libembiosbootcfg
from optparse import *

parser = OptionParser("usage: %prog [options] <embiosbin> <embiosapp> <outfile>")
parser.add_option("--run-from", type = "int", metavar = "ADDR",
                  help = "Ensures that the app is executed from memory address ADDR")
parser.add_option("--compressed", action = "store_true", default = False,
                  help = "Specify this if the executable is compressed")
(options, args) = parser.parse_args()
if len(args) != 3: parser.error("incorrect number of arguments")

file = open(args[0], "rb")
data = file.read()
file.close()

file = open(args[1], "rb")
app = file.read()
file.close()

config = {"reset": True, "trymmap": True}
config["mmapaddr"] = 0x08000000 + len(data)
config["mmapsize"] = len(app)
if options.compressed: config["mmapcomp"] = True
if options.run_from:
    config["mmapcopy"] = True
    config["mmapdest"] = options.run_from

data = libembiosbootcfg.configure(data, **config)

file = open(args[2], "wb")
file.write(data + app)
file.close()
