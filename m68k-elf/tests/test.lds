ENTRY(_start)

MEMORY
{
    rom (rwx) : ORIGIN = 0x0, LENGTH = 16M
    ram (rw)  : ORIGIN = 0x80000000, LENGTH = 16M
}

SECTIONS
{
    .text : {
        __code_start__ = .;
        *(.text*)
        *(.rodata*)
    } > rom

    .got : {
        *(.got)
        LONG(0)
    } > ram

    .data.reloc : {
        . += __reloc_size__;
        LONG(0)
    } > ram

    .data : {
        *(.data*)
    } > ram

    .bss : {
        *(.bss*)
        *(COMMON)
    } > ram
}
