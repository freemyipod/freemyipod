#!/usr/bin/env python3
"""Read, inspect and write nano5g NAND through the emCORE worker protocol."""
import argparse
import os
import struct
import sys
import time
import zlib

from . import session as transport
from . import partitions
from .session import ToolError
hexdump = partitions.hexdump

# ------------------------------------------------------------------------------------------- constants (nand5g.h)
MAGIC, VERSION, STATUS_BYTES = 0x4e47354e, 1, 0x250
STATE_NAMES = {0: "IDLE", 1: "QUEUED", 2: "RUNNING", 3: "DONE"}
RESULT_NAMES = {0: "OK", 1: "LIB_ERROR", 2: "PRECOND", 3: "BAD_ARG", 4: "NO_MEMORY", 5: "PANIC"}
ACCEPT_NAMES = {0: "OK", 1: "BUSY", 2: "BAD_ARG", 3: "NOT_INIT"}
JOB_NAMES = {0: "none", 1: "step", 2: "read_rom", 3: "read_page", 4: "ftl_read", 6: "block_read",
             7: "write_open", 8: "block_write", 9: "flush"}
STEP_NAMES = {1: "clock gates 5/9", 2: "NAND pad snapshot", 3: "driver init (pins, READ ID, chip table)", 4: "FTL mount"}
GEOM_NAMES = ("pages_per_block", "num_ce", "vendor", "blocks_per_ce", "bytes_per_page", "spare_per_page", "layout",
              "refresh_threshold", "ecc_bits", "sector_bytes", "boot_page_bytes", "chipid", "dies_per_ce",
              "die_block_stride", "total_blocks", "fmc_timing_word")
# Rockbox driver codes (nand-target.h) as the harness stores them (uint32)
LIB_RC_NAMES = {0: "ok", 1: "erased", 0xfffffffe: "ECC (uncorrectable)", 0xfffffffd: "TIMEOUT", 0xfffffffc: "NOCHIP",
                0xfffffffb: "UNSUPPORTED layout", 0xfffffffa: "FTL", 0xfffffff9: "WRITE"}
ROM_RC_NAMES = {0: "BLANK (erased sector seen)", 1: "OK", 3: "UNCORRECTABLE", 5: "TIMEOUT"}
from .protocol import Cmd

CMD_STATUS, CMD_STEP = Cmd.NAND_STATUS, Cmd.NAND_STEP
CMD_READ_ROM, CMD_READ_PAGE = Cmd.NAND_READ_ROM, Cmd.NAND_READ_PAGE
CMD_FTL_OPEN, CMD_FTL_READ = Cmd.NAND_FTL_OPEN, Cmd.NAND_FTL_READ
CMD_BLOCK_READ = Cmd.NAND_BLOCK_READ
CMD_WRITE_CAPS, CMD_WRITE_OPEN = Cmd.NAND_WRITE_CAPS, Cmd.NAND_WRITE_OPEN
CMD_BLOCK_WRITE, CMD_FLUSH = Cmd.NAND_BLOCK_WRITE, Cmd.NAND_FLUSH
WRITE_PROTOCOL = 1
RP_DATA, RP_META, RP_MAXCORR, RP_SECSTAT, RP_BYTES = 0x0, 0x2000, 0x2010, 0x2020, 0x2040
BLOCK_BYTES = 4096
RESULT_BYTES = 256 * 1024

# status record layout: (name, struct format); must match struct n5g_nand_status (size 0x250)
_FIELDS = [("magic", "I"), ("version", "I"), ("size", "I"), ("busy", "I"), ("state", "I"), ("step_reached", "I"),
           ("step_ok_mask", "I"), ("jobs_started", "I"), ("jobs_done", "I"), ("job_kind", "I"), ("job_arg", "3I"),
           ("job_result", "I"), ("job_lib_rc", "I"), ("job_detail", "4I"), ("job_t_start", "I"), ("job_t_end", "I"),
           ("progress_done", "I"), ("progress_total", "I"), ("step_status", "8I"), ("chip_ids", "8I"), ("geom", "16I"),
           ("ftl_pages", "I"), ("ftl_bpp", "I"), ("blockio_blocks", "I"), ("result_addr", "I"), ("result_cap", "I"),
           ("result_len", "I"), ("clk_mhz", "I"), ("pwrcon0_before", "I"), ("pwrcon0_after", "I"), ("gpio_after", "3I"),
           ("fil_pages", "I"), ("fil_uecc", "I"), ("fil_timeouts", "I"), ("alloc_bytes", "I"), ("alloc_count", "I"),
           ("alloc_fail", "I"), ("panic_count", "I"), ("panic_line", "I"), ("panic_t_us", "I"), ("panic_file", "32s"),
           ("wmr_asserts", "I"), ("err_job_seq", "I"), ("snap_valid", "I"), ("fmc_snap", "24I"), ("ecc_snap", "6I"),
           ("fmss_snap", "28I"), ("now_us", "I"), ("hb_word", "I"), ("reserved", "I")]
_FMT = "<" + "".join(f for _, f in _FIELDS)
assert struct.calcsize(_FMT) == STATUS_BYTES


class Bunch(dict):
    __getattr__ = dict.__getitem__


def parse_status(raw):
    """Decode the 0x250-byte n5g_nand_status record into a Bunch (arrays become tuples)."""
    if len(raw) < STATUS_BYTES:
        raise ToolError("status record too short (%d bytes)" % len(raw))
    vals = struct.unpack(_FMT, raw[:STATUS_BYTES])
    out, i = Bunch(), 0
    for name, f in _FIELDS:
        n = int(f[:-1] or 1)
        if f[-1] == "s":
            out[name] = vals[i].split(b"\0")[0].decode("ascii", "replace")
            i += 1
        elif n == 1:
            out[name] = vals[i]
            i += 1
        else:
            out[name] = tuple(vals[i:i + n])
            i += n
    return out


def decode_secstat(b):
    """Per-512-B-sector verdict byte: 0..16 corrected bits, 0xfe erased, 0xff uncorrectable (include/fil.h)."""
    if b == 0xfe:
        return "erased"
    if b == 0xff:
        return "UNCORRECTABLE"
    return "%d bit(s) corrected" % b if b else "clean"


def format_status(st):
    L = ["magic %08x version %d size 0x%x" % (st.magic, st.version, st.size),
         "job #%d/%d %s state %s busy %d result %s lib_rc 0x%08x (%s) args %s detail %s" % (
             st.jobs_done, st.jobs_started, JOB_NAMES.get(st.job_kind, st.job_kind), STATE_NAMES.get(st.state, st.state),
             st.busy, RESULT_NAMES.get(st.job_result, st.job_result), st.job_lib_rc,
             LIB_RC_NAMES.get(st.job_lib_rc, "?"), " ".join("%x" % v for v in st.job_arg),
             " ".join("%x" % v for v in st.job_detail)),
         "steps ok mask 0x%x reached %d: %s" % (st.step_ok_mask, st.step_reached, ", ".join(
             "%d=%s" % (i, "-" if v == 0xffffffff else "RUN" if v == 0xfffffffe else "ok" if v == 0 else "0x%x" % v)
             for i, v in enumerate(st.step_status) if i)),
         "FIL pages %d uecc %d timeouts %d; alloc %d B in %d (fail %d); panics %d%s; wmr asserts %d" % (
             st.fil_pages, st.fil_uecc, st.fil_timeouts, st.alloc_bytes, st.alloc_count, st.alloc_fail, st.panic_count,
             (" last %s:%d" % (st.panic_file, st.panic_line)) if st.panic_count else "", st.wmr_asserts),
         "FTL pages %d x %d B, BlockIo blocks %d; result buffer 0x%08x cap 0x%x len 0x%x; clk %d MHz" % (
             st.ftl_pages, st.ftl_bpp, st.blockio_blocks, st.result_addr, st.result_cap, st.result_len, st.clk_mhz)]
    if st.pwrcon0_before or st.pwrcon0_after:
        L.append("PWRCON0 before 0x%08x after 0x%08x; GPIO c0/e0/100 %s" % (
            st.pwrcon0_before, st.pwrcon0_after, " ".join("%08x" % v for v in st.gpio_after)))
    if st.snap_valid:
        L.append("register snapshot of failed job #%d:" % st.err_job_seq)
        L.append("  FMC  000: " + " ".join("%08x" % v for v in st.fmc_snap))
        L.append("  ECC  800: " + " ".join("%08x" % v for v in st.ecc_snap))
        L.append("  FMSS c00: " + " ".join("%08x" % v for v in st.fmss_snap))
    return "\n".join(L)


# ------------------------------------------------------------------------------------------- sessions
class Session5G:
    """Transport independent part: needs cmd(c, a, b, d) -> (w1, w2, w3), read(addr, size) -> bytes, sleep(s), now() -> s."""
    settle = 0.05
    poll = 0.05

    def status(self):
        addr, size, _ = self.cmd(CMD_STATUS)
        if size != STATUS_BYTES:
            raise ToolError("device status size 0x%x != tool 0x%x (rebuild/update)" % (size, STATUS_BYTES))
        st = parse_status(self.read(addr, size))
        if st.magic != MAGIC or st.version != VERSION:
            raise ToolError("status record magic/version %08x/%d not N5GN/%d" % (st.magic, st.version, VERSION))
        return st

    def job(self, c, a=0, b=0, timeout=10.0, progress=False, d=0):
        """Queue one job, poll until it is done. Raises ToolError (after printing the record) on a stuck job."""
        self.sleep(self.settle)
        acc, job_id, _ = self.cmd(c, a, b, d)
        if acc != 0:
            raise ToolError("command %d not accepted: %s" % (c, ACCEPT_NAMES.get(acc, acc)))
        t0 = self.now()
        last = -1
        while True:
            self.sleep(self.poll)
            st = self.status()
            if st.jobs_done == job_id and st.state == 3:
                return st
            if progress and st.progress_done != last:
                last = st.progress_done
                print("  ... %s %d/%d  (FIL pages %d, %.0f s)" % (STATE_NAMES.get(st.state), st.progress_done,
                      st.progress_total, st.fil_pages, self.now() - t0), file=sys.stderr)
            if self.now() - t0 > timeout:
                print(format_status(st), file=sys.stderr)
                raise ToolError("job %d not done after %.0f s (FIL pages %d)" % (job_id, timeout, st.fil_pages))

    def result(self, st, n):
        return self.read(st.result_addr, n)

    def read(self, addr, size):          # chunked READ_MEM (<= 0xff0 per control transfer)
        out = b""
        while size:
            n = min(size, 0xf00)
            out += self.read_chunk(addr, n)
            addr += n
            size -= n
        return out


def make_dev_session(**kw):
    class DevSession(Session5G, transport.Session):
        def __init__(self, settle=0.05, poll=0.05, ep0=True, bulk_write=False):
            self.settle, self.poll = settle, poll
            transport.Session.__init__(self, settle=settle, poll=poll, ep0=ep0, bulk_write=bulk_write)

        def cmd(self, c, a=0, b=0, d=0):
            return transport.Session.cmd(self, c, a, b, d)

        def read(self, addr, size):
            # libemcore splits control reads itself and batches bulk reads.
            return transport.Session.read(self, addr, size)

        def write_memory(self, addr, data):
            return transport.Session.write_memory(self, addr, data)

        def no_retry(self):
            return self

        def sleep(self, s):
            time.sleep(s)

        def now(self):
            return time.time()
    return DevSession(**kw)


def make_write_session(**kw):
    """Device session for mutating commands: EP0 by default, no USB replay."""
    s = make_dev_session(**kw)
    return s.no_retry()


def check(st, ok_lib=(0,)):
    if st.job_result != 0 or st.job_lib_rc not in ok_lib:
        print(format_status(st), file=sys.stderr)
        raise ToolError("job failed: %s, lib rc 0x%08x" % (RESULT_NAMES.get(st.job_result, st.job_result), st.job_lib_rc))
    return st


class LogicalReader:
    """Bounded logical-sector reads shared by NAND and partition tools."""
    def __init__(self, session):
        self.s = session
        st = session.status()
        if st.busy or not st.blockio_blocks or not st.step_ok_mask & (1 << 4):
            raise ToolError("finish n5g.py nand init --ftl first; the monitor must be idle")
        self.total = st.blockio_blocks
        self.maximum = min(RESULT_BYTES, st.result_cap) // BLOCK_BYTES
        if self.maximum < 1:
            raise ToolError("NAND result buffer is smaller than one logical sector")

    def bounds(self, lba, count):
        if lba < 0 or count < 1 or lba >= self.total or count > self.total - lba:
            raise ValueError("logical sector range %d+%d outside the FTL (%d sectors)" % (lba, count, self.total))

    def read(self, lba, count):
        self.bounds(lba, count)
        out = bytearray()
        while count:
            n = min(count, self.maximum)
            if self.s.status().busy:
                raise ToolError("another monitor client has queued a job")
            st = check(self.s.job(CMD_BLOCK_READ, lba, n, timeout=120.0))
            data = self.s.result(st, n * BLOCK_BYTES)
            if len(data) != n * BLOCK_BYTES:
                raise ToolError("short NAND read")
            out.extend(data)
            lba += n
            count -= n
        return bytes(out)


class Writer(LogicalReader):
    """Logical 4 KiB-sector writes through commands 128..131 (Rockbox ftl_write / ftl_flush inside the kernel).
    Needs a session with write_memory(addr, data) and the read-only ladder up to the FTL mount (step 4). Data is
    staged in the harness result buffer and checked by CRC-32 on the device before the first NAND operation."""
    def __init__(self, s):
        self.s = s
        protocol, flags, maximum = s.cmd(CMD_WRITE_CAPS)
        if protocol != WRITE_PROTOCOL:
            raise ToolError("writer protocol %d on the device, tool expects %d (rebuild/update)" % (protocol, WRITE_PROTOCOL))
        if not flags & 1:
            raise ToolError("this kernel was built read-only (N5G_NAND_WRITE=0): no writer")
        if not 1 <= maximum <= RESULT_BYTES // BLOCK_BYTES:
            raise ToolError("implausible writer chunk limit %d" % maximum)
        st = s.status()
        if st.busy or not st.blockio_blocks or not st.step_ok_mask & (1 << 4):
            raise ToolError("finish the read-only ladder (n5g.py nand init --ftl) first; the monitor must be idle")
        self.total, self.maximum, self.staging = st.blockio_blocks, maximum, st.result_addr
        self.opened = False

    def open(self):
        if not self.opened:
            check(self.s.job(CMD_WRITE_OPEN, timeout=600.0))
            self.opened = True

    def write(self, lba, data):
        if not data or len(data) % BLOCK_BYTES:
            raise ValueError("write payload must contain whole %d-byte sectors" % BLOCK_BYTES)
        self.bounds(lba, len(data) // BLOCK_BYTES)
        self.open()
        for off in range(0, len(data), self.maximum * BLOCK_BYTES):
            chunk = data[off:off + self.maximum * BLOCK_BYTES]
            if self.s.status().busy:
                raise ToolError("another monitor client has queued a job")
            self.s.write_memory(self.staging, chunk)
            checksum = zlib.crc32(chunk) & 0xffffffff   # the device checks this before touching the NAND
            check(self.s.job(CMD_BLOCK_WRITE, lba + off // BLOCK_BYTES, len(chunk) // BLOCK_BYTES, d=checksum, timeout=600.0))

    def flush(self):
        if self.opened:
            check(self.s.job(CMD_FLUSH, timeout=600.0))

    def verify(self, lba, data):
        for off in range(0, len(data), self.maximum * BLOCK_BYTES):
            chunk = data[off:off + self.maximum * BLOCK_BYTES]
            if self.read(lba + off // BLOCK_BYTES, len(chunk) // BLOCK_BYTES) != chunk:
                raise ToolError("read-back mismatch at LBA %d" % (lba + off // BLOCK_BYTES))


# ------------------------------------------------------------------------------------------- subcommands
def cmd_status(s, a):
    st = s.status()
    print(format_status(st))
    return st


def cmd_step(s, a):
    st = check(s.job(CMD_STEP, a.n, a.arg, timeout=a.timeout or 120.0, progress=a.n == 4))
    print("step %d (%s): ok, took %d us" % (a.n, STEP_NAMES.get(a.n, "?"), (st.job_t_end - st.job_t_start) & 0xffffffff))
    return st


def cmd_init(s, a):
    for n in (1, 2, 3, 4)[:4 if a.ftl else 3]:
        a.n, a.arg = n, 0
        cmd_step(s, a)
    return cmd_id(s, a)


def cmd_id(s, a):
    st = s.status()
    if not st.step_ok_mask & (1 << 3):
        print("step 3 (FIL init) has not succeeded; IDs below are whatever the last attempt left", file=sys.stderr)
    for ce, v in enumerate(st.chip_ids):
        print("CE%d: %08x  bytes %s" % (ce, v, " ".join("%02x" % x for x in struct.pack("<I", v))))
    first = next((v for v in st.chip_ids if v not in (0, 0xffffffff)), None)
    if first is not None:
        print("decoded ID of the first populated CE (legacy ID-byte layout; the driver table below is authoritative):")
        for k, val in partitions.decode_nand_id(first):
            print("  %-18s %s" % (k, val))
    print("driver geometry (chip table, FIL_GetDeviceInfo):")
    for n, v in zip(GEOM_NAMES, st.geom):
        extra = "   <- cfg word (platform configuration row)" if n == "config" else \
                "   <- FMSS param RAM 0xd4c: FMC_CTRL0 read timing bits" if n == "fmc_timing_word" else ""
        print("  %-18s 0x%x (%d)%s" % (n, v, v, extra))
    print("FMC clock handed to FIL_CalcTiming: %d MHz" % st.clk_mhz)
    return st


def cmd_romread(s, a):
    st = check(s.job(CMD_READ_ROM, a.page, 1 if a.reset else 0), ok_lib=(1,))
    data = s.result(st, 0x600)
    print("ROM-format read of bank 0 page %d: %s" % (a.page, ROM_RC_NAMES.get(st.job_lib_rc, st.job_lib_rc)))
    if a.output:
        open(a.output, "wb").write(data)
        print("wrote %s (0x600 bytes)" % a.output)
    else:
        print(hexdump(data, limit=256))


def cmd_rawread(s, a):
    st = check(s.job(CMD_READ_PAGE, a.bank, a.page), ok_lib=(0, 1))
    bpp = st.geom[4]
    r = s.result(st, RP_BYTES)
    data, meta = r[RP_DATA:RP_DATA + bpp], r[RP_META:RP_META + 12]
    nsec = bpp // 512
    sec = r[RP_SECSTAT:RP_SECSTAT + nsec]
    print("bank %d page %d: rc 0x%x (%s), maxcorr byte %d, meta %s" % (
        a.bank, a.page, st.job_lib_rc, LIB_RC_NAMES.get(st.job_lib_rc, "?"), r[RP_MAXCORR], meta.hex()))
    print("ECC verdict per 512-B sector: " + ", ".join("%d:%s" % (i, decode_secstat(b)) for i, b in enumerate(sec)))
    if a.output:
        open(a.output, "wb").write(data + meta)
        print("wrote %s (%d data + 12 meta)" % (a.output, bpp))
    else:
        print(hexdump(data, limit=256))
    return st, data, meta, sec


def cmd_ftl_open(s, a):
    a.n, a.arg = 4, 0
    return cmd_step(s, a)


def logical_read(s, lba, count, ftl):
    st = s.status()
    if not st.step_ok_mask & (1 << 4):
        raise ToolError("FTL not open: run `ftl-open` first")
    unit = st.ftl_bpp if ftl else BLOCK_BYTES
    if count * unit > RESULT_BYTES:
        raise ToolError("count %d x %d B exceeds the %d KiB result buffer" % (count, unit, RESULT_BYTES // 1024))
    st = check(s.job(CMD_FTL_READ if ftl else CMD_BLOCK_READ, lba, count, timeout=120.0))
    return st, s.result(st, count * unit)


def cmd_lread(s, a):
    st, data = logical_read(s, a.lba, a.count, a.ftl)
    unit = st.ftl_bpp if a.ftl else BLOCK_BYTES
    if a.output:
        open(a.output, "wb").write(data)
        print("wrote %s (%d x %d B)" % (a.output, a.count, unit))
    else:
        print(hexdump(data, base=a.lba * unit, limit=512))
    return data


def cmd_dump(s, a):
    st = s.status()
    if not st.step_ok_mask & (1 << 4):
        raise ToolError("FTL not open: run `ftl-open` first")
    unit = st.ftl_bpp if a.ftl else BLOCK_BYTES
    total = st.ftl_pages if a.ftl else st.blockio_blocks
    count = a.count if a.count is not None else total - a.start
    if a.start + count > total:
        raise ToolError("range %d+%d exceeds the disk (%d units of %d B)" % (a.start, count, total, unit))
    per = max(1, min(a.chunk, RESULT_BYTES // unit))
    done, t0 = 0, time.time()
    with open(a.output, "wb") as f:
        while done < count:
            n = min(per, count - done)
            _, data = logical_read(s, a.start + done, n, a.ftl)
            f.write(data)
            done += n
            print("  %d/%d units (%.1f%%)" % (done, count, 100.0 * done / count), file=sys.stderr)
    print("wrote %s: units %d..%d (%d B, %.1f s)" % (a.output, a.start, a.start + count - 1, count * unit, time.time() - t0))


def format_private_header(block):
    """Decode area 0 as read by PrivateData PE RVA 0x420; no tag verification.

    FirmwareStatus is a raw disk field, not the bootloader's cached GetVariable
    result. Legacy headers return zero regardless of the bytes at +0x40.
    """
    if len(block) < 512:
        return "private header: truncated (need the first 512 bytes)"
    words = struct.unpack_from("<4I", block)
    if words[:3] == (1, 0, 4):
        return ("private header: legacy {1, 0, 4}\n"
                "    FirmwareStatus: bootloader returns 0; bytes at +0x40 are ignored")
    if words != (0x44565250, 2, 0, 4):
        return "private header: unsupported prefix (expected legacy {1, 0, 4} or PRVD {2, 0, 4})"
    status = struct.unpack_from("<I", block, 0x40)[0]
    if status & 1:
        policy = "reject normal OS attempt (0x8000000a); bit 0 takes precedence"
    elif status & 2:
        policy = "allow this policy check without requesting rsrc validation info"
    else:
        policy = "request rsrc validation info before allowing the normal OS attempt"
    return ("private header: PRVD version 2, format fields {0, 4}\n"
            "    FirmwareStatus (+0x40, raw u32): 0x%08x\n"
            "    bit 0 (restore-in-progress / boot rejection): %d\n"
            "    bit 1 (resource-validation shortcut): %d\n"
            "    higher bits (not tested by Bds_CheckBoot): 0x%08x\n"
            "    Bds policy if header accepted and ChipId selects this path: %s\n"
            "    integrity tag (+0x80, 16 bytes): %s (NOT verified)" %
            (status, status & 1, (status >> 1) & 1, status & 0xfffffffc,
             policy, block[0x80:0x90].hex()))


def cmd_mbr(s, a):
    st, lba0 = logical_read(s, 0, 1, False)
    m = partitions.parse_mbr(lba0)
    print("MBR signature %s" % ("ok" if m.valid else "MISSING"))
    for p in m.partitions:
        print("  part %d: type 0x%02x active 0x%02x start lba %d size %d (%.1f MiB)" % (
            p.index, p.type, p.active, p.start, p.size, p.size * 4096 / 1048576.0))
    for p in m.partitions:
        if p.size == 0:
            continue
        _, blk = logical_read(s, p.start, 1, False)
        if not partitions.is_stop_sign(blk):
            print("  part %d: first bytes %s" % (p.index, blk[:16].hex()))
            if p.type == 0x3e:
                print("  part %d: %s" % (p.index, format_private_header(blk)))
            continue
        diroff = partitions.fw_dir_offset(blk)
        first = p.start + (diroff + BLOCK_BYTES) // BLOCK_BYTES
        _, d = logical_read(s, first, 2, False)
        o = (diroff + BLOCK_BYTES) % BLOCK_BYTES
        ents = partitions.parse_fw_directory(d[o:o + 0x640])
        print("  part %d: firmware partition (STOP sign), dirOffset 0x%x, %d entries" % (p.index, diroff, len(ents)))
        for e in ents:
            lpn, off, ln = partitions.fw_file_location(p.start, e)
            print("    %-4s type 0x%08x devOffset 0x%08x len 0x%08x load 0x%08x -> block %d +0x%x" % (
                e.name, e.type, e.devoffset, e.length, e.addr, lpn, off))


def cmd_write_caps(s, a):
    protocol, flags, maximum = s.cmd(CMD_WRITE_CAPS)
    print("writer protocol %d, writer %s, up to %d sectors (%d KiB) per write command" % (
        protocol, "built in" if flags & 1 else "NOT built in (read-only kernel)", maximum, maximum * BLOCK_BYTES // 1024))
    return protocol, flags, maximum


def cmd_write_open(s, a):
    w = Writer(s)
    w.open()
    print("FTL open for writing (%d sectors x %d B); nothing written" % (w.total, BLOCK_BYTES))
    return w


def cmd_lwrite(s, a):
    data = open(a.file, "rb").read()
    if not data or len(data) % BLOCK_BYTES:
        raise ToolError("%s: %d bytes is not a whole number of %d-byte sectors" % (a.file, len(data), BLOCK_BYTES))
    w = Writer(s)
    n = len(data) // BLOCK_BYTES
    w.bounds(a.lba, n)
    print("writing %d sector(s) at LBA %d..%d from %s" % (n, a.lba, a.lba + n - 1, a.file))
    w.write(a.lba, data)
    if not a.no_flush:
        w.flush()
        print("flushed")
    if not a.no_verify:
        w.verify(a.lba, data)
        print("read back: identical")
    return w


def cmd_flush(s, a):
    w = Writer(s)
    w.opened = True      # flush is harmless when nothing was written; the device answers PRECOND if never opened
    w.flush()
    print("flushed")
    return w


HANDLERS = {"status": cmd_status, "step": cmd_step, "init": cmd_init, "id": cmd_id, "romread": cmd_romread,
            "rawread": cmd_rawread, "ftl-open": cmd_ftl_open, "lread": cmd_lread, "dump": cmd_dump, "mbr": cmd_mbr,
            "write-caps": cmd_write_caps, "write-open": cmd_write_open,
            "lwrite": cmd_lwrite, "flush": cmd_flush}
WRITE_COMMANDS = ("write-open", "lwrite", "flush")


def build_parser():
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("--settle", type=float, default=0.05, help="seconds to wait before each command (default 0.05)")
    ap.add_argument("--poll", type=float, default=0.05, help="status poll interval in seconds")
    ap.add_argument("--timeout", type=float, default=None, help="step timeout override in seconds")
    ap.add_argument("--bulk", action="store_true", help="allow the bulk-IN endpoint for READ_MEM (default: EP0 only, robust)")
    sub = ap.add_subparsers(dest="cmd", required=True)
    ai = partitions.auto_int
    sub.add_parser("status")
    p = sub.add_parser("step"); p.add_argument("n", type=int); p.add_argument("--arg", type=ai, default=0)
    p = sub.add_parser("init"); p.add_argument("--ftl", action="store_true", help="also open the FTL (step 4, long)")
    sub.add_parser("id")
    p = sub.add_parser("romread"); p.add_argument("page", type=ai); p.add_argument("-o", "--output")
    p.add_argument("--reset", action="store_true", help="fmc_reset_bank(0) first")
    p = sub.add_parser("rawread"); p.add_argument("bank", type=ai); p.add_argument("page", type=ai); p.add_argument("-o", "--output")
    sub.add_parser("ftl-open")
    p = sub.add_parser("lread"); p.add_argument("lba", type=ai); p.add_argument("count", type=ai, nargs="?", default=1)
    p.add_argument("-o", "--output"); p.add_argument("--ftl", action="store_true", help="units = FTL pages instead of 4 KiB blocks")
    p = sub.add_parser("dump"); p.add_argument("-o", "--output", required=True); p.add_argument("--start", type=ai, default=0)
    p.add_argument("--count", type=ai, default=None); p.add_argument("--chunk", type=ai, default=32)
    p.add_argument("--ftl", action="store_true")
    sub.add_parser("mbr")
    sub.add_parser("write-caps")
    sub.add_parser("write-open")
    p = sub.add_parser("lwrite"); p.add_argument("lba", type=ai); p.add_argument("file")
    p.add_argument("--no-flush", action="store_true"); p.add_argument("--no-verify", action="store_true")
    sub.add_parser("flush")
    return ap


def main(argv=None):
    a = build_parser().parse_args(argv)
    s = None
    try:
        if a.cmd in WRITE_COMMANDS:
            s = make_write_session(settle=a.settle, poll=a.poll, ep0=True)
        else:
            s = make_dev_session(settle=a.settle, poll=a.poll, ep0=not a.bulk)
        HANDLERS[a.cmd](s, a)
    except ToolError as e:
        print("ERROR: %s" % e, file=sys.stderr)
        return 1
    finally:
        if s is not None:
            s.close()
    return 0


if __name__ == "__main__":
    sys.exit(main())
