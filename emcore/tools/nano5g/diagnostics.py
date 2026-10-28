#!/usr/bin/env python3
"""Run a nano5g hardware diagnostic by name, or list available diagnostics.
Results and console output are saved to the selected log and report files.
Symbols are read from the kernel ELF produced by this repository.
"""
import argparse
import json
import os
import struct
import subprocess
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
DEFAULT_ELF = os.path.join(ROOT, "build", "ipodnano5g", "emcore.elf")
HWTYPE_N5G = 0x47354e49
USEC = 0x3c7000b4
CLK = 0x3c500000
MIU = 0x3d700000
OTG = 0x38400000
PHY = 0x3c400000
CHIPID = 0x3d100000
LCD = 0x38300000
DMAC0 = 0x38200000
L1 = 0x2202c000
GUARD = 0xaffebeaf
PMU = (0, 0xe6)
ACCEL = (0, 0x32)                 # PROVISIONAL bus 0 (accel.c); WHO_AM_I 0x0f expected 0x12
LCD_MODE_PIXEL, LCD_CLKCON2 = 0x01100db8, 0x80001014
THREAD = struct.Struct("<20IQ9I4B")   # thread.h v3; emCORE uses short enums (one byte each at the end)
STATES = ["free", "suspended", "ready", "running", "blocked", "DEFUNCT", "DEFUNCT_ACK"]
# Assembled routines (arm-none-eabi-as -mcpu=arm1176jzf-s; source in the comments)
STORE = [0xe59f0008, 0xe59f1008, 0xe5801000, 0xe12fff1e, 0, 0]
#   ldr r0,=addr; ldr r1,=value; str r1,[r0]; bx lr                         literals: [4] addr, [5] value
STORE_IMM = lambda v: [0xe59f0008, 0xe3a01000 | v, 0xe5801000, 0xe12fff1e, 0]
#   ldr r0,=addr; mov r1,#v; str r1,[r0]; bx lr                             literal: [4] addr
DCACHE = [0xe3a02000, 0xe59f3028, 0xee072f1a, 0xee171fda, 0xe3110001, 0x0a000001, 0xe2533001, 0x1afffff9,
          0xee072f9a, 0xe59f000c, 0xe5801000, 0xe5803004, 0xe12fff1e, 0x00100000, 0]
#   r3 = 0x100000; loop: clean entire D (c7,c10,0); r1 = dirty status (c7,c10,6); until bit0 = 0 or r3 = 0;
#   drain; result[0] = r1, result[1] = r3                                    literal: [14] result
MEMTEST = [0xe92d00f0, 0xe59f0074, 0xe59f1074, 0xe59f2074, 0xe1a03000, 0xe0234002, 0xe4834004, 0xe1530001,
           0x3afffffb, 0xe3a05000, 0xee075f1e, 0xee075f9a, 0xe1a03000, 0xe3a06000, 0xe3a07000, 0xe5934000,
           0xe0235002, 0xe1540005, 0x0a000002, 0xe2866001, 0xe3570000, 0x01a07003, 0xe2833004, 0xe1530001,
           0x3afffff5, 0xe59f0020, 0xe5806000, 0xe5807004, 0xe59f4018, 0xe5804008, 0xe8bd00f0, 0xe12fff1e,
           0, 0, 0, 0, 0x600dc0de]
#   fill [start,end) with addr^key; clean+invalidate D; verify; result = {errors, first bad, 0x600dc0de}
#   literals: [32] start, [33] end, [34] key, [35] result
SLEEPTEST = [0xe92d4030, 0xe59f302c, 0xe5934000, 0xe59f0028, 0xe59fc028, 0xe12fff3c, 0xe59f3018, 0xe5935000,
             0xe59f001c, 0xe5804000, 0xe5805004, 0xe59f4014, 0xe5804008, 0xe8bd8030, USEC, 0, 0, 0, 0x600dc0de]
#   t0 = usec; sleep(usecs) (the kernel's own); t1 = usec; result = {t0, t1, 0x600dc0de}
#   literals: [15] usecs, [16] &sleep, [17] result
UDF = [0xe7f000f0]
FIPS197 = (bytes(range(16)), bytes.fromhex("00112233445566778899aabbccddeeff"),
           bytes.fromhex("69c4e0d86a7b0430d8cdb78070b4c55a"))   # FIPS-197 C.1, AES-128


def words(ws):
    return struct.pack("<%dI" % len(ws), *ws)


# ---------------------------------------------------------------- transports: one primitive, raw()
from .session import Session as DeviceTransport


class NoAnswer(Exception):
    pass


class Refused(Exception):
    pass


# READ_MEM/WRITE_MEM run inside the USB monitor: an access that faults (unmapped VA, absent RAM, an unclocked
# block) aborts there and the device stops answering (seen in simulation: READ_MEM of 0xfffffffc). Only these
# windows are ever accessed; a rung that probes beyond them says so in its [plan] line and passes allow=True.
SAFE = [(0x00000000, 0x00030000, "vectors (VA 0 -> SRAM)"), (0x08000000, 0x0c000000, "SDRAM 64 MiB"),
        (0x22000000, 0x22030000, "SRAM, known 192 KiB"),
        (CLK, CLK + 0x100, "clocks"), (MIU, MIU + 0x200, "SDRAM controller"), (OTG, OTG + 0x1000, "USB OTG"),
        (PHY, PHY + 0x40, "USB PHY"), (CHIPID, CHIPID + 0x40, "chip id"), (LCD, LCD + 0x100, "LCD"),
        (DMAC0, DMAC0 + 0x200, "DMAC0"), (0x3c700000, 0x3c700200, "timers")]


def safe(addr, size):
    return any(a <= addr and addr + size <= b for a, b, _ in SAFE)


# ---------------------------------------------------------------- symbols
class Elf:
    def __init__(self, path):
        self.path = path
        self.sym = {}
        for line in subprocess.check_output(["arm-none-eabi-nm", "-n", path], text=True).splitlines():
            p = line.split()
            if len(p) == 3:
                self.sym.setdefault(p[2], int(p[0], 16))
        self.funcs = []          # (start, end, name) of FUNC symbols: the bytes that must never change
        self.objects = []
        for line in subprocess.check_output(["arm-none-eabi-readelf", "-sW", path], text=True).splitlines():
            p = line.split()
            if len(p) >= 8 and p[0].rstrip(":").isdigit() and p[3] in ("FUNC", "OBJECT"):
                v, n = int(p[1], 16), int(p[2], 0)
                (self.funcs if p[3] == "FUNC" else self.objects).append((v & ~1, (v & ~1) + max(n, 1), p[7]))
        self.funcs.sort()

    def section(self, name):
        import tempfile
        with tempfile.NamedTemporaryFile() as tmp:
            subprocess.check_call(["arm-none-eabi-objcopy", "-O", "binary", "--only-section=" + name, self.path, tmp.name])
            return open(tmp.name, "rb").read()

    def is_code(self, addr):
        return any(a <= addr < b for a, b, _ in self.funcs)

    def lookup(self, addr):
        for a, b, name in self.funcs + self.objects:
            if a <= addr < b:
                return "%s+%#x" % (name, addr - a)
        return "?"


def align(n, a):
    return (n + a - 1) // a * a


# ---------------------------------------------------------------- the ladder
class Ladder:
    def __init__(self, transport, elf, log=None, out=print):
        self.t, self.elf, self.out = transport, elf, out
        self.log = log
        s = elf.sym
        self.tail = (align(s["_bssend"], 0x1000), s["_lcdfbstart"])             # SDRAM above .bss, below the LCD fb
        self.sramfree = (align(s["_ibssend"], 0x1000), L1)                       # SRAM above .ibss, below the L1 table
        self.facts = {}

    # ---- primitives (each one is itself checked by an early rung)
    def cmd(self, *ws, replen=16, data=b""):
        r = self.t.raw(struct.pack("<%dI" % len(ws), *ws) + data, replen)
        if len(r) < 16 or struct.unpack_from("<I", r)[0] != 1:
            raise NoAnswer("monitor command %d answered %r" % (ws[0], r[:16]))
        return r

    def rd(self, addr, size, allow=False):
        out = b""
        while size:
            n = min(size, 1024)
            r = self.cmd(4, addr, n, 0, replen=16 + n)
            out += r[16:16 + n]
            addr += n
            size -= n
        return out

    def rw(self, addr, n=1, allow=False):
        return struct.unpack("<%dI" % n, self.rd(addr, 4 * n, allow))

    def wr(self, addr, data, allow=False):
        # 16-byte header + at most 48 data bytes = one 64-byte EP0 packet per command: no multi-packet
        # control OUT (unproven on this port)
        for off in range(0, len(data), 48):
            chunk = data[off:off + 48]
            self.cmd(5, addr + off, len(chunk), 0, data=chunk)

    def i2c(self, bus, slave, reg):
        r = self.t.raw(struct.pack("<IBBBBII", 8, bus, slave, reg, 1, 0, 0), 17)
        if struct.unpack_from("<I", r)[0] != 1:
            raise NoAnswer("I2C read answered %r" % r[:16])
        return r[16]

    def flush(self):
        self.cmd(20, 0, 0, 0)

    def thread(self, name_ptr, entry, stack, stacksize, prio=32):
        r = self.cmd(19, name_ptr, entry, stack, stacksize, 1, prio, 2, 0, 0, 0, 0)
        return struct.unpack_from("<I", r, 4)[0]

    def threads(self):
        r = self.cmd(15, 0, 0, 0)
        ver, t = struct.unpack_from("<II", r, 4)
        out, seen = [], set()
        while t and t not in seen and len(seen) < 64:
            seen.add(t)
            if not safe(t, THREAD.size):
                raise Refused("thread list points to %08x" % t)
            f = THREAD.unpack(self.rd(t, THREAD.size))
            name = self.rd(f[18], 24).split(b"\0")[0].decode("ascii", "replace") if safe(f[18], 24) else \
                "(name pointer %08x)" % f[18]
            out.append(dict(addr=t, name=name, state=f[17], pc=f[15], stack=f[28], block=f[30], prio=f[32]))
            t = f[22]
        return ver, out

    def usec(self):
        return self.rw(USEC)[0]

    # ---- the kernel console ring (usbdebug.c dbgconsendbuf, 4 KiB), read without consuming it
    def console_mark(self):
        return self.rw(self.elf.sym["dbgconsendwriteidx"])[0] % 4096

    def console_since(self, mark=None):
        buf = self.rd(self.elf.sym["dbgconsendbuf"], 4096)
        w = self.rw(self.elf.sym["dbgconsendwriteidx"])[0] % 4096
        if mark is None:                      # the whole ring, oldest first
            data = buf[w:] + buf[:w]
        else:
            data = buf[mark:w] if mark <= w else buf[mark:] + buf[:w]
        return [l for l in data.replace(b"\0", b"").decode("ascii", "replace").splitlines() if l.strip()]

    # ---- reporting
    def say(self, s):
        self.out(s)

    def plan(self, s):
        self.say("[plan] " + s)

    def check(self, ok, s):
        self.say(("[PASS] " if ok else "[FAIL] ") + s)
        if not ok:
            self.fails += 1
        return ok

    def info(self, s):
        self.say("[info] " + s)

    def alive(self, what):
        try:
            a = self.usec()
            self.t.sleep(0.05)
            b = self.usec()
        except Exception as e:  # noqa: BLE001
            return self.check(False, "device answers after %s (%r)" % (what, e))
        return self.check(b != a, "device answers after %s, us timer %u -> %u" % (what, a, b))

    def identity(self, quiet=False):
        """Live .sram/.sdram code bytes equal the ELF: the symbols used for writing are the booted kernel's."""
        bad = []
        for name, start, end in ((".sram", "_sramstart", "_sramend"), (".sdram", "_sdramstart", "_sdramend")):
            a, b = self.elf.sym[start], self.elf.sym[end]
            want, live = self.elf.section(name), self.rd(a, b - a)
            code = [i for i in range(0, min(len(want), len(live)), 4)
                    if want[i:i + 4] != live[i:i + 4] and self.elf.is_code(a + i)]
            if code:
                bad.append("%s: %d code words differ, first %08x (%s)" % (name, len(code), a + code[0],
                                                                          self.elf.lookup(a + code[0])))
            elif not quiet:
                self.info("%s %08x..%08x code identical to %s" % (name, a, b, os.path.basename(self.elf.path)))
        return bad

    def scratch(self, region, label):
        """A 4 KiB work block in a free region: code +0, name +0x3c0, result +0x400, stack +0x800 (0x800)."""
        base = region[0]
        self.plan("%s work block %08x: code +0, name +0x3c0, result +0x400, stack +0x800" % (label, base))
        return base

    def run_code(self, base, tag, code, wait_s=1.0, result_words=4, done=None):
        self.wr(base + 0x400, bytes(4 * result_words))
        self.wr(base, words(code))
        self.wr(base + 0x3c0, tag.encode() + b"\0")
        got = self.rd(base, 4 * len(code))
        self.check(got == words(code), "code written and read back at %08x" % base)
        self.plan("FLUSH CACHE (cmd 20)")
        self.flush()
        self.plan("CREATE THREAD '%s' at %08x, stack %08x (prio 32)  <-- the risky step" % (tag, base, base + 0x800))
        tp = self.thread(base + 0x3c0, base, base + 0x800, 0x800)
        self.info("thread struct %08x" % tp)
        deadline = self.t.now() + wait_s
        while True:
            self.t.sleep(0.05)
            res = self.rw(base + 0x400, result_words)
            if (done(res) if done else any(res)) or self.t.now() >= deadline:
                return tp, res

    def thread_gone(self, tag):
        _, ts = self.threads()
        left = [t for t in ts if t["name"] == tag]
        return self.check(not left or left[0]["state"] in (0,), "thread '%s' finished and left the scheduler%s" % (
            tag, "" if not left else " (state %s)" % STATES[left[0]["state"]]))

    # ---------------------------------------------------------------- rungs (read-only)
    def r_identity(self):
        """Booted image = this build; USB monitor, timer and vectors work; no module, so the ladder is first."""
        r = self.cmd(1, 0, 0, 0)
        rev, maj, mino, patch, sw, hw = struct.unpack_from("<IBBBBI", r, 4)
        self.check(hw == HWTYPE_N5G, "GET VERSION: emCORE %d.%d.%d r%d, hwtypeid %08x (nano5g %08x)" % (
            maj, mino, patch, rev, hw, HWTYPE_N5G))
        a = self.usec(); self.t.sleep(0.1); b = self.usec()
        self.check(b != a, "us timer advances (%u -> %u)" % (a, b))
        bad = self.identity()
        self.check(not bad, "kernel code equals the ELF%s" % (": " + "; ".join(bad) if bad else ""))
        va0, sram = self.rd(0, 32), self.rd(0x22000000, 32)
        self.check(va0 == sram and va0 == self.elf.section(".sram")[:32], "vectors at VA 0 = SRAM copy = image")
        st = struct.unpack_from("<I", self.cmd(132, 0, 0, 0), 4)[0]
        self.check(st == 0, "module loader idle (state %d): no module loaded before the ladder" % st)

    def r_bootreport(self):
        """emCORE's own boot records over USB: n5g_init_report (targetinit.c: clocks, gates, SDRAM, CP15 SCTLR,
        PMU at boot) and the whole console log, instead of reading the LCD."""
        for line in self.console_since():
            self.say("[console] " + line)
        r = struct.unpack("<15I8Bi", self.rd(self.elf.sym["n5g_init_report"], 72))
        self.check(r[0] == 0x4947354e, "n5g_init_report magic %08x ('N5GI': targetinit_late ran)" % r[0])
        c0, c1, pll, lock = r[1:5]
        self.check((c0, c1, pll) == (0x40009000, 0x00404040, 0x0900ce01) and lock & 1,
                   "clocks at boot %08x %08x %08x lock %08x" % (c0, c1, pll, lock))
        sctlr = r[14]
        bits = {"M (MMU)": 0, "C (D-cache)": 2, "Z (branch prediction)": 11, "I (I-cache)": 12, "XP (ARMv6 tables)": 23}
        self.info("SCTLR %08x: " % sctlr + ", ".join("%s %d" % (n, sctlr >> b & 1) for n, b in bits.items()))
        self.check(sctlr & 1, "MMU on (crt0)")
        c, i = sctlr >> 2 & 1, sctlr >> 12 & 1
        self.check(c == i, "D- and I-cache both %s (C %d, I %d)" % ("on" if c else "off", c, i))
        if not c:
            self.info("caches off: an N5G_NOCACHE build (the page table rung shows the matching descriptors)")
        pmu = dict(zip((0x02, 0x04, 0x05, 0x0a, 0x30, 0x31, 0x33, 0x34), r[15:23]))
        self.info("PMU at boot " + " ".join("%02x=%02x" % kv for kv in sorted(pmu.items())) +
                  (" (warm flag was set; power_init clears it)" if pmu[0x02] & 0x80 else ""))
        self.facts["boot_pmu"] = pmu
        self.facts["wheel_rc"] = r[23] & 0xffffffff

    def r_wheel(self):
        """clickwheel.c PROVISIONAL query/reply protocol: init result and the poller's counters.
        Turn the wheel or press a button during this rung to see a reply change."""
        rc = struct.unpack("<15I8Bi", self.rd(self.elf.sym["n5g_init_report"], 72))[23] & 0xffffffff
        syms = ("clickwheel_last_reply", "clickwheel_replies", "clickwheel_timeouts")
        before = [self.rw(self.elf.sym[n])[0] for n in syms]
        self.t.sleep(2.0)
        after = [self.rw(self.elf.sym[n])[0] for n in syms]
        self.info("clickwheel_init rc %08x; last reply %08x -> %08x, replies %d -> %d, timeouts %d -> %d" % (
            rc, before[0], after[0], before[1], after[1], before[2], after[2]))
        self.check(rc == 0, "clickwheel_init succeeded (rc %08x)" % rc)
        self.check(after[1] > 0, "the wheel controller has answered (%d replies)" % after[1])

    def r_clocks(self):
        """stage1.S: CPU/bus clocks as programmed; gate words vs stage 1 (open item)."""
        c0, c1, c2 = self.rw(CLK, 3)
        pll, lock = self.rw(CLK + 0x20)[0], self.rw(CLK + 0x40)[0]
        self.check(c0 == 0x40009000, "CLKCON0 %08x (stage1 L8 0x40009000: FCLK = PLL0)" % c0)
        self.check(c1 == 0x00404040, "CLKCON1 %08x (stage1 L7 0x00404040: HCLK = PCLK = FCLK/2)" % c1)
        self.check(pll == 0x0900ce01, "PLL0PMS %08x (24 MHz x 206 / 18 = 274.67 MHz)" % pll)
        self.check(lock & 1, "PLLLOCK %08x bit 0 (locked)" % lock)
        self.check(c2 == LCD_CLKCON2, "CLKCON2 %08x (lcd.c LCD clock 0x80001014)" % c2)
        stage1 = (0x001fa7e5, 0xfc2ae96d, 0x000001f3, 0x000380ff, 0x06dcf779)
        live = [self.rw(CLK + off)[0] for off in (0x48, 0x4c, 0x58, 0x68, 0x6c)]
        for i, (want, got) in enumerate(zip(stage1, live)):
            self.info("PWRCON%d %08x (stage 1 %08x; opened since %08x, closed since %08x; bit set = off)" % (
                i, got, want, want & ~got, got & ~want))

    def r_usec(self, seconds=3.0):
        """timer.c PROVISIONAL: timer 4 CON b6 / PRE 0xb counts 1 MHz. Compared with the host clock."""
        d0 = self.usec()
        h0 = self.t.now()
        self.t.sleep(seconds)
        d1 = self.usec()
        h1 = self.t.now()
        dev, host = (d1 - d0) & 0xffffffff, (h1 - h0) * 1e6
        err = (dev - host) / host
        self.info("device %u us in %.0f host us; includes up to two USB round trips" % (dev, host))
        self.check(abs(err) < 0.02, "us timer rate within 2%% of 1 MHz (%+.2f%%)" % (100 * err))
        tb = self.rw(0x3c700020, 6)
        self.info("timer B (scheduler): CON %08x CMD %08x DATA0 %08x DATA1 %08x PRE %08x CNT %08x" % tb)

    def r_usbcaps(self):
        """target.h PROVISIONAL: FIFO sizes and endpoint set are the 4G's; GHWCFG1..4 say what the core has."""
        h1, h2, h3, h4 = self.rw(OTG + 0x44, 4)
        self.info("GHWCFG1..4 %08x %08x %08x %08x" % (h1, h2, h3, h4))
        num_ep, depth = (h2 >> 10) & 0xf, h3 >> 16
        ded, n_in = (h4 >> 25) & 1, (h4 >> 26) & 0xf
        self.info("device endpoints %d, FIFO depth %d words, dedicated TX FIFOs %s, IN endpoints %d, "
                  "dynamic FIFO %d" % (num_ep, depth, "yes" if ded else "NO (shared)", n_in, (h2 >> 19) & 1))
        # emCORE USB_ENDPOINTS (target.h): IN 1, 3; OUT 2, 4 (plus control 0)
        need = {1: "in", 2: "out", 3: "in", 4: "out"}
        for ep, d in need.items():
            dirbits = (h1 >> (2 * ep)) & 3
            ok = ep <= num_ep and dirbits in (0, 1 if d == "in" else 2)
            self.check(ok, "endpoint %d usable for %s (GHWCFG1 bits %d: 0 bidir, 1 IN, 2 OUT)" % (ep, d.upper(), dirbits))
        # synopsysotg.c writes a TX FIFO register only when target.h gives it a size (txfifosize {0x40, 0x100, 0,
        # 0x100}): TX2 belongs to the OUT-only endpoint 2 and keeps the core's reset default, which is not checked.
        used = {0: 0x40, 1: 0x100, 3: 0x100}
        rx, nptx = self.rw(OTG + 0x24)[0], self.rw(OTG + 0x28)[0]
        regions = [("RX", 0, rx & 0xffff), ("TX0", nptx & 0xffff, nptx >> 16)]
        for n in range(1, 4):
            f = self.rw(OTG + 0x100 + 4 * n)[0]
            if n in used:
                regions.append(("TX%d" % n, f & 0xffff, f >> 16))
            else:
                self.info("TX%d FIFO not programmed by emCORE (endpoint %d is OUT only): reset default words %d..%d"
                          % (n, n, f & 0xffff, (f & 0xffff) + (f >> 16)))
        for name, start, size in regions:
            self.check(start + size <= depth, "%s FIFO as programmed: words %d..%d within depth %d" % (
                name, start, start + size, depth))
            n = int(name[2:]) if name.startswith("TX") else None
            if n is not None:
                self.check(size == used[n], "%s FIFO size %d = target.h txfifosize[%d] %d" % (name, size, n, used[n]))
        total = sum(z for _, _, z in regions)
        self.info("emCORE uses %d of %d FIFO words" % (total, depth))
        spans = sorted((s, s + z, n) for n, s, z in regions if z)
        overlap = [a[2] + "/" + b[2] for a, b in zip(spans, spans[1:]) if b[0] < a[1]]
        self.check(not overlap, "programmed FIFOs do not overlap%s" % (" (" + ", ".join(overlap) + ")" if overlap else ""))

    def r_chipid(self):
        """Device-only fact: chip-id word 0x3d100008; synopsysotg: PHY+4 = 0x10 | (CHIPID+8 bit 0) (PROVISIONAL)."""
        ids = self.rw(CHIPID, 8)
        self.info("CHIPID 0x3d100000.. " + " ".join("%08x" % x for x in ids))
        phy4 = self.rw(PHY + 4)[0]
        self.check(phy4 == 0x10 | (ids[2] & 1), "PHY+4 %08x = 0x10 | CHIPID+8 bit 0 (%d)" % (phy4, ids[2] & 1))
        self.info("PHY " + " ".join("%08x" % x for x in self.rw(PHY, 8)))

    def r_sdram(self):
        """stage1.S SDRAM setup: controller state; 64 MiB (not wrapping), by READS only."""
        m = {off: self.rw(MIU + off)[0] for off in (0x100, 0x108, 0x140, 0x144)}
        self.check(m[0x100] == 0x09101030, "MIU+0x100 %08x (0x1030 | normal-mode 0x09100000)" % m[0x100])
        self.check(m[0x108] == 0x0002100b, "MIU+0x108 %08x (auto refresh on)" % m[0x108])
        self.check(m[0x144] & 3 == 3, "MIU+0x144 %08x bits 1:0 = 3 (DLL lock polled by stage 1)" % m[0x144])
        self.info("MIU+0x140 %08x" % m[0x140])
        top = self.elf.sym["_sdramstart"]
        code = self.rd(top, 0x400)
        for mib in (16, 32, 48):
            low = self.rd(top - mib * 0x100000, 0x400)
            self.check(low != code, "kernel code at %08x not mirrored %d MiB lower (no address wrap)" % (top, mib))

    def r_pagetable(self):
        """stage1.S P: L1 table at 0x2202c000, 290 entries; everything else faults (no stray mapping)."""
        t = self.rw(L1, 4096)
        desc = t[0x080] & 0xfffff     # SDRAM descriptor bits (0x5c06 write-back, 0x4c02 with N5G_NOCACHE)
        want = {0: 0x22004c02, 0x220: 0x22004c02}
        want.update({0x200 + i: 0x20004c02 + (i << 20) for i in range(32)})
        want.update({0x080 + i: 0x08000000 + desc + (i << 20) for i in range(64)})
        want.update({0x880 + i: 0x08004c02 + (i << 20) for i in range(64)})
        want.update({0x380 + i: 0x38004c02 + (i << 20) for i in range(128)})
        self.check(desc in (0x5c06, 0x4c02), "SDRAM sections %s (descriptor bits %05x)" % (
            {0x5c06: "write-back cached", 0x4c02: "non-cacheable (N5G_NOCACHE)"}.get(desc, "UNEXPECTED"), desc))
        wrong = [i for i in range(4096) if t[i] != want.get(i, 0)]
        self.check(not wrong, "all 4096 entries as stage 1 wrote them (%d differ%s)" % (
            len(wrong), ": " + ", ".join("VA %08x = %08x" % (i << 20, t[i]) for i in wrong[:6]) if wrong else ""))

    def r_threads(self):
        """Scheduler ABI 3, every thread alive, every stack guard intact (thread.h, the 4G rung 4)."""
        ver, ts = self.threads()
        self.check(ver == 3, "scheduler thread info version %d" % ver)
        for t in ts:
            guard = self.rw(t["stack"])[0] if safe(t["stack"], 4) else None   # the idle thread has none
            self.info("%08x %-18s %-11s prio %3d pc %s" % (t["addr"], t["name"], STATES[t["state"]] if t["state"] < 7
                                                         else t["state"], t["prio"], self.elf.lookup(t["pc"])))
            if t["name"].startswith("ladder-"):
                if t["state"] in (5, 6):
                    self.info("'%s' was killed by an earlier ladder rung (udf): expected, not a kernel thread" % t["name"])
                continue
            self.check(t["state"] not in (5, 6), "thread '%s' not killed by a panic" % t["name"])
            if guard is not None:
                self.check(guard == GUARD, "thread '%s' stack guard %08x" % (t["name"], guard))
        names = {t["name"] for t in ts}
        for n in ("idle thread", "synopsysotg", "monitor worker", "power"):
            self.check(n in names, "kernel thread '%s' present" % n)

    def r_lcd(self):
        """lcd.c: panel detected and initialised by the boot, controller idle in pixel mode."""
        info = self.rd(self.elf.sym["lcd_panel_info"], 28)
        magic, ident, ptype, tries, ran, clk_ok, st_to, rd_to = struct.unpack_from("<I4sbBBBII", info)
        self.check(magic == 0x5044434c, "lcd_panel_info magic %08x ('LCDP')" % magic)
        self.check(ptype >= 0 and ran == 1, "panel id %s -> type %d, init table sent %d, %d detection tries" % (
            ident.hex(" "), ptype, ran, tries))
        self.check(clk_ok == 1, "LCD clock CLKCON2 read back (clk_ok %d)" % clk_ok)
        self.check(st_to == 0 and rd_to == 0, "no LCD status/read timeouts (%d/%d)" % (st_to, rd_to))
        aborts = self.rw(self.elf.sym["lcd_dma_aborts"])[0]
        self.check(aborts == 0, "no LCD DMA aborts (%d)" % aborts)
        con = self.rw(LCD)[0]
        self.check(con in (LCD_MODE_PIXEL, 0x01000c20), "LCDCON %08x (pixel 0x01100db8 or command mode)" % con)

    def r_pmu(self):
        """PMU on I2C bus 0 at 0xe6 ; boot policy: backlight <= 0x40, warm flag cleared,
        USB input limit 500 mA after SET_CONFIGURATION. Reads only (no read-to-clear registers)."""
        t0 = self.rw(self.elf.sym["i2c_timeouts"])[0]
        regs = {r: self.i2c(PMU[0], PMU[1], r) for r in (0x02, 0x04, 0x05, 0x0a, 0x30, 0x31, 0x33, 0x34)}
        t1 = self.rw(self.elf.sym["i2c_timeouts"])[0]
        self.info("PMU " + " ".join("%02x=%02x" % kv for kv in sorted(regs.items())))
        self.check(t1 == t0, "no I2C timeouts during the reads (%d -> %d)" % (t0, t1))
        self.check(regs[0x30] <= 0x40, "backlight 0x30 = %02x <= 0x40" % regs[0x30])
        self.check(not regs[0x02] & 0x80, "warm-boot flag 0x02 bit 7 clear (power_init)")
        self.check(regs[0x33] & 3 == 2, "USB input limit 0x33 bits 1:0 = 2 (500 mA): %02x" % regs[0x33])
        self.facts["pmu"] = regs

    def r_accel(self):
        """accel.c PROVISIONAL: accelerometer on bus 0 at 0x32 (inferred from the EFI SMBus handle)."""
        who = self.i2c(ACCEL[0], ACCEL[1], 0x0f)
        boot = self.rd(self.elf.sym["accel_whoami"], 1)[0]
        self.check(who == 0x12, "WHO_AM_I %02x (expected 0x12; the boot read %02x)" % (who, boot))

    # ---------------------------------------------------------------- rungs (writing to free memory)
    def _free_region(self, region, label):
        a, b = region
        self.info("%s free region %08x..%08x (%d KiB) from the ELF layout" % (label, a, b, (b - a) // 1024))
        if not self.check(b - a >= 0x2000, "%s region large enough" % label):
            return False
        first = self.rd(a, 0x200)
        self.t.sleep(0.5)
        self.check(self.rd(a, 0x200) == first, "%s start unchanged over 0.5 s (nothing else writes there)" % label)
        return True

    def _write_test(self, region, label):
        if not self._free_region(region, label):
            return
        a = region[0]
        for size in (4, 64, 1024):
            data = bytes((a + i * 7 + size) & 0xff for i in range(size))
            self.plan("WRITE_MEM %d B at %08x (%s)" % (size, a, label))
            self.wr(a, data)
            self.check(self.rd(a, size) == data, "%d B read back" % size)
        self.wr(a, bytes(1024))
        self.alive("%s writes" % label)

    def r_tail(self):
        """SDRAM above .bss and below the LCD framebuffer is unused (ls.x); WRITE_MEM works there."""
        self._write_test(self.tail, "SDRAM tail")

    def r_sram(self):
        """SRAM above .ibss and below the L1 table is unused (ls.x, the ROM's DFU buffer is dead)."""
        self._write_test(self.sramfree, "SRAM")

    def r_flush(self):
        """FLUSH CACHE (cmd 20): clean entire D-cache + invalidate I-cache from the USB monitor."""
        self.plan("FLUSH CACHE (cmd 20)")
        self.flush()
        self.alive("FLUSH CACHE")

    def _code(self, region, label, tag):
        base = self.scratch(region, label)
        code = list(STORE)
        code[4], code[5] = base + 0x400, 0x600dc0de
        tp, res = self.run_code(base, tag, code)
        self.check(res[0] == 0x600dc0de, "code in %s ran and stored 0x600dc0de (got %08x)" % (label, res[0]))
        self.thread_gone(tag)
        self.alive("code in %s" % label)

    def r_sramcode(self):
        """A thread can run code from free SRAM (uncached) and exit through thread_exit."""
        self._code(self.sramfree, "SRAM", "ladder-sram")

    def r_tailcode(self):
        """A thread can run code from cached SDRAM after FLUSH CACHE (D written back, I invalidated)."""
        self._code(self.tail, "SDRAM tail", "ladder-tail")

    def r_sleep(self):
        """timer.c PROVISIONAL: the scheduler timer B (CON 0x1040, PRE 0xb) wakes sleep() on time. The device read
        PRE back as 0; this measures the kernel's own sleep() against the verified microsecond timer."""
        base = self.scratch(self.tail, "SDRAM tail")
        tb = self.rw(0x3c700020, 6)
        self.info("timer B: CON %08x CMD %08x DATA0 %08x DATA1 %08x PRE %08x CNT %08x (timer.c writes PRE 0xb)" % tb)
        for usecs in (20000, 100000):
            code = list(SLEEPTEST)
            code[15], code[16], code[17] = usecs, self.elf.sym["sleep"], base + 0x400
            tp, res = self.run_code(base, "ladder-sl", code, wait_s=1.0 + usecs / 1e5, done=lambda r: r[2] == 0x600dc0de)
            took = (res[1] - res[0]) & 0xffffffff
            self.check(res[2] == 0x600dc0de, "sleep(%d) returned" % usecs)
            self.check(0.95 * usecs <= took <= 1.5 * usecs + 12000,
                       "sleep(%d us) took %d us (%.2fx): scheduler timer rate %s" % (
                           usecs, took, took / usecs, "plausible" if 0.95 * usecs <= took <= 1.5 * usecs + 12000
                           else "WRONG (prescaler?)"))
            self.thread_gone("ladder-sl")
        self.alive("sleep test")

    def r_icache(self):
        """ARM1176 erratum 411920 concern: after an in-place overwrite + FLUSH CACHE the new code runs."""
        base = self.scratch(self.tail, "SDRAM tail")
        seen = []
        for val in (0xa, 0xb):
            code = STORE_IMM(val)
            code[4] = base + 0x400
            self.run_code(base, "ladder-ic%x" % val, code)
            seen.append(self.rw(base + 0x400)[0])
            self.thread_gone("ladder-ic%x" % val)
        self.check(seen[0] == 0xa, "routine A ran (%X)" % seen[0])
        self.check(seen[1] == 0xb, "after overwrite + FLUSH CACHE routine %s ran (%X)%s" % (
            "B" if seen[1] == 0xb else "A", seen[1], "" if seen[1] == 0xb else ": STALE I-CACHE LINE"))
        self.alive("I-cache test")

    def r_dcache(self):
        """N33 Cpu RVA 0x238/0x270 : entire-D clean is repeated until c7,c10,6 bit 0 clears."""
        base = self.scratch(self.sramfree, "SRAM")
        code = list(DCACHE)
        code[14] = base + 0x400
        tp, res = self.run_code(base, "ladder-dc", code, done=lambda r: r[1] != 0 or r[0] != 0)
        status, left = res[0], res[1]
        self.check(not status & 1 and left > 0, "dirty status bit 0 cleared after %d clean pass(es) (status %08x)" % (
            0x100000 - left, status))
        self.alive("D-cache clean loop")

    def r_aes(self):
        """Hardware AES with a user key, FIPS-197 C.1 (key/IV byte order assumption)."""
        buf = align(self.tail[0] + 0x1000, 32)
        key, pt, ct = FIPS197
        self.plan("AES buffer %08x: 32-B key field, 16-B IV, 16-B block" % buf)
        self.wr(buf, key + bytes(16) + bytes(16) + pt)
        flags = 1 | 8              # 128-bit key, ECB
        for encrypt, want, what in ((1, ct, "encrypt"),):
            self.plan("HWKEYAES %s ECB, user key (cmd 25)" % what)
            rc = struct.unpack_from("<i", self.t.raw(struct.pack("<IBBHII", 25, encrypt, flags, 0, buf, 16), 16), 4)[0]
            got = self.rd(buf + 48, 16)
            self.check(rc == 0 and got == want, "%s: rc %d, %s (%s)" % (what, rc, got.hex(), "FIPS-197" if got == want
                                                                       else "expected " + want.hex()))
        self.wr(buf, bytes(64))

    def r_memtest(self, size=None):
        """SDRAM integrity and sustained write load : fill the tail with addr^key on the device,
        clean+invalidate, verify. Default: the whole tail except the work block."""
        base = self.scratch(self.tail, "SDRAM tail")
        start, end = base + 0x1000, self.tail[1]
        if size:
            end = min(end, start + size)
        code = list(MEMTEST)
        code[32], code[33], code[34], code[35] = start, end, 0x5a5aa5a5, base + 0x400
        self.plan("memtest %08x..%08x (%d KiB) of SDRAM writes + verify" % (start, end, (end - start) // 1024))
        tp, res = self.run_code(base, "ladder-mt", code, wait_s=10.0, done=lambda r: r[2] == 0x600dc0de)
        self.check(res[2] == 0x600dc0de, "memtest finished")
        self.check(res[0] == 0, "no bad words (%d, first at %08x)" % (res[0], res[1]))
        self.wr(start, bytes(min(0x400, end - start)))
        self.alive("SDRAM memtest")

    # ---------------------------------------------------------------- explicit rungs
    def r_sramsize(self):
        """SRAM is 448 KiB (inferred from Apple's constants), i.e. RAM above 0x22030000.
        RISKY: if it is absent, the access faults inside the USB monitor and the device stops answering;
        that silence is the answer (192 KiB). Alias candidates are read first and restored afterwards."""
        probe, cands = 0x2206ff00, (0x2202ff00, 0x2200ff00, 0x2203ff00)
        self.plan("READ 64 B at %08x (above the known 192 KiB)  <-- faults here if the SRAM is absent" % probe)
        before = self.rd(probe, 64, allow=True)
        saved = {c: self.rd(c, 64, allow=True) for c in cands}
        if before in saved.values() and any(before != bytes(64) for _ in [0]):
            self.check(False, "%08x mirrors a lower address: SRAM wraps (not written)" % probe)
            return
        pat = bytes((0xc3 ^ i) & 0xff for i in range(64))
        self.plan("WRITE 64 B pattern at %08x, then compare %s" % (probe, ", ".join("%08x" % c for c in cands)))
        self.wr(probe, pat, allow=True)
        got = self.rd(probe, 64, allow=True)
        aliased = [c for c in cands if self.rd(c, 64, allow=True) == pat and saved[c] != pat]
        for c in aliased:
            self.wr(c, saved[c], allow=True)
        self.wr(probe, before, allow=True)
        self.check(got == pat, "pattern read back at %08x" % probe)
        self.check(not aliased, "no alias at %s" % (", ".join("%08x" % c for c in aliased) or "the candidates"))
        self.alive("SRAM size probe")

    def r_udf(self):
        """Exception path: an undefined instruction in a ladder thread -> emCORE panics that thread only."""
        base = self.scratch(self.sramfree, "SRAM")
        self.plan("thread executing 'udf' -> undefined handler -> panic, thread killed (expected)")
        self.run_code(base, "ladder-udf", list(UDF), wait_s=1.0, done=lambda r: False)
        _, ts = self.threads()
        t = [x for x in ts if x["name"] == "ladder-udf"]
        self.check(bool(t) and t[0]["state"] in (5, 6), "ladder thread killed by the panic (%s)" % (
            STATES[t[0]["state"]] if t else "gone"))
        self.alive("the exception")

    RUNGS = ["identity", "bootreport", "clocks", "usec", "usbcaps", "chipid", "sdram", "pagetable", "threads", "lcd",
             "pmu", "accel", "wheel", "tail", "sram", "flush", "sramcode", "tailcode", "sleep", "icache", "dcache", "aes", "memtest"]
    EXPLICIT = ["sramsize", "udf"]
    EXPECTED_PANIC = {"udf"}

    def run(self, name, **kw):
        fn = getattr(self, "r_" + name)
        self.say("===== rung %s: %s" % (name, " ".join(fn.__doc__.split())))
        self.fails = 0
        self.facts = {}
        verdict, error, mark = "PASS", None, None
        self.journal(dict(event="start", rung=name))
        try:
            mark = self.console_mark()
            fn(**kw)
        except Refused as e:
            self.check(False, "refused on the host: %s" % e)
        except NoAnswer as e:
            verdict, error = "NO ANSWER", str(e)
        except Exception as e:  # noqa: BLE001  (USB loss surfaces as library errors)
            verdict, error = "NO ANSWER", repr(e)
        console = []
        if mark is not None and name != "bootreport":
            try:
                console = self.console_since(mark)
            except Exception:  # noqa: BLE001
                pass
        for line in console:
            self.say("[console] " + line)
            if "*PANIC*" in line and name not in self.EXPECTED_PANIC:
                self.fails += 1
        if verdict == "PASS" and self.fails:
            verdict = "FAIL"
        if error:
            self.say("[FAIL] the device stopped answering: %s" % error)
            self.say("       The last [plan] line above names the step. Read the LCD now (its last lines are the "
                     "device's own report), note whether it re-enumerates by itself (reset) or stays silent (hang).")
        self.say("===== rung %s: %s" % (name, verdict))
        self.journal(dict(event="end", rung=name, verdict=verdict, error=error, facts=self.facts, console=console))
        return verdict

    def journal(self, rec):
        if not self.log:
            return
        rec.update(time=time.time(), elf=os.path.basename(self.elf.path))
        with open(self.log, "a") as f:
            f.write(json.dumps(rec) + "\n")
            f.flush()
            os.fsync(f.fileno())


def last_results(log):
    res = {}
    if log and os.path.exists(log):
        for line in open(log):
            r = json.loads(line)
            if r.get("event") == "end":
                res[r["rung"]] = r["verdict"]
            elif r.get("event") == "start":
                res.setdefault(r["rung"], "started, no result (device lost?)")
                if res[r["rung"]] in ("PASS", "FAIL", "NO ANSWER"):
                    res[r["rung"]] = "started, no result (device lost?)"
    return res


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("rung", help="'list', or one rung name/number")
    ap.add_argument("--elf", default=DEFAULT_ELF)
    ap.add_argument("--log", default="n5g-ladder.jsonl")
    ap.add_argument("--report", default="n5g-ladder-report.txt", help="every printed line is appended here")
    ap.add_argument("--seconds", type=float, default=3.0, help="usec: host interval")
    ap.add_argument("--size", type=lambda x: int(x, 0), help="memtest: limit the region (bytes)")
    a = ap.parse_args(argv)
    order = Ladder.RUNGS + Ladder.EXPLICIT
    if a.rung == "list":
        done = last_results(a.log)
        for i, n in enumerate(order):
            doc = " ".join(getattr(Ladder, "r_" + n).__doc__.split())
            print("%2d %-10s %-38s %s" % (i, n, done.get(n, "-"), doc[:110]))
        print("explicit (run last): %s" % ", ".join(Ladder.EXPLICIT))
        return 0
    name = order[int(a.rung)] if a.rung.isdigit() and int(a.rung) < len(order) else a.rung
    if name not in order:
        print("unknown rung %r; 'list' shows them" % a.rung)
        return 1
    report = open(a.report, "a")
    report.write("\n### %s  rung %s\n" % (time.strftime("%Y-%m-%d %H:%M:%S"), name))

    def out(text):
        print(text, flush=True)
        report.write(text + "\n")
        report.flush()
        os.fsync(report.fileno())
    try:
        lad = Ladder(DeviceTransport(), Elf(a.elf), a.log, out=out)
    except Exception as e:  # noqa: BLE001
        print("cannot open the device: %r" % (e,))
        return 1
    kw = {"usec": dict(seconds=a.seconds), "memtest": dict(size=a.size)}.get(name, {})
    try:
        verdict = lad.run(name, **kw)
    finally:
        lad.t.close()
        report.close()
    if verdict == "PASS":
        i = order.index(name)
        if i + 1 < len(Ladder.RUNGS):
            print("next: python3 emcore/tools/n5g.py diag %s" % order[i + 1])
    return 0 if verdict == "PASS" else 2


if __name__ == "__main__":
    sys.exit(main())
