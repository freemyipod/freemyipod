# iPod nano 5G target

## Startup and memory

`stage1.S` runs in SRAM at `0x22000000`, initializes SDRAM and the translation
table, then copies the kernel to `0x08000000`. `crt0.S` relocates sections and
starts emCORE. `ls.x` reserves these regions:

| Region | Address |
|---|---|
| Kernel load image / subsequent allocation pool | `0x08000000` |
| Kernel SDRAM sections | `0x0bf00000` |
| LCD framebuffer | `0x0bfd0000` |
| SRAM sections and stacks | `0x22000000`, below `0x2202c000` |
| Translation table | `0x2202c000` |

The target has 64 MiB of SDRAM and a 240 × 376 RGB565 panel. Hardware register
addresses live in `s5l8730.h`. Comments describing observed register sequences
use the original firmware's address space, rather than live kernel addresses.

## NAND

`nand/nand-nano5g.c` provides geometry and target initialization.
`nand/nand-controller.h`, `nand-read.h`, and `nand-write.h` implement the physical
operations. `nand/ftl-nano5g.c`, `ftl-write.h`, and `ftl-vfl-write.h` implement
logical translation and writes. These compile with the regular emCORE target
rules and call its timer, clock and cache APIs directly. Hardware definitions
live in `s5l8730.h`; the NAND worker serializes monitor access.

The five source programs in `nand/fmss/` are assembled into
`build/ipodnano5g/nand/fmss-nano5g.h`. Generated microcode is never copied from
another build. The kernel does not expose NAND as its general filesystem;
`nand5g.c` runs host-requested jobs in a worker thread.

## Monitor protocol

Common monitor commands are dispatched by `../../usb/usbdebug.c`.
Host constants are in `../../tools/nano5g/protocol.py`.

| Command | Nano5g operation |
|---|---|
| 25 | AES encryption; status in response word 1 |
| 68 | USB diagnostic/recovery mask, initially zero |
| 104 | Synchronous reset |
| 120–125, 127 | NAND status, initialization and reads |
| 126 | Reserved |
| 128–131 | Writer capabilities, open, write and flush |
| 132–134 | Module info, reserve and activate |
| 200 | Monitor heartbeat and NAND progress |

`nand5g.h` defines the status record, job IDs and result buffer layout.
NAND jobs return promptly from the USB dispatcher; status polling observes their
completion. The writer uses CRC-32 over staged data and reports failed jobs.
`N5G_NAND_WRITE=0` removes write-command support while retaining reads.

`hwkeyaes_ext.h` defines encryption buffers: 32 bytes of key storage, 16 bytes
of IV, then the payload. Extended flags select key size, ECB, and IV write-back.
User keys are cleared from engine registers after use.

## Modules

`module5g.h` defines the module header, imports and service table. ABI 1 contains
core services; ABI 2 appends peripheral leases. The loader validates CRC and
relocations, activates the module, and keeps its storage until reset.
A loaded module owns its command dispatch and excludes the built-in NAND worker.

Leases pause emCORE users of I2C, LCD and the wheel. Release restores the
controller configuration; LCD release repaints the console. NAND leases confirm
the exclusion already established by module activation.

Pack modules from a zero-linked ARM ELF with `--emit-relocs`. The `.text`
section starts at zero and `.bss` immediately follows it. Entry symbols are
`n5g_module_start`, `n5g_nand_usb_command`, and `n5g_nand_heartbeat`.
The host command is `python3 emcore/tools/n5g.py module pack` from the repository
root; use its `--help` for arguments.
