#!/usr/bin/env python3
"""Build and run direct-LCD-RAM Neo regressions on disposable flash."""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import struct
import subprocess
import sys
import tempfile

from neo_reader import FONT

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[3]


def build_images(variant, log_path):
    targets = [('os3k', None), ('applets/NeoFontTerminal', None),
               ('applets/UCBLogo', None), ('applets/MicroPython', variant)]
    with Path(log_path).open('w') as log:
        for directory, target in targets:
            command = ['make', '-C', str(ROOT/'betawise'/directory), '-j2']
            if target:
                command.append(target)
            subprocess.run(command, stdout=log, stderr=subprocess.STDOUT, check=True)


def prepare_session(out, firmware, variant, mame):
    for sub in ['roms/asneo', 'nvram/asneo', 'cfg', 'snap']:
        (out/sub).mkdir(parents=True, exist_ok=True)
    flash = bytearray(b'\xff' * 0x400000)
    small = (firmware/'smallos3kneorom.os3kos').read_bytes()
    system = (firmware/'os3kneorom.os3kos').read_bytes()
    flash[:len(small)] = small
    if hashlib.sha256(system).hexdigest() != '32fe55bb50c1b58326bd303f14973b8402ddf8bf6662b210e4d5e9f931c7e6c0':
        raise ValueError('cursor helpers require Neo 3.21 firmware')
    source = 0
    for pos in range(0x50, 0x70, 8):
        addr, size = struct.unpack_from('>II', system, pos)
        if not addr and not size:
            break
        size = (size+1023)&~1023
        dest = addr-0x400000
        if not (0 <= dest <= len(flash)-size and source+size <= len(system)):
            raise ValueError('invalid firmware segment')
        flash[dest:dest+size] = system[source:source+size]
        source += size
    images = [firmware/'alphawordplus.os3kapp', ROOT/'betawise/applets/UCBLogo/UCBLogo.OS3KApp',
              ROOT/f'betawise/applets/MicroPython/MicroPython-{variant}.OS3KApp',
              ROOT/'betawise/applets/NeoFontTerminal/NeoFontTerminal.OS3KApp']
    offset = 0x70000
    for path in images:
        data = path.read_bytes()
        if (data[:4] != bytes.fromhex('c0ffeead') or int.from_bytes(data[4:8], 'big') != len(data)
                or offset+len(data) > len(flash) or any(b != 255 for b in flash[offset:offset+len(data)])):
            raise ValueError(f'invalid applet placement: {path}')
        flash[offset:offset+len(data)] = data
        offset += len(data)
    for name in ['smallos3kneorom.os3kos', 'os3kneorom.os3kos']:
        shutil.copyfile(firmware/name, out/'roms/asneo'/name)
    inputs = images + [firmware/'smallos3kneorom.os3kos', firmware/'os3kneorom.os3kos', mame,
                       HERE/'NeoHarness.lua', HERE/'NeoLCD.lua', HERE/'neo_reader.py', HERE/'check.py', FONT]
    manifest = [dict(path=str(p), sha256=hashlib.sha256(p.read_bytes()).hexdigest()) for p in inputs]
    (out/'manifest.json').write_text(json.dumps(manifest, indent=2)+'\n')
    (out/'nvram/asneo/flashnv').write_bytes(flash)
    (out/'cfg/asneo.cfg').write_text('<mameconfig version="10"><system name="asneo"><input>'
        '<port tag=":NOR_PROFILE" type="CONFIG" mask="1" defvalue="0" value="1" />'
        '</input></system></mameconfig>\n')


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--mame', type=Path, default=ROOT/'mame/neodiag')
    p.add_argument('--firmware', type=Path, default=ROOT/'Ghidra/Alphasmart Neo/SmartApplets')
    p.add_argument('--output', type=Path)
    p.add_argument('--no-build', action='store_true')
    p.add_argument('--baseline-only', action='store_true', help='run the 12-check Backspace regression')
    p.add_argument('--prepare-only', action='store_true', help='stage fresh flash without running tests')
    p.add_argument('--variant', choices=['minimal', 'core', 'basic', 'extra'], default='extra')
    p.add_argument('--verify-only', type=Path, help='recheck a case containing checks.json and LCD records')
    p.add_argument('--verify-rendering', action='store_true', help='cross-check RAM against rendered pixels')
    args = p.parse_args()
    try:
        if args.verify_only:
            return subprocess.call([sys.executable, str(HERE/'check-repl.py'), '--verify-only', str(args.verify_only)])
        out = args.output or Path(tempfile.mkdtemp(prefix='neo-ready-'))
        if args.prepare_only:
            if args.output:
                out.mkdir(parents=True, exist_ok=False)
            if not args.no_build:
                build_images(args.variant, out/'build.log')
            prepare_session(out, args.firmware, args.variant, args.mame)
            print(f'Prepared Neo session: {out}')
            return 0
        command = [sys.executable, str(HERE/'check-repl.py'), '--variant', args.variant,
                   '--mame', str(args.mame), '--firmware', str(args.firmware), '--output', str(out)]
        if args.no_build:
            command.append('--no-build')
        if args.baseline_only:
            command += ['--case', 'backspace']
        if args.verify_rendering:
            command.append('--verify-rendering')
        return subprocess.call(command)
    except (OSError, ValueError, subprocess.SubprocessError) as error:
        print(f'FAIL: {error}', file=sys.stderr)
        return 1


if __name__ == '__main__':
    sys.exit(main())
