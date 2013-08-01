#include "global.h"

#ifndef SRAM_BASE
#define SRAM_BASE 0x22000000
#endif
#ifndef SRAM_SIZE
#define SRAM_SIZE DEFAULT_SRAM_SIZE
#endif
#ifndef SDRAM_BASE
#define SDRAM_BASE 0x08000000
#endif
#ifndef SDRAM_SIZE
#define SDRAM_SIZE DEFAULT_SDRAM_SIZE
#endif

#ifndef CODE_REGION
#define CODE_REGION SDRAM
#endif
#ifndef BSS_REGION
#define BSS_REGION SDRAM
#endif
#ifndef DMABSS_REGION
#define DMABSS_REGION SRAM
#endif
#ifndef STACK_REGION
#define STACK_REGION SRAM
#endif

#ifndef STACK_SIZE
#define STACK_SIZE 0x1000
#endif

#ifndef IRQ_STACK_SIZE
#define IRQ_STACK_SIZE 0x1000
#endif

#ifndef ABORT_STACK_SIZE
#define ABORT_STACK_SIZE 0
#endif

MEMORY
{
    SRAM : ORIGIN = SRAM_BASE, LENGTH = SRAM_SIZE
    SDRAM : ORIGIN = SDRAM_BASE, LENGTH = SDRAM_SIZE
}

_entry = _vectors;
ENTRY(_entry)

SECTIONS
{

    .text :
    {
        _text = .;
        KEEP(*(.vectors))
        *(.text)
        *(.text.*)
        *(.rodata)
        *(.rodata.*)
        *(.dmarodata)
        *(.dmarodata.*)
        *(.data)
        *(.data.*)
        *(.dmadata)
        *(.dmadata.*)
        _text_end = .;
        . = ALIGN(1 << CACHEALIGN_BITS);
    } >CODE_REGION

    _text_size = _text_end - _text;

    .bss (NOLOAD) :
    {
        . = ALIGN(1 << CACHEALIGN_BITS);
        _bss = .;
        *(.bss)
        *(.bss.*)
        *(COMMON)
        . = ALIGN(1 << CACHEALIGN_BITS);
        _bss_end = .;
    } >BSS_REGION

    _bss_size = _bss_end - _bss;

    .dmabss (NOLOAD) :
    {
        . = ALIGN(1 << CACHEALIGN_BITS);
        _dmabss = .;
        *(.dmabss)
        *(.dmabss.*)
        . = ALIGN(1 << CACHEALIGN_BITS);
        _dmabss_end = .;
    } >DMABSS_REGION

    _dmabss_size = _dmabss_end - _dmabss;

    .stack (NOLOAD) :
    {
        . = ALIGN(1 << CACHEALIGN_BITS);
        _stack = .;
        . = . + STACK_SIZE;
        _stack_top = .;
        . = . + IRQ_STACK_SIZE;
        _irq_stack_top = .;
        . = . + ABORT_STACK_SIZE;
        _abort_stack_top = .;
        . = ALIGN(1 << CACHEALIGN_BITS);
        _stack_end = .;
    } >STACK_REGION

    _stack_size = _stack_end - _stack;

    DISCARD :
    {
        *(.note.*)
    }

    .stab            0 : { *(.stab) }
    .stabstr         0 : { *(.stabstr) }
    .stab.excl       0 : { *(.stab.excl) }
    .stab.exclstr    0 : { *(.stab.exclstr) }
    .stab.index      0 : { *(.stab.index) }
    .stab.indexstr   0 : { *(.stab.indexstr) }
    .comment         0 : { *(.comment) }
    .debug           0 : { *(.debug) }
    .line            0 : { *(.line) }
    .debug_srcinfo   0 : { *(.debug_srcinfo) }
    .debug_sfnames   0 : { *(.debug_sfnames) }
    .debug_aranges   0 : { *(.debug_aranges) }
    .debug_pubnames  0 : { *(.debug_pubnames) }
    .debug_info      0 : { *(.debug_info .gnu.linkonce.wi.*) }
    .debug_abbrev    0 : { *(.debug_abbrev) }
    .debug_line      0 : { *(.debug_line) }
    .debug_frame     0 : { *(.debug_frame) }
    .debug_str       0 : { *(.debug_str) }
    .debug_loc       0 : { *(.debug_loc) }
    .debug_macinfo   0 : { *(.debug_macinfo) }
    .debug_weaknames 0 : { *(.debug_weaknames) }
    .debug_funcnames 0 : { *(.debug_funcnames) }
    .debug_typenames 0 : { *(.debug_typenames) }
    .debug_varnames  0 : { *(.debug_varnames) }

}
