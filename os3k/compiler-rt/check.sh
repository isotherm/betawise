#!/usr/bin/env bash
set -euo pipefail

here=$(cd -- "$(dirname -- "$0")" && pwd)
cross=${CROSS_COMPILE:-m68k-linux-gnu-}
qemu=${QEMU_M68K:-qemu-m68k}
build=$(mktemp -d)
trap 'rm -r -- "$build"' EXIT
cat > "$build/start.S" <<'ASM'
    .global _start
_start:
    move.l #_GLOBAL_OFFSET_TABLE_,%a5
    bsr smoke
    move.l %d0,%d1
    moveq #1,%d0
    trap #0
    .section .note.GNU-stack,"",@progbits
ASM

"${cross}gcc" -m68000 -msoft-float -msep-data -ffreestanding -Os \
    -ffunction-sections -fdata-sections -nostdlib -static -Wl,--gc-sections \
    "$build/start.S" "$here/smoke.c" "$here/../libos3k.a" \
    -o "$build/smoke.elf"
"$qemu" -cpu m68000 "$build/smoke.elf" || {
    status=$?
    echo "compiler-rt arithmetic check failed at case $status" >&2
    exit "$status"
}
echo "compiler-rt arithmetic check passed on QEMU m68000"
