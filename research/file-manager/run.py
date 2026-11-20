#!/usr/bin/env python3
"""Cold-boot stock AlphaWord plus File Manager in disposable AS3000 flash/RAM."""
import os
import pathlib
import subprocess
import sys

HERE = pathlib.Path(__file__).resolve().parent
ROOT = HERE.parent.parent
OUT = pathlib.Path(sys.argv[1] if len(sys.argv) > 1 else '/tmp/os3k-file-manager').resolve()
OUT.mkdir(parents=True, exist_ok=False)
roms = ROOT.parent / 'mame/roms'
flash = bytearray(b'\xff' * 0x100000)
small = (roms / 'asma3k/smallos3krom.os3kos').read_bytes()
full = (ROOT / 'tests/mame/asma3k/OS3KROM.OS3KOS').read_bytes()
alpha = (roms / 'asma3k/alphawordplus.os3kapp').read_bytes()
assert len(full) == 0x43BF0 and int.from_bytes(alpha[4:8], 'big') == len(alpha)
flash[:len(small)] = small
flash[0xC0000:] = full[:0x40000]
flash[0x4000:0x7BF0] = full[0x40000:]
flash[0x8000:0x8000+len(alpha)] = alpha
# MAME persists its big-endian bus's native uint16_t backing store in host order.
if sys.byteorder == 'little':
    flash[0::2], flash[1::2] = flash[1::2], flash[0::2]
nvram = OUT / 'nvram/asma3k'
nvram.mkdir(parents=True)
(nvram / 'flash').write_bytes(flash)
script = OUT / 'probe.lua'
script.write_text((ROOT / 'tests/mame/asma3k/asma3k.lua').read_text() + (HERE / 'probe.lua').read_text())
with (OUT / 'mame.log').open('w') as log:
    result = subprocess.run([str(ROOT.parent / 'mame/mame'), 'asma3k', '-rompath', str(roms),
        '-nvram_directory', str(OUT / 'nvram'), '-snapshot_directory', str(OUT / 'snap'),
        '-applet', str(ROOT / 'applets/FileManager/FileManager.OS3KApp'),
        '-video', 'none', '-sound', 'none', '-nothrottle', '-seconds_to_run', '40',
        '-autoboot_delay', '0', '-autoboot_script', str(script)], cwd=OUT,
        env={**os.environ, 'SDL_VIDEODRIVER':'dummy'}, stdout=log, stderr=subprocess.STDOUT)
text = (OUT / 'mame.log').read_text()
print(text)
if result.returncode or 'LUA ERROR' in text or 'PASS retained name and tag after delete cancellation' not in text:
    raise SystemExit(1)
print(OUT)
