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
import os
import struct

bitmaps = ["sidepane.ucl", "warning.ucl", "installing.ucl", \
           "formatting.ucl", "copying.ucl", "flashing.ucl"]

flashfiles = [("ildrcfg ", 2, 0, 0, "iloader.cfg.ucl"), \
              ("iloader ", 2, 0, 0, "iloader.embiosapp.ucl"), \
              ("umsboot ", 2, 0, 0, "umsboot-ipodclassic.ucl"), \
              ("uninst  ", 2, 0, 0, "uninstaller-classic.embiosapp.ucl"), \
              ("embiosldr", 12, 8, 0, "embiosldr-ipodclassic.bin"), \
              ("embios  ", 2, 0, 0, "embios-ipodclassic.ucl")]

firstinstfiles = [(1, "/iLoader", 1), \
                  (2, "/iLoader/iLoader.cfg", "../iloader/themes/ipodclassic-default/iLoader/iloader.cfg", 1), \
                  (2, "/iLoader/theme.ucl", "../iloader/themes/ipodclassic-default/iLoader/theme.ucl", 2)]

commonfiles = [(2, "/iLoader/NORFlash.bak", -2, 3)]

if len(sys.argv) > 4 and sys.argv[4] != "-":
    pathlen = len(sys.argv[4])
    for d in os.walk(sys.argv[4]):
        prefix = d[0].replace("\\", "/")[pathlen:] + "/"
        for dir in d[1]:
            if dir != ".svn":
                firstinstfiles.append((1, prefix + dir, 1))
        for f in d[2]:
            if not prefix.find("/.svn/") > -1:
                firstinstfiles.append((2, prefix + f, d[0] + "/" + f, os.path.getsize(d[0] + "/" + f) / 500000 + 1))

file = open(sys.argv[1], "rb")
installer = file.read()
file.close()
installer = installer.ljust((len(installer) + 3) & ~3, '\0')

for f in bitmaps:
  file = open("build/" + f, "rb")
  fdata = file.read()
  file.close()
  fdata = fdata.ljust((len(fdata) + 3) & ~3, '\0')
  installer = installer + struct.pack("<I", len(fdata)) + fdata

statusfirst = 0
statuscommon = 0
scriptsize = 20

for f in flashfiles:
  scriptsize = scriptsize + 4
  if (f[1] & 4) == 0: scriptsize = scriptsize + 8
  if f[3] == 0: scriptsize = scriptsize + 8

for f in firstinstfiles:
  scriptsize = scriptsize + 12
  if f[0] > 1: scriptsize = scriptsize + 8

for f in commonfiles:
  scriptsize = scriptsize + 12
  if f[0] > 1: scriptsize = scriptsize + 8

scriptsize = ((len(installer) + scriptsize + 15) & ~15) - len(installer)

filedata = ""
flash = ""
for f in flashfiles:
  flash = flash + struct.pack("BBBB", f[3], f[1], f[2], 1)
  if f[3] == 0:
    file = open("flashfiles/" + f[4], "rb")
    fdata = file.read()
    file.close()
    flash = flash + struct.pack("<II", scriptsize + len(filedata), len(fdata))
    filedata = filedata + fdata.ljust((len(fdata) + 15) & ~15, '\0')
  if (f[1] & 4) == 0: flash = flash + f[0]

firstinstall = ""
for f in firstinstfiles:
  size = 0
  nameptr = scriptsize + len(filedata)
  filedata = filedata + (f[1] + "\0").ljust((len(f[1]) + 16) & ~15, '\0')
  if f[0] == 1:
    firstinstall = firstinstall + struct.pack("<III", f[0], nameptr, f[2])
    statusfirst = statusfirst + f[2]
  else:
    if type(f[2]) == str:
      file = open(f[2], "rb")
      fdata = file.read()
      file.close()
      ptr = scriptsize + len(filedata)
      size = len(fdata)
      filedata = filedata + fdata.ljust((len(fdata) + 15) & ~15, '\0')
    else:
      ptr = f[2]
    firstinstall = firstinstall + struct.pack("<IIiII", f[0], nameptr, ptr, size, f[3])
    statusfirst = statusfirst + f[3]

common = ""
for f in commonfiles:
  size = 0
  nameptr = scriptsize + len(filedata)
  filedata = filedata + (f[1] + "\0").ljust((len(f[1]) + 16) & ~15, '\0')
  if f[0] == 1:
    common = common + struct.pack("<III", f[0], nameptr, f[2])
    statuscommon = statuscommon + f[2]
  else:
    if type(f[2]) == str:
      file = open("../iloader/themes/default/iloader/" + f[2], "rb")
      fdata = file.read()
      file.close()
      ptr = scriptsize + len(filedata)
      size = len(fdata)
      filedata = filedata + fdata.ljust((len(fdata) + 15) & ~15, '\0')
    else:
      ptr = f[2]
    common = common + struct.pack("<IIiII", f[0], nameptr, ptr, size, f[3])
    statuscommon = statuscommon + f[3]

script = flash + struct.pack("<IIII", 0, len(flash) + 16 + len(firstinstall), \
                             statusfirst, statuscommon) \
       + firstinstall + common + struct.pack("<I", 0)
file = open(sys.argv[2], "wb")
file.write(installer + script.ljust(scriptsize, '\0') + filedata)
file.close()
