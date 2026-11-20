#!/usr/bin/env python3
"""Exercise the AS3000 serial updater's battery gate without programming flash."""
import hashlib
import os
from pathlib import Path
import subprocess
import sys

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
OUT = Path(sys.argv[1] if len(sys.argv) > 1 else '/tmp/as3000-flash-power').resolve()
OUT.mkdir(parents=True, exist_ok=False)
rom = ROOT / 'tests/mame/asma3k/OS3KROM.OS3KOS'
assert hashlib.sha256(rom.read_bytes()).hexdigest() == '732dca63399d883071ce41cc07ad352fa19c506b7b8c307ace16d5c53e6f8708'
script = OUT / 'probe.lua'
script.write_text((ROOT / 'tests/mame/asma3k/asma3k.lua').read_text() + (HERE / 'probe.lua').read_text())
with (OUT / 'mame.log').open('w') as log:
    result = subprocess.run([str(ROOT.parent / 'mame/mame'), 'asma3k',
        '-rompath', str(ROOT.parent / 'mame/roms'), '-nvram_directory', str(OUT / 'nvram'),
        '-applet', str(ROOT / 'applets/HelloWorld/HelloWorld.OS3KApp'), '-video', 'none',
        '-sound', 'none', '-nothrottle', '-seconds_to_run', '14', '-autoboot_delay', '0',
        '-autoboot_script', str(script)], cwd=OUT, env={**os.environ, 'SDL_VIDEODRIVER':'dummy'},
        stdout=log, stderr=subprocess.STDOUT)
text = (OUT / 'mame.log').read_text()
print(text)
if result.returncode or 'LUA ERROR' in text or 'PASS battery-powered admission gate' not in text:
    raise SystemExit(1)
