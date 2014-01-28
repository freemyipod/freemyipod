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
import struct
import usb.core


class ipoddfu:
  def __init__(self, generation = 0, type = 0):
    # iPod Nano 2G, Bootrom DFU
    try:
      self.dev = usb.core.find(idVendor=0x05ac, idProduct=0x1220)
      if self.dev and generation in [0, 2] and type in [0, 1]:
        self.dev.set_configuration(1)
        self.generation = 2;
        self.type = 1;
        print("Connected to S5L8701 Bootrom DFU mode, USB version %s"  % self.dev.bcdDevice)
        return
    except usb.core.USBError: pass
    # iPod Nano 3G and iPod Classic 1G/2G/3G, Bootrom DFU
    try:
      self.dev = usb.core.find(idVendor=0x05ac, idProduct=0x1223)
      if self.dev and generation in [0, 3] and type in [0, 1]:
        self.dev.set_configuration(1)
        self.generation = 3;
        self.type = 1;
        print("Connected to S5L8702 Bootrom DFU mode, USB version %s"  % self.dev.bcdDevice)
        return
    except usb.core.USBError: pass
    # iPod Nano 3G, Bootrom DFU
    try:
      self.dev = usb.core.find(idVendor=0x05ac, idProduct=0x1224)
      if self.dev and generation in [0, 3] and type in [0, 1]:
        self.dev.set_configuration(1)
        self.generation = 3;
        self.type = 1;
        print("Connected to S5L8702 Bootrom DFU mode, USB version %s"  % self.dev.bcdDevice)
        return
    except usb.core.USBError: pass
    # iPod Nano 4G, Bootrom DFU
    try:
      self.dev = usb.core.find(idVendor=0x05ac, idProduct=0x1225)
      if self.dev and generation in [0, 4] and type in [0, 1]:
        self.dev.set_configuration(1)
        self.generation = 4;
        self.type = 1;
        print("Connected to S5L8720 Bootrom DFU mode, USB version %s"  % self.dev.bcdDevice)
        return
    except usb.core.USBError: pass
    # iPod Nano 5G, Bootrom DFU
    try:
      self.dev = usb.core.find(idVendor=0x05ac, idProduct=0x1231)
      if self.dev and generation in [0, 5] and type in [0, 1]:
        self.dev.set_configuration(1)
        self.generation = 5;
        self.type = 1;
        print("Connected to S5L8730 Bootrom DFU mode, USB version %s"  % self.dev.bcdDevice)
        return
    except usb.core.USBError: pass
    # iPod Nano 6G, Bootrom DFU
    try:
      self.dev = usb.core.find(idVendor=0x05ac, idProduct=0x1225)
      if self.dev and generation in [0, 6] and type in [0, 1]:
        self.dev.set_configuration(1)
        self.generation = 6;
        self.type = 1;
        print("Connected to S5L8723 Bootrom DFU mode, USB version %s"  % self.dev.bcdDevice)
        return
    except usb.core.USBError: pass
    
    # iPod Nano 2G, NOR DFU
    try:
      self.dev = usb.core.find(idVendor=0x05ac, idProduct=0x1240)
      if self.dev and generation in [0, 2] and type in [0, 2]:
        self.dev.set_configuration(1)
        self.generation = 2;
        self.type = 2;
        print("Connected to iPod Nano 2G NOR DFU mode, USB version %s"  % self.dev.bcdDevice)
        return
    except usb.core.USBError: pass
    # iPod Nano 3G, WTF
    try:
      self.dev = usb.core.find(idVendor=0x05ac, idProduct=0x1242)
      if self.dev and generation in [0, 3] and type in [0, 2]:
        self.dev.set_configuration(1)
        self.generation = 3;
        self.type = 2;
        print("Connected to iPod Nano 3G WTF mode, USB version %s"  % self.dev.bcdDevice)
        return
    except usb.core.USBError: pass
    # iPod Classic 1G, WTF
    try:
      self.dev = usb.core.find(idVendor=0x05ac, idProduct=0x1241)
      if self.dev and generation in [0, 11] and type in [0, 2]:
        self.dev.set_configuration(1)
        self.generation = 11;
        self.type = 2;
        print("Connected to iPod Classic 1G WTF mode, USB version %s"  % self.dev.bcdDevice)
        return
    except usb.core.USBError: pass
    # iPod Classic 2G, WTF
    try:
      self.dev = usb.core.find(idVendor=0x05ac, idProduct=0x1245)
      if self.dev and generation in [0, 12] and type in [0, 2]:
        self.dev.set_configuration(1)
        self.generation = 12;
        self.type = 2;
        print("Connected to iPod Classic 2G WTF mode, USB version %s"  % self.dev.bcdDevice)
        return
    except usb.core.USBError: pass
    # iPod Classic 3G, WTF
    try:
      self.dev = usb.core.find(idVendor=0x05ac, idProduct=0x1247)
      if self.dev and generation in [0, 13] and type in [0, 2]:
        self.dev.set_configuration(1)
        self.generation = 13;
        self.type = 2;
        print("Connected to iPod Classic 3G WTF mode, USB version %s"  % self.dev.bcdDevice)
        return
    except usb.core.USBError: pass
    # iPod Nano 4G, WTF
    try:
      self.dev = usb.core.find(idVendor=0x05ac, idProduct=0x1243)
      if self.dev and generation in [0, 4] and type in [0, 2]:
        self.dev.set_configuration(1)
        self.generation = 4;
        self.type = 2;
        print("Connected to iPod Nano 4G WTF mode, USB version %s"  % self.dev.bcdDevice)
        return
    except usb.core.USBError: pass
    # iPod Nano 5G, WTF
    try:
      self.dev = usb.core.find(idVendor=0x05ac, idProduct=0x1243)
      if self.dev and generation in [0, 5] and type in [0, 2]:
        self.dev.set_configuration(1)
        self.generation = 5;
        self.type = 2;
        print("Connected to iPod Nano 5G WTF mode, USB version %s"  % self.dev.bcdDevice)
        return
    except usb.core.USBError: pass
    # iPod Nano 6G, WTF
    try:
      self.dev = usb.core.find(idVendor=0x05ac, idProduct=0x1243)
      if self.dev and generation in [0, 6] and type in [0, 2]:
        self.dev.set_configuration(1)
        self.generation = 6;
        self.type = 2;
        print("Connected to iPod Nano 6G WTF mode, USB version %s"  % self.dev.bcdDevice)
        return
    except usb.core.USBError: pass

    raise Exception("Could not find specified DFU device (generation = %d, type = %d)" % (generation, type))

  @staticmethod
  def crc32(data):
    crc_table = []
    for i in range(256):
      t = i;
      for j in range(8):
        if t & 1:
          t = (t >> 1) ^ 0xedb88320
        else:
          t = t >> 1
      crc_table.append(t)

    crc = 0xffffffff
    for i in range(len(data)):
      crc = (crc >> 8) ^ crc_table[(crc ^ struct.unpack("B", data[i:i+1])[0]) & 0xff];

    return crc


  def getcpu(self):
    result = self.handle.controlMsg(0xa1, 0xff, 0x3f, 2, 0, 100)
    return struct.pack("%dB" % len(result), *result)


  def upload(self, data, exploit = 0):
    if exploit == 1 and self.generation == 2 and self.type == 1:
      data = f.read().ljust(0x200f0, b"\0") \
           + b"\xb8\x48\x02\x22\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0" \
           + b"\0\0\0\x22\0\0\0\x22\0\0\0\x22\0\0\0\x22" \
           + b"\0\0\0\x22\0\0\0\x22\0\0\0\x22\0\0\0\x22" \
           + b"\0\0\0\x22\0\0\0\x22\0\0\0\x22\0\0\0\x22" \
           + b"\0\0\0\x22\0\0\0\x22\0\0\0\x22\0\0\0\x22" \
           + b"\0\0\0\x22\0\0\0\x22\0\0\0\x22\0\0\0\x22" \
           + b"\0\0\0\x22\0\0\0\x22\0\0\0\x22\0\0\0\x22" \
           + b"\0\0\0\x22\0\0\0\x22\0\0\0\x22\0\0\0\x22" \
           + b"\0\0\0\x22\0\0\0\x22\0\0\0\x22\0\0\0\x22"

    data = data + struct.pack("<I", self.crc32(data))

    sys.stdout.write("Upload: .")
    sys.stdout.flush()
    for index in range((len(data) + 2047) // 2048):
      self.dev.ctrl_transfer(0x21, 1, index, 0, data[2048 * index : 2048 * (index + 1)],   100)
      result = (0, 0, 0, 0, 0, 0)
      while result[4] != 0x05:
        result = self.dev.ctrl_transfer(0xa1, 3, 0, 0, 6, 100)
      sys.stdout.write(".")
      sys.stdout.flush()

    self.dev.ctrl_transfer(0x21, 1, index, 0, "",  100)
    result = (0, 0, 0, 0, 0, 0)
    index = 0
    try:
      while result[4] != 0x02 and index < 1000:
        result = self.dev.ctrl_transfer(0xa1, 3, 0, 0, 6, 100)
        index = index + 1
    except:
      pass

    if (exploit == 0 and (index == 1000 or result[4] == 0x02)) or \
       (exploit == 1 and (index == 1000 or result[4] != 0x04)):
      print(" failed: %X / %X" % (result[4], result[0]))
      raise Exception("DFU upload failed! (%X / %X)" % (result[4], result[0]))
    else:
      print(" done")


  def uploadfile(self, file, exploit = 0):
    f = open(file, "rb")
    data = f.read()
    f.close()
    self.upload(data, exploit)
