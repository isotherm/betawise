#!/usr/bin/env python3
"""Compare stock AS3000 allocation with two temporary fixed-limit ROM edits.

This is an admission/reporting experiment, not a complete archive-protection patch.
Only private copies of firmware under the requested temporary output are edited.
"""
import hashlib
import os
from pathlib import Path
import subprocess
import sys

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
OUT = Path(sys.argv[1] if len(sys.argv) > 1 else '/tmp/as3000-flash-boundary').resolve()
OUT.mkdir(parents=True, exist_ok=False)
original = (ROOT / 'tests/mame/asma3k/OS3KROM.OS3KOS').read_bytes()
assert hashlib.sha256(original).hexdigest() == '732dca63399d883071ce41cc07ad352fa19c506b7b8c307ace16d5c53e6f8708'
applet = ROOT / 'applets/HelloWorld/HelloWorld.OS3KApp'
end = 0x408000 + len(applet.read_bytes())
for name, limit, response in [('stock', 0x4C0000, 0x46), ('two-constant-experiment', 0x480000, 0x88)]:
    run = OUT / name
    roms = run / 'roms/asma3k'
    roms.mkdir(parents=True)
    image = bytearray(original)
    for offset in (0x117E, 0x267E):
        assert image[offset:offset+4] == bytes.fromhex('004c0000')
        image[offset:offset+4] = limit.to_bytes(4, 'big')
    (roms / 'os3krom.os3kos').write_bytes(image)
    (roms / 'smallos3krom.os3kos').symlink_to(ROOT.parent / 'mame/roms/asma3k/smallos3krom.os3kos')
    script = run / 'probe.lua'
    script.write_text(f'local test_case="{name}"\nlocal expected_end=0x{limit:X}\nlocal expected_response=0x{response:X}\nlocal applet_end=0x{end:X}\n' +
        (ROOT / 'tests/mame/asma3k/asma3k.lua').read_text() + (HERE / 'probe.lua').read_text())
    with (run / 'mame.log').open('w') as log:
        result = subprocess.run([str(ROOT.parent / 'mame/mame'), 'asma3k', '-rompath',
            str(run / 'roms')+';'+str(ROOT.parent / 'mame/roms'), '-nvram_directory', str(run / 'nvram'),
            '-applet', str(applet), '-video', 'none', '-sound', 'none', '-nothrottle', '-seconds_to_run', '18',
            '-autoboot_delay', '0', '-autoboot_script', str(script)], cwd=run,
            env={**os.environ, 'SDL_VIDEODRIVER':'dummy'}, stdout=log, stderr=subprocess.STDOUT)
    text = (run / 'mame.log').read_text()
    print(name, run)
    if result.returncode or 'LUA ERROR' in text or 'PASS '+name not in text:
        print(text)
        raise SystemExit(1)
    for line in text.splitlines():
        if line.startswith(('FREE ', 'RAM_FLOOR_', 'BEGIN ', 'PASS ')): print(line)
