#!/usr/bin/env python3
# Copyright (C) 2026 Vikotr Krapivin (standalone bundle extraction and adaptation)
#
# This file is part of emCORE.
#
# emCORE is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 2 of the License, or
# (at your option) any later version.
#
# emCORE is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with emCORE. If not, see <http://www.gnu.org/licenses/>.

"""NAND ID, partition and firmware-directory decoding."""
import struct
LPN_BYTES = 4096

class Bunch(dict):
    __getattr__ = dict.__getitem__

def u32(b, off):
    return struct.unpack_from("<I", b, off)[0]

def parse_mbr(sector):
    """MBR at LBA 0 (LBA = 4 KiB FTL page): 4 entries at +0x1be, signature 55 AA at +0x1fe."""
    if len(sector) < 512:
        raise ValueError("sector too short")
    sig = sector[0x1fe] == 0x55 and sector[0x1ff] == 0xAA
    parts = []
    for i in range(4):
        e = sector[0x1be + 16 * i: 0x1be + 16 * (i + 1)]
        parts.append(Bunch(index=i, active=e[0], type=e[4], start=u32(e, 8), size=u32(e, 12)))
    return Bunch(valid=sig, partitions=parts)

STOP_SIGN_PREFIX = b"{{~~  /-----\\"

def is_stop_sign(block):
    """Firmware partition block 0 starts with the 0x7c-byte iPod 'STOP sign' text."""
    return block[:len(STOP_SIGN_PREFIX)] == STOP_SIGN_PREFIX

def fourcc(v):
    """Directory ids are u32 LE whose big-endian characters spell the name ('osos' = 0x6f736f73)."""
    return struct.pack(">I", v).decode("latin-1")

def fw_dir_offset(block0):
    """dirOffset = u32 at +0x104; the directory sits at partition byte offset dirOffset + 4096."""
    return u32(block0, 0x104)

def parse_fw_directory(buf):
    """Up to 40 entries of 0x28 bytes, ending at the first entry whose word 0 is 0."""
    out = []
    for i in range(40):
        off = i * 0x28
        if off + 0x28 > len(buf):
            break
        w0 = u32(buf, off)
        if w0 == 0:
            break
        out.append(Bunch(index=i, type=w0, id=u32(buf, off + 4), name=fourcc(u32(buf, off + 4)),
                         devoffset=u32(buf, off + 0xc), length=u32(buf, off + 0x10), addr=u32(buf, off + 0x14),
                         entryoffset=u32(buf, off + 0x18), checksum=u32(buf, off + 0x1c),
                         version=u32(buf, off + 0x20), loadaddr=u32(buf, off + 0x24)))
    return out

def fw_file_location(part_start_lpn, entry):
    """(first lpn, byte offset in that page, length) of a file's data: partition offset devOffset + 4096."""
    off = part_start_lpn * LPN_BYTES + entry.devoffset + LPN_BYTES
    return off // LPN_BYTES, off % LPN_BYTES, entry.length

def hexdump(data, base=0, width=16, limit=None):
    lines = []
    data = data if limit is None else data[:limit]
    for i in range(0, len(data), width):
        chunk = data[i:i + width]
        asc = "".join(chr(c) if 32 <= c < 127 else "." for c in chunk)
        lines.append("%06x  %-*s %s" % (base + i, width * 3, " ".join("%02x" % c for c in chunk), asc))
    return "\n".join(lines)

NAND_MAKERS = {0xad: "Hynix", 0x98: "Toshiba", 0xec: "Samsung", 0x2c: "Micron", 0x89: "Intel", 0x45: "SanDisk", 0x01: "Spansion/AMD"}

def decode_nand_id(word):
    """Decode a 4-byte READ ID answer (little-endian word as the harness stores it) using the legacy Samsung/Hynix/
    Toshiba ID-byte convention (3rd/4th byte layouts; vendor datasheets agree on these bits for 2K/4K/8K parts).
    Returns a list of (field, value) pairs; everything beyond maker/device is marked 'legacy layout'."""
    b = struct.pack("<I", word)
    if word in (0, 0xffffffff):
        return [("present", "no chip (bus %s)" % ("floating high" if word else "low"))]
    maker, dev, b3, b4 = b[0], b[1], b[2], b[3]
    dies = 1 << (b3 & 3)
    levels = 1 << ((b3 >> 2 & 3) + 1)                       # 2 = SLC, 4 = MLC, 8 = TLC
    simul = 1 << (b3 >> 4 & 3)
    page_kib = 1 << (b4 & 3)
    spare_per_512 = 16 if (b4 & 4) else 8
    block_kib = 64 << (b4 >> 4 & 3)
    width = 16 if (b4 & 0x40) else 8
    out = [("maker", "0x%02x %s" % (maker, NAND_MAKERS.get(maker, "unknown"))),
           ("device code", "0x%02x" % dev),
           ("dies per CE", "%u (byte3[1:0])" % dies),
           ("cell type", "%u-level (%s) (byte3[3:2])" % (levels, {2: "SLC", 4: "MLC", 8: "TLC"}.get(levels, "?"))),
           ("simult. pages", "%u (byte3[5:4])" % simul),
           ("interleave/cache", "%s/%s (byte3[6]/[7])" % ("yes" if b3 & 0x40 else "no", "yes" if b3 & 0x80 else "no")),
           ("page size", "%u KiB (byte4[1:0])" % page_kib),
           ("spare", "%u B per 512 B -> %u B per page (byte4[2])" % (spare_per_512, spare_per_512 * page_kib * 2)),
           ("block size", "%u KiB -> %u pages (byte4[5:4])" % (block_kib, block_kib // page_kib)),
           ("bus width", "x%u (byte4[6])" % width)]
    return out

def auto_int(v):
    return int(v, 0)
