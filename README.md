# emCORE

This repository contains the emCORE kernel, applications, libraries and host tools.
The nano5g target supports RAM boot on the S5L8730-based iPod nano 5G.

## Build nano5g

Required: GNU Make 4.3 or newer, a host C compiler, Python 3, and
`arm-none-eabi-gcc`/binutils with newlib headers. No other source checkout is needed.

```sh
cd emcore
make nano5g -j4
```

The nano5g build lives in `emcore`; all its outputs go under `emcore/build`.
From the repository root, `make nano5g` remains available as a shortcut, and
`make` shows the available build entry points.
`make -C emcore` and `make all` retain the original target set; nano5g is opt-in.
`make nano5g` (also `make n5g`) compiles the
host FMSS assembler, generates the NAND microcode header, compiles the kernel
and SRAM loader, and packages the image and symbol file.

| Output | Purpose |
|---|---|
| `emcore/build/ipodnano5g/emcore-nano5g.dfu` | Unsigned IMG1 image containing the loader and kernel |
| `emcore/build/ipodnano5g/emcore-nano5g.bin` | Combined payload without the IMG1 header |
| `emcore/build/ipodnano5g/emcore-nano5g.nm` | Kernel symbols |
| `emcore/build/ipodnano5g/emcore.elf` | Kernel ELF, also used by diagnostics |
| `emcore/build/ipodnano5g/stage1.elf` | SRAM loader ELF |
| `emcore/build/ipodnano5g/nand/fmss-nano5g.h` | Generated FMSS programs |
| `emcore/build/host/fmssasm` | Host assembler built from `tools/fmssasm.c` |

Use `make nano5g-clean` to remove nano5g build outputs. `make clean` removes
the kernel build directory.
`make n5g-kernel` and `make n5g-stage1` build individual components.
These commands work inside `emcore` or through the repository-root shortcuts.
Existing targets retain their kernel build and UCL packaging rules.

Build options can be changed between invocations without a manual clean:

| Option | Effect |
|---|---|
| `N5G_NAND_WRITE=0` | Disable the NAND harness's write commands |
| `N5G_NAND_DISABLE=1` | Omit the NAND harness and module loader |
| `N5G_NOCACHE=1` | Disable caches and use non-cacheable SDRAM |
| `N5G_BACKLIGHT_MAX=0x40` | Backlight ceiling (default `0x40`) |
| `N5G_BACKLIGHT_BOOT=0x20` | Initial backlight level (default `0x20`) |
| `CROSS=arm-none-eabi-` | Cross-toolchain prefix |
| `HOSTCC=cc` | Host compiler for the FMSS assembler |
| `PYTHON=python3` | Python used by the image packer |

The image is for RAM boot; generating it does not install emCORE on NAND.
Use a loader that can boot an unsigned nano5g IMG1 image. The running monitor
uses USB ID `ffff:e000`; DFU mode uses `05ac:1231`.

## Host tools

Run the following commands from the repository root.
The nano5g commands share the existing `emcore/tools/libemcore.py` transport.
Device access requires PyUSB and the system libusb library:

```sh
python3 -m venv .venv
.venv/bin/python -m pip install -r requirements.txt
```

Linux users can install `udev/50-ipod-nano5g.rules` into `/etc/udev/rules.d/`
and reload udev rules to allow access without running tools as root.
Offline image/module packaging and command help do not require PyUSB.

```sh
python3 emcore/tools/n5g.py --help
python3 emcore/tools/n5g.py diag list
python3 emcore/tools/n5g.py diag identity
python3 emcore/tools/n5g.py nand init --ftl
python3 emcore/tools/n5g.py nand mbr
python3 emcore/tools/n5g.py firmware list
python3 emcore/tools/n5g.py firmware dump osos -o osos.bin
python3 emcore/tools/n5g.py firmware dump-all firmware-backup
python3 emcore/tools/n5g.py firmware replace osos osos-patched.bin
python3 emcore/tools/n5g.py nand lread 0 8 -o first-sectors.bin
python3 emcore/tools/n5g.py nand lwrite 128 sectors.bin
python3 emcore/tools/n5g.py nand flush
python3 emcore/tools/n5g.py private show
python3 emcore/tools/n5g.py private set 2
python3 emcore/tools/n5g.py module pack module.elf -o module.n5m --abi 2
python3 emcore/tools/n5g.py module load module.n5m
```

NAND logical sectors are 4096 bytes. `lwrite` flushes and reads back by default;
`--no-flush` and `--no-verify` are available. Commands do not automatically replay
writes or reset the device after a failed transfer. NAND initialization starts
only on a host command; boot creates an idle worker and its result buffer.

`firmware` reads entries such as `osos`, `rsrc`, `disk`, and `diag` from the
firmware partition after `nand init --ftl`. Dumps include all stored entry bytes,
including image headers. Replacement requires exactly the existing entry length,
preserves neighboring bytes in partial sectors, then flushes and verifies the
written data. It preserves partition sizes and directory metadata, including
checksum fields; it does not decode, sign, or rebuild images. Writes are
incremental, so an interrupted replacement can leave a partially updated entry.
Replacement reports byte counts and percentages while writing and verifying,
and announces when it is flushing.

`firmware replace` uses USB bulk transfers for uploads and read-back verification
by default. Add `--control` to use the previous control-transfer mode:

```sh
python3 emcore/tools/n5g.py firmware replace osos osos-patched.bin --control
```

Bulk mode batches uploads instead of sending 64 bytes per control write. It
retains the staged-data CRC check, per-chunk verification, flush, and final
read-back verification. Transfer failures stop the command without retries.

The module loader accepts one module per boot before NAND jobs have started.
ABI 2 exposes peripheral leases for I2C, LCD, NAND and the click wheel. Kernel
services, memory layouts and command numbers are described in
[the target notes](emcore/target/ipodnano5g/README.md).
