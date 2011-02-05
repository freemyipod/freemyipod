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
import libemcorebootcfg
from optparse import *

parser = OptionParser("usage: %prog [options] <emcorebin> <emcoreapp> <outfile>")
(options, args) = parser.parse_args()
if len(args) != 3: parser.error("incorrect number of arguments")

file = open(args[0], "rb")
data = file.read()
file.close()

file = open(args[1], "rb")
app = file.read()
file.close()

data = libemcorebootcfg.configure(data, (1, app, None, None))

file = open(args[2], "wb")
file.write(data)
file.close()
