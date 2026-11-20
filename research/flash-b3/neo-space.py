#!/usr/bin/env python3
"""Inspect private NEO update packages; never equate omitted bytes with free NOR."""
import hashlib
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
WORKSPACE = ROOT.parent

def erased_runs(data, base, minimum=256):
    result = []
    start = None
    for offset, byte in enumerate(data + b'\0'):
        if byte == 255 and start is None:
            start = offset
        elif byte != 255 and start is not None:
            if offset - start >= minimum:
                result.append({'begin': hex(base+start), 'end_exclusive': hex(base+offset),
                               'bytes': offset-start})
            start = None
    return result

inventory = json.loads((ROOT / 'research/abi-audit/rom-inventory.json').read_text())
result = []
for version in ('neo_3_01', 'neo_3_21'):
    entry = next(item for item in inventory if item['name'] == version)
    image = (WORKSPACE / entry['path']).read_bytes()
    assert hashlib.sha256(image).hexdigest() == entry['sha256']
    if version == 'neo_3_01':
        # Historical 2005 layout, not the 2013 rounded transfer mapper.
        segments = [(0x5C0000, image[:0x40000])]
    else:
        segments = []
        cursor = 0
        for pos in range(0x50, 0x70, 8):
            address = int.from_bytes(image[pos:pos+4], 'big')
            length = int.from_bytes(image[pos+4:pos+8], 'big')
            if address == length == 0:
                break
            transfer = (length + 1023) & ~1023
            assert cursor + transfer <= len(image)
            segments.append((address, image[cursor:cursor+transfer]))
            cursor += transfer
        assert cursor == len(image)
    result.append({'version': version, 'sha256': entry['sha256'],
        'segments': [{'begin': hex(address), 'transfer_bytes': len(data),
                      'erased_runs_at_least_256': erased_runs(data, address)}
                     for address, data in segments]})
print(json.dumps(result, indent=2))
