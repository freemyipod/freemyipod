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

# note: handles commands 1 to 21

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
      blocklen = size
        
      if (blocklen > size):
        blocklen = size
      if (blocklen > self.cout_maxsize - 0x10):
        blocklen = self.cout_maxsize - 0x10
      
      blocklen = (blocklen & 0xFFFFFFF0) +  (offset & 0xF)
      
      self.handle.bulkWrite(self.__coutep, struct.pack("<IIII", 5, offset, blocklen, 0) + data[boffset:boffset+blocklen])
      response = self.__getbulk(self.handle, self.__cinep, 0x10)
      self.__checkstatus(response)
      
      offset += blocklen
      boffset += blocklen
      size -= blocklen

    # write data with DMA, if it makes sense (-> much data) and isn't forbidden
    if (usedma) and (size > 2 * (self.cout_maxsize - 16)): 
      if (freezesched):
        self.freezescheduler(1, 0)
    
      while (size > (self.cout_maxsize - 16)):
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
        self.freezescheduler(0, 0)
        
    # write rest of data
    while (size > 0):
      blocklen = size
      
      if (blocklen > self.cout_maxsize - 0x10):
        blocklen = self.cout_maxsize - 0x10
    
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
      blocklen = size
      
      if (blocklen > size):
        blocklen = size
      if (blocklen > self.cin_maxsize - 0x10):
        blocklen = self.cin_maxsize - 0x10
        
      blocklen = (blocklen & 0xFFFFFFF0) +  (offset & 0xF)
      
      self.handle.bulkWrite(self.__coutep, struct.pack("<IIII", 4, offset, blocklen, 0))
      response = self.__getbulk(self.handle, self.__cinep, 0x10 + blocklen)
      self.__checkstatus(response)
      
      data += response[0x10:]
      
      offset += blocklen
      size -= blocklen

    # read data with DMA, if it makes sense (-> much data) and isn't forbidden
    if (usedma) and (size > 2 * (self.cin_maxsize - 16)):
      if (freezesched):
        self.freezescheduler(1, 0)

      while (size > (self.cin_maxsize - 16)):
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
        self.freezescheduler(0, 0)
        
    # read rest of data
    while (size > 0):
      blocklen = size
      
      if (blocklen > self.cin_maxsize - 0x10):
        blocklen = self.cin_maxsize - 0x10
    
      self.handle.bulkWrite(self.__coutep, struct.pack("<IIII", 4, offset, blocklen, 0))
      response = self.__getbulk(self.handle, self.__cinep, 0x10 + blocklen)
      self.__checkstatus(response)
      
      data += response[0x10:]
      
      offset += blocklen
      size -= blocklen
      
    return data

 
  def uploadfile(self, offset, file, usedma = 1, freezesched = 0, silent = 0):
    self.__myprint("Uploading %s to 0x%08x..." % (file, offset), silent)
    f = open(file, "rb")

    while True:
      data = f.read(262144)
      if data == "": break
      self.write(offset, data, usedma, freezesched)
      offset += len(data)
      self.__myprint(".")

    self.__myprint(" done\n", silent) 
    
  
  def downloadfile(self, offset, size, file, usedma = 1, freezesched = 0, silent = 0):
    self.__myprint("Downloading 0x%x bytes from 0x%08x to %s..." % (size, offset, file), silent)
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
    self.__myprint("Uploading 0x%08x to 0x%08x..." % (data, offset), silent)
    data = struct.pack('<I', data)
    self.write(offset, data, 0, 0)
    self.__myprint(" done\n", silent)


  def downloadint(self, offset, silent = 0):
    self.__myprint("Downloading an integer from 0x%08x..." % (offset), silent)
    data = self.read(offset, 4, 0, 0)
    number = struct.unpack('<I', data)
    self.__myprint(" done\nValue was: 0x%08x\n" % (number), silent)
    
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
      
      readbytes, buffersize, bytesleft = struct.unpack("<III", response[4:])
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
      
      sendbytes = struct.unpack("<I", response[4:])[0]
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
      
      readbytes = struct.unpack("<III", response[4:])[0]
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
      self.__myprint("Suspending thread 0x%08x..." % threadid, silent)
      suspend = 1
    else:
      self.__myprint("Unsuspending thread 0x%08x..." % threadid, silent)
      suspend = 0
      
    self.handle.bulkWrite(self.__coutep, struct.pack("<IIII", 17, suspend, threadid, 0))
    response = self.__getbulk(self.handle, self.__cinep, 0x10)
    self.__checkstatus(response)
    
    self.__myprint(" done\n", silent) 


  def killthread(self, threadid, silent = 0):
    self.__myprint("Killing thread 0x%08x..." % threadid, silent)
      
    self.handle.bulkWrite(self.__coutep, struct.pack("<IIII", 18, threadid, 0, 0))
    response = self.__getbulk(self.handle, self.__cinep, 0x10)
    self.__checkstatus(response)
    
    self.__myprint(" done\n", silent) 
    
    
  def createthread(self, namepointer, entrypoint, stackpointer, stacksize, type, priority, state, silent = 0):
    self.__myprint("Creating thread...", silent)
      
    self.handle.bulkWrite(self.__coutep, struct.pack("<IIIIIIII", 19, namepointer, entrypoint, stackpointer, stacksize, type, priority, state))
    response = self.__getbulk(self.handle, self.__cinep, 0x10)
    self.__checkstatus(response)
    
    if (struct.unpack("<i", response[4:8])[0] < 0):
      self.__myprint(" failed, error code: 0x%x" % (struct.unpack("<i", response[4:8])[0]), silent)
    else:
      self.__myprint(" done\n, thread ID: 0x%x" % (struct.unpack("<I", response[4:8])[0]), silent)     
    
    
  def getprocinfo(self, silent = 0):
    """
      printout on console window:
        <silent> = 0: Process information struct version, Process information table size
        <silent> = 1: nothing
    """
    # inline functions ----------------------------------------------
    def procinfotolist(processinfo, structver):
      if (structver == 1):   # Process information struct version == 1
        ptr = 0
        process_n = 0
        retval = []
        while ptr < len(processinfo):
          if struct.unpack("<I", processinfo[ptr + 68:ptr + 72])[0] == 0:    # THREAD_FREE
            ptr += 120
            process_n += 1
            continue
          
          retval.append({})
          
          retval[process_n]['regs'] = struct.unpack("<IIIIIIIIIIIIIIII", processinfo[ptr:ptr + 64])
          ptr += 16 * 0x4
          retval[process_n]['cpsr'] = struct.unpack("<I", processinfo[ptr:ptr + 4])[0]
          ptr += 1 * 0x4
          retval[process_n]['state'] = struct.unpack("<I", processinfo[ptr:ptr + 4])[0]
          ptr += 1 * 0x4
          retval[process_n]['name_ptr'] = struct.unpack("<I", processinfo[ptr:ptr + 4])[0]
          ptr += 1 * 0x4
          retval[process_n]['cputime_current'] = struct.unpack("<I", processinfo[ptr:ptr + 4])[0]
          ptr += 1 * 0x4
          retval[process_n]['cputime_total'] = struct.unpack("<Q", processinfo[ptr:ptr + 8])[0]
          ptr += 1 * 0x8
          retval[process_n]['startusec'] = struct.unpack("<I", processinfo[ptr:ptr + 4])[0]
          ptr += 1 * 0x4
          retval[process_n]['queue_next_ptr'] = struct.unpack("<I", processinfo[ptr:ptr + 4])[0]
          ptr += 1 * 0x4
          retval[process_n]['timeout'] = struct.unpack("<I", processinfo[ptr:ptr + 4])[0]
          ptr += 1 * 0x4
          retval[process_n]['blocked_since'] = struct.unpack("<I", processinfo[ptr:ptr + 4])[0]
          ptr += 1 * 0x4
          retval[process_n]['blocked_by_ptr'] = struct.unpack("<I", processinfo[ptr:ptr + 4])[0]
          ptr += 1 * 0x4
          retval[process_n]['stack_ptr'] = struct.unpack("<I", processinfo[ptr:ptr + 4])[0]
          ptr += 1 * 0x4
          retval[process_n]['err_no'] = struct.unpack("<I", processinfo[ptr:ptr + 4])[0]
          ptr += 1 * 0x4
          retval[process_n]['block_type'] = struct.unpack("<B", processinfo[ptr:ptr + 1])[0]
          ptr += 1 * 0x1
          retval[process_n]['thread_type'] = struct.unpack("<B", processinfo[ptr:ptr + 1])[0]
          ptr += 1 * 0x1
          retval[process_n]['priority'] = struct.unpack("<B", processinfo[ptr:ptr + 1])[0]
          ptr += 1 * 0x1
          retval[process_n]['cpuload'] = struct.unpack("<B", processinfo[ptr:ptr + 1])[0]
          ptr += 1 * 0x1
          
          process_n += 1
          
        return retval
      
      
    def state2name(state, structver):
      if structver == 1:
        if state == 0: return "THREAD_FREE"
        elif state == 1: return "THREAD_SUSPENDED"
        elif state == 2: return "THREAD_READY"
        elif state == 3: return "THREAD_RUNNING"
        elif state == 4: return "THREAD_BLOCKED"
        elif state == 5: return "THREAD_DEFUNCT"
        elif state == 6: return "THREAD_DEFUNCT_ACK"
        else: return "UNKNOWN"
      else: return "UNKNOWN"
      
    def blocktype2name(blocktype, structver):
      if structver == 1:
        if blocktype == 0: return "THREAD_NOT_BLOCKED"
        elif blocktype == 1: return "THREAD_BLOCK_SLEEP"
        elif blocktype == 2: return "THREAD_BLOCK_MUTEX"
        elif blocktype == 3: return "THREAD_BLOCK_WAKEUP"
        elif blocktype == 4: return "THREAD_DEFUNCT_STKOV"
        elif blocktype == 5: return "THREAD_DEFUNCT_PANIC"
        else: return "UNKNOWN"
      else: return "UNKNOWN"
      
    def threadtype2name (threadtype, structver):
      if structver == 1:
        if threadtype == 0: return "USER_THREAD"
        elif threadtype == 1: return "OS_THREAD"
        elif threadtype == 2: return "CORE_THREAD"
        else: return "UNKNOWN"
      else: return "UNKNOWN"
      
    def procinfotostring(procinfolist, structver):
      processinfoprint = ""
      ptr = 0
      while structver == 1 and ptr < len(procinfolist):      # Process information struct version == 1
        processinfoprint += "--------------------------------------------------------------------------------\n"
        processinfoprint += "R0: 0x%08x,  R1: 0x%08x,  R2: 0x%08x,  R3: 0x%08x,\n"\
                            % (procinfolist[ptr]['regs'][0], procinfolist[ptr]['regs'][1], procinfolist[ptr]['regs'][2], procinfolist[ptr]['regs'][3])\
                            + "R4: 0x%08x,  R5: 0x%08x,  R6: 0x%08x,  R7: 0x%08x,\n"\
                            % (procinfolist[ptr]['regs'][4], procinfolist[ptr]['regs'][5], procinfolist[ptr]['regs'][6], procinfolist[ptr]['regs'][7])\
                            + "R8: 0x%08x,  R9: 0x%08x,  R10: 0x%08x, R11: 0x%08x,\n"\
                            % (procinfolist[ptr]['regs'][8], procinfolist[ptr]['regs'][9], procinfolist[ptr]['regs'][10], procinfolist[ptr]['regs'][11])\
                            + "R12: 0x%08x, SP: 0x%08x,  LR: 0x%08x,  PC: 0x%08x\n" \
                            % (procinfolist[ptr]['regs'][12], procinfolist[ptr]['regs'][13], procinfolist[ptr]['regs'][14], procinfolist[ptr]['regs'][15])
        processinfoprint += "cpsr: 0x%08x      " %             (procinfolist[ptr]['cpsr'])
        processinfoprint += "state: %s      " %                 (state2name([procinfolist[ptr]['state']], structver))
        processinfoprint += "nameptr: 0x%08x\n" %               (procinfolist[ptr]['name_ptr'])
        processinfoprint += "current cpu time: 0x%08x      " %  (procinfolist[ptr]['cputime_current'])
        processinfoprint += "total cpu time: 0x%016x\n" %       (procinfolist[ptr]['cputime_total'])
        processinfoprint += "startusec: 0x%08x      " %         (procinfolist[ptr]['startusec'])
        processinfoprint += "queue next ptr: 0x%08x\n" %        (procinfolist[ptr]['queue_next_ptr'])
        processinfoprint += "timeout: 0x%08x\n" %               (procinfolist[ptr]['timeout'])
        processinfoprint += "blocked since: 0x%08x      " %     (procinfolist[ptr]['blocked_since'])
        processinfoprint += "blocked by ptr: 0x%08x\n" %        (procinfolist[ptr]['blocked_by_ptr'])
        processinfoprint += "err_no: 0x%08x      " %            (procinfolist[ptr]['err_no'])
        processinfoprint += "block type: %s\n" %                (blocktype2name([procinfolist[ptr]['block_type']], structver))
        processinfoprint += "thread type: %s\n" %               (threadtype2name([procinfolist[ptr]['thread_type']], structver))
        processinfoprint += "priority: 0x%02x      " %          (procinfolist[ptr]['priority'])
        processinfoprint += "cpu load: 0x%02x\n" %              (procinfolist[ptr]['cpuload'])
          
        ptr += 1
        
      processinfoprint += "--------------------------------------------------------------------------------\n"
        
      return processinfoprint
     
    # reading code --------------------------------------------------
    self.__myprint("Retrieving process information...", silent)
    
    offset = 0
    blocklen = tablesize = self.cin_maxsize - 0x10
    procinfo = ""
    structversion = 0
    
    # reading loop
    while (offset < tablesize):
      self.handle.bulkWrite(self.__coutep, struct.pack("<IIII", 15, offset, blocklen, 0))
      response = self.__getbulk(self.handle, self.__cinep, blocklen + 0x10)
      self.__checkstatus(response)
      
      tablesize = struct.unpack("<I", response[8:12])[0]
      
      if tablesize <= offset + blocklen:
        blocklen = tablesize - offset
        procinfo += response[0x10:0x10 + blocklen]
        structversion = struct.unpack("<I", response[4:8])[0]
        tablesize = struct.unpack("<I", response[8:12])[0]
      else:
        procinfo += response[0x10:0x10 + blocklen]
      
      offset += blocklen
      
      blocklen = self.cin_maxsize - 0x10
      if blocklen > tablesize - offset:
        blocklen = tablesize - offset
    
    
    out = (structversion, tablesize, procinfotolist(procinfo, structversion))

    self.__myprint(" done\n"\
                   + "Process information struct version: 0x%08x\n" % out[0]\
                   + "Total size of process information table: 0x%08x\n" % out[1]\
                   + procinfotostring(out[2], 1)\
                   + "\n\n")
    
    return out
  
    
  def execimage(self, offset, silent = 0):
    self.__myprint("Executing emBIOS executable image at 0x%08x..." % offset, silent)
      
    self.handle.bulkWrite(self.__coutep, struct.pack("<IIII", 21, offset, 0, 0))
    response = self.__getbulk(self.handle, self.__cinep, 0x10)
    self.__checkstatus(response)
    
    self.__myprint(" done\n    execimage() return code: 0x%08x\n" % struct.unpack("<I", response[4:8])[0], silent)
    
    return struct.unpack("<I", response[4:8])[0]
    
    
  def execfirmware(self, offset, silent = 0):
    self.__myprint("Executing firmware image at 0x%08x..." % offset, silent)
      
    self.handle.bulkWrite(self.__coutep, struct.pack("<IIII", 24, offset, 0, 0))
    response = self.__getbulk(self.handle, self.__cinep, 0x10)
    self.__checkstatus(response)
    
    self.__myprint(" done\n", silent)
    
    return
    
    
#===================================================================================== 


  def readrawbootflash(self, addr_bootflsh, addr_mem, size, silent = 0):
    self.__myprint("Reading 0x%x bytes from 0x%08x at bootflash to 0x%08x..." % (size, addr_bootflsh, addr_mem), silent)
      
    self.handle.bulkWrite(self.__coutep, struct.pack("<IIII", 22, addr_mem, addr_bootflsh, size))
    response = self.__getbulk(self.handle, self.__cinep, 0x10)
    self.__checkstatus(response)
    
    self.__myprint(" done\n", silent)


  def writerawbootflash(self, addr_mem, addr_bootflsh, size, silent = 0):
    self.__myprint("Writing 0x%x bytes from 0x%08x to bootflash at 0x%08x..." % (size, addr_fmem, addr_bootflsh), silent)
      
    self.handle.bulkWrite(self.__coutep, struct.pack("<IIII", 23, addr_mem, addr_bootflsh, size))
    response = self.__getbulk(self.handle, self.__cinep, 0x10)
    self.__checkstatus(response)
    
    self.__myprint(" done\n", silent)
    

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
  
