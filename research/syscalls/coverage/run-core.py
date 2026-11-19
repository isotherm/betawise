#!/usr/bin/env python3
"""Execute maintained legacy syscall cases against original AS3000/Neo firmware."""
from pathlib import Path
import argparse
import hashlib
import json
import os
import subprocess

from syscall_coverage import instrument

HERE = Path(__file__).resolve().parent
REPO = HERE.parents[2]
WORKSPACE = REPO.parent
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('output', type=Path)
parser.add_argument('--group', choices=['display', 'keyboard', 'time', 'file', 'applet', 'stdlib'])
parser.add_argument('--rom', choices=['as3000', 'neo_3_21'])
args = parser.parse_args()
out = args.output.resolve()
out.mkdir(parents=True, exist_ok=True)
inventory = json.loads((REPO / 'research/abi-audit/rom-inventory.json').read_text())
for rom in inventory:
    assert hashlib.sha256((WORKSPACE / rom['path']).read_bytes()).hexdigest() == rom['sha256']
roms = out / 'roms/asneo'
roms.mkdir(parents=True, exist_ok=True)
for name in ['os3kneorom.os3kos', 'smallos3kneorom.os3kos']:
    link = roms / name
    if not link.exists():
        link.symlink_to(WORKSPACE / 'Ghidra/Alphasmart Neo/SmartApplets' / name)
coverage = {}
for name in ([args.rom] if args.rom else ['as3000', 'neo_3_21']):
    neo = name != 'as3000'
    for group in ([args.group] if args.group else ['display', 'keyboard', 'time', 'file', 'applet', 'stdlib']):
        folder = out / f'{name}-{group}'
        folder.mkdir(parents=True, exist_ok=True)
        script = (HERE / 'core.lua.in').read_text().replace('@NEO@', str(neo).lower()).replace('@GROUP@', group)
        (folder / 'probe.lua').write_text(instrument(script, neo))
        command = [WORKSPACE / 'mame' / ('neodiag' if neo else 'mame'), 'asneo' if neo else 'asma3k',
                   '-rompath', out / 'roms' if neo else WORKSPACE / 'mame/roms',
                   '-nvram_directory', folder / 'nvram', '-cfg_directory', folder / 'cfg',
                   '-video', 'none', '-sound', 'none', '-nothrottle', '-seconds_to_run', '120',
                   '-autoboot_delay', '0', '-autoboot_script', folder / 'probe.lua']
        with (folder / 'mame.log').open('w') as log:
            subprocess.run(list(map(str, command)), cwd=folder,
                           env={**os.environ, 'SDL_VIDEODRIVER': 'dummy'},
                           stdout=log, stderr=subprocess.STDOUT, check=True, timeout=240)
        text = (folder / 'mame.log').read_text()
        assert 'PASS core complete' in text and 'FAIL' not in text and 'LUA ERROR' not in text, text[-2200:]
        slots = sorted({int(line.split()[3]) for line in text.splitlines() if line.startswith('PASS core slot ')})
        coverage[f'{name}-{group}'] = slots
        print('PASS', name, group, len(slots), 'distinct syscalls', flush=True)
(out / 'coverage.json').write_text(json.dumps(coverage, indent=2) + '\n')
