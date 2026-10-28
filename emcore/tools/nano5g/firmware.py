#!/usr/bin/env python3
"""Dump or replace stored nano5g firmware entries without changing their sizes.

Entries such as osos and rsrc are copied as stored, including their headers.
Replacement preserves the MBR, firmware directory, and all bytes outside the
selected entry. No image decoding, signing, or checksum rewriting is performed.
"""
import argparse
import os
from pathlib import Path
import sys
import tempfile

from . import nand as N
from . import partitions

SECTOR = N.BLOCK_BYTES
DIRECTORY_BYTES = 40 * 0x28
CHUNK_BYTES = N.RESULT_BYTES


def read_bytes(reader, offset, length):
    """Read an arbitrary byte range through the sector interface."""
    first, skip = divmod(offset, SECTOR)
    if not length:
        return b""
    data = reader.read(first, (skip + length + SECTOR - 1) // SECTOR)
    return data[skip:skip + length]


class FirmwarePartition:
    def __init__(self, reader, partition=None):
        self.reader = reader
        mbr = partitions.parse_mbr(reader.read(0, 1))
        if not mbr.valid:
            raise N.ToolError("MBR signature missing")
        found = []
        for part in mbr.partitions:
            if not part.size or (partition is not None and part.index != partition):
                continue
            if part.start < 1 or part.start + part.size > reader.total:
                raise N.ToolError("partition %d extends outside the FTL" % part.index)
            header = reader.read(part.start, 1)
            if partitions.is_stop_sign(header):
                found.append((part, header))
        if len(found) != 1:
            raise N.ToolError("found %d firmware partitions; select one with --partition INDEX" % len(found))
        self.part, header = found[0]
        self.base = self.part.start * SECTOR
        self.size = self.part.size * SECTOR
        self.directory = SECTOR + partitions.fw_dir_offset(header)
        if self.directory < SECTOR or self.directory + DIRECTORY_BYTES > self.size:
            raise N.ToolError("firmware directory extends outside its partition")
        raw = read_bytes(reader, self.base + self.directory, DIRECTORY_BYTES)
        self.entries = partitions.parse_fw_directory(raw)
        for entry in self.entries:
            start = SECTOR + entry.devoffset
            if start + entry.length > self.size:
                raise N.ToolError("entry %r extends outside the firmware partition" % entry.name)

    def entry(self, name):
        entries = [e for e in self.entries if e.name == name]
        if len(entries) != 1:
            raise N.ToolError("expected one entry named %r, found %d" % (name, len(entries)))
        return entries[0]

    def chunks(self, entry):
        pos = self.base + SECTOR + entry.devoffset
        remaining = entry.length
        while remaining:
            count = min(remaining, CHUNK_BYTES - pos % SECTOR)
            yield pos, count
            pos += count
            remaining -= count

    def dump(self, entry, output):
        output = Path(output)
        temporary = None
        try:
            with tempfile.NamedTemporaryFile(dir=output.parent, prefix=".n5g-", delete=False) as stream:
                temporary = Path(stream.name)
                for pos, count in self.chunks(entry):
                    stream.write(read_bytes(self.reader, pos, count))
            os.replace(temporary, output)
        finally:
            if temporary is not None and temporary.exists():
                temporary.unlink()

    def replace(self, entry, source, writer_factory, progress=None):
        start = SECTOR + entry.devoffset
        end = start + entry.length
        if entry.length and (start < SECTOR or (start < self.directory + DIRECTORY_BYTES and end > self.directory)):
            raise N.ToolError("entry overlaps the partition header or firmware directory")
        for other in self.entries:
            other_start = SECTOR + other.devoffset
            if other.index != entry.index and other.length and start < other_start + other.length and end > other_start:
                raise N.ToolError("entry overlaps %r" % other.name)
        with open(source, "rb") as stream:
            size = os.fstat(stream.fileno()).st_size
            if size != entry.length:
                raise N.ToolError("replacement is %d bytes; %s requires exactly %d bytes (no resizing)" %
                                  (size, entry.name, entry.length))
            if not size:
                return
            if progress:
                progress("writing", 0, size)
            writer = writer_factory()
            done = 0
            # Preserve neighboring bytes in partial first/last sectors.
            for pos, count in self.chunks(entry):
                data = stream.read(count)
                if len(data) != count:
                    raise N.ToolError("replacement file became shorter while reading")
                lba, skip = divmod(pos, SECTOR)
                sectors = (skip + count + SECTOR - 1) // SECTOR
                if skip or count % SECTOR:
                    payload = bytearray(writer.read(lba, sectors))
                    payload[skip:skip + count] = data
                else:
                    payload = data
                writer.write(lba, payload)
                writer.verify(lba, payload)
                done += count
                if progress:
                    progress("writing", done, size)
            if stream.read(1):
                raise N.ToolError("replacement file grew while reading")
            if progress:
                progress("flushing", 0, size)
            writer.flush()
            # Verify the requested contents again after publishing the checkpoint.
            stream.seek(0)
            done = 0
            if progress:
                progress("verifying", 0, size)
            for pos, count in self.chunks(entry):
                if read_bytes(writer, pos, count) != stream.read(count):
                    raise N.ToolError("firmware read-back mismatch at byte offset 0x%x" % pos)
                done += count
                if progress:
                    progress("verifying", done, size)


def replacement_progress(phase, done, total):
    if phase == "flushing":
        print("  flushing...", file=sys.stderr, flush=True)
    else:
        print("  %s: %d/%d bytes (%.1f%%)" %
              (phase, done, total, 100.0 * done / total), file=sys.stderr, flush=True)


def dump_name(entry, entries):
    name = entry.name
    if not name or name in (".", "..") or any(c not in "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_.-" for c in name):
        name = "%08x" % entry.id
    if sum(e.name == entry.name for e in entries) > 1:
        name = "%02d-%s" % (entry.index, name)
    return name + ".bin"


def build_parser():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--partition", type=int, choices=range(4), help="MBR partition index (otherwise detect automatically)")
    sub = parser.add_subparsers(dest="command", required=True)
    sub.add_parser("list", help="list firmware entry names, sizes and offsets")
    p = sub.add_parser("dump", help="dump one entry, including its stored headers")
    p.add_argument("name"); p.add_argument("-o", "--output", required=True)
    p = sub.add_parser("dump-all", help="dump every entry to a directory")
    p.add_argument("directory")
    p = sub.add_parser("replace", help="overwrite one entry with an exactly equal-size file, flush and verify")
    p.add_argument("name"); p.add_argument("file")
    transport = p.add_mutually_exclusive_group()
    transport.add_argument("--bulk", dest="bulk", action="store_true", help="use USB bulk uploads and read-back transfers (default)")
    transport.add_argument("--control", dest="bulk", action="store_false", help="use the previous control-transfer mode with 64-byte uploads")
    p.set_defaults(bulk=True)
    return parser


def main(argv=None):
    args = build_parser().parse_args(argv)
    session = None
    try:
        if args.command == "replace":
            session = N.make_write_session(ep0=not args.bulk, bulk_write=args.bulk)
        else:
            session = N.make_dev_session(ep0=True)
        fw = FirmwarePartition(N.LogicalReader(session), args.partition)
        if args.command == "list":
            print("firmware partition %d: LBA %d, %d sectors" % (fw.part.index, fw.part.start, fw.part.size))
            for entry in fw.entries:
                print("%-4s %10d bytes  partition offset 0x%08x  checksum 0x%08x" %
                      (entry.name, entry.length, SECTOR + entry.devoffset, entry.checksum))
        elif args.command == "dump":
            entry = fw.entry(args.name)
            fw.dump(entry, args.output)
            print("dumped %s: %d bytes to %s" % (entry.name, entry.length, args.output))
        elif args.command == "dump-all":
            directory = Path(args.directory)
            directory.mkdir(parents=True, exist_ok=True)
            for entry in fw.entries:
                output = directory / dump_name(entry, fw.entries)
                fw.dump(entry, output)
                print("dumped %s: %d bytes to %s" % (entry.name, entry.length, output))
        else:
            entry = fw.entry(args.name)
            fw.replace(entry, args.file, lambda: N.Writer(session), progress=replacement_progress)
            print("replaced %s: %d bytes, flushed and verified" % (entry.name, entry.length))
    except (N.ToolError, ValueError, OSError) as error:
        print("ERROR: %s" % error, file=sys.stderr)
        return 1
    finally:
        if session is not None:
            session.close()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
