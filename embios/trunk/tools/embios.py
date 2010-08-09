#
#
#    Copyright 2010 benedikt93, partially derived from TheSeven's ipod tools
#
#
#    This file is part of the freemyipod.org iPod tools.
#
#    FreeMyIPods' emBIOS and related tools are free software: you can redistribute it and/or
#    modify it under the terms of the GNU General Public License as
#    published by the Free Software Foundation, either version 2 of the
#    License, or (at your option) any later version.
#
#    FreeMyIPods' emBIOS and related tools are distributed in the hope that they will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
#    See the GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License along
#    with FreeMyIPods' emBIOS and related tools.  If not, see <http://www.gnu.org/licenses/>.
#
#


import sys
import time
import libembios


def usage():
  print ""
  print "Please provide a command and (if needed) parameters as command line arguments"
  print ""
  print "Available commands:"
  print ""
  print "  getinfo <infotype>"
  print "    Get info on the running emBIOS."
  print "    <infotype> may be either off 'version', 'packetsize', 'usermemrange'."
  print ""
  print "  reset <force>"
  print "    Resets the device"
  print "    If <force> is 1, the reset will be forced, otherwise it will be gracefully,"
  print "    which may take some time."
  print ""
  print "  poweroff <force>"
  print "    Powers the device off"
  print "    If <force> is 1, the poweroff will be forced, otherwise it will be gracefully,"
  print "    which may take some time."
  print ""
  print "  i2crecv <bus> <slave> <addr> <size>"
  print "    Reads data from an I2C device"
  print "      <bus> the bus index"
  print "      <slave> the slave address"
  print "      <addr> the start address on the I2C device"
  print "      <size> the number of bytes to read"
  print ""
  print "  i2csend <bus> <slave> <addr> <db1> <db2> ... <dbN>"
  print "    Writes data to an I2C device"
  print "      <bus> the bus index"
  print "      <slave> the slave address"
  print "      <addr> the start address on the I2C device"
  print "      <db1> ... <dbN> the data in single bytes, seperated by whitespaces, eg. 0x37 0x56 0x45 0x12"
  print ""
  print "  getprocessinformation <offset> <size> / getprocinfo <offset> <size>"
  print "    Fetches data on the currently running processes"
  print "      <offset> the offset in the data field"
  print "      <size> the number of bytes to be fetched"
  print "     ATTENTION: this function will be print the information to the console window."
  print "                If several threads are running this might overflow the window,"
  print "                causing not everything to be shown."
  print ""
  print "  lockscheduler"
  print "    Locks (freezes) the scheduler"
  print ""
  print "  unlockscheduler"
  print "    Unlocks the scheduler"
  print ""
  print "  suspendthread <threadid>"
  print "    Suspends/resumes the thread with thread ID <threadid>"
  print ""
  print "  resumethread <threadid>"
  print "    Resumes the thread with thread ID <threadid>"
  print ""
  print "  killthread <threadid>"
  print "    Kills the thread with thread ID <threadid>"
  print ""
  print "  createthread <namepointer> <entrypoint> <stackpointer> <stacksize>, <type> <priority> <state>"
  print "    Creates a new thread and returns its thread ID"
  print "      <namepointer> a pointer to the thread's name"
  print "      <entrypoint> a pointer to the entrypoint of the thread"
  print "      <stackpointer> a pointer to the stack of the thread"
  print "      <stacksize> the size of the thread's stack"
  print "      <type> the thread type, vaild are: 0 => user thread, 1 => system thread"
  print "      <priority> the priority of the thread, from 1 to 255"
  print "      <state> the thread's initial state, valid are: 1 => ready, 0 => suspended"
  print ""
  print "  flushcaches"
  print "    Flushes the CPUs data and instruction caches."
  print ""
  print "All numbers are hexadecimal!"
  exit(2)


def parsecommand(dev, argv):
  if len(argv) < 2: usage()

  elif argv[1] == "getinfo":
    if len(argv) != 3: usage()
    dev.getinfo(argv[2])
  
  elif argv[1] == "reset":
    if len(argv) != 3: usage()
    dev.reset(int(argv[2]))
  
  elif argv[1] == "poweroff":
    if len(argv) != 3: usage()
    dev.poweroff(int(argv[2]))

  elif argv[1] == "i2cread":
    if len(argv) != 6: usage()
    dev.i2crecv(int(argv[2]), int(argv[3]), int(argv[4]), int(argv[5]))
    
  elif argv[1] == "i2csend":
    if len(argv) < 6: usage()
    data = ""
    ptr = 5
    while ptr < lean(argv):
      data += struct.pack("<B", int(argv[ptr]))
      ptr += 1
    dev.i2csend(int(argv[2]), int(argv[3]), int(argv[4]), data)
    
  elif argv[1] == "getprocessinformation" or argv[1] == "getprocinfo":
    if len(argv) != 4: usage()
    dev.getprocinfo(int(argv[2]), int(argv[3]))
    
  elif argv[1] == "lockscheduler":
    if len(argv) != 2: usage()
    dev.freezescheduler(1)
    
  elif argv[1] == "unlockscheduler":
    if len(argv) != 2: usage()
    dev.freezescheduler(0)
    
  elif argv[1] == "suspendthread":
    if len(argv) != 3: usage()
    dev.suspendthread(1, int(argv[2]))
    
  elif argv[1] == "resumethread":
    if len(argv) != 3: usage()
    dev.suspendthread(0, int(argv[2]))
    
  elif argv[1] == "killthread":
    if len(argv) != 3: usage()
    dev.killthread(int(argv[2]))
    
  elif argv[1] == "createthread":
    if len(argv) != 9: usage()
    dev.createthread(int(argv[2]), int(argv[3]), int(argv[4]), int(argv[5]), int(argv[6]), int(argv[7]), int(argv[8]))
    
  elif argv[1] == "flushcaches":
    if len(argv) != 2: usage()
    dev.flushcaches()
    
  else: usage()


dev = libembios.embios()
parsecommand(dev, sys.argv)
