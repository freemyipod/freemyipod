ENTRY(__start)
OUTPUT_FORMAT(elf32-littlearm)
OUTPUT_ARCH(arm)
STARTUP(build/ipodnano4g/target/ipodnano4g/crt0.o)

MEMORY
{
    INIT : ORIGIN = 0x08000000, LENGTH = 0x01eff000
    INITBSS : ORIGIN = 0x09e00000, LENGTH = 0x0017f000
    INITSTACK : ORIGIN = 0x09f7f000, LENGTH = 0x00001000
    SRAM : ORIGIN = 0x22000000, LENGTH = 0x00030000
    SDRAM : ORIGIN = 0x09f80000, LENGTH = 0x00100000
}

SECTIONS
{
    .init :
    {
        _initstart = .;
	*(.inithead*)
        *(.initcode*)
        *(.initrodata*)
        *(.initdata*)
        . = ALIGN(0x4);
        _initend = .;
    } > INIT

    .sram :
    {
        _sramstart = .;
        KEEP(*(.intvect))
        *(.intvect)
        *(.icode*)
        *(.irodata*)
        *(.idata*)
        . = ALIGN(0x4);
        _sramend = .;
    } > SRAM AT> INIT
    _sramsource = LOADADDR(.sram);

    .sdram :
    {
        _sdramstart = .;
        *(.text*)
        *(.glue_7)
        *(.glue_7t)
        . = ALIGN(0x4);
        *(.rodata*)
        . = ALIGN(0x4);
        *(.data*)
        . = ALIGN(0x4);
        _sdramend = .;
    } > SDRAM AT> INIT
    _sdramsource = LOADADDR(.sdram);

    .initbss (NOLOAD) :
    {
        _initbssstart = .;
        *(.initbss*)
        . = ALIGN(0x4);
        _initstackstart = .;
        . += 0x400;
        _initstackend = .;
        _initbssend = .;
    } > INITBSS

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
        *(.stack*)
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

    .initstack (NOLOAD) :
    {
        _loadspaceend = .;
        *(.initstack*)
        *(COMMON)
        . = ALIGN(0x4);
    } > INITSTACK

    /DISCARD/ :
    {
        *(.eh_frame)
    }

}
