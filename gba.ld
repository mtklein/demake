/* GBA linker script: ROM at 0x08000000, IWRAM 32K, EWRAM 256K. */
OUTPUT_FORMAT("elf32-littlearm")
OUTPUT_ARCH(arm)
ENTRY(_start)

MEMORY
{
    rom   (rx)  : ORIGIN = 0x08000000, LENGTH = 32M
    iwram (rwx) : ORIGIN = 0x03000000, LENGTH = 0x7F00  /* keep clear of BIOS stacks */
    ewram (rw)  : ORIGIN = 0x02000000, LENGTH = 256K
}

SECTIONS
{
    .text :
    {
        KEEP(*(.crt0))
        *(.text*)
        *(.glue_7) *(.glue_7t)
        . = ALIGN(4);
    } > rom

    .rodata :
    {
        *(.rodata*)
        . = ALIGN(4);
    } > rom

    /* ARM code that must run from IWRAM (IRQ handler, hot loops). */
    .iwram : ALIGN(4)
    {
        __iwram_start = .;
        *(.iwram*)
        . = ALIGN(4);
        __iwram_end = .;
    } > iwram AT> rom
    __iwram_lma = LOADADDR(.iwram);

    .data : ALIGN(4)
    {
        __data_start = .;
        *(.data*)
        . = ALIGN(4);
        __data_end = .;
    } > iwram AT> rom
    __data_lma = LOADADDR(.data);

    .bss (NOLOAD) : ALIGN(4)
    {
        __bss_start = .;
        *(.bss*)
        *(COMMON)
        . = ALIGN(4);
        __bss_end = .;
    } > iwram

    /* Big buffers live in EWRAM: __attribute__((section(".ewram_bss"))) */
    .ewram_bss (NOLOAD) : ALIGN(4)
    {
        __ewram_bss_start = .;
        *(.ewram_bss*)
        . = ALIGN(4);
        __ewram_bss_end = .;
    } > ewram

    __rom_end = LOADADDR(.data) + SIZEOF(.data);

    /DISCARD/ : { *(.ARM.exidx*) *(.ARM.attributes) *(.comment) }
}
