#!/usr/bin/env bash
set -euo pipefail

cd -- "$(dirname -- "$0")"
cross=${CROSS_COMPILE:-m68k-linux-gnu-}
qemu=${QEMU_M68K:-qemu-m68k}
build=$(mktemp -d)
trap 'rm -rf -- "$build"' EXIT

link() {
    "${cross}gcc" -Os -g0 -m68000 -msep-data -fomit-frame-pointer \
        -ffreestanding -fdata-sections -ffunction-sections -DQEMU_TEST \
        -static -nostdlib -Wl,-Ttest.lds,--build-id=none,--emit-relocs \
        -Wl,--defsym,__reloc_size__="$1" crt0.S test.c externs.c -o "$build/test.elf"
}

relocations() {
    "${cross}readelf" -Wr "$build/test.elf" |
        awk '$3 == "R_68K_32" && $5 != "__reloc_table_offset" {
            if ($1 !~ /^8/) {
                print "Absolute fixup outside RAM: " $0 > "/dev/stderr"
                bad = 1
            } else {
                print $1
            }
        }
        END {exit bad}'
}

link 0
relocations > "$build/offsets"
link "$((4 * $(wc -l < "$build/offsets")))"
relocations > "$build/final-offsets"
cmp "$build/offsets" "$build/final-offsets"
{
    cat "$build/final-offsets"
    echo 00000000
} | xxd -r -p > "$build/relocs.bin"
"${cross}objcopy" --update-section .data.reloc="$build/relocs.bin" "$build/test.elf"

"${cross}objcopy" --adjust-start 0x10000000 \
    --change-section-address .text+0x10000000 \
    --change-section-address .data.reloc+0x10000000 \
    --change-section-address .got+0x20000000 \
    --change-section-address .data+0x20000000 \
    --change-section-address .bss+0x20000000 \
    "$build/test.elf" "$build/test-moved.elf"

for image in test test-moved; do
    if ! "$qemu" -cpu m68000 "$build/$image.elf"; then
        echo "$image failed" >&2
        exit 1
    fi
    echo "$image passed"
done
