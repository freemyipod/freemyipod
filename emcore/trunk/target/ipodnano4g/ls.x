ENTRY(__start)
OUTPUT_FORMAT(elf32-littlearm)
OUTPUT_ARCH(arm)
STARTUP(build/ipodnano4g/target/ipodnano4g/crt0.o)

MEMORY
{
    INIT : ORIGIN = 0x08000000, LENGTH = 0x01f80000
    SRAM : ORIGIN = 0x22000000, LENGTH = 0x00030000
    SDRAM : ORIGIN = 0x09f80000, LENGTH = 0x00080000
}

SECTIONS
{
    .inithead :
    {
        _initheadstart = .;
        _poolstart = .;
	*(.inithead*)
        . = ALIGN(0x4);
        _initheadend = .;
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
        _poolend = .;
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

    .init :
    {
        _initstart = .;
        *(.initcode*)
        *(.initrodata*)
        *(.initdata*)
        *(.inittail*)
        . = ALIGN(0x4);
        _initend = .;
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
        *(.stack*)
        . = ALIGN(0x4);
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
