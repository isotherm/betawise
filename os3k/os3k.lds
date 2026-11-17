/* The ELF is linked at flash-relative addresses and tagged RAM addresses.
 * objcopy uses the LMAs to lay out the flat OS3K image in this order:
 * header, zero-terminated RAM fixup offsets, initial GOT/data, code/rodata,
 * and footer. The NOBITS section contributes only to the RAM allocation.
 */
OUTPUT_FORMAT(elf32-m68k)
ENTRY(Os3kEntry)

MEMORY
{
    rom (rx) : ORIGIN = 0, LENGTH = 16M
    ram (rw) : ORIGIN = 0x80000000, LENGTH = 16M
}

SECTIONS
{
    os3k_header : {
        KEEP(*(os3k_header))
    } > rom

    .data.reloc : {
        . += __os3k_reloc_size * 4;
        LONG(0)
    } > rom

    .rom.ram : {
        *(.got.plt)
        *(.got .got.*)
        __os3k_got_end = .;
        *(.data .data.*)
        . = ALIGN(2);
    } > ram AT > rom

    .rom : {
        KEEP(*(.text.Os3kEntry))
        *(.text .text.*)
        *(.rodata .rodata.*)
        . = ALIGN(2);
        KEEP(*(os3k_footer))
    } > rom

    .ram (NOLOAD) : {
        *(.bss .bss.*)
        *(COMMON)
        . = ALIGN(2);
    } > ram

    __os3k_entry_offset = ABSOLUTE(Os3kEntry);
    __os3k_fixups_offset = ABSOLUTE(ADDR(.data.reloc));
    __os3k_image_offset = LOADADDR(.rom.ram);
    __os3k_image_size = SIZEOF(.rom.ram);
    __os3k_got_size = ABSOLUTE(__os3k_got_end - ORIGIN(ram));
    __os3k_bss_offset = ADDR(.ram) - ORIGIN(ram);
    __os3k_bss_size = SIZEOF(.ram);
    __os3k_rom_size = ADDR(.rom) + SIZEOF(.rom);
    __os3k_ram_size = __os3k_bss_offset + __os3k_bss_size;

    /DISCARD/ : {
        *(.comment .note* .eh_frame*)
    }
}

ASSERT(SIZEOF(os3k_header) == 0x94, "OS3K applet header must be 148 bytes")
ASSERT(ADDR(.rom.ram) == ORIGIN(ram), "GOT must begin at A5")
ASSERT(__os3k_got_size % 4 == 0, "GOT must contain whole longwords")
