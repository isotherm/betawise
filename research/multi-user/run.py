#!/usr/bin/env python3
"""Run multi-user qualification against disposable AS3000 flash/RAM."""
import os
import json
from pathlib import Path
import subprocess
import sys
ROOT = Path(__file__).resolve().parents[2]
out = Path(sys.argv[1]).resolve()
out.mkdir(parents=True, exist_ok=False)
roms = ROOT.parent / 'mame/roms'
flash = bytearray(b'\xff' * 0x100000)
small = (roms / 'asma3k/smallos3krom.os3kos').read_bytes()
full = (ROOT / 'tests/mame/asma3k/OS3KROM.OS3KOS').read_bytes()
flash[:len(small)] = small
flash[0xC0000:] = full[:0x40000]
flash[0x4000:0x7BF0] = full[0x40000:]
pos = 0x8000
for name in ['alphawordplus.os3kapp', 'keywords.os3kapp']:
    data = (roms / 'asma3k' / name).read_bytes()
    flash[pos:pos+len(data)] = data
    pos += len(data)
if sys.byteorder == 'little':
    flash[0::2], flash[1::2] = flash[1::2], flash[0::2]
nvram = out / 'nvram/asma3k'
nvram.mkdir(parents=True)
(nvram / 'flash').write_bytes(flash)
script = out / 'probe.lua'
targets=json.loads((ROOT / 'research/abi-audit/rom-inventory.json').read_text())[0]['targets']
script.write_text('local slots={' + ','.join(f'[{i}]={v}' for i,v in enumerate(targets)) + '}\n' + (ROOT / 'research/multi-user/probe.lua').read_text(errors='replace'))
with (out / 'mame.log').open('w') as log:
    result = subprocess.run([str(ROOT.parent / 'mame/mame'), 'asma3k', '-rompath', str(roms),
        '-nvram_directory', str(out / 'nvram'), '-video', 'none', '-sound', 'none',
        '-nothrottle', '-seconds_to_run', '180', '-autoboot_delay', '0',
        '-autoboot_script', str(script)], cwd=out,
        env={**os.environ, 'SDL_VIDEODRIVER': 'dummy'}, stdout=log, stderr=subprocess.STDOUT)
print((out / 'mame.log').read_text(errors='replace'))
if result.returncode or 'PASS multi-user qualification complete' not in (out / 'mame.log').read_text(errors='replace'):
    raise SystemExit(1)
