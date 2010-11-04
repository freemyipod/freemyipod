ENTRY(main)
OUTPUT_FORMAT(elf32-littlearm)
OUTPUT_ARCH(arm)
STARTUP(build/main.o)
_filename = 0x0863f575;
_loaddest = 0x08000000;

MEMORY
{
    SRAM : ORIGIN = 0x22000000, LENGTH = 0x0002c000
}

SECTIONS
{
    .text :
    {
        *(.text*)
        *(.glue_7)
        *(.glue_7t)
        . = ALIGN(0x4);
        *(.rodata*)
        . = ALIGN(0x4);
        *(.data*)
        . = ALIGN(0x4);
    } > SRAM

    .bss (NOLOAD) :
    {
        _bssstart = .;
        *(.bss*)
        *(COMMON)
        . = ALIGN(0x4);
        _bssend = .;
    } > SRAM

    /DISCARD/ :
    {
        *(.eh_frame)
    }

}
