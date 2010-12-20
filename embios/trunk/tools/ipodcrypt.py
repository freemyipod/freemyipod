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
  print "  s5l8701-cryptdfu <infile> <outfile>"
  print "  s5l8701-decryptdfu <infile> <outfile>"
  print "  s5l8701-cryptfirmware <infile> <outfile>"
  print "  s5l8701-decryptfirmware <infile> <outfile>"
  print "  s5l8702-cryptnor <infile> <outfile>"
  print "  s5l8702-decryptnor <infile> <outfile>"
  print "  s5l8702-genpwnage <infile> <outfile>"
  exit(2)


def parsecommand(argv):
  if len(argv) != 4: usage()

  elif argv[1] == "s5l8701-cryptdfu":
    libipodcrypto.s5l8701cryptdfufile(argv[2], argv[3])

  elif argv[1] == "s5l8701-decryptdfu":
    libipodcrypto.s5l8701decryptdfufile(argv[2], argv[3])

  elif argv[1] == "s5l8701-cryptfirmware":
    libipodcrypto.s5l8701cryptfirmwarefile(argv[2], argv[3])

  elif argv[1] == "s5l8701-decryptfirmware":
    libipodcrypto.s5l8701decryptfirmwarefile(argv[2], argv[3])

  elif argv[1] == "s5l8702-cryptnor":
    libipodcrypto.s5l8702cryptnorfile(argv[2], argv[3])

  elif argv[1] == "s5l8702-decryptnor":
    libipodcrypto.s5l8702decryptnorfile(argv[2], argv[3])

  elif argv[1] == "s5l8702-genpwnage":
    libipodcrypto.s5l8702genpwnagefile(argv[2], argv[3])

  else: usage()


parsecommand(sys.argv)
