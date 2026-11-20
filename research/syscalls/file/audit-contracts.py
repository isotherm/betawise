#!/usr/bin/env python3
"""Extract issue #15's 38 ROM spans and assert discriminating instruction anchors.

Private ROM bytes/disassembly are written only to the requested evidence directory.
The anchors complement manual data-flow review and live tests; they do not certify
all branches or infer contracts merely from opcode presence.
"""
from pathlib import Path
import argparse
import hashlib
import json
import struct
import subprocess

REPO = Path(__file__).resolve().parents[3]
WORKSPACE = REPO.parent
SLOTS = [*range(102, 113), *range(114, 120), 122, 123, 125, 126, 127,
         *range(128, 136), 137, 138, *range(140, 146)]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('output', type=Path)
args = parser.parse_args()
out = args.output.resolve()
out.mkdir(parents=True, exist_ok=True)
records = []
for rom in json.loads((REPO / 'research/abi-audit/rom-inventory.json').read_text()):
    path = WORKSPACE / rom['path']
    blob = path.read_bytes()
    assert hashlib.sha256(blob).hexdigest() == rom['sha256']
    folder = out / rom['name']
    folder.mkdir(exist_ok=True)
    def span(slot):
        start = rom['targets'][slot]
        end = min(t for t in rom['targets'] if t > start)
        return blob[start-rom['base']:end-rom['base']]
    def anchor(slot, offset, expected):
        expected = bytes.fromhex(expected)
        assert span(slot)[offset:offset+len(expected)] == expected, (rom['name'], slot, hex(offset))
    # CMP logical,reserved; BLS selects reserved, otherwise adds logical: MAX.
    for offset in (0x1c, 0x60):
        anchor(109, offset, '202c0004 b0ac0010 6304 d196 6006 202c0010 d196')
    # When current+40 <= saved it selects current+40; otherwise saved: MIN.
    anchor(110, 0x76, '70fa be80 6618 7028 d0ac0004 b0ac0008 6306 2e2c0008 6006 7e28 deac0004')
    anchor(110, 0xbc, 'beac000c 6304 2e2c000c')  # unsigned capacity clamp
    anchor(140, 0, '70ff 23c0')
    anchor(141, 0x1e, '30280014 6002 4240 4e75')
    anchor(134, 0, '1039')  # MOVE.B absolute into D0, not MOVEQ+MOVE.B
    anchor(144, 0x34, '4200')  # CLR.B rejection
    anchor(145, 0x3c, '4200')
    # Follow A1A0's BSR to the actual shared resolver, not its pointer return alone.
    handler = rom['targets'][104]
    resolver = handler + 0x0e + struct.unpack('>h', span(104)[0x0e:0x10])[0]
    rb = blob[resolver-rom['base']:resolver-rom['base']+0x1ce]
    assert bytes.fromhex('70f9 2080') in rb  # output status = -7
    assert bytes.fromhex('70ff 2080') in rb  # output status = -1
    for slot in SLOTS:
        start = rom['targets'][slot]
        body = span(slot)
        (folder / f'{slot:03}.bin').write_bytes(body)
        command = ['m68k-linux-gnu-objdump', '-D', '-b', 'binary', '-m', 'm68k:68000',
                   '--adjust-vma=' + str(rom['base']), '--start-address=' + str(start),
                   '--stop-address=' + str(start+len(body)), str(path)]
        (folder / f'{slot:03}.dis').write_bytes(subprocess.check_output(command))
        records.append(dict(rom=rom['name'], slot=slot, opcode=f'A{slot*4:03X}',
                            target=f'{start:08X}', span_bytes=len(body),
                            sha256=hashlib.sha256(body).hexdigest()))
    print('PASS static', rom['name'], len(SLOTS), 'spans and selector/width/resolver anchors')
(out / 'manifest.json').write_text(json.dumps(records, indent=2) + '\n')
