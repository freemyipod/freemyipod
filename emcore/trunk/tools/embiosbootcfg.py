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

import sys
import libembiosbootcfg
from optparse import *

parser = OptionParser("usage: %prog [options] <infile> <outfile>")
filegroup = OptionGroup(parser, "Booting from a file",
                        "Use these options to configure emBIOS to try "
		        "booting from a file on a FAT32 partition")
filegroup.add_option("--file", help = "Boot from FILE")
filegroup.add_option("--file-compressed", action = "store_true", default = False,
                     help = "Specify this if FILE is compressed")
filegroup.add_option("--file-run-from", type = "int", metavar = "ADDR",
                     help = "Make sure FILE is executed from memory address ADDR")
parser.add_option_group(filegroup)
flashgroup = OptionGroup(parser, "Booting from a boot flash image",
                         "Use these options to configure emBIOS to try "
                         "booting from an image located in the boot flash")
flashgroup.add_option("--flash", metavar = "NAME", help = "Boot from flash image NAME")
flashgroup.add_option("--flash-compressed", action = "store_true", default = False,
                      help = "Specify this if the image is compressed")
flashgroup.add_option("--flash-run-from", type = "int", metavar = "ADDR",
                      help = "Make sure the image is executed from memory address ADDR")
parser.add_option_group(flashgroup)
mmapgroup = OptionGroup(parser, "Booting from a memory region",
                        "Use these options to configure emBIOS to try "
                        "booting from a memory location, such as an embedded "
			"app or an app located on a memory-mapped flash")
mmapgroup.add_option("--mmap-addr", metavar = "ADDR", help = "Boot from memory location ADDR")
mmapgroup.add_option("--mmap-size", metavar = "SIZE",
                     help = "Specifies the size of the executable at ADDR in bytes")
mmapgroup.add_option("--mmap-compressed", action = "store_true", default = False,
                     help = "Specify this if the executable is compressed")
mmapgroup.add_option("--mmap-run-from", type = "int", metavar = "COPYADDR",
                     help = "Copy the executable to COPYADDR before executing it")
parser.add_option_group(mmapgroup)
(options, args) = parser.parse_args()
if len(args) != 2: parser.error("incorrect number of arguments")

if (options.mmap_addr and not options.mmap_size) or \
   (not options.mmap_addr and options.mmap_size):
    parser.error("either none or both of --mmap-addr and --map-size must be specified")

file = open(args[0], "rb")
data = file.read()
file.close()

config = {"reset": True}
if options.file:
    config["tryfile"] = True
    config["filename"] = options.file
    if options.file_compressed: config["filecomp"] = True
    if options.file_run_from:
        config["filecopy"] = True
        config["filedest"] = options.file_run_from
if options.flash:
    config["tryflash"] = True
    config["flashname"] = options.flash
    if options.flash_compressed: config["flashcomp"] = True
    if options.flash_run_from:
        config["flashcopy"] = True
        config["flashdest"] = options.flash_run_from
if options.mmap_addr:
    config["trymmap"] = True
    config["mmapaddr"] = options.mmap_addr
    config["mmapsize"] = options.mmap_size
    if options.mmap_compressed: config["mmapcomp"] = True
    if options.mmap_run_from:
        config["mmapcopy"] = True
        config["mmapdest"] = options.mmap_run_from

data = libembiosbootcfg.configure(data, **config)

file = open(args[1], "wb")
file.write(data)
file.close()
