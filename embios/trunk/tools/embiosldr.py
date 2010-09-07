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
import time
import libembiosldr


def usage():
  print ""
  print "Please provide a command and (if needed) parameters as command line arguments"
  print ""
  print "Available commands:"
  print ""
  print "  upload <address> <file>"
  print "    Uploads the specified file to the specified memory address on the device."
  print ""
  print "  download <address> <size> <file>"
  print "    Downloads <size> bytes of data from the specified address on the device,"
  print "    and stores it in the specified file."
  print ""
  print "  execute <address> <stack>"
  print "    Executes code at the specified address in the device's memory."
  print "    The stack pointer will be set to <stack> before jumping to <address>."
  print "    iBugger will probably lose control of the device,"
  print "    if the code isn't explicitly written for it."
  print ""
  print "  run <file>"
  print "    Loads the specified file to 0x08000000 (SDRAM) and executes it."
  print "    This is what you usually want to do."
  print ""
  print "All numbers are hexadecimal!"
  exit(2)


def parsecommand(dev, argv):
  if len(argv) < 2: usage()

  elif argv[1] == "upload":
    if len(argv) != 4: usage()
    dev.upload(int(argv[2], 16), argv[3])

  elif argv[1] == "download":
    if len(argv) != 5: usage()
    dev.download(int(argv[2], 16), int(argv[3], 16), argv[4])

  elif argv[1] == "execute":
    if len(argv) != 4: usage()
    dev.execute(int(argv[2], 16), int(argv[3], 16))

  elif argv[1] == "run":
    if len(argv) != 3: usage()
    dev.run(argv[2])

  else: usage()


dev = libembiosldr.embiosldr()
parsecommand(dev, sys.argv)
