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

if len(sys.argv) < 3:
  print "Syntax: concatenate.py <outfile> <infile1> [infile2 [...]]"
  exit(2)

data = ""

for i in range(2, len(sys.argv)):
  print("%08X: %s" % (len(data), sys.argv[i]))
  data = data + file(sys.argv[i], "rb").read()

file(sys.argv[1], "wb").write(data)
