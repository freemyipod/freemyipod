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
import math
import struct
import time
import usb


class embios:
  def __init__(self, devtype = 0, type = 0):
    busses = usb.busses()
 
    for bus in busses:
      devices = bus.devices
      for dev in devices:
        if dev.idVendor == 0xffff and dev.idProduct == 0xe000:
          # get endpoints
          self.__coutep = dev.configurations[0].interfaces[0][0].endpoints[0].address
          self.__cinep = dev.configurations[0].interfaces[0][0].endpoints[1].address
          self.__doutep = dev.configurations[0].interfaces[0][0].endpoints[2].address
          self.__dinep = dev.configurations[0].interfaces[0][0].endpoints[3].address
          
          handle = dev.open()
          handle.setConfiguration(1)
          handle.claimInterface(0)
          
          # get version info
          handle.bulkWrite(self.__coutep, struct.pack("<IIII", 1, 0, 0, 0))
          response = self.__getbulk(handle, self.__cinep, 0x10)
          self.__checkstatus(response)
          i = struct.unpack("<IIBBBBI", response)
          
          if devtype in [0, i[6]] and type in [0, i[5]]:
            # correct device
            self.handle = handle
            self.dev = dev
            
            self.svnrev, self.major, self.minor, self.patch, self.type, self.devtype = i[1:]
            self.__myprint("Connected to emBIOS %s v%d.%d.%d (SVN revision: %d) on %s, USB version %s\n" \
                  % (self.type2name(self.type), self.major, self.minor, self.patch, self.svnrev, \
                     self.devtype2name(self.devtype), dev.deviceVersion))
                     
            # get packet size info
            self.getinfo("packetsize", 1)
            
            return
          
          # wrong device
          handle.releaseInterface()

    raise Exception("Could not find specified device (devtype = %d, type = %d)" % (devtype, type))


#=====================================================================================    
    
    
  @staticmethod
  def __myprint(data, silent = 0):
    if not silent:
      sys.stdout.write(data)
      sys.stdout.flush()

      
  @staticmethod
  def __gethexviewprintout(data, title, showaddr):
    printout_temp = struct.unpack("%dB" % (len(data)), data)
      
    printout = title + ":\n"
    pointer = 0
    pointer2 = 0
    
    while (pointer < len(printout_temp)):
      pointer2 = 0
      if (showaddr): printout += "0x%08x     " % (pointer)
      while (pointer2 < 0x10) and (pointer < len(printout_temp)):
        printout += ("%2x " % (printout_temp[pointer]))
        pointer += 1
        pointer2 += 1
      printout += "\n"
        
    if (pointer2 != 0x10):
      printout += "\n"
      
    return printout

    
  @staticmethod
  def __getbulk(handle, endpoint, size):
    data = handle.bulkRead(endpoint, size, 1000)
    return struct.pack("%dB" % len(data), *data)


  @staticmethod
  def __checkstatus(data):
    errorcode = struct.unpack("<I", data[:4])[0]
    if errorcode == 1:
      # everything went fine
      return
    elif errorcode == 2:
      print("\nError: Device doesn't support this function!")
      raise Exception("emBIOS device doesn't support this function!")
    elif errorcode == 3:
      print("\nError: Device is busy!")
      raise Exception("emBIOS device is busy!")
    else:
      print("\nUnknown error %d" % errorcode)
      raise Exception("Unknown emBIOS error %d" % errorcode)


  @staticmethod
  def type2name(type):
    if type == 1: return "Debugger"
    else: return "UNKNOWN (0x%08x)" % type


  @staticmethod
  def devtype2name(devtype):
    if devtype == 0x47324e49: return "iPod Nano 2G"
    if devtype == 0x47334e49: return "iPod Nano 3G"
    if devtype == 0x47344e49: return "iPod Nano 4G"
    if devtype == 0x4c435049: return "iPod Classic"
    else: return "UNKNOWN (0x%08x)" % devtype


#=====================================================================================
    
    
  def getinfo (self, infotype, silent = 0):
    if (infotype == "version"):
      self.handle.bulkWrite(self.__coutep, struct.pack("<IIII", 1, 0, 0, 0))
      response = self.__getbulk(self.handle, self.__cinep, 0x10)
      self.__checkstatus(response)
      
      i = struct.unpack("<IIBBBBI", response)
      
      self.svnrev, self.major, self.minor, self.patch, self.type, self.devtype = i[1:]
      
      self.__myprint("emBIOS %s v%d.%d.%d (SVN revision: %d) on %s, USB version %s\n" \
              % (self.type2name(self.type), self.major, self.minor, self.patch, self.svnrev, \
                 self.devtype2name(self.devtype), self.dev.deviceVersion)\
            , silent)
      
    elif (infotype == "packetsize"):
      self.handle.bulkWrite(self.__coutep, struct.pack("<IIII", 1, 1, 0, 0))
      response = self.__getbulk(self.handle, self.__cinep, 0x10)
      self.__checkstatus(response)
      
      i = struct.unpack("<IHHII", response)
      
      self.cout_maxsize = i[1]
      self.cin_maxsize = i[2]
      self.dout_maxsize = i[3]
      self.din_maxsize = i[4]
      
      self.__myprint("Maximum packet sizes:\n     Command out: %d bytes\n     Command in: %d bytes\n     Data out: %d bytes\n     Data in: %d bytes\n" \
              % (self.cout_maxsize, self.cin_maxsize, self.dout_maxsize, self.din_maxsize)
            , silent)
    
    elif (infotype == "usermemrange"):
      self.handle.bulkWrite(self.__coutep, struct.pack("<IIII", 1, 1, 0, 0))
      response = self.__getbulk(self.handle, self.__cinep, 0x10)
      self.__checkstatus(response)
      
      i = struct.unpack("<IIII", response)
      
      self.usermem_lower = i[1]
      self.usermem_upper = i[2]
      
      self.__myprint("User memory range:\n     Lower bound (inclusive): %x\n     Upper bound (exclusive) %x\n" \
              % (self.usermem_lower, self.usermem_upper)
            , silent)
      
    else:
      self.__myprint("Unsupported type of info: %d" % (infotype))
    
  
  def reset(self, force, silent = 0):
    """ Resets the device.
      <force>: if 0, the reset will be gracefully, otherwise it will be forced.
      <silent>: if not 0, nothing will be printed to the console window
    """
    if (force == 0):
      force = 1
    else:
      force = 0
  
    self.__myprint("Resetting device...", silent)
    self.handle.bulkWrite(self.__coutep, struct.pack("<IIII", 2, force, 0, 0))
    
    if (force == 1):
      # reset not forced
      response = self.__getbulk(self.handle, self.__cinep, 0x10)
      self.__checkstatus(response)
    
    self.__myprint(" done\n", silent)  
  

  def poweroff(self, force, silent = 0):
    """ Powers the device off.
      <force>: if 0, the poweroff will be gracefully, otherwise it will be forced.
      <silent>: if not 0, nothing will be printed to the console window
    """
    if (force == 0):
      force = 1
    else:
      force = 0
  
    self.__myprint("Powering device off...", silent)
    self.handle.bulkWrite(self.__coutep, struct.pack("<IIII", 3, force, 0, 0))
    
    if (force == 1):
      # shutdown not forced
      response = self.__getbulk(self.handle, self.__cinep, 0x10)
      self.__checkstatus(response)
    
    self.__myprint(" done\n", silent)
  
  
#=====================================================================================
  
 
  def write(self, offset, data, usedma, freezesched, *range):
    boffset = 0
      
    size = len(data)
      
    if len(range) > 0:
      boffset = range[0]
    if len(range) > 1:
      size = range[1]
        
    if (size == 0):
      return
     
    # correct alignment
    while (offset & 0xF) != 0:
      blocklen = (0x10 - offset % 0x10)
      
      if (blocklen > size):
        blocklen = size
      if (blocklen > self.cout_maxsize - 0x10):
        blocklen = self.cout_maxsize
      
      self.handle.bulkWrite(self.__coutep, struct.pack("<IIII", 5, offset, blocklen, 0) + data[boffset:boffset+blocklen])
      response = self.__getbulk(self.handle, self.__cinep, 0x10)
      self.__checkstatus(response)
      
      offset += blocklen
      boffset += blocklen
      size -= blocklen

    # write data with DMA, if it makes sense (-> much data) and isn't forbidden
    if (usedma): 
      if (freezesched):
        self.freezescheduler(1)
    
      while (size > 0x1F):
        blocklen = size
     
        if (blocklen > self.dout_maxsize):
          blocklen = self.dout_maxsize
          
        self.handle.bulkWrite(self.__coutep, struct.pack("<IIII", 7, offset, blocklen, 0))
        response = self.__getbulk(self.handle, self.__cinep, 0x10)
        self.__checkstatus(response)
      
        self.handle.bulkWrite(self.__doutep, data[boffset:boffset+blocklen])
      
        offset += blocklen
        boffset += blocklen
        size -= blocklen
        
      if (freezesched):
        self.freezescheduler(0)
        
    # write rest of data
    while (size > 0):
      blocklen = size
      
      if (blocklen > self.cout_maxsize - 0x10):
        blocklen = self.cout_maxsize
    
      self.handle.bulkWrite(self.__coutep, struct.pack("<IIII", 5, offset, blocklen, 0) + data[boffset:boffset+blocklen])
      response = self.__getbulk(self.handle, self.__cinep, 0x10)
      self.__checkstatus(response)
      
      offset += blocklen
      boffset += blocklen
      size -= blocklen
    

  def read(self, offset, size, usedma, freezesched):
    if (size == 0):
      return
    
    data = ""
    
    # correct alignment
    while (offset & 0xF) != 0:
      blocklen = (0x10 - offset % 0x10)
      
      if (blocklen > size):
        blocklen = size
      if (blocklen > self.cin_maxsize - 0x10):
        blocklen = self.cin_maxsize
      
      self.handle.bulkWrite(self.__coutep, struct.pack("<IIII", 4, offset, blocklen, 0))
      response = self.__getbulk(self.handle, self.__cinep, 0x10 + blocklen)
      self.__checkstatus(response)
      
      data += response[0x10:]
      
      offset += blocklen
      size -= blocklen

    # read data with DMA, if it makes sense (-> much data) and isn't forbidden
    if (usedma):
      if (freezesched):
        self.freezescheduler(1)

      while (size > 0x1F):
        blocklen = size
      
        if (blocklen > self.din_maxsize):
          blocklen = self.din_maxsize
          
        self.handle.bulkWrite(self.__coutep, struct.pack("<IIII", 6, offset, blocklen, 0))
        response = self.__getbulk(self.handle, self.__cinep, 0x10)
        self.__checkstatus(response)
      
        data += self.__getbulk(self.handle, self.__doutep, blocklen)
      
        offset += blocklen
        size -= blocklen
        
      if (freezesched):
        self.freezescheduler(0)
        
    # read rest of data
    while (size > 0):
      blocklen = size
      
      if (blocklen > self.cin_maxsize - 0x10):
        blocklen = self.cin_maxsize
    
      self.handle.bulkWrite(self.__coutep, struct.pack("<IIII", 4, offset, blocklen, 0))
      response = self.__getbulk(self.handle, self.__cinep, 0x10 + blocklen)
      self.__checkstatus(response)
      
      data += response[0x10:]
      
      offset += blocklen
      size -= blocklen
      
    return data

 
  def uploadfile(self, offset, file, usedma = 1, freezesched = 0, silent = 0):
    self.__myprint("Uploading %s to 0x%8x..." % (file, offset), silent)
    f = open(file, "rb")

    while True:
      data = f.read(262144)
      if data == "": break
      self.write(offset, data, usedma, freezesched)
      offset += len(data)
      self.__myprint(".")

    self.__myprint(" done\n", silent) 
    
  
  def downloadfile(self, offset, size, file, usedma = 1, freezesched = 0, silent = 0):
    self.__myprint("Downloading 0x%x bytes from 0x%8x to %s..." % (size, offset, file), silent)
    f = open(file, "wb")

    while True:
      blocklen = size
      if blocklen == 0: break
      if blocklen > 262144: blocklen = 262144
      f.write(self.read(offset, blocklen, usedma, freezesched))
      offset += blocklen
      size -= blocklen
      self.__myprint(".")

    self.__myprint(" done\n", silent)
    
    
  def uploadint(self, offset, data, silent = 0):
    self.__myprint("Uploading 0x%8x to 0x%8x..." % (data, offset), silent)
    self.write(offset, data, 0, 0)
    self.__myprint(" done\n", silent)


  def downloadint(self, offset, silent = 0):
    self.__myprint("Downloading 0x%8x from 0x%8x..." % (data, offset), silent)
    data = self.read(offset, data, 0, 0)
    self.__myprint(" done\nValue was: 0x%8x\n" % (data), silent)
    
    return data
    
    
#=====================================================================================  
    
    
  def i2crecv(self, bus, slave, addr, size, silent = 0):
    if (size > self.cin_maxsize - 0x10) or (size > 0xFF):
      raise Exception ("The data exceeds the maximum amount that can be received with this instruction.")
  
    self.__myprint("Reading 0x%2x bytes from 0x%2x at I2C device at bus 0x%2x, slave adress 0x%2x ..." % (size, addr, bus, slave), silent)
    
    self.handle.bulkWrite(self.__coutep, struct.pack("<IBBBBII", 8, bus, slave, addr, size, 0, 0))
    data = self.__getbulk(self.handle, self.__cinep, 0x10 + size)
    self.__checkstatus(response)
    
    self.__myprint(" done\n data was:\n%s\n" % (self.__gethexviewprintout(data[16:])), silent)
    
    return data[16:]


  def i2csend(self, bus, slave, addr, data, silent = 0):
    size = len(data)
    if (size > self.cout_maxsize - 0x10) or (size > 0xFF):
      raise Exception ("The data exceeds the maximum amount that can be send with this instruction.")
  
    self.__myprint("Writing 0x%2x bytes to 0x%2x at I2C device at bus 0x%2x, slave adress 0x%2x ..." % (size, addr, bus, slave), silent)
  
    self.handle.bulkWrite(self.__coutep, struct.pack("<IBBBBII", 9, bus, slave, addr, size, 0, 0) + data)
    response = self.__getbulk(self.handle, self.__cinep, 0x10)
    self.__checkstatus(response)
    
    self.__myprint(" done\n", silent)
    
    
#=====================================================================================      

  def readusbcon(self, size, outtype = "", file = "", silent = 0):
    """ reads from USB console
      <size>: number of bytes to be read, if its length exceeds the Command In endpoint packet size - 0x10, it will be read in several steps
      <outtype>: how the data will be put out
        "file" => writes data to file <file>
        "printstring" => prints data as a string to the console window
        "printhex" => prints a hexview view of the data to the console window
        "" => only returns the data
      <silent>: if 0, nothing will be written to the console window (even if <outtype> defines something else)
      
      in every case, the data will be returned in an array with additional information
        [len, buffersize, datainbuffer, data]
          where len is the length of the data actually read,
                buffersize is the on-device read buffer size,
                datainbuffer is the number of bytes still left in the on device buffer,
                data is the actual data
      
      in case that within 5 secs, it's not possible to read <size> bytes, a timeout will occur
    """
    out_data = ""
    readbytes = 0
    buffersize = 0
    bytesleft = 0
    timeoutcounter = 0
    
    self.__myprint("Reading 0x%x bytes from USB console..." % (size), silent)
    
    while size > 0 and timoutcounter < 50:
      blocklen = size
      
      if size > self.cin_maxsize - 0x10:
        blocklen = self.cin_maxsize - 0x10
      
      self.handle.bulkWrite(self.__coutep, struct.pack("<IIII", 10, blocklen, 0, 0))
      response = self.__getbulk(self.handle, self.__cinep, blocklen + 0x10)
      self.__checkstatus(response)
      
      readbytes, buffersize, bytesleft = struct.unpack("<III", response[:12])
      out_data += response[0x10:0x10+readbytes]
      size -= blocklen
      
      if not bytesleft > 0:   # no data left to read => wait a bit and prevent an infinite loop trying to read data when there is none
        timeoutcounter += 1
        time.sleep(0.1)
      else:
        timeoutcounter -= 3
        if timeoutcounter < 0:
          timeoutcounter = 0
          
    self.__myprint(" done\n", silent)
    self.__myprint("\nBytes read: 0x%x\nOn-device buffersize: 0x%x\nBytes still in device's buffer: 0x%x\n\n"\
                    % (len(out_data), buffersize, bytesleft)                      
                    , silent)
    
    if (outtype == "file"):
      f = open(file, "wb")
      f.write(out_data)
      
    elif (outtype == "printstring"):
      self.__myprint(out_data, silent)
      self.__myprint("\n\n", silent)
      
    elif (outtype == "printhex"):
      self.__myprint(self.__gethexviewprintout(out_data, "", 1), silent)
      self.__myprint("\n\n", silent)
    
    elif (outtype == ""):
      pass    # return only
      
    else:
      raise Exception ("Invalid argument for <outtype>: '%s'." % (outtype))
    
    return [len(out_data), buffersize, bytesleft, out_data]
    
    
  def writeusbcon(self, data, silent = 0, *range):
    """ writes to USB console
      <data>: the data to be written
      <range>: the range in <data> that should be written, in the from [offset, length]
      <silent>: if 0, nothing will be written to the console window
      
      if the data to be written exceeds the Command Out endpoint packet size - 0x10, it will be written in several steps
    """
    size = len(data)
    boffset = 0
    
    if len(range) > 0:
      boffset = range[0]
    if len(range) > 1:
      size = range[1]
    
    self.__myprint("Writing 0x%x bytes to USB console..." % (size), silent)
    
    timeoutcounter = 0
    
    while (size > 0) and (timeoutcounter < 50):
      blocklen = size
      if blocklen > self.cout_maxsize - 0x10:
        blocklen = self.cout_maxsize - 0x10
    
      self.handle.bulkWrite(self.__coutep, struct.pack("<IIII", 11, size, 0, 0) + data[boffset:boffset+blocklen])
      response = self.__getbulk(self.handle, self.__cinep, 0x10)
      self.__checkstatus(response)
      
      sendbytes = struct.unpack("<I", response[4:8])
      if sendbytes < blocklen:   # not everything has been written, need to resent some stuff but wait a bit before doing so
        time.sleep(0.1)
        timeoutcounter += 1
      elif timeoutcounter > 0:    # lower timeoutcounter again
        timeoutcounter -= 3
        if timeoutcounter < 0:
          timeoutcounter = 0
      
      size -= sendbytes
      boffset += sendbytes

      
    if (timeoutcounter >=50):
      raise Exception("Timeout, 0x%x bytes couldn't be send." % size)
    
    self.__myprint(" done\n", silent)
    
    return size   # number of bytes that have not been sent
    

  def readdevcon(self, bitmask, size, outtype = "", file = "", silent = 0):
    """ reads from one or more of the device's consoles
      <bitmask>: bitmask of consoles to be read from
      <size>: number of bytes to be read, if its length exceeds the Command In endpoint packet size - 0x10, it will be read in several steps
      <outtype>: how the data will be put out
        "file" => writes data to file <file>
        "printstring" => prints data as a string to the console window
        "printhex" => prints a hexview view of the data to the console window
        "" => only returns the data
      <silent>: if 0, nothing will be written to the console window (even if <outtype> defines something else)
      
      in every case, the data will be returned
      
      in case that within 5 secs, it's not possible to read <size> bytes, a timeout will occur
    """
    out_data = ""
    readbytes = 0
    timeoutcounter = 0
    
    self.__myprint("Reading 0x%x bytes from device's console(s)..." % (size), silent)
    
    while size > 0 and timoutcounter < 50:
      blocklen = size
      
      if size > self.cin_maxsize - 0x10:
        blocklen = self.cin_maxsize - 0x10
      
      self.handle.bulkWrite(self.__coutep, struct.pack("<IIII", 13, bitmask, blocklen, 0))
      response = self.__getbulk(self.handle, self.__cinep, blocklen + 0x10)
      self.__checkstatus(response)
      
      readbytes = struct.unpack("<III", response[4:8])
      out_data += response[0x10:0x10+readbytes]
      size -= blocklen
      
      if not readbytes > 0:   # no data read => wait a bit and prevent an infinite loop trying to read data when there is none
        timeoutcounter += 1
        time.sleep(0.1)
      else:
        timeoutcounter -= 3
        if timeoutcounter < 0:
          timeoutcounter = 0
          
    self.__myprint(" done\n", silent)
    self.__myprint("\nBytes read: 0x%x\n\n" % (len(out_data)), silent)
    
    if (outtype == "file"):
      f = open(file, "wb")
      f.write(out_data)
      
    elif (outtype == "printstring"):
      self.__myprint(out_data, silent)
      self.__myprint("\n\n", silent)
      
    elif (outtype == "printhex"):
      self.__myprint(self.__gethexviewprintout(out_data, "", 1), silent)
      self.__myprint("\n\n", silent)
    
    elif (outtype == ""):
      pass    # return only
      
    else:
      raise Exception ("Invalid argument for <outtype>: '%s'." % (outtype))
    
    return out_data
    
    
  def writedevcon(self, bitmask, data, silent = 0, *range):
    """ writes to USB console
      <bitmask>: bitmask of consoles to be written to
      <data>: the data to be written
      <range>: the range in <data> that should be written, in the from [offset, length]
      <silent>: if 0, nothing will be written to the console window
      
      if the data to be written exceeds the Command Out endpoint packet size - 0x10, it will be written in several steps
    """
    size = len(data)
    boffset = 0
    
    if len(range) > 0:
      boffset = range[0]
    if len(range) > 1:
      size = range[1]
    
    self.__myprint("Writing 0x%x bytes to device's console(s)..." % (size), silent)
    
    timeoutcounter = 0
    
    while size > 0:
      blocklen = size
      if blocklen > self.cout_maxsize - 0x10:
        blocklen = self.cout_maxsize - 0x10
    
      self.handle.bulkWrite(self.__coutep, struct.pack("<IIII", 12, bitmask, size, 0) + data[boffset:boffset+blocklen])
      response = self.__getbulk(self.handle, self.__cinep, 0x10)
      self.__checkstatus(response)
      
      size -= blocklen
      boffset += blocklen
    
    self.__myprint(" done\n", silent)
   
       
  def flushconsolebuffers(self, bitmask, silent = 0):
    self.__myprint("Flushing device console('s) buffer('s)...")
    
    self.handle.bulkWrite(self.__coutep, struct.pack("<IIII", 14, bitmask, 0, 0))
    response = self.__getbulk(self.handle, self.__cinep, 0x10)
    self.__checkstatus(response)
    
    self.__myprint(" done\n") 

    
#===================================================================================== 

  
  def freezescheduler(self, freeze, silent = 0):
    if (freeze):
      self.__myprint("Freezing scheduler...", silent)
      freeze = 1
    else:
      self.__myprint("Unfreezing scheduler...", silent)
      freeze = 0
      
    self.handle.bulkWrite(self.__coutep, struct.pack("<IIII", 16, freeze, 0, 0))
    response = self.__getbulk(self.handle, self.__cinep, 0x10)
    self.__checkstatus(response)
    
    self.__myprint(" done\n", silent)    
    
    
  def suspendthread(self, suspend, threadid, silent = 0):
    if (suspend):
      self.__myprint("Suspending thread 0x%8x...", silent) % threadid
      suspend = 1
    else:
      self.__myprint("Unsuspending thread 0x%8x...", silent) % threadid
      suspend = 0
      
    self.handle.bulkWrite(self.__coutep, struct.pack("<IIII", 17, suspend, threadid, 0))
    response = self.__getbulk(self.handle, self.__cinep, 0x10)
    self.__checkstatus(response)
    
    self.__myprint(" done\n", silent) 


  def killthread(self, threadid, silent = 0):
    self.__myprint("Killing thread 0x%8x...", silent) % threadid
      
    self.handle.bulkWrite(self.__coutep, struct.pack("<IIII", 18, threadid, 0, 0))
    response = self.__getbulk(self.handle, self.__cinep, 0x10)
    self.__checkstatus(response)
    
    self.__myprint(" done\n", silent) 
    
    
  def createthread(self, namepointer, entrypoint, stackpointer, stacksize, type, priority, state, silent = 0):
    self.__myprint("Creating thread...", silent)
      
    self.handle.bulkWrite(self.__coutep, struct.pack("<IIIIIIII", 19, namepointer, entrypoint, stackpointer, stacksize, type, priority, state))
    response = self.__getbulk(self.handle, self.__cinep, 0x10)
    self.__checkstatus(response)
    
    if (struct.unpack("<i", response[4:8]) < 0):
      self.__myprint(" failed, error code: 0x%x" % (struct.unpack("<i", response[4:8])), silent)
    else:
      self.__myprint(" done\n, thread ID: 0x%x" % (struct.unpack("<I", response[4:8])), silent)     
    
    
  def getprocinfo(self, offset, size, silent = 0):
    """
      printout on console window:
        <silent> = 0: Process information struct version, Process information table size
        <silent> = 1: nothing

      {'regs': [16I], 'cpsr': I, 'state': I, 'namepointer': I, 'cputime_current': I, 'cputime_total': Q, 'startusec': I,
      'queue_next_pointer': I, 'timeout': I, 'blocked_since': I, 'blocked_by_pointer': I, 'stackpointer': I, 'block_type': B, 'thread_type': B, 'priority': B, 'cpuload': B} 
    """
    if (size > self.cin_maxsize - 0x10):
      raise Exception ("The data exceeds the maximum amount that can be received with this instruction.")
    
    self.__myprint("Retrieving process information...", silent)
    
    self.handle.bulkWrite(self.__coutep, struct.pack("<IIII", 15, offset, size, 0))
    response = self.__getBulk(self.handle, self.__cinep, size + 0x10)
    self.__checkstatus(response)
    
    out = []
    out[0] = struct.unpack("<I", response[4:8])[0]    # Process information struct version
    out[1] = struct.unpack("<I", response[8:12])[0]    # Process information table size
    out[2] = response     # raw received data
    
    if (struct.unpack("<I", response[4:8])[0] == 1):   # Process information struct version == 1
      p = 0x10
      process_n = 3   # actually process 0, but there are alread three other elements in out
      while True:
      # regs ==================================================
        keylen = 16
        key_offset = 0
        
        while (offset > 0) and (key_offset < 0x10):
          offset -= 0x4
          out[process_n]['regs'][key_offset] = None
          key_offset += 1
        
        while (p+(keylen*0x4) - 0x10 > size) and (keylen > 0): keylen -= 1
        if (p+(keylen*0x4) - 0x10 > size): break
        
        while (key_offset < keylen):
          out[process_n]['regs'][key_offset] = struct.unpack("<I", response[p + (key_offset * 0x4) :])[0]
          key_offset += 1
       
        p += 16 * 0x4
        
      # cpsr ==================================================
        if (offset > 0):
          offset -= 4
          out[process_n]['cpsr'] = None
          
        if (p+0x4 - 0x10 > size): break
        
        out[process_n]['cpsr'] = struct.unpack("<I", response[p:])[0]
        
        p += 0x4
        
      # state =================================================
        if (offset > 0):
          offset -= 4
          out[process_n]['state'] = None
          
        if (p+0x4 - 0x10 > size): break
        
        out[process_n]['state'] = struct.unpack("<I", response[p:])[0]
        
        p += 0x4
        
      # namepointer ===========================================
        if (offset > 0):
          offset -= 4
          out[process_n]['namepointer'] = None
          
        if (p+0x4 - 0x10 > size): break
        
        out[process_n]['namepointer'] = struct.unpack("<I", response[p:])[0]
        
        p += 0x4
        
      # cputime_current =======================================
        if (offset > 0):
          offset -= 4
          out[process_n]['cputime_current'] = None
          
        if (p+0x4 - 0x10 > size): break
        
        out[process_n]['cputime_current'] = struct.unpack("<I", response[p:])[0]
        
        p += 0x4
        
      # cputime_total =========================================
        if (offset > 0):
          offset -= 4
          out[process_n]['cputime_total'] = None
          
        if (p+0x4 - 0x10 > size): break
        
        out[process_n]['cputime_total'] = struct.unpack("<Q", response[p:])[0]
        
        p += 0x8
        
      # startusec =============================================
        if (offset > 0):
          offset -= 4
          out[process_n]['startusec'] = None
          
        if (p+0x4 - 0x10 > size): break
        
        out[process_n]['startusec'] = struct.unpack("<I", response[p:])[0]
        
        p += 0x4
        
      # queue_next_pointer ====================================
        if (offset > 0):
          offset -= 4
          out[process_n]['queue_next_pointer'] = None
          
        if (p+0x4 - 0x10 > size): break
        
        out[process_n]['queue_next_pointer'] = struct.unpack("<I", response[p:])[0]
        
        p += 0x4
        
      # timeout ===========================================
        if (offset > 0):
          offset -= 4
          out[process_n]['timeout'] = None
          
        if (p+0x4 - 0x10 > size): break
        
        out[process_n]['timeout'] = struct.unpack("<I", response[p:])[0]
        
        p += 0x4
        
      # blocked_since =========================================
        if (offset > 0):
          offset -= 4
          out[process_n]['blocked_since'] = None
          
        if (p+0x4 - 0x10 > size): break
        
        out[process_n]['blocked_since'] = struct.unpack("<I", response[p:])[0]
        
        p += 0x4
        
      # blocked_by_pointer ====================================
        if (offset > 0):
          offset -= 4
          out[process_n]['blocked_by_pointer'] = None
          
        if (p+0x4 - 0x10 > size): break
        
        out[process_n]['blocked_by_pointer'] = struct.unpack("<I", response[p:])[0]
        
        p += 0x4
        
      # stackpointer ==========================================
        if (offset > 0):
          offset -= 4
          out[process_n]['stackpointer'] = None
          
        if (p+0x4 - 0x10 > size): break
        
        out[process_n]['stackpointer'] = struct.unpack("<I", response[p:])[0]
        
        p += 0x4
        
      # block_type ============================================
        if (offset > 0):
          offset -= 1
          out[process_n]['block_type'] = None
          
        if (p+0x1 - 0x10 > size): break
        
        out[process_n]['block_type'] = struct.unpack("<B", response[p:])[0]
        
        p += 0x1
        
      # thread_type ===========================================
        if (offset > 0):
          offset -= 1
          out[process_n]['thread_type'] = None
          
        if (p+0x1 - 0x10 > size): break
        
        out[process_n]['thread_type'] = struct.unpack("<B", response[p:])[0]
        
        p += 0x1
        
      # priority ==============================================
        if (offset > 0):
          offset -= 1
          out[process_n]['priority'] = None
          
        if (p+0x1 - 0x10 > size): break
        
        out[process_n]['priority'] = struct.unpack("<B", response[p:])[0]
        
        p += 0x1
        
      # cpuload ===============================================
        if (offset > 0):
          offset -= 1
          out[process_n]['cpuload'] = None
          
        if (p+0x1 - 0x10 > size): break
        
        out[process_n]['cpuload'] = struct.unpack("<B", response[p:])[0]
        
        p += 0x1
        
      process_n += 1
        
    procinfoprint = ""
    
    try:
      i = 0
      while out[0] == 1 and not silent:      # Process information struct version == 1 && !silent
        processinfoprint += "--------------------------------------------------------------------------------"
        processinfoprint += "R0: 0x%08x,  R1: 0x%08x,  R2: 0x%08x,  R3: 0x%08x,\n\
                             R4: 0x%08x,  R5: 0x%08x,  R6: 0x%08x,  R7: 0x%08x,\n\
                             R8: 0x%08x,  R9: 0x%08x,  R10: 0x%08x, R11: 0x%08x,\n\
                             R12: 0x%08x, R13: 0x%08x, LR: 0x%08x,  PC: 0x%08x\n" \
                             % (out[i+3]['regs'][0], out[i+3]['regs'][1], out[i+3]['regs'][2], out[i+3]['regs'][3], \
                                out[i+3]['regs'][4], out[i+3]['regs'][5], out[i+3]['regs'][6], out[i+3]['regs'][7], \
                                out[i+3]['regs'][8], out[i+3]['regs'][9], out[i+3]['regs'][10], out[i+3]['regs'][11], \
                                out[i+3]['regs'][12], out[i+3]['regs'][13], out[i+3]['regs'][14], out[i+3]['regs'][15] )
        processinfoprint += "cpsr: 0b%032b      " % (out[i+3]['cpsr'])
        states = ["THREAD_FREE", "THREAD_SUSPENDED", "THREAD_READY", "THREAD_RUNNING", "THREAD_BLOCKED", "THREAD_DEFUNCT", "THREAD_DEFUNCT_ACK"]
        processinfoprint += "state: %s      " % (states[out[i+3]['state']])
        processinfoprint += "nameptr: 0x%08x\n" % (out[i+3]['namepointer'])
        processinfoprint += "current cpu time: 0x%08x      " % (out[i+3]['cputime_current'])
        processinfoprint += "total cpu time: 0x%016x\n" % (out[i+3]['cputime_total'])
        processinfoprint += "startusec: 0x%08x      " % (out[i+3]['startusec'])
        processinfoprint += "queue next ptr: 0x%08x\n" % (out[i+3]['queue_next_pointer'])
        processinfoprint += "timeout: 0x%08x\n" % (out[i+3]['timeout'])
        processinfoprint += "blocked since: 0x%08x      " % (out[i+3]['blocked_since'])
        processinfoprint += "blocked by ptr: 0x%08x\n" % (out[i+3]['blocked_by_pointer'])
        processinfoprint += "stackptr: 0x%08x      " % (out[i+3]['stackpointer'])
        blocktype = ["THREAD_NOT_BLOCKED", "THREAD_BLOCK_SLEEP", "THREAD_BLOCK_MUTEX", "THREAD_BLOCK_WAKEUP", "THREAD_DEFUNCT_STKOV", "THREAD_DEFUNCT_PANIC"]
        processinfoprint += "block type: %s\n" % (blocktype[out[i+3]['block_type']])
        threadtype = ["USER_THREAD", "SYSTEM_THREAD"]
        processinfoprint += "thread type: %s\n" % (threadtype[out[i+3]['thread_type']])
        processinfoprint += "priority: 0x%02x      " % (out[i+3]['priority'])
        processinfoprint += "cpu load: 0x%02x\n" % (out[i+3]['cpuload'])
        
        i += 1
        
    except IndexError:
      processinfoprint += "--------------------------------------------------------------------------------"
    
    self.__myprint(" done\n\
                    Process information struct version: 0x%8x\n\
                    Total size of process information table: 0x%8x\n\
                    %s"
                  % (out[0], out[1], procinfoprint)
                  , silent)
    
    return out
    
    
    
#===================================================================================== 

 
  def flushcaches(self, silent = 0):
    self.__myprint("Flushing caches...", silent)
    
    self.handle.bulkWrite(self.__coutep, struct.pack("<IIII", 20, 0, 0, 0))
    response = self.__getbulk(self.handle, self.__cinep, 0x10)
    self.__checkstatus(response)
    
    self.__myprint(" done\n", silent) 
    
  
#======================================================================================
# backlight control, remnant from libibugger adjusted to work with libembios ==========


  def backlighton(self, fade, brightness, silent = 0):
    self.__myprint("Turning on backlight...", silent)
    if self.devtype == 2:
      self.i2csend(0, 0xe6, 0x2b, struct.pack("<B", fade), 1)
      self.i2csend(0, 0xe6, 0x28, struct.pack("<B", int(brightness * 46)), 1)
      self.i2csend(0, 0xe6, 0x29, struct.pack("<B", 1), 1)
      self.__myprint(" done\n", silent)
    elif self.devtype == 4:
      self.i2csend(0, 0xe6, 0x30, struct.pack("<B", int(brightness * 250)), 1)
      self.i2csend(0, 0xe6, 0x31, struct.pack("<B", 3), 1)
      self.__myprint(" done\n", silent)
    else: self.__myprint(" unsupported (%s)\n" % self.devtype2name(self.devtype), silent)


  def backlightoff(self, fade, silent = 0):
    self.__myprint("Turning off backlight...", silent)
    if self.devtype == 2:
      self.i2csend(0, 0xe6, 0x2b, struct.pack("<B", fade), 1)
      self.i2csend(0, 0xe6, 0x29, struct.pack("<B", 0), 1)
      self.__myprint(" done\n", silent)
    elif self.devtype == 4:
      self.i2csend(0, 0xe6, 0x31, struct.pack("<B", 2), 1)
      self.__myprint(" done\n", silent)
    else: self.__myprint(" unsupported (%s)\n" % self.devtype2name(self.devtype), silent)
  