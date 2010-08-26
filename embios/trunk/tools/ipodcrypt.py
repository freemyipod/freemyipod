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
import libipodcrypto


def usage():
  print ""
  print "Please provide a command and (if needed) parameters as command line arguments"
  print ""
  print "Available commands:"
  print "  nano2g-cryptdfu <infile> <outfile>"
  print "  nano2g-decryptdfu <infile> <outfile>"
  print "  nano2g-cryptfirmware <infile> <outfile>"
  print "  nano2g-decryptfirmware <infile> <outfile>"
  exit(2)


def parsecommand(argv):
  if len(argv) != 4: usage()

  elif argv[1] == "nano2g-cryptdfu":
    libipodcrypto.nano2gcryptdfufile(argv[2], argv[3])

  elif argv[1] == "nano2g-decryptdfu":
    libipodcrypto.nano2gdecryptdfufile(argv[2], argv[3])

  elif argv[1] == "nano2g-cryptfirmware":
    libipodcrypto.nano2gcryptfirmwarefile(argv[2], argv[3])

  elif argv[1] == "nano2g-decryptfirmware":
    libipodcrypto.nano2gdecryptfirmwarefile(argv[2], argv[3])

  else: usage()


parsecommand(sys.argv)
