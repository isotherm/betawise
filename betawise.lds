OUTPUT_FORMAT(binary)

SECTIONS
{
    . = 0x0;
    .text : { *(bw_header) *(.text*) *(.rodata*) *(bw_footer) }
    __bw_rom_size = .;
    . = 0x0;
    .bss : { *(COMMON) *(.bss*) }
    __bw_bss_size = .;
    /DISCARD/ : { *(*) }
}
