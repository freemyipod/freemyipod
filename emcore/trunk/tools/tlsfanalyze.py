#!/usr/bin/env python
#
#
#    Copyright 2011 TheSeven
#
#
#    This file is part of emCORE.
#
#    emCORE is free software: you can redistribute it and/or
#    modify it under the terms of the GNU General Public License as
#    published by the Free Software Foundation, either version 2 of the
#    License, or (at your option) any later version.
#
#    emCORE is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
#    See the GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License
#    along with emCORE.  If not, see <http://www.gnu.org/licenses/>.
#
#


import sys
import struct
from misc import ExtendedCStruct
from ctypes import *


def usage():
  print("Usage: %s <image> [image_base] [tlsf_pool] [SL_INDEX_COUNT_LOG2] [FL_INDEX_MAX]" % (sys.argv[0]))
  exit(2)

  
image_base = 0x08000000
tlsf_pool = 0x08000000
SL_INDEX_COUNT_LOG2 = 5
FL_INDEX_MAX = 30

if len(sys.argv) < 2: usage()
filename = sys.argv[1]
if len(sys.argv) > 2: image_base = int(sys.argv[2])
if len(sys.argv) > 3: tlsf_pool = int(sys.argv[3])
if len(sys.argv) > 4: SL_INDEX_COUNT_LOG2 = int(sys.argv[4])
if len(sys.argv) > 5: FL_INDEX_MAX = int(sys.argv[5])
if len(sys.argv) > 6: usage()

file = open(filename, "rb")
data = file.read()
file.close()
              
SL_INDEX_COUNT = 1 << SL_INDEX_COUNT_LOG2
FL_INDEX_SHIFT = SL_INDEX_COUNT_LOG2 + 2
FL_INDEX_COUNT = FL_INDEX_MAX - FL_INDEX_SHIFT + 1
SMALL_BLOCK_SIZE = 1 << FL_INDEX_SHIFT


class block_header_t(ExtendedCStruct):
  _fields_ = [("prev_phys_block", c_uint32),
              ("size", c_uint32),
              ("next_free", c_uint32),
              ("prev_free", c_uint32)]
  
  def is_last(self):
    return self.get_size() == 0;
    
  def is_null(self):
    return self._address_ == self.next_free and self.size == 0 and self.prev_phys_block == 0
  
  def is_free(self):
    return (self.size & block_header_free_bit) != 0;
  
  def is_prev_free(self):
    return (self.size & block_header_prev_free_bit) != 0;
    
  def get_address(self):
    return self._address_
    
  def get_size(self):
    return self.size & 0xfffffffc
    
  def get_prev_free(self):
    if not self.is_free(): raise Exception("Trying to get previous free block of non-free block")
    return block_header_t(self._data_, self._base_, self.prev_free)
  
  def get_next_free(self):
    if not self.is_free(): raise Exception("Trying to get next free block of non-free block")
    return block_header_t(self._data_, self._base_, self.next_free)
  
  def get_prev_phys(self):
    if not self.is_prev_free(): raise Exception("Trying to get non-free previous physical block")
    return block_header_t(self._data_, self._base_, self.prev_phys_block)
  
  def get_next_phys(self):
    return self.get_next_by_offset(self.get_size() + 4)
  
  def get_next_by_offset(self, offset):
    return block_header_t(self._data_, self._base_, self._address_ + offset)
  

class pool_t(ExtendedCStruct):
  _fields_ = [("fl_bitmap", c_uint32),
              ("sl_bitmap", c_uint32 * FL_INDEX_COUNT),
              ("blocks", c_uint32 * SL_INDEX_COUNT * FL_INDEX_COUNT)]
              

block_header_free_bit = 1
block_header_prev_free_bit = 2
block_size_min = sizeof(block_header_t) - 4;
block_size_max = 1 << FL_INDEX_MAX

pool = pool_t(data, image_base, tlsf_pool)
block = block_header_t(data, image_base, tlsf_pool + sizeof(pool_t) - 4)
prev_free = False
prev_addr = 0
blocks = []
free_blocks = []
bytes_used = 0
bytes_free = 0

while True:
  if block.is_prev_free() != prev_free:
    print("Block %08X previous free indicator is %d, expected %d" % (block.get_address(), block.is_prev_free(), prev_free))
  if prev_free and prev_addr != block.prev_phys_block:
    print("Block %08X previous physical block address is wrong. Got %08X, should be %08X" % (block.get_address(), block.prev_phys_block, prev_addr))
  prev_free = block.is_free()
  prev_addr = block.get_address()
  if block.is_last(): break
  if blocks.count(prev_addr) > 0:
    print("Block loop detected at %08X" % (prev_addr))
    break
  blocks.append(prev_addr)
  if prev_free:
    print("%08X: %08X bytes free" % (prev_addr + 4, block.get_size() + 4))
    free_blocks.append(prev_addr)
    bytes_free = bytes_free + block.get_size() + 4
  else:
    owner_address = prev_addr - image_base + block.get_size() - 4
    owner = struct.unpack("<I", data[owner_address : owner_address + 4])[0]
    print("%08X: %08X+8 bytes owned by %08X" % (prev_addr + 8, block.get_size() - 4, owner))
    bytes_used = bytes_used + block.get_size() + 4
  try: block = block.get_next_phys()
  except:
    print("Block %08X has invalid size: %08X" % (prev_addr, block.get_size()))
    print("Fatal error in block chain, continuing with map check")
    break

handled_blocks = []
for i in range(FL_INDEX_COUNT):
  fl_map = (pool.fl_bitmap >> i) & 1
  sl_list = pool.sl_bitmap[i]
  if fl_map == 0:
    if sl_list != 0:
      print("[%d:%d] Second-level map must be null, but isn't" % (i, j))
  elif sl_list == 0:
    print("[%d:%d] No free blocks in second-level map, but first-level map indicates there are some" % (i, j))
  for j in range(SL_INDEX_COUNT):
    sl_map = (sl_list >> j) & 1
    ba = pool.blocks[i][j]
    block = block_header_t(data, image_base, ba)
    if sl_map == 0:
      if not block.is_null():
        print("[%d:%d:%08X] Block list must be null, but isn't" % (i, j, ba))
      continue
    elif block.is_null():
      print("[%d:%d:%08X] Block list is null, but second-level map indicates there are free blocks" % (i, j, ba))
    blocks = []
    while not block.is_null():
      fatal = False
      addr = block.get_address()
      if blocks.count(addr) > 0:
        print("[%d:%d:%08X] Detected block loop" % (i, j, addr))
        break
      blocks.append(addr)
      if not block.is_free():
        print("[%d:%d:%08X] Non-free block on free list" % (i, j, addr))
        fatal = True
      if block.is_prev_free():
        print("[%d:%d:%08X] Block should have coalesced with previous one" % (i, j, addr))
      try:
        if block.get_next_phys().is_free():
          print("[%d:%d:%08X] Block should have coalesced with next one" % (i, j, addr))
      except:
        print("Block %08X has invalid size: %08X" % (addr, block.get_size()))
        fatal = True
      size = block.get_size()
      if size < block_size_min:
        print("[%d:%d:%08X] Block violates minimum size: %d (should be at least %d)" % (i, j, addr, size, block_size_min))
      if size > block_size_max:
        print("[%d:%d:%08X] Block violates maximum size: %d (should be at most %d)" % (i, j, addr, size, block_size_max))
      if size < SMALL_BLOCK_SIZE:
        fl = 0
        sl = size / (SMALL_BLOCK_SIZE / SL_INDEX_COUNT)
      else:
        fl = 32 - FL_INDEX_SHIFT;
        if (size & 0xffff0000) == 0:
          size = size << 16
          fl = fl - 16
        if (size & 0xff000000) == 0:
          size = size << 8
          fl = fl - 8
        if (size & 0xf0000000) == 0:
          size = size << 4
          fl = fl - 4
        if (size & 0xc0000000) == 0:
          size = size << 2
          fl = fl - 2
        if (size & 0x80000000) == 0:
          size = size << 1
          fl = fl - 1
        sl = (block.get_size() >> (fl - SL_INDEX_COUNT_LOG2 + FL_INDEX_SHIFT - 1)) ^ (1 << SL_INDEX_COUNT_LOG2)
      if fl != i or sl != j:
        print("Block %08X is in wrong free list: [%d:%d] (should be [%d:%d])" % (addr, i, j, fl, sl))
      if free_blocks.count(addr) != 1:
        print("[%d:%d:%08X] Block is in free list, but was not found in pool" % (i, j, addr))
      if handled_blocks.count(addr) > 0:
        print("[%d:%d:%08X] Block appears in multiple free lists" % (i, j, addr))
      else: handled_blocks.append(addr)
      if fatal:
        print("Fatal error in block chain, continuing with next chain")
        break
      block = block.get_next_free()
      
for addr in free_blocks:
  if handled_blocks.count(addr) != 1:
    print("Free block %08X does not appear in any free list" % (addr))
