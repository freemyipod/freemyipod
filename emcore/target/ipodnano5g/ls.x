/*
 * iPod nano 5G (S5L8730) memory layout.
 *
 * INIT, 0x08000000: stage1.S copies emcore.bin here. crt0.S relocates the
 * SRAM and SDRAM sections, then this range becomes the malloc pool.
 *
 * SRAM, 0x22000000: exception vectors, ICODE/IDATA, and IRQ/abort stacks.
 * Keep all sections below the page table at 0x2202c000..0x2202ffff.
 * The page table also maps the vectors to virtual address zero.
 *
 * SDRAM, 0x0bf00000: kernel .text/.rodata/.data/.bss. The 832 KiB window
 * includes the Rockbox FTL tables. The LCD framebuffer occupies the last
 * 192 KiB of SDRAM, starting at 0x0bfd0000.
 *
 * The 64 MiB SDRAM size follows bl_PreEfi:0x2200b4b4. stage1.S tests both
 * ends of the range before relocating the kernel. SRAM above 0x22030000
 * is not used; its size has not been established on the device.
 */
ENTRY(__start)
OUTPUT_FORMAT(elf32-littlearm)
OUTPUT_ARCH(arm)
STARTUP(build/ipodnano5g/target/ipodnano5g/crt0.o)

MEMORY
{
    INIT : ORIGIN = 0x08000000, LENGTH = 0x03f00000
    SRAM : ORIGIN = 0x22000000, LENGTH = 0x0002c000   /* below the stage-1 page table at 0x2202c000 */
    SDRAM : ORIGIN = 0x0bf00000, LENGTH = 0x000d0000   /* .text/.rodata/.data/.bss, up to the LCD fb */
    LCD  : ORIGIN = 0x0bfd0000, LENGTH = 0x00030000
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
        . = ALIGN(0x4);
        _initend = .;
    } > INIT

    .inittail :
    {
        _inittailstart = .;
        *(.inittail*)
        . = ALIGN(0x4);
        _inittailend = .;
    } > INIT

    .ibss (NOLOAD) :
    {
        _ibssstart = .;
        *(.ibss*)
        . = ALIGN(0x4);
        _abortstackstart = .;
        . += 0x400;
        _abortstackend = .;
        _irqstackstart = .;
        . += 0x400;
        _irqstackend = .;
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

    .lcdfb (NOLOAD) :
    {
        _lcdfbstart = .;
        *(.lcdfb*)
        . = ALIGN(0x4);
        _lcdfbend = .;
    } > LCD

    /DISCARD/ :
    {
        *(.eh_frame)
    }

    /* stage1.S builds the first-level page table at 0x2202c000 and it stays the live TTBR0 (crt0 only enables the
       MMU). .ibss must end below it or clearibss zeroes live descriptors. */
    ASSERT(_ibssend <= 0x2202C000, ".ibss overflowed into the 0x2202C000 stage-1 MMU page table (SRAM budget)")

    /* framebuffer must fit its pinned region and stay clear of the 64 MiB SDRAM end */
    ASSERT(_lcdfbend <= 0x0C000000, ".lcdfb (LCD framebuffer) overflowed past the 0x0C000000 SDRAM end")

    /* the ROM's DFU buffer is 0x25200 B and its datalen check is < 0x24c01: the load
       image (.inithead..inittail) plus stage 1 (~1.5 KiB) must fit. tools/nano5g/image.py enforces the exact limit. */
    ASSERT(_inittailend - 0x08000000 <= 0x24000, "emcore.bin exceeds the ROM DFU budget (0x24c00 minus stage 1 and the IMG1 footer)")
}
