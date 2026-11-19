#!/usr/bin/env python3
"""Run all maintained syscall suites and fail if a defined syscall lacks execution coverage."""
from pathlib import Path
import json
import re
import struct
import subprocess
import sys

HERE = Path(__file__).resolve().parent
REPO = HERE.parents[2]
WORKSPACE = REPO.parent
if len(sys.argv) != 2:
    raise SystemExit(f'usage: {sys.argv[0]} OUTPUT_DIRECTORY')
out = Path(sys.argv[1]).resolve()
out.mkdir(parents=True, exist_ok=True)
assert not list(out.rglob('mame.log')), 'Use a fresh output directory; stale passes must not count.'


def run(name, command):
    with (out / f'{name}.log').open('w') as log:
        result = subprocess.run(list(map(str, command)), cwd=REPO,
                                stdout=log, stderr=subprocess.STDOUT)
    if result.returncode:
        raise RuntimeError(f'{name} failed: {(out / (name + ".log")).read_text()[-2500:]}')
    print('PASS', name, flush=True)


run('build', ['make', '-C', REPO / 'os3k', '-j1'])
run('qemu', ['make', '-C', REPO / 'tests/qemu', '-j1', 'check'])
run('docs', [sys.executable, HERE / 'check-docs.py'])
# This also validates hashes, complete tables, and all holes in all three ROMs.
run('catalog', [sys.executable, HERE.parent / 'catalog-roms.py'])
run('abi', [sys.executable, REPO / 'research/abi-audit/run.py', out / 'abi'])
run('core', [sys.executable, HERE / 'run-core.py', out / 'core'])
for name in ['irda', 'text', 'dialog', 'markup', 'wireless', 'speech']:
    script = 'run-neo-probe.py' if name == 'dialog' else 'run-probe.py'
    run(name, [sys.executable, HERE.parent / name / script, out / name])

symbols = {int(slot): name for slot, name in re.findall(
    r'^DEFINE_SYSCALL(?:_NEO)?\((\d+),\s*(\w+)\)',
    (REPO / 'os3k/syscall.c').read_text(), re.M)}
empty = set(map(int, re.findall(r'^// EMPTY_SYSCALL\((\d+)\)',
                              (REPO / 'os3k/syscall.c').read_text(), re.M)))
assert set(symbols) | empty == set(range(285)), 'Uncatalogued dispatch slot'
assert not set(symbols) & empty
roms = json.loads((REPO / 'research/abi-audit/rom-inventory.json').read_text())
for rom in roms:
    image = (WORKSPACE / rom['path']).read_bytes()
    table = struct.unpack_from('>' + 'I' * rom['slots'], image, rom['table_offset'])
    assert list(table) == rom['targets']
    assert all(target == 0 for slot, target in enumerate(table) if slot in empty)

# Count observed native dispatch loads only after every suite has passed.
coverage = {slot: [] for slot in symbols}
coverage_by_rom = {'as3000': set(), 'neo_3_21': set()}
for log in sorted(out.rglob('mame.log')):
    text = log.read_text(errors='replace')
    relative = log.relative_to(out)
    model = 'as3000' if any(part.startswith(('as3000', 'as-'))
                               for part in relative.parts) else 'neo_3_21'
    assert not re.search(r'FAIL (?:ABI|core|extended|text|service)|LUA ERROR', text), str(log)
    for slot in sorted(set(map(int, re.findall(r'^COVER_SYSCALL (\d+)$', text, re.M)))):
        assert slot in symbols, f'Probe dispatched empty/unknown slot {slot}: {log}'
        coverage[slot].append(str(relative))
        coverage_by_rom[model].add(slot)
report = ['slot\topcode\tsymbol\tpassing_MAME_logs']
for slot, symbol in sorted(symbols.items()):
    report.append(f'{slot}\tA{4 * slot:03X}\t{symbol}\t' + ';'.join(coverage[slot]))
(out / 'coverage.tsv').write_text('\n'.join(report) + '\n')
missing = [f'{slot}: {symbols[slot]}' for slot, logs in coverage.items() if not logs]
assert not missing, 'Missing execution coverage:\n' + '\n'.join(missing)
for rom in roms:
    if rom['name'] in coverage_by_rom:
        required = {slot for slot, target in enumerate(rom['targets']) if target}
        missing = sorted(required - coverage_by_rom[rom['name']])
        assert not missing, f"{rom['name']}: unexecuted populated entries {missing}"
summary = {'defined_syscalls': len(symbols), 'covered_syscalls': len(coverage),
           'empty_slots_checked': len(empty),
           'per_rom_executed': {name: len(slots) for name, slots in coverage_by_rom.items()}, 'live_roms': ['as3000', 'neo_3_21'],
           'table_checked_roms': [r['name'] for r in roms]}
(out / 'summary.json').write_text(json.dumps(summary, indent=2) + '\n')
print(f'PASS all {len(symbols)} syscalls; {len(empty)} empty slots checked. Report: {out / "coverage.tsv"}')
