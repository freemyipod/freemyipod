ENTRY(__emcore_entrypoint)
OUTPUT_FORMAT(elf32-littlearm)
OUTPUT_ARCH(arm)

MEMORY
{
    VIRTUAL : ORIGIN = 0x00000000, LENGTH = 0x10000000
}

SECTIONS
{
    .text :
    {
	KEEP(.emcoreentrypoint*)
	*(.emcoreentrypoint*)
        *(.text*)
        *(.glue_7)
        *(.glue_7t)
        . = ALIGN(0x4);
    } > VIRTUAL

    .data :
    {
        *(.rodata*)
        . = ALIGN(0x4);
        *(.data*)
        . = ALIGN(0x4);
    } > VIRTUAL

    .bss (NOLOAD) :
    {
        __bss_start = .;
        *(.bss*)
        *(COMMON)
        __bss_end = .;
    } > VIRTUAL

    /DISCARD/ :
    {
        *(.eh_frame)
    }

}
