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
        if group == 'applet':
            base = 0x70400 if neo else 0x36400
            result = 0x74800 if neo else 0x38800
            flags = ['-m68000', '-msoft-float', '-msep-data', '-fomit-frame-pointer',
                     '-fshort-enums', '-fno-store-merging', '-Os', '-fno-builtin',
                     '-ffunction-sections', '-fdata-sections', '-Wall', '-Wextra', '-Werror',
                     '-I' + str(REPO / 'os3k'), '-DRESULT_ADDRESS=' + str(result)]
            source = HERE.parent / 'applet/contract-probe.c'
            for obj, src in [('contract', source), ('stubs', REPO / 'os3k/syscall.c')]:
                subprocess.run(['m68k-linux-gnu-gcc', *flags, '-c', str(src), '-o', str(folder / (obj + '.o'))], check=True)
            (folder / 'link.ld').write_text('SECTIONS { . = %d; .text : { *(.text*) *(.rodata*) } .got : { _GLOBAL_OFFSET_TABLE_ = .; *(.got*) } }' % base)
            subprocess.run(['m68k-linux-gnu-ld', '--gc-sections', '-e', 'contract_width_probe', '-T', str(folder / 'link.ld'),
                            str(folder / 'contract.o'), str(folder / 'stubs.o'), '-o', str(folder / 'contract.elf')], check=True)
            subprocess.run(['m68k-linux-gnu-objcopy', '-O', 'binary', str(folder / 'contract.elf'), str(folder / 'contract.bin')], check=True)
            blob = (folder / 'contract.bin').read_bytes()
            assert len(blob) < 0x1000
            (folder / 'contract.dis').write_bytes(subprocess.check_output(['m68k-linux-gnu-objdump', '-d', str(folder / 'contract.elf')]))
            symbols = {row.split()[2]: int(row.split()[0], 16) for row in subprocess.check_output(['m68k-linux-gnu-nm', str(folder / 'contract.elf')], text=True).splitlines() if len(row.split()) == 3}
            hook = """if group=='applet' then
                add(141,{},function()
                    local b={%s};for i,v in ipairs(b) do m:write_u8(%d+i-1,v) end;cpu.state.A5.value=%d
                    local header=data+0x1000;m:write_u32(%d+4,header);m:write_u16(header+0x14,0xbeef)
                    m:write_u32(%d,1);m:write_u8(%d,0x81)
                end,function()
                    for i,v in ipairs({0,0,0xbeef,0,0x81,0x70617373}) do eq(m:read_u32(%d+4*(i-1)),v) end
                end)
                cases[#cases].target=%d
            end
""" % (','.join(map(str, blob)), base, symbols['_GLOBAL_OFFSET_TABLE_'], 0xe0a if neo else 0xd82,
                        0x35f0 if neo else 0x3560, 0x3dfb if neo else 0x3d6b, result, base)
            script = script.replace('local index,frame,phase,failed=', hook + 'local index,frame,phase,failed=')
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
        (folder / 'evidence-hashes.json').write_text(json.dumps({p.name: hashlib.sha256(p.read_bytes()).hexdigest() for p in sorted(folder.glob('case-*.ram'))} | {'mame.log': hashlib.sha256((folder / 'mame.log').read_bytes()).hexdigest()}, indent=2) + '\n')
        slots = sorted({int(line.split()[3]) for line in text.splitlines() if line.startswith('PASS core slot ')})
        coverage[f'{name}-{group}'] = slots
        print('PASS', name, group, len(slots), 'distinct syscalls', flush=True)
(out / 'coverage.json').write_text(json.dumps(coverage, indent=2) + '\n')
