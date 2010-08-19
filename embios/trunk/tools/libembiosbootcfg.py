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
#    You should have received a copy of the GNU General Public License
#    along with emBIOS.  If not, see <http://www.gnu.org/licenses/>.
#
#

import struct

def configure(binary, **args):
    start = binary.index("emBIboot", 0, 512)
    version = struct.unpack("<I", binary[start + 8 : start + 12])[0]
    if version != 0: raise ValueError("Unknown boot configuration data version")
    (tryfile, filename, fileflags, filedest,
     tryflash, flashname, flashflags, flashdest,
     trymmap, mmapaddr, mmapsize, mmapflags, mmapdest) \
     = struct.unpack("<I256sIII8sIIIIIII", binary[start + 12 : start + 320])
    if "reset" in args and args["reset"]:
	tryfile = 0
	filename = "\0" * 256
	fileflags = 0
	filedest = 0
	tryflash = 0
	flashname = "\0" * 8
	flashflags = 0
	flashdest = 0
	trymmap = 0
	mmapaddr = 0
	mmapsize = 0
	mmapflags = 0
	mmapdest = 0
    if "tryfile" in args: tryfile = 1 if args["tryfile"] else 0
    if "filename" in args: filename = args["filename"].ljust(256, "\0")
    if "filecomp" in args:
        if args["filecomp"]: fileflags = fileflags | 1
        else: fileflags = fileflags & ~1
    if "filecopy" in args:
        if args["filecopy"]: fileflags = fileflags | 2
        else: fileflags = fileflags & ~2
    if "filedest" in args: filedest = args["filedest"]
    if "tryflash" in args: tryflash = 1 if args["tryflash"] else 0
    if "flashname" in args: flashname = args["flashname"].ljust(8)
    if "flashcomp" in args:
        if args["flashcomp"]: flashflags = flashflags | 1
        else: flashflags = flashflags & ~1
    if "flashcopy" in args:
        if args["flashcopy"]: flashflags = flashflags | 2
        else: flashflags = flashflags & ~2
    if "flashdest" in args: flashdest = args["flashdest"]
    if "trymmap" in args: trymmap = 1 if args["trymmap"] else 0
    if "mmapaddr" in args: mmapaddr = args["mmapaddr"]
    if "mmapsize" in args: mmapsize = args["mmapsize"]
    if "mmapcomp" in args:
        if args["mmapcomp"]: mmapflags = mmapflags | 1
        else: mmapflags = mmapflags & ~1
    if "mmapcopy" in args:
        if args["mmapcopy"]: mmapflags = mmapflags | 2
        else: mmapflags = mmapflags & ~2
    if "mmapdest" in args: mmapdest = args["mmapdest"]
    data = struct.pack("<I256sIII8sIIIIIII", tryfile, filename, fileflags, filedest,
                                             tryflash, flashname, flashflags, flashdest,
                                             trymmap, mmapaddr, mmapsize, mmapflags, mmapdest)
    return binary[:start + 12] + data + binary[start + 320:]
