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

# Keep the link-time offsets, but map neither test image at address zero.
# Hosted runners can reject low mappings through vm.mmap_min_addr.
map_image() {
    local image=$1 flash_bias=$2 ram_bias=$3
    "${cross}objcopy" --adjust-start "$flash_bias" \
        --change-section-address ".text+$flash_bias" \
        --change-section-address ".data.reloc+$flash_bias" \
        --change-section-address ".got+$ram_bias" \
        --change-section-address ".data+$ram_bias" \
        --change-section-address ".bss+$ram_bias" \
        "$build/test.elf" "$build/$image.elf"
}

map_image test-base 0x20000 0
map_image test-moved 0x10020000 0x20000000

for image in test-base test-moved; do
    if ! "$qemu" -cpu m68000 "$build/$image.elf"; then
        echo "$image failed" >&2
        exit 1
    fi
    echo "$image passed"
done

# Package the same test payload with the production linker and entry stub.
# Stub out firmware-dependent C services so QEMU can drive the message ABI.
os3k=$(realpath ../../os3k)
cat > "$build/version.h" <<'VERSION'
#define BETAWISE_VERSION_MAJOR 0
#define BETAWISE_VERSION_MINOR 0
#define BETAWISE_VERSION_REVISION " "
VERSION
"${cross}gcc" -Os -m68000 -msep-data -fomit-frame-pointer -fshort-enums \
    -ffreestanding -ffunction-sections -fdata-sections -DOS3K_QEMU_SMOKE \
    -I"$build" -I"$os3k" -c test.c -o "$build/test.o"
"${cross}gcc" -Os -m68000 -msep-data -c externs.c -o "$build/externs.o"
"${cross}gcc" -m68000 -c "$os3k/crt0.S" -o "$build/crt0.o"
make -C "$build" -f "$os3k/../Makefile.common" CROSS_COMPILE="$cross" \
    OS3K_LIB= 'OBJ=test.o externs.o crt0.o' smoke.OS3KApp
for text_address in 0x10020 0x320020; do
    for ram_bias in 0 2; do
        "${cross}gcc" -m68000 -static -nostdlib -Wa,-I,"$build",--defsym,ram_bias="$ram_bias" \
            -Wl,-Ttext,"$text_address",--build-id=none dispatch.S -o "$build/dispatch.elf"
        if ! "$qemu" -cpu m68000 "$build/dispatch.elf"; then
            echo "packed applet dispatch failed at $text_address with RAM bias $ram_bias" >&2
            exit 1
        fi
        echo "packed applet dispatch passed at $text_address with RAM bias $ram_bias"
    done
done
