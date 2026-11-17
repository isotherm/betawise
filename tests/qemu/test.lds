ENTRY(_start)

PHDRS
{
    flash PT_LOAD FLAGS(5);
    ram PT_LOAD FLAGS(6);
}

SECTIONS
{
    .text 0 : {
        __code_start__ = .;
        *(.text.startup.qemu)
        *(.text*)
        *(.rodata*)
        . = ALIGN(4);
        . += 0x8000;
        *(.far.text)
    } :flash

    .data.reloc : {
        __reloc_table_offset = ABSOLUTE(.);
        . += __reloc_size__;
        LONG(0)
    } :flash

    .got 0x80000000 : {
        *(.got*)
        LONG(0)
    } :ram

    .data : {
        *(.data*)
    } :ram

    .bss : {
        *(.bss*)
        *(COMMON)
    } :ram
}
ASSERT(extern_far_func - test_case_3 > 0x7fff, "far call must cross the PC16 range")
