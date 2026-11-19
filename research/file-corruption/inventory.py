#!/usr/bin/env python3
"""Hash local firmware and enumerate file-service call candidates; no ROM edits.

Run from any directory. Private listings go to the supplied output directory.
Candidates are byte-level leads, not proof of reachable instructions.
"""
from pathlib import Path
import hashlib
import json
import re
import subprocess
import sys

WORKSPACE = Path(__file__).resolve().parents[3]
HERE = Path(__file__).resolve().parent
OUT = Path(sys.argv[1]).resolve()
OUT.mkdir(parents=True, exist_ok=True)
roms = json.loads((HERE.parent / 'abi-audit/rom-inventory.json').read_text())
vendors = json.loads((HERE.parent / 'abi-audit/vendor-image-inventory.json').read_text())

inventory = {}
for root in ('Ghidra', 'betawise/tests/mame', 'mame/roms'):
    for p in (WORKSPACE / root).rglob('*'):
        if p.is_file() and p.suffix.lower() in ('.os3kos', '.os3kapp'):
            b = p.read_bytes()
            h = hashlib.sha256(b).hexdigest()
            inventory.setdefault(h, {'bytes': len(b), 'paths': []})['paths'].append(str(p.relative_to(WORKSPACE)))
(OUT / 'local-images.json').write_text(json.dumps(inventory, indent=2) + '\n')

for r in roms:
    b = (WORKSPACE / r['path']).read_bytes()
    assert hashlib.sha256(b).hexdigest() == r['sha256']
    listing = subprocess.check_output([
        'm68k-linux-gnu-objdump', '-D', '-b', 'binary', '-m', 'm68k:68000',
        '--adjust-vma=' + str(r['base']), str(WORKSPACE / r['path'])], text=True)
    (OUT / (r['name'] + '.dis')).write_text(listing)

results = []
for n, entry in enumerate(vendors):
    b = (WORKSPACE / entry['path']).read_bytes()
    assert hashlib.sha256(b).hexdigest() == entry['sha256']
    # Anchor on the contiguous file-service block, not individual A-line words.
    signature = b''.join((0xa000 + 4*s).to_bytes(2, 'big') for s in range(100, 139))
    anchor = b.find(signature)
    stubs = {anchor + 2*(s-100): s for s in range(100,139)} if anchor >= 0 else {}
    hits = set()
    for i in range(0,len(b)-6,2):
        w = int.from_bytes(b[i:i+2], 'big')
        if w in (0x4eba, 0x4efa, 0x6100) or w & 0xf1ff == 0x41fa:
            target = i+2+int.from_bytes(b[i+2:i+4], 'big', signed=True)
            if target in stubs: hits.add((i,stubs[target],'pc16'))
        if w & 0xff00 == 0x6100 and w & 255 not in (0,255):
            target = i+2+int.from_bytes(b[i+1:i+2], 'big', signed=True)
            if target in stubs: hits.add((i,stubs[target],'bsr8'))
        if w in (0x4eb9,0x4ef9):
            target = int.from_bytes(b[i+2:i+6], 'big')
            if target in stubs: hits.add((i,stubs[target],'absolute'))
        form = w & 0xf1ff
        if form in (0x207c,0x203c,0x307c,0x303c):
            wide = form in (0x207c,0x203c)
            size = 4 if wide else 2
            value = int.from_bytes(b[i+2:i+2+size], 'big', signed=True)
            reg = ((w>>9)&7) + (8 if form in (0x207c,0x307c) else 0)
            j = i+2+size
            op = int.from_bytes(b[j:j+2], 'big')
            ext = int.from_bytes(b[j+2:j+4], 'big')
            if (op in (0x4ebb,0x4efb) or op & 0xf1ff == 0x41fb) and ext>>12 == reg and not ext & 0x100:
                v = value if ext & 0x800 else ((value+32768)%65536)-32768
                target = j+2+v+int.from_bytes(b[j+3:j+4], 'big', signed=True)
                if target in stubs: hits.add((j,stubs[target],'indexed-pc'))
    listing = subprocess.check_output(['m68k-linux-gnu-objdump','-D','-b','binary','-m','m68k:68000',str(WORKSPACE / entry['path'])],text=True)
    (OUT / f'vendor-{n}.dis').write_text(listing)
    results.append({**entry, 'index':n, 'file_stubs':hex(anchor) if anchor>=0 else None,
                    'references':[{'offset':hex(i),'slot':s,'form':t} for i,s,t in sorted(hits)]})
(OUT / 'file-callers.json').write_text(json.dumps(results,indent=2)+'\n')
print(f'{len(inventory)} unique local images; {len(vendors)} applets; {sum(len(r["references"]) for r in results)} file-call candidates')
