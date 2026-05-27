#!/bin/bash

rm -f test.elf test-moved.elf

m68k-linux-gnu-gcc \
    -Os \
    -msep-data \
    -mcpu=68000 \
    -fomit-frame-pointer \
    -ffreestanding \
    -fdata-sections \
    -ffunction-sections \
    -ffixed-a5 \
    -static \
    -nostdlib \
    -Wl,-Ttest.lds \
    -Wl,-z noexecstack \
    -Wl,--build-id=none \
    -Wl,--emit-relocs \
    -Wl,--defsym __reloc_size__=0 \
    crt0.S test.c externs.c \
    -o test.elf

RELOC_SIZE=$(($(m68k-linux-gnu-readelf -r test.elf | grep R_68K_32 | cut -f 1 -d ' ' | wc -l)*4))

m68k-linux-gnu-gcc \
    -Os \
    -msep-data \
    -mcpu=68000 \
    -fomit-frame-pointer \
    -ffreestanding \
    -fdata-sections \
    -ffunction-sections \
    -static \
    -nostdlib \
    -Wl,-Ttest.lds \
    -Wl,-z noexecstack \
    -Wl,--build-id=none \
    -Wl,--emit-relocs \
    -Wl,--defsym __reloc_size__=$RELOC_SIZE \
    crt0.S test.c externs.c \
    -o test.elf

m68k-linux-gnu-readelf -r test.elf | grep R_68K_32 | cut -f 1 -d ' ' | xxd -r -p > relocs.bin
m68k-linux-gnu-objcopy --update-section .data.reloc=relocs.bin test.elf
rm -f relocs.bin

m68k-linux-gnu-objcopy \
    --adjust-start 0x10000000 \
    --change-section-address .text+0x10000000 \
    --change-section-address .got+0x20000000 \
    --change-section-address .data.reloc+0x20000000 \
    --change-section-address .data+0x20000000 \
    --change-section-address .bss+0x20000000 \
    test.elf test-moved.elf

qemu-m68k test.elf
echo "obase=2; $?" | bc

qemu-m68k test-moved.elf
echo "obase=2; $?" | bc
