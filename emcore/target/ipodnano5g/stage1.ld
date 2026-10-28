/* iPod nano 5G stage-1 SRAM loader: position-dependent at 0x22000000 (the ROM's DFU load address).
   tools/nano5g/image.py appends {u32 length, emcore.bin} at _stage1_end (16-byte aligned). */
ENTRY(_stage1_entry)
OUTPUT_FORMAT(elf32-littlearm)
OUTPUT_ARCH(arm)
SECTIONS
{
    . = 0x22000000;
    .text : { KEEP(*(.text.entry)) *(.text*) *(.rodata*) }
    . = ALIGN(16);
    _stage1_end = .;
    /DISCARD/ : { *(.comment) *(.ARM.attributes) *(.ARM.exidx*) *(.eh_frame*) *(.data*) *(.bss*) }
}
ASSERT(_stage1_end - 0x22000000 <= 0x1000, "stage 1 must stay tiny (4 KiB)")
