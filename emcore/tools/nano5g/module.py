#!/usr/bin/env python3
"""Pack or load a nano5g SDRAM module."""
import argparse
import pathlib
import struct
import zlib

MAGIC, ABI, MAX_SIZE = 0x4d47354e, 1, 2 * 1024 * 1024
# ABI 2 appends peripheral leases (module5g.h); only a lease kernel activates it.
ABIS = (1, 2)
HEADER = struct.Struct('<10I')
from .protocol import Cmd
INFO, RESERVE, ACTIVATE = Cmd.MODULE_INFO, Cmd.MODULE_RESERVE, Cmd.MODULE_ACTIVATE


def parse_module(data):
    if len(data) < HEADER.size or len(data) > MAX_SIZE:
        raise ValueError('invalid module file size')
    h = HEADER.unpack_from(data)
    magic, abi, kind, text, bss, entry, command, heartbeat, count, reserved = h
    if (magic, kind, reserved) != (MAGIC, 1, 0) or abi not in ABIS:
        raise ValueError('unsupported module format, ABI or kind')
    if text < 4 or text % 4 or text + bss > MAX_SIZE or len(data) != HEADER.size + text + count * 4:
        raise ValueError('invalid module section sizes')
    if any(off % 4 or off >= text for off in (entry, command, heartbeat)):
        raise ValueError('invalid module entry points')
    previous = -1
    for (off,) in struct.iter_unpack('<I', data[HEADER.size + text:]):
        if off % 4 or off > text - 4 or off <= previous:
            raise ValueError('invalid or duplicate relocation')
        if struct.unpack_from('<I', data, HEADER.size + off)[0] > text + bss:
            raise ValueError('relocation points outside module')
        previous = off
    return h


def load(session, data, expected_caps=()):
    """Load once and (optionally) check the module's command-128 capability prefix.

    Callers with an extensible registry can check magic/version here, then
    validate the advertised registry count and layout when they read it.
    The default still checks all three capability words exactly.
    """
    h = parse_module(data)
    state, _, _ = session.cmd(INFO)
    if state:
        raise ValueError('a module is already staged/loaded; reset before loading another')
    status = session.status()
    if status.busy or status.jobs_started:
        raise ValueError('load the module before the NAND ladder; reset emCORE first')
    size = max(len(data), h[3] + h[4])
    rc, addr, cap = session.cmd(RESERVE, size)
    if rc or cap != size or addr % 32 or not 0x08000000 <= addr < 0x0c000000 - size:
        raise ValueError('module allocation rejected (rc=%d)' % rc)
    session.write_memory(addr, data)
    if session.read(addr, len(data)) != data:
        raise ValueError('module upload read-back mismatch; reset before retrying')
    rc, _, _ = session.cmd(ACTIVATE, len(data), zlib.crc32(data) & 0xffffffff)
    if rc:
        raise ValueError('module activation rejected (rc=%d); reset before retrying' % rc)
    if session.cmd(INFO)[0] != 2 or session.cmd(128)[:len(expected_caps)] != expected_caps:
        raise ValueError('module did not publish the expected capabilities')
    return addr


def pack_elf(data, abi=ABI):
    if data[:7] != b'\x7fELF\x01\x01\x01':
        raise ValueError('expected little-endian ELF32')
    h = struct.unpack_from('<HHIIIIIHHHHHH', data, 16)
    if h[0:2] != (2, 40):
        raise ValueError('expected linked ARM executable')
    shoff, shsize, shcount, strindex = h[5], h[10], h[11], h[12]
    if shsize != 40:
        raise ValueError('invalid section table')
    sections = [struct.unpack_from('<10I', data, shoff + i * shsize) for i in range(shcount)]
    def contents(s):
        out = data[s[4]:s[4] + s[5]]
        if len(out) != s[5]: raise ValueError('truncated ELF section')
        return out
    strings = contents(sections[strindex])
    def string(table, off):
        return table[off:table.index(0, off)].decode('ascii')
    names = {string(strings, s[0]): i for i, s in enumerate(sections)}
    ti, bi = names['.text'], names['.bss']
    text, bss = sections[ti], sections[bi]
    if text[3] or bss[3] != text[5]:
        raise ValueError('module must have contiguous text/BSS starting at zero')
    if any(s[2] & 2 and i not in (ti, bi) and s[5] for i, s in enumerate(sections)):
        raise ValueError('unexpected allocated ELF section')
    payload = contents(text)
    total = text[5] + bss[5]
    symbols, symbol_tables = {}, {}
    for i, s in enumerate(sections):
        if s[1] != 2: continue
        if s[9] != 16: raise ValueError('invalid symbol table')
        table = contents(sections[s[6]])
        entries = list(struct.iter_unpack('<IIIBBH', contents(s)))
        symbol_tables[i] = entries
        for name, value, size, info, other, index in entries[1:]:
            if index == 0: raise ValueError('undefined symbol: ' + string(table, name))
            if name: symbols[string(table, name)] = value
    relocs = []
    for s in sections:
        if s[1] != 9: continue
        if not sections[s[7]][2] & 2: continue  # Non-allocated debug metadata.
        if s[7] != ti: raise ValueError('relocations outside text')
        if s[9] != 8: raise ValueError('invalid relocation table')
        for off, info in struct.iter_unpack('<II', contents(s)):
            kind, sym = info & 255, info >> 8
            symbol = symbol_tables[s[6]][sym]
            if kind == 2:  # R_ARM_ABS32
                if symbol[5] not in (ti, bi) or off % 4 or off > len(payload) - 4:
                    raise ValueError('non-local or unaligned ABS32 relocation')
                if struct.unpack_from('<I', payload, off)[0] > total:
                    raise ValueError('relocated address outside module')
                relocs.append(off)
            elif kind in (1, 3, 28, 29):  # PC24, REL32, CALL, JUMP24
                if symbol[5] not in (ti, bi):
                    raise ValueError('relative reference outside module')
            elif kind not in (0, 40):  # NONE, V4BX (linker already fixed instruction)
                raise ValueError('unsupported ARM relocation %d' % kind)
    relocs.sort()
    if not relocs or len(relocs) != len(set(relocs)):
        raise ValueError('missing or duplicate relocations; link with --emit-relocs')
    header = HEADER.pack(MAGIC, abi, 1, len(payload), bss[5], symbols['n5g_module_start'],
                         symbols['n5g_nand_usb_command'], symbols['n5g_nand_heartbeat'], len(relocs), 0)
    out = header + payload + struct.pack('<%dI' % len(relocs), *relocs)
    parse_module(out)
    return out


def main(argv=None):
    p = argparse.ArgumentParser(description=__doc__)
    sub = p.add_subparsers(dest="operation", required=True)
    pack = sub.add_parser("pack", help="pack a zero-linked ARM ELF with relocations")
    pack.add_argument("elf", type=pathlib.Path)
    pack.add_argument("-o", "--output", type=pathlib.Path, required=True)
    pack.add_argument("--abi", type=int, choices=ABIS, default=ABI)
    loader = sub.add_parser("load", help="upload and activate a module")
    loader.add_argument("file", type=pathlib.Path)
    sub.add_parser("info", help="show module loader state")
    a = p.parse_args(argv)
    if a.operation == "pack":
        blob = pack_elf(a.elf.read_bytes(), a.abi)
        a.output.write_bytes(blob)
        print("%d bytes -> %s" % (len(blob), a.output))
        return 0
    from .nand import make_write_session
    session = make_write_session(ep0=True)
    try:
        if a.operation == "info":
            print("state=%d address=0x%08x size=%d" % session.cmd(INFO))
        else:
            addr = load(session, a.file.read_bytes())
            print("module active at 0x%08x" % addr)
    finally:
        session.close()
    return 0
