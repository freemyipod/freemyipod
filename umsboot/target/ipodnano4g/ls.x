ENTRY(__start)
OUTPUT_FORMAT(elf32-littlearm)
OUTPUT_ARCH(arm)
STARTUP(build/ipodnano4g/target/ipodnano4g/crt0.o)

MEMORY
{
    SRAM : ORIGIN = 0x22000000, LENGTH = 0x0002c000
    SRAM2 : ORIGIN = 0x2202d000, LENGTH = 0x00002ffc
    SDRAM : ORIGIN = 0x08000000, LENGTH = 0x02000000
}
_ramdiskptr = 0x2202fffc;

SECTIONS
{
    .init :
    {
        _ramdiskstart = .;
        . = ALIGN(4);
        *(.inithead*)
        . = ALIGN(4);
        *(.initcode*)
        . = ALIGN(4);
        *(.initrodata*)
        . = ALIGN(4);
        *(.initdata*)
        . = ALIGN(4);
        _initend = .;
    } > SDRAM

    .sram :
    {
        _sramstart = .;
        KEEP(*(.intvect))

        . = ALIGN(4);
        *(.intvect)
        . = ALIGN(4);
        *(.icode*)
        . = ALIGN(4);
        *(.text*)
        . = ALIGN(4);
        *(.glue_7)
        . = ALIGN(4);
        *(.glue_7t)
        . = ALIGN(4);
        *(.irodata*)
        . = ALIGN(4);
        *(.rodata*)
        . = ALIGN(4);
        *(.idata*)
        . = ALIGN(4);
        *(.data*)
        . = ALIGN(4);
        _sramend = .;
    } > SRAM AT> SDRAM
    _sramsource = LOADADDR(.sram);

    .initbss (NOLOAD) :
    {
        _initbssstart = .;
        . = ALIGN(4);
        *(.initbss*)
        . = ALIGN(4);
        _initbssend = .;
    } > SDRAM

    .ibss (NOLOAD) :
    {
        _bssstart = .;
        . = ALIGN(4);
        *(.ibss*)
        . = ALIGN(4);
        _bssend = .;
    } > SRAM

    .ibss2 (NOLOAD) :
    {
        _bss2start = .;
        . = ALIGN(4);
        *(.bss*)
        . = ALIGN(4);
        *(COMMON)
        . = ALIGN(4);
        _mainstackstart = .;
        . += 0x1000;
        _mainstackend = .;
        . = ALIGN(4);
        _irqstackstart = .;
        . += 0x400;
        _irqstackend = .;
        . = ALIGN(4);
        _abortstackstart = .;
        . += 0x400;
        _abortstackend = .;
        . = ALIGN(4);
        _bss2end = .;
    } > SRAM2

    /DISCARD/ :
    {
        *(.eh_frame)
    }

    .ramdisk _ramdiskstart (NOLOAD) :
    {
        *(.ramdisk*)
        _ramdiskend = .;
    } > SDRAM
}
