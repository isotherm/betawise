OUTPUT_FORMAT(binary)

SECTIONS
{
    . = 0x0;
    header : { *(header) }    
    .text : { *(.text*) *(.rodata*) }
    footer : { *(footer) }
    __rom_size = .;
    . = 0x0;
    .bss : { *(.bss*) }
    /DISCARD/ : { *(*) }
}
