#!/usr/bin/env python3
"""Compile serial ABI probes and fill MAME templates (no firmware code copied)."""
from pathlib import Path
import subprocess
import sys

here = Path(__file__).resolve().parent
repo = here.parents[2]
out = Path(sys.argv[1]).resolve()
out.mkdir(parents=True, exist_ok=True)
flags = ['-m68000', '-msoft-float', '-msep-data', '-fomit-frame-pointer',
         '-fshort-enums', '-fno-store-merging', '-Os', '-fno-builtin',
         '-ffunction-sections', '-Wall', '-Wextra', '-Werror', f'-I{repo / "os3k"}']

def run(args):
    subprocess.run(list(map(str, args)), check=True)

# Build only the current raw syscall entries, avoiding a dependency on an
# already rebuilt archive or the separate pointer-bridge remediation.
run(['m68k-linux-gnu-gcc', *flags, '-c', repo/'os3k/syscall.c', '-o', out/'serial-stubs.o'])
for name, base, buffer in [('as', 0x38000, 0x39000), ('neo', 0x70000, 0x71000)]:
    stem = out / ('serial-' + name)
    run(['m68k-linux-gnu-gcc', *flags, f'-DPROBE_BUFFER={buffer}', '-c',
         here/'serial-probe.c', '-o', stem.with_suffix('.o')])
    run(['m68k-linux-gnu-ld', '--gc-sections', '-u', 'timeout_probe', '-e', 'probe', f'-Ttext={base:#x}',
         stem.with_suffix('.o'), out/'serial-stubs.o', '-o', stem.with_suffix('.elf')])
    run(['m68k-linux-gnu-objcopy', '-O', 'binary', stem.with_suffix('.elf'), stem.with_suffix('.bin')])
    symbols = {}
    for line in subprocess.check_output(['m68k-linux-gnu-nm', stem.with_suffix('.elf')], text=True).splitlines():
        fields = line.split()
        if len(fields) == 3:
            symbols[fields[2]] = int(fields[0], 16)
    script = (here/('serial-' + name + '-probe.lua.in')).read_text()
    for key, value in {
        'BYTES': ','.join(map(str, stem.with_suffix('.bin').read_bytes())),
        'GOT': str(symbols['_GLOBAL_OFFSET_TABLE_']),
        'PROBE': str(symbols['probe']),
        'TIMEOUT': str(symbols['timeout_probe']),
        'TX_CALLBACK': str(symbols['tx_callback']),
    }.items():
        script = script.replace('@' + key + '@', value)
    assert '@' not in script, 'Unexpanded template placeholder'
    stem.with_suffix('.lua').write_text(script)
