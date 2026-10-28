#!/usr/bin/env python3
# Copyright (C) 2026 Vikotr Krapivin
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

"""Inspect or update the nano5g private partition's FirmwareStatus.

The PRVD header tag encrypts the first 16 bytes of its SHA-1 digest using
hardware AES selector 2 in CBC mode with a zero IV. NAND access uses the
shared worker protocol and writer.
"""
import argparse
import hashlib
import hmac
import os
import struct
import sys

from . import nand as N
from . import partitions
from .aes import Engine

SECTOR = N.BLOCK_BYTES
PRIVATE_TYPE = 0x3e
PRIVATE_SECTORS = 4
PREFIX = struct.pack('<4I', 0x44565250, 2, 0, 4)   # 'PRVD', version 2, 0, 4
STATUS_OFF, TAG_OFF = 0x40, 0x80


class LogicalReader:
    """Reads through the harness; needs the FTL mounted (step 4) and an idle monitor."""
    def __init__(self, session):
        self.session = session
        st = session.status()
        if st.busy or not st.blockio_blocks or not st.step_ok_mask & (1 << 4):
            raise N.ToolError('finish n5g nand init --ftl first; the monitor must be idle')
        self.total = st.blockio_blocks

    def read(self, lba, count):
        if lba < 0 or count < 1 or lba + count > self.total:
            raise N.ToolError('logical read outside the FTL')
        if self.session.status().busy:
            raise N.ToolError('another monitor client has queued a job')
        _, data = N.logical_read(self.session, lba, count, False)
        if len(data) != count * SECTOR:
            raise N.ToolError('short logical read')
        return bytes(data)


def private_lba(mbr, total):
    """The private partition's first LBA from the raw MBR; the layout is checked, never assumed."""
    m = partitions.parse_mbr(mbr)
    if not m.valid:
        raise N.ToolError('MBR signature missing')
    parts = [p for p in m.partitions if p.type]
    private = [p for p in parts if p.type == PRIVATE_TYPE]
    if len(private) != 1 or private[0].size != PRIVATE_SECTORS:
        raise N.ToolError('expected exactly one four-sector private partition (type 0x3e)')
    end = 1  # LBA 0 is the MBR, not part of a partition
    for part in sorted(parts, key=lambda p: p.start):
        if not part.size or part.start < end or part.start + part.size > total:
            raise N.ToolError('invalid, overlapping or out-of-range MBR partitions')
        end = part.start + part.size
    return private[0].start


def integrity_tag(prefix, engine):
    if len(prefix) != TAG_OFF:
        raise ValueError('private header digest requires exactly 0x80 bytes')
    digest = hashlib.sha1(prefix).digest()[:16]
    tag, _ = engine.encrypt(digest, keysel=2, iv=bytes(16))
    if len(tag) != 16:
        raise N.ToolError('short hardware AES tag')
    return tag


def verify_header(sector, engine):
    if len(sector) != SECTOR or sector[:16] != PREFIX:
        raise N.ToolError('expected a complete PRVD v2 sector; legacy/malformed headers are not writable here')
    if not hmac.compare_digest(integrity_tag(sector[:TAG_OFF], engine), sector[TAG_OFF:TAG_OFF + 16]):
        raise N.ToolError('private-header integrity tag mismatch; NAND left unchanged')
    return struct.unpack_from('<I', sector, STATUS_OFF)[0]


def build_header(engine, status):
    """OSOS 0x08191ae4: zero one sector, prefix, u32 status, tag over the first 0x80 bytes."""
    if status not in (0, 2):
        raise ValueError('supported FirmwareStatus values are 0 and 2')
    sector = bytearray(SECTOR)
    sector[:16] = PREFIX
    struct.pack_into('<I', sector, STATUS_OFF, status)
    sector[TAG_OFF:TAG_OFF + 16] = integrity_tag(sector[:TAG_OFF], engine)
    return bytes(sector)


def inspect(reader, engine):
    mbr = reader.read(0, 1)
    if len(mbr) != SECTOR:
        raise N.ToolError('short MBR read')
    lba = private_lba(mbr, reader.total)
    sector = reader.read(lba, 1)
    return mbr, lba, sector, verify_header(sector, engine)


def set_status(reader, engine, writer_factory, status=2):
    """Return (LBA, old status, changed). No retries or implicit device resets."""
    if status not in (0, 2):
        raise ValueError('supported FirmwareStatus values are 0 and 2')
    mbr, lba, old_sector, old_status = inspect(reader, engine)
    if old_status == status:
        return lba, old_status, False
    writer = writer_factory()  # checks the writer capability and the ladder before anything is opened
    new_sector = build_header(engine, status)
    # All crypto is done before write-open, which may itself commit FTL metadata.
    writer.open()
    if writer.read(0, 1) != mbr or writer.read(lba, 1) != old_sector:
        raise N.ToolError('MBR/private header changed since inspection; slot write cancelled')
    writer.write(lba, new_sector)
    writer.flush()
    writer.verify(lba, new_sector)
    return lba, old_status, True


def describe(lba, status, sector):
    bit0, bit1 = status & 1, (status >> 1) & 1
    return ('private partition at LBA %d (%d sectors); PRVD v2 header, tag verified with AES selector 2\n'
            'FirmwareStatus 0x%08x: bit0 %d, bit1 %d (%s), higher bits 0x%x\n'
            'tag %s' % (lba, PRIVATE_SECTORS, status, bit0, bit1,
                        'Bds skips the rsrc validation request' if bit1 else 'no Bds shortcut',
                        status >> 2, sector[TAG_OFF:TAG_OFF + 16].hex()))


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = parser.add_subparsers(dest='command', required=True)
    sub.add_parser('show', help='read the private header and verify its tag with the device AES')
    setter = sub.add_parser('set', help='assign status 0 or 2, flush and read back; no interactive prompt')
    setter.add_argument('value', type=lambda s: int(s, 0), choices=(0, 2))
    args = parser.parse_args(argv)
    session = engine = None
    try:
        session = N.make_write_session(ep0=True)
        reader = LogicalReader(session)
        engine = Engine(session.em)
        if args.command == 'show':
            _, lba, sector, status = inspect(reader, engine)
            print(describe(lba, status, sector))
        else:
            lba, old, changed = set_status(reader, engine, lambda: N.Writer(session), args.value)
            if changed:
                print('FirmwareStatus at LBA %d: 0x%08x -> 0x%08x (written, flushed, read back)' % (lba, old, args.value))
            else:
                print('FirmwareStatus at LBA %d already 0x%08x; nothing written' % (lba, old))
    except (N.ToolError, RuntimeError, ValueError) as e:
        print('ERROR: %s' % e, file=sys.stderr)
        return 1
    finally:
        if engine is not None:
            try:
                engine.close()
            except Exception as e:  # the result above stands; the leak is reported
                print('warning: AES buffer not freed: %s' % e, file=sys.stderr)
        if session is not None:
            session.close()
    return 0


if __name__ == '__main__':
    sys.exit(main())
