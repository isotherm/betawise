#!/usr/bin/env python3
"""Execute actual m68k driver code with a scoped Intel CUI fixture in AS3000 MAME."""
import os
import pathlib
import subprocess
import sys

HERE = pathlib.Path(__file__).resolve().parent
ROOT = HERE.parent.parent
OUT = pathlib.Path(sys.argv[1] if len(sys.argv) > 1 else '/tmp/os3k-flash-b3').resolve()
MAME = ROOT.parent / 'mame/mame'
SYMBOLS = {}
for line in subprocess.check_output(['m68k-linux-gnu-nm', '-P', str(HERE / 'FlashProbe.elf')], text=True).splitlines():
    fields = line.split()
    if fields[0].startswith(('g_', '_FlashWorker')):
        SYMBOLS[fields[0]] = int(fields[2], 16)
common = (ROOT / 'tests/mame/asma3k/asma3k.lua').read_text()
fixture = (HERE / 'model.lua').read_text()
for name, emulate, device_id in [('bottom', True, 0xD3), ('top', True, 0xD2), ('unsupported', True, 0x99), ('stock-mame', False, 0)]:
    run = OUT / name
    run.mkdir(parents=True, exist_ok=False)
    declarations = f'local test_case="{name}"\nlocal emulate={str(emulate).lower()}\nlocal device_id={device_id}\n'
    declarations += 'local symbols={' + ','.join(f'{k}=0x{v:X}' for k, v in SYMBOLS.items()) + '}\n'
    script = run / 'probe.lua'
    script.write_text(declarations + common + fixture)
    with (run / 'mame.log').open('w') as log:
        result = subprocess.run([str(MAME), 'asma3k', '-rompath', str(ROOT.parent / 'mame/roms'),
            '-nvram_directory', str(run / 'nvram'), '-snapshot_directory', str(run / 'snap'),
            '-applet', str(HERE / 'FlashProbe.OS3KApp'), '-video', 'none', '-sound', 'none', '-nothrottle',
            '-seconds_to_run', '17', '-autoboot_delay', '0', '-autoboot_script', str(script)],
            cwd=run, stdout=log, stderr=subprocess.STDOUT, env={**os.environ, 'SDL_VIDEODRIVER':'dummy'})
    text = (run / 'mame.log').read_text()
    print(name, run)
    if result.returncode or 'LUA ERROR' in text or f'PASS {name} ' not in text:
        print(text)
        raise SystemExit(1)
    print(next(line for line in text.splitlines() if line.startswith('PASS ')))
