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
#    You should have received a copy of the GNU General Public License along
#    with emCORE.  If not, see <http://www.gnu.org/licenses/>.
#
#


import sys
import struct

file = open(sys.argv[1], "rb")
stub = file.read()
file.close()

file = open(sys.argv[2], "rb")
payload = file.read()
file.close()

file = open(sys.argv[3], "wb")
file.write(stub + struct.pack("<I", len(payload)) + payload)
file.close()
