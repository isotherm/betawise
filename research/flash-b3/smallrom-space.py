#!/usr/bin/env python3
"""Check bit-clear hook encodings; omitted package tails are NOT proven free."""
import hashlib
import json
from pathlib import Path

WORKSPACE = Path(__file__).resolve().parents[3]
CASES = [
    ('as3000', 'Alphasmart 3000', 'SMALLOS3KROM.OS3KOS', 0xBD8, 0x403F98, 0x404000),
    ('neo05', 'Alphasmart 3000', 'SMALLOS3KNeoROM.OS3KOS', 0x1328, 0x405F98, 0x406000),
    ('neo13', 'Alphasmart Neo', 'smallos3kneorom.os3kos', 0x143E, 0x405F98, 0x406000),
]
rows = []
for model, folder, filename, offset, target, boundary in CASES:
    path = WORKSPACE/'Ghidra'/folder/'SmartApplets'/filename
    data = path.read_bytes()
    instruction = data[offset:offset+6]
    assert instruction[:2] == bytes.fromhex('4EB9')
    old = int.from_bytes(instruction[2:], 'big')
    assert old & target == target and target % 2 == 0
    end = 0x400000+len(data)
    assert end <= target < boundary
    candidates = [value for value in range((end+1)&~1, boundary-5, 2)
                  if value & old == value]
    minimum = min((old^value).bit_count() for value in candidates)
    assert (old^target).bit_count() == minimum
    clear_mask = old ^ target
    and_mask = (~clear_mask) & 0xFFFFFFFF
    assert old & and_mask == target
    assert target & (~old & 0xFFFFFFFF) == 0
    old_bytes, new_bytes = old.to_bytes(4,'big'), target.to_bytes(4,'big')
    changed_bytes = [index for index in range(4) if old_bytes[index] != new_bytes[index]]
    for previous, replacement in zip(instruction, instruction[:2]+new_bytes):
        assert previous & replacement == replacement
    rows.append(dict(model=model,path=str(path.relative_to(WORKSPACE)),
        sha256=hashlib.sha256(data).hexdigest(),
        instruction_address=hex(0x400000+offset),original_target=hex(old),
        candidate_target=hex(target),changed_bits=(old^target).bit_count(),
        minimum_tail_bits=minimum,and_mask=f'0x{and_mask:08X}',
        zero_to_one_bits=target & (~old & 0xFFFFFFFF),
        changed_address_bytes=changed_bytes,
        changed_16bit_words=sorted({index//2 for index in changed_bytes}),
        image_end_exclusive=hex(end),next_package_region=hex(boundary),
        omitted_bytes=boundary-end,bytes_from_candidate=boundary-target,
        qualification='Encoding only: tail contents and non-use not established'))
print(json.dumps(rows,indent=2))
