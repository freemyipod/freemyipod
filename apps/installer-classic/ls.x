ENTRY(__embios_executable_hdr)
OUTPUT_FORMAT(elf32-littlearm)
OUTPUT_ARCH(arm)

MEMORY
{
    LOWERRAM : ORIGIN = 0x08000000, LENGTH = 0x00f00000
    UPPERRAM : ORIGIN = 0x08f00000, LENGTH = 0x03000000
}

SECTIONS
{
    .bss (NOLOAD) :
    {
        __bss_start = .;
        *(.bss*)
        *(COMMON)
        __bss_end = .;
        *(.stack*)
    } > LOWERRAM

    .text :
    {
	KEEP(.execheader*)
	*(.execheader*)
        *(.text*)
        *(.glue_7)
        *(.glue_7t)
        . = ALIGN(0x4);
        *(.rodata*)
        . = ALIGN(0x4);
        *(.data*)
        . = ALIGN(0x10);
	_scriptstart = .;
    } > UPPERRAM

    /DISCARD/ :
    {
        *(.eh_frame)
    }

}
