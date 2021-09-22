/*
 * ld does not garbage collect sections with binary output format.
 * As a workaround, generate an ELF file to `objcopy` to binary.
*/
OUTPUT_FORMAT(elf32-m68k)

SECTIONS
{
    . = 0x0;
    .text : {
        KEEP(*(os3k_header))
        KEEP(*(.text.BwProcessMessage))
        *(.text*)
        *(.rodata*)
        KEEP(*(os3k_footer))
    }
    __os3k_rom_size = .;
    . = 0x0;
    .bss : {
        KEEP(*(COMMON))
        KEEP(*(.bss*))
    }
    __os3k_bss_size = .;
    /DISCARD/ : {
        *(*)
    }
}
