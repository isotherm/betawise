#!/usr/bin/env python3
"""Write the big-endian RAM fixup table for an OS3K ELF."""
import argparse
import re
import struct
import subprocess
from pathlib import Path

RAM_BASE = 0x80000000
HEADER_FIELDS = {
    4: "__os3k_rom_size",
    8: "__os3k_ram_size",
    0x84: "Os3kEntry",
}


def readelf(executable, image, option):
    return subprocess.check_output([executable, option, image], text=True)


def fixups(executable, image):
    symbols = {}
    for line in readelf(executable, image, "-sW").splitlines():
        fields = line.split()
        if len(fields) >= 8 and re.fullmatch(r"\d+:", fields[0]):
            index = int(fields[0][:-1])
            symbols[index] = (fields[6], fields[7], int(fields[1], 16))
    values = {name: value for _, name, value in symbols.values()}
    got_size = values["__os3k_got_size"]
    image_size = values["__os3k_image_size"]
    rom_size = values["__os3k_rom_size"]
    ram_size = values["__os3k_ram_size"]
    got_base = values.get("_GLOBAL_OFFSET_TABLE_", RAM_BASE)
    if got_size and got_base != RAM_BASE:
        raise ValueError(f"GOT base is {got_base:#x}, expected {RAM_BASE:#x}")

    offsets = set()
    for line in readelf(executable, image, "-rW").splitlines():
        fields = line.split()
        if len(fields) < 7 or not fields[2].startswith("R_68K_"):
            continue
        kind = fields[2]
        section, name, value = symbols[int(fields[1], 16) >> 8]
        if kind in {"R_68K_GOT8O", "R_68K_GOT16O", "R_68K_GOT32O"} and section == "ABS":
            raise ValueError(f"absolute GOT target cannot be rebased: {line.strip()}")
        if kind != "R_68K_32":
            continue
        address = int(fields[0], 16)
        if HEADER_FIELDS.get(address) == name or section == "ABS":
            continue
        offset = address - RAM_BASE
        if not got_size <= offset <= image_size - 4 or not offset or offset % 2:
            raise ValueError(f"absolute relocation outside initialized data: {line.strip()}")
        addend = int(fields[6], 16)
        target = value + (addend if fields[5] == "+" else -addend)
        if not (0 <= target <= rom_size or RAM_BASE <= target <= RAM_BASE + ram_size):
            raise ValueError(f"unsupported pointer target: {line.strip()}")
        if offset in offsets:
            raise ValueError(f"duplicate fixup at RAM offset {offset:#x}")
        offsets.add(offset)
    return sorted(offsets)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("readelf")
    parser.add_argument("image")
    parser.add_argument("table", type=Path)
    args = parser.parse_args()
    try:
        offsets = fixups(args.readelf, args.image)
    except (KeyError, ValueError) as error:
        parser.exit(1, f"{args.image}: {error}\n")
    args.table.write_bytes(b"".join(struct.pack(">I", offset) for offset in [*offsets, 0]))
    print(len(offsets))


if __name__ == "__main__":
    main()
