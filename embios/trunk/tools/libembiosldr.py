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
import math
import struct
import time
import usb


class embiosldr:
  def __init__(self, generation = 0):
    busses = usb.busses()
 
    for bus in busses:
      devices = bus.devices
      for dev in devices:
        if dev.idVendor == 0xffff and dev.idProduct == 0xe112:
          handle = dev.open()
          handle.setConfiguration(1)
          handle.claimInterface(0)
          if generation in [0, 2]:
            self.devtype = 2;
            self.maxin = 528;
            self.maxout = 528
            self.handle = handle
            print("Connected to emBIOS Loader Recovery Mode on iPod Nano 2G, USB version %s" % dev.deviceVersion)
            return
          handle.releaseInterface()

    raise Exception("Could not find specified device (generation = %d)" % generation)


  @staticmethod
  def __myprint(data):
    sys.stdout.write(data)
    sys.stdout.flush()


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
      raise Exception("Device doesn't support this function!")
    else:
      print("\nUnknown error %d" % errorcode)
      raise Exception("Unknown error %d" % errorcode)


  def __readstatus(self):
    return self.__getbulk(self.handle, 0x83, 0x10)


  @staticmethod
  def devtype2name(devtype):
    if devtype == 2: return "iPod Nano 2G"
    else: return "UNKNOWN (%8x)" % devtype


  def write(self, offset, data, *range):
    if offset & 3 != 0 or len(data) & 3 != 0:
      raise Exception("Unaligned data write!")

    boffset = 0
    size = len(data)
    if len(range) > 0:
      boffset = range[0]
    if len(range) > 1:
      size = range[1]

    maxblk = self.maxout - 0x10

    while True:
      blocklen = size
      if blocklen == 0: break
      if blocklen > maxblk: blocklen = maxblk
      self.handle.bulkWrite(4, struct.pack("<IIII", 2, offset, int(blocklen / 4), 0) \
                             + data[boffset:boffset+blocklen])
      self.__checkstatus(self.__readstatus())
      offset += blocklen
      boffset += blocklen
      size -= blocklen


  def read(self, offset, size):
    if offset & 3 != 0 or size & 3 != 0:
      raise Exception("Unaligned data read!")

    maxblk = self.maxin - 0x10

    data = ""

    while True:
      blocklen = size
      if blocklen == 0: break
      if blocklen > maxblk: blocklen = maxblk
      self.handle.bulkWrite(4, struct.pack("<IIII", 1, offset, int(blocklen / 4), 0))
      block = self.__getbulk(self.handle, 0x83, 0x10 + blocklen)
      self.__checkstatus(block)
      offset += blocklen
      data += block[0x10:]
      size -= blocklen

    return data


  def execute(self, addr, stack):
    self.__myprint("Passing control to code at 0x%8x..." % addr)
    self.handle.bulkWrite(4, struct.pack("<IIII", 0, addr, stack, 0))
    self.__myprint(" done\n")


  def upload(self, offset, file):
    self.__myprint("Uploading %s to 0x%8x..." % (file, offset))
    f = open(file, "rb")

    while True:
      data = f.read(65536)
      if data == "": break
      self.write(offset, data)
      offset += len(data)
      self.__myprint(".")

    self.__myprint(" done\n")


  def download(self, offset, size, file):
    self.__myprint("Downloading 0x%x bytes from 0x%8x to %s..." % (size, offset, file))
    f = open(file, "wb")

    while True:
      blocklen = size
      if blocklen == 0: break
      if blocklen > 65536: blocklen = 65536
      f.write(self.read(offset, blocklen))
      offset += blocklen
      size -= blocklen
      self.__myprint(".")

    self.__myprint(" done\n")


  def run(self, file):
    self.upload(0x08000000, file)
    self.execute(0x08000000, 0x0a000000)

