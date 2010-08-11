#!/usr/bin/env python
#
#
#    Copyright 2010 TheSeven, benedikt93
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

# note: handles commands 1 to 20

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
  print ""
  print "  uploadfile <offset> <file> <usedma> <freezescheduler>"
  print "    Uploads a file to the iPod"
  print "      <offset>: the address to upload the file to"
  print "      <file>: the path to the file"
  print "      <usedma>: if 0, DMA will not be used when uploading the file,"
  print "          otherwise it will be used. It can be omitted, default is then 1"
  print "      <freezescheduler>: if not 0, the scheduler will be frozen during DMA access"
  print "          to prevent non-consistent data after the transfer."
  print "          It can be omitted, default is then 0"
  print ""
  print "  downloadfile <offset> <size> <file> <usedma> <freezescheduler>"
  print "    Uploads a file to the iPod"
  print "      <offset>: the address to upload the file to"
  print "      <size>: the number of bytes to be read"
  print "      <file>: the path to the file"
  print "      <usedma>: if 0, DMA will not be used when downloading the file,"
  print "          otherwise it will be used. It can be omitted, default is then 1"
  print "      <freezescheduler>: if not 0, the scheduler will be frozen during DMA access"
  print "          to prevent non-consistent data after the transfer"
  print "          It can be omitted, default is then 0"
  print ""
  print "  uploadint <offset> <data>"
  print "    Uploads a single integer to the iPod"
  print "      <offset>: the address to upload the integer to"
  print "      <data>: the integer to upload"
  print ""
  print "  downloadint <offset>"
  print "    Downloads a single integer from the iPod and prints it to the console window"
  print "      <offset>: the address to download the integer from"
  print ""
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
  print "      <db1> ... <dbN> the data in single bytes, seperated by whitespaces,"
  print "                      eg. 0x37 0x56 0x45 0x12"
  print ""
  print ""
  print "  readusbconsole <size> <outtype> <file>"
  print "    Reads data from the USB console."
  print "      <size>: the number of bytes to read"
  print "      <outtype>: defines how to output the result:"
  print "        'file': writes the result to file <file>"
  print "        'printstring': writes the result as string to the console window"
  print "        'printhex': writes the result in hexedit notation to the console window"
  print "      <file>: the file to write the result to, can be omitted"
  print "               if <outtype> is not 'file'"
  print ""
  print "  writeusbconsole file <file> <offset> <length>"
  print "    Writes the file <file> to the USB console."
  print "      Optional params <offset> <length>: specify the range in <file> to write"
  print "  writeusbconsole direct <i1> <i2> ... <iN>"
  print "    Writes the integers <i1> ... <iN> to the USB console."
  print ""
  print "  readdevconsole <bitmask> <size> <outtype> <file>"
  print "    Reads data from one or more of the device's consoles."
  print "      <bitmask>: the bitmask of the consoles to read from"
  print "      <size>: the number of bytes to read"
  print "      <outtype>: defines how to output the result:"
  print "        'file': writes the result to file <file>"
  print "        'printstring': writes the result as string to the console window"
  print "        'printhex': writes the result in hexedit notation to the console window"
  print "      <file>: the file to write the result to, can be omitted"
  print "               if <outtype> is not 'file'"
  print ""
  print "  writedevconsole file <bitmask> <file> <offset> <length>"
  print "    Writes the file <file> to the device consoles specified by <bitmask>"
  print "      Optional params <offset> <length>: specify the range in <file> to write"
  print "  writedevconsole direct <bitmask> <i1> <i2> ... <iN>"
  print "    Writes the integers <i1> ... <iN> to the device consoles specified"
  print "                         by <bitmask>"
  print ""
  print "  flushconsolebuffers <bitmask>"
  print "    flushes one or more of the device consoles' buffers."
  print "      <bitmask>: the bitmask of the consoles to be flushed"
  print ""
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
  print "  createthread <namepointer> <entrypoint> <stackpointer> <stacksize> <type> <priority> <state>"
  print "    Creates a new thread and returns its thread ID"
  print "      <namepointer> a pointer to the thread's name"
  print "      <entrypoint> a pointer to the entrypoint of the thread"
  print "      <stackpointer> a pointer to the stack of the thread"
  print "      <stacksize> the size of the thread's stack"
  print "      <type> the thread type, vaild are: 0 => user thread, 1 => system thread"
  print "      <priority> the priority of the thread, from 1 to 255"
  print "      <state> the thread's initial state, valid are: 1 => ready, 0 => suspended"
  print ""
  print "  execimage <offset>"
  print "    Executes the emBIOS executable image at <offset>."
  print ""
  print ""
  print "  readrawbootflash <addr_bootflsh> <addr_mem> <size>"
  print "    Reads <size> bytes from bootflash to memory."
  print "      <addr_bootflsh>: the address in bootflash to read from"
  print "      <addr_mem>: the address in memory to copy the data to"
  print ""
  print "  writerawbootflash <addr_mem> <addr_bootflsh> <size>"
  print "    Writes <size> bytes from memory to bootflash."
  print "    Don't call this unless you really know what you're doing."
  print "      <addr_mem>: the address in memory to copy the data from"
  print "      <addr_bootflsh>: the address in bootflash to write to"
  print ""
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
    dev.reset(int(argv[2]), 16)
  
  elif argv[1] == "poweroff":
    if len(argv) != 3: usage()
    dev.poweroff(int(argv[2]), 16)

    
  elif argv[1] == "uploadfile":
    if len(argv) < 4 or len(argv) > 6: usage()
    if len(argv) > 4:
      usedma = int(argv[4], 16)
      if len(argv) > 5:
        freezesched = int(argv[5], 16)
      else:
        freezesched = 0
    else:
      freezesched = 0
      usedma = 1
    dev.uploadfile(int(argv[2], 16), argv[3], usedma, freezesched)
  
  elif argv[1] == "downloadfile":
    if len(argv) < 5 or len(argv) > 7: usage()
    if len(argv) > 5:
      usedma = int(argv[5], 16)
      if len(argv) > 6:
        freezesched = int(argv[6], 16)
      else:
        freezesched = 0
    else:
      freezesched = 0
      usedma = 1
    dev.downloadfile(int(argv[2], 16), int(argv[3], 16), argv[4], usedma, freezesched)
  
  elif argv[1] == "uploadint":
    if len(argv) != 4: usage()
    dev.uploadint(int(argv[2], 16), int(argv[3], 16))
  
  elif argv[1] == "downloadint":
    if len(argv) != 3: usage()
    dev.downloadint(int(argv[2], 16))
  
  
  elif argv[1] == "i2cread":
    if len(argv) != 6: usage()
    dev.i2crecv(int(argv[2], 16), int(argv[3], 16), int(argv[4], 16), int(argv[5], 16))
    
  elif argv[1] == "i2csend":
    if len(argv) < 6: usage()
    data = ""
    ptr = 5
    while ptr < len(argv):
      data += struct.pack("<B", int(argv[ptr], 16))
      ptr += 1
    dev.i2csend(int(argv[2], 16), int(argv[3], 16), int(argv[4], 16), data)
  
  
  elif argv[1] == "readusbconsole":
    if len(argv) not in [4, 5]: usage()
    if len(argv) == 4: argv[4] = ""
    dev.readusbcon(int(argv[2], 16), argv[3], argv[4])
  
  elif argv[1] == "writeusbconsole":
    if len(argv) < 4: usage()
    
    if argv[2] == "file":
      f = open(argv[3], "rb")
      data = f.read()
      
      if len(argv) > 4:
        offset = int(argv[4], 16)
      else:
        offset = 0
      if len(argv) > 5:
        size = int(argv[5], 16)
      else:
        size = len(data)
      if len(argv) > 6: usage()
      
      dev.writeusbcon(data, 0, offset, size)
      
    if argv[2] == "direct":
      data = ""
      ptr = 3
      while ptr < len(argv):
        data += struct.pack("<I", int(argv[ptr], 16))
        ptr += 1
      dev.writeusbcon(data)
  
  elif argv[1] == "readdevconsole":
    if len(argv) not in [5, 6]: usage()
    if len(argv) == 5: argv[5] = ""
    dev.readusbcon(int(argv[2], 16), int(argv[3], 16), argv[4], argv[5])
  
  elif argv[1] == "writedevconsole":
    if len(argv) < 5: usage()
    
    if argv[2] == "file":
      f = open(argv[4], "rb")
      data = f.read()
      
      if len(argv) > 5:
        offset = int(argv[5], 16)
      else:
        offset = 0
      if len(argv) > 6:
        size = int(argv[6], 16)
      else:
        size = len(data)
      if len(argv) > 7: usage()
      
      dev.writeusbcon(int(argv[3], 16), data, 0, offset, size)
      
    if argv[2] == "direct":
      data = ""
      ptr = 4
      while ptr < len(argv):
        data += struct.pack("<I", int(argv[ptr], 16))
        ptr += 1
      dev.writeusbcon(int(argv[3], 16), data)
  
  elif argv[1] == "flushconsolebuffers":
    if len(argv) != 3: usage()
    dev.flushconsolebuffers(int(argv[2], 16))
   
   
  elif argv[1] == "getprocessinformation" or argv[1] == "getprocinfo":
    if len(argv) != 4: usage()
    dev.getprocinfo(int(argv[2], 16), int(argv[3], 16))
    
  elif argv[1] == "lockscheduler":
    if len(argv) != 2: usage()
    dev.freezescheduler(1)
    
  elif argv[1] == "unlockscheduler":
    if len(argv) != 2: usage()
    dev.freezescheduler(0)
    
  elif argv[1] == "suspendthread":
    if len(argv) != 3: usage()
    dev.suspendthread(1, int(argv[2], 16))
    
  elif argv[1] == "resumethread":
    if len(argv) != 3: usage()
    dev.suspendthread(0, int(argv[2], 16))
    
  elif argv[1] == "killthread":
    if len(argv) != 3: usage()
    dev.killthread(int(argv[2], 16))
    
  elif argv[1] == "createthread":
    if len(argv) != 9: usage()
    dev.createthread(int(argv[2], 16), int(argv[3], 16), int(argv[4], 16), int(argv[5], 16), int(argv[6], 16), int(argv[7], 16), int(argv[8], 16))
    
  elif argv[1] == "execimage":
    if len(argv) != 3: usage()
    dev.execimage(int(argv[2], 16))
    
    
  elif argv[1] == "readrawbootflash":
    if len(argv) != 5: usage()
    dev.readrawbootflash(int(argv[2]), int(argv[3]), int(argv[4]))
    
  elif argv[1] == "writerawbootflash":
    if len(argv) != 5: usage()
    dev.writerawbootflash(int(argv[2]), int(argv[3]), int(argv[4]))
    
  elif argv[1] == "flushcaches":
    if len(argv) != 2: usage()
    dev.flushcaches()
    
  else: usage()

dev = libembios.embios()
parsecommand(dev, sys.argv)
