ENTRY(_start)
OUTPUT_FORMAT(elf32-littlearm)
OUTPUT_ARCH(arm)
STARTUP(build/ipodnano4g/target/ipodnano4g/crt0.o)

MEMORY
{
    INIT : ORIGIN = 0x08000000, LENGTH = 0x01f00000
    SRAM : ORIGIN = 0x22000000, LENGTH = 0x00030000
    SDRAM : ORIGIN = 0x09f00000, LENGTH = 0x00100000
}

SECTIONS
{
    .init : {
        *(.initcode*)
        *(.initrodata*)
        *(.initdata*)
        . = ALIGN(0x4);
    } > INIT

    .intvect : {
        _sramstart = .;
        KEEP(*(.intvect))
        *(.intvect)
    } > SRAM AT> INIT
    _sramsource = LOADADDR(.intvect);

    .iram :
    {
        *(.icode*)
        *(.irodata*)
        *(.idata*)
        . = ALIGN(0x4);
        _sramend = .;
    } > SRAM AT> INIT

    .text :
    {
        _sdramstart = .;
        *(.text*)
        *(.glue_7)
        *(.glue_7t)
        . = ALIGN(0x4);
    } > SDRAM AT> INIT
    _sdramsource = LOADADDR(.text);

    .rodata :
    {
        *(.rodata*)
        . = ALIGN(0x4);
    } > SDRAM AT> INIT

    .data :
    {
        *(.data*)
        . = ALIGN(0x4);
        _sdramend = .;
    } > SDRAM AT> INIT

    .initbss (NOLOAD) :
    {
        _initbssstart = .;
        *(.initbss*)
        . = ALIGN(0x4);
        _initstackstart = .;
        . += 0x4000;
        _initstackend = .;
        _initbssend = .;
    } > INIT

    .ibss (NOLOAD) :
    {
        _ibssstart = .;
        *(.ibss*)
        . = ALIGN(0x4);
        _irqstackstart = .;
        . += 0x400;
        _irqstackend = .;
        _abortstackstart = .;
        . += 0x400;
        _abortstackend = .;
        *(.stack)
        _ibssend = .;
    } > SRAM

    .bss (NOLOAD) :
    {
        _bssstart = .;
        *(.bss*)
        *(COMMON)
        . = ALIGN(0x4);
        _bssend = .;
    } > SDRAM

    /DISCARD/ :
    {
        *(.eh_frame)
    }

}
