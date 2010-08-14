#!/usr/bin/env python
#
#
#    Copyright 2010 TheSeven
#
#
#    This file is part of iLoader.
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
#    with iLoader.  If not, see <http://www.gnu.org/licenses/>.
#
#


# 0 = nop
# 1 = terminate
# 2 = undefined
# 3 = readflash <addr> <*filename>
# 4 = readfile <addr> <*filename>
# 5 = script <baseaddr> <entrypoint>
# 6 = error <*addr>
# 7 = jmp <*addr>
# 8 = button <*addrcenter> <*addrright> <*addrleft> <*addrplay> <*addrmenu> <timeout>
# 9 = menu <*entries> <selected> <*addrright> <*addrleft> <*addrplay> <*addrmenu> <timeout>
# a = fillrect <x> <y> <width> <height> <color>
# b = text <x> <y> <fgcolor> <bgcolor> <*text>
# c = displaybmp <x> <y> <addr>
# d = blit
# e = backlight <state> <brightness> <fade>
# f = poweroff
# 10 = rbchecksum <addr>
# 11 = sleep <microseconds>
# 12 = clockgate <gateid> <onoff>
# 13 = exec <addr>
# 14 = unpackucl <src> <dest>

# <*text> <x> <y> <fgcolor> <bgcolor> <fgactive> <bgactive> <*addr>


import sys
import struct

if len(sys.argv) != 3:
  print "Syntax: compileconfig.py <scriptfile> <binfile>"
  exit(2)

opcodes = {"terminate":     (1, 0), \
           "readflash":     (3, 2), \
           "readfile":      (4, 2), \
           "script":        (5, 2), \
           "error":         (6, 1), \
           "jmp":           (7, 1), \
           "button":        (8, 6), \
           "menu":          (9, 7), \
           "fillrect":      (10, 5), \
           "text":          (11, 5), \
           "displaybmp":    (12, 3), \
           "blit":          (13, 0), \
           "backlight":     (14, 3), \
           "poweroff":      (15, 0), \
           "rbchecksum":    (16, 1), \
           "sleep":         (17, 1), \
           "clockgate":     (18, 2), \
           "exec":          (19, 1), \
           "unpackucl":     (20, 2), \
           ".word":         (-1, 1), \
           ".ascii":        (-2, 1), \
           ".menuentry":    (-3, 8)}

def error(linenum, line, message):
  print("Error at line %d: %s" % (linenum, line))
  print(message)
  exit(1)

def parseerror(linenum, line, expected, got):
  error(linenum, line, "Parse error: expected %s, got \"%s\"" % (expected, got))

labels =  {}
strings =  {}
binary = ""
xrefs = []
collect = ""
state = 0
linenum = 1
line = ""

file = open(sys.argv[1], "r")

while True:
  char = file.read(1)
  if len(char) == 0: break
  if char == "\n":
    line = ""
    linenum = linenum + 1
  else: line = line + char
  if char == "#" and not state in (6, 7):
    file.readline()
    line = ""
    linenum = linenum + 1
  elif state == 0:
    if char == " " or char == "\n" or char == "\r" or char == "\t": pass
    elif (char >= "A" and char <= "Z") or (char >= "a" and char <= "z") \
        or (char >= "0" and char <= "9") or char == "_" or char == ".":
      collect = char
      state = 1
    else: parseerror(linenum, line, "token", char)
  elif state == 1:
    if (char >= "A" and char <= "Z") or (char >= "a" and char <= "z") \
        or (char >= "0" and char <= "9") or char == "_" or char == ".":
      collect = collect + char
    elif char == "(":
      try: opcode = opcodes[collect]
      except: parseerror(linenum, line, "instruction", collect)
      if opcode[0] >= 0: binary = binary + struct.pack("<I", opcode[0]);
      argcount = 0
      state = 3
    elif char == ":":
      labels[collect] = len(binary)
      state = 0
    elif char == " " or char == "\n" or char == "\r" or char == "\t":
      state = 2
    else: parseerror(linenum, line, "\" \" or \":\"", char)
  elif state == 2:
    if char == " " or char == "\n" or char == "\r" or char == "\t": pass
    elif char == "(":
      try: opcode = opcodes[collect]
      except: parseerror(linenum, line, "instruction", collect)
      if opcode[0] >= 0: binary = binary + struct.pack("<I", opcode[0]);
      argcount = 0
      state = 3
    elif char == ":":
      labels[collect] = len(binary)
      state = 0
    else: parseerror(linenum, line, "\"(\" or \":\"", char)
  elif state == 3:
    if char == " " or char == "\n" or char == "\r" or char == "\t": pass
    elif char == "\"":
      collect = ""
      state = 6
    elif char == ")":
      if argcount != opcode[1]:
        error(linenum, line, "Wrong argument count: expected %d, got %d" % (opcode[1], argcount))
      state = 0
    elif (char >= "A" and char <= "Z") or (char >= "a" and char <= "z") \
        or (char >= "0" and char <= "9") or char == "_":
      collect = char
      state = 4
    else: parseerror(linenum, line, "argument", char)
  elif state == 4:
    if char == " " or char == "\n" or char == "\r" or char == "\t":
      state = 5
    elif char == ",":
      try: collect = int(collect)
      except:
        try: collect = int(collect, 16)
        except:
          xrefs.append((len(binary), 0, collect))
          collect = 0
      if opcode[0] != -2: binary = binary + struct.pack("<I", collect);
      argcount = argcount + 1;
      state = 3
    elif char == ")":
      try: collect = int(collect)
      except:
        try: collect = int(collect, 16)
        except:
          xrefs.append((len(binary), 0, collect))
          collect = 0
      if opcode[0] != -2: binary = binary + struct.pack("<I", collect);
      if argcount + 1 != opcode[1]:
        error(linenum, line, "Wrong argument count: expected %d, got %d" % (opcode[1], argcount))
      state = 0
    elif (char >= "A" and char <= "Z") or (char >= "a" and char <= "z") \
        or (char >= "0" and char <= "9") or char == "_":
      collect = collect + char
    else: parseerror(linenum, line, "immediate", char)
  elif state == 5:
    if char == " " or char == "\n" or char == "\r" or char == "\t": pass
    elif char == ",":
      try: collect = int(collect)
      except:
        try: collect = int(collect, 16)
        except:
          xrefs.append((len(binary), 0, collect))
          collect = 0
      if opcode[0] != -2: binary = binary + struct.pack("<I", collect);
      argcount = argcount + 1;
      state = 3
    elif char == ")":
      try: collect = int(collect)
      except:
        try: collect = int(collect, 16)
        except:
          xrefs.append((len(binary), 0, collect))
          collect = 0
      if opcode[0] != -2: binary = binary + struct.pack("<I", collect);
      if argcount + 1 != opcode[1]:
        error(linenum, line, "Wrong argument count: expected %d, got %d" % (opcode[1], argcount))
      state = 0
    else: parseerror(linenum, line, "\",\" or \")\"", char)
  elif state == 6:
    if char == "\"":
      if opcode[0] == -2: binary = binary + collect
      else:
        strings[collect] = 0
        xrefs.append((len(binary), 1, collect))
        collect = "0"
      state = 5
    elif char == "\\":
      state = 7
    else:
      collect = collect + char
  elif state == 7:
    if char == "0": collect = collect + "\0"
    else: collect = collect + char
    state = 6

for string in strings.keys():
  strings[string] = len(binary)
  binary = binary + string

for xref in xrefs:
  if xref[1] == 1: target = strings[xref[2]]
  else: target = labels[xref[2]]
  binary = binary[:xref[0]] + struct.pack("<I", target) + binary[xref[0] + 4:]
  
file.close()
file = open(sys.argv[2], "wb")
file.write(binary)
file.close()
