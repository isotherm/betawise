#!/usr/bin/env python3
"""Run the Neo regression on disposable flash, with offline screen assertions."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import struct
import subprocess
import sys
import tempfile

from neo_reader import FONT, stable_read

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[3]


def build_images(variant, log_path):
    """Build current sources before collecting acceptance evidence."""
    targets = [('os3k', None), ('applets/NeoFontTerminal', None),
               ('applets/UCBLogo', None), ('applets/MicroPython', variant)]
    with Path(log_path).open('w') as log:
        for directory, target in targets:
            command = ['make', '-C', str(ROOT/'betawise'/directory), '-j2']
            if target: command.append(target)
            subprocess.run(command, stdout=log, stderr=subprocess.STDOUT, check=True)


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--mame', type=Path, default=ROOT / 'mame/neodiag')
    p.add_argument('--firmware', type=Path, default=ROOT / 'Ghidra/Alphasmart Neo/SmartApplets')
    p.add_argument('--output', type=Path, help='new directory for logs, decoded text and disposable NVRAM')
    p.add_argument('--no-build', action='store_true', help='use existing built applets explicitly')
    p.add_argument('--baseline-only', action='store_true', help='run only the original 12-check regression')
    p.add_argument('--prepare-only', action='store_true', help='stage disposable flash without running MAME')
    p.add_argument('--variant', choices=['minimal', 'core', 'basic', 'extra'], default='extra')
    p.add_argument('--verify-only', type=Path, help='recheck an existing capture directory without running MAME')
    args = p.parse_args()
    out = args.verify_only or args.output or Path(tempfile.mkdtemp(prefix='neo-offline-'))
    try:
        if not args.verify_only:
            if args.output:
                out.mkdir(parents=True, exist_ok=False)
            if not args.no_build:
                build_images(args.variant, out/'build.log')
            for sub in ['roms/asneo', 'nvram/asneo', 'cfg', 'snap']:
                (out/sub).mkdir(parents=True, exist_ok=True)
            flash = bytearray(b'\xff' * 0x400000)
            small = (args.firmware/'smallos3kneorom.os3kos').read_bytes()
            system = (args.firmware/'os3kneorom.os3kos').read_bytes()
            flash[:len(small)] = small
            assert hashlib.sha256(system).hexdigest() == '32fe55bb50c1b58326bd303f14973b8402ddf8bf6662b210e4d5e9f931c7e6c0', 'capture helper requires Neo 3.21 firmware'
            assert system[6:24] == b'System 3 Neo      ', 'unexpected system package'
            source = 0
            for pos in range(0x50, 0x70, 8):
                addr, size = struct.unpack_from('>II', system, pos)
                if not addr and not size:
                    break
                size = (size + 1023) & ~1023
                dest = addr - 0x400000
                assert 0 <= dest <= len(flash)-size and source+size <= len(system)
                flash[dest:dest+size] = system[source:source+size]
                source += size
            images = [args.firmware/'alphawordplus.os3kapp',
                      ROOT/'betawise/applets/UCBLogo/UCBLogo.OS3KApp',
                      ROOT/f'betawise/applets/MicroPython/MicroPython-{args.variant}.OS3KApp',
                      ROOT/'betawise/applets/NeoFontTerminal/NeoFontTerminal.OS3KApp']
            offset = 0x70000
            manifest = []
            for image in images:
                data = image.read_bytes()
                assert data[:4] == bytes.fromhex('c0ffeead')
                assert int.from_bytes(data[4:8], 'big') == len(data)
                assert offset+len(data) <= len(flash) and all(b == 255 for b in flash[offset:offset+len(data)])
                flash[offset:offset+len(data)] = data
                offset += len(data)
                manifest.append({'path': str(image), 'sha256': hashlib.sha256(data).hexdigest()})
            for name in ['smallos3kneorom.os3kos', 'os3kneorom.os3kos']:
                path = args.firmware/name
                shutil.copyfile(path, out/'roms/asneo'/name)
                manifest.append({'path': str(path), 'sha256': hashlib.sha256(path.read_bytes()).hexdigest()})
            manifest.append({'path': str(args.mame), 'sha256': hashlib.sha256(args.mame.read_bytes()).hexdigest()})
            for script in [HERE/'MicroPythonBackspace.lua', HERE/'neo_reader.py', HERE/'check.py']:
                manifest.append({'path': str(script), 'sha256': hashlib.sha256(script.read_bytes()).hexdigest()})
            manifest.append({'path': str(FONT), 'sha256': hashlib.sha256(FONT.read_bytes()).hexdigest()})
            (out/'manifest.json').write_text(json.dumps(manifest, indent=2)+'\n')
            (out/'nvram/asneo/flashnv').write_bytes(flash)
            (out/'cfg/asneo.cfg').write_text('<mameconfig version="10"><system name="asneo"><input>'
                '<port tag=":NOR_PROFILE" type="CONFIG" mask="1" defvalue="0" value="1" />'
                '</input></system></mameconfig>\n')
            if args.prepare_only:
                print(f'Prepared Neo session: {out}')
                return 0
            with (out/'mame.log').open('w') as log:
                result = subprocess.run([str(args.mame.resolve()), 'asneo', '-rompath', str((out/'roms').resolve()),
                    '-nvram_directory', str((out/'nvram').resolve()), '-cfg_directory', str((out/'cfg').resolve()),
                    '-snapshot_directory', str((out/'snap').resolve()), '-video', 'none', '-sound', 'none',
                    '-nothrottle', '-skip_gameinfo', '-autoboot_delay', '0', '-autoboot_script',
                    str(HERE/'MicroPythonBackspace.lua'), '-seconds_to_run', '200'],
                    env=dict(os.environ, SDL_VIDEODRIVER='dummy'), stdout=log, stderr=subprocess.STDOUT, timeout=360)
            log = (out/'mame.log').read_text()
            if result.returncode or 'LUA ERROR' in log or 'FAIL' in log or 'PASS Neo cursor assertions' not in log:
                raise ValueError('MAME regression failed; see mame.log')
        # These exact screen suffixes replace the former human/AI screenshot checks.
        expected = ['>>> a=1234', '>>> a=123', '>>> a\n123\n>>>',
                    '>>> a\n123\n>>>', '>>> a\n123\n>>>',
                    '>>> a=124\n>>> a\n124\n>>>', '>>> a\n124\n>>>', '>>> a\n124\n>>> a\n124\n>>>', '>>> 6*7\n42\n>>>',
                    '>>> len("abcdefghijklmnopqrstuvwxyzABCDEFGHI\nJKLMNOPQRSTUVWXYZ")',
                    '52\n>>>', '1\n>>> 1\n1\n>>> 1\n1\n>>> 1\n1\n>>>']
        report = []
        for index, suffix in enumerate(expected, 1):
            paths = sorted((out/'snap').glob(f'checkpoint-{index:02d}-*.png'))
            reading, selected = stable_read(paths)
            if not reading.text.endswith(suffix):
                raise ValueError(f'checkpoint {index}: expected suffix {suffix!r}, got {reading.text!r}')
            (out/f'checkpoint-{index:02d}.txt').write_text(reading.text+'\n')
            report.append({'checkpoint': index, 'selected': selected, 'text': reading.text, 'font_sha256': reading.font_sha256})
        (out/'results.json').write_text(json.dumps(report, indent=2)+'\n')
        print(f'PASS: 12 Neo screen assertions and cursor regression; artifacts: {out}')
        if not args.verify_only and not args.baseline_only:
            subprocess.run([sys.executable, str(HERE/'check-repl.py'), '--no-build', '--variant', args.variant,
                            '--mame', str(args.mame), '--firmware', str(args.firmware),
                            '--output', str(out/'extended')], check=True)
        return 0
    except (OSError, ValueError, AssertionError, subprocess.SubprocessError) as error:
        print(f'FAIL: {error}; artifacts: {out}', file=sys.stderr)
        return 1


if __name__ == '__main__':
    sys.exit(main())
