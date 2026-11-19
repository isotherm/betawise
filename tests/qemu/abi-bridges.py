#!/usr/bin/env python3
"""Exercise production pointer bridges with D0-only mock ROM results under QEMU.

Uses real ABI flags and -fno-builtin; checks NULL/non-NULL results and argument
forwarding for every bridge. Firmware behavior is covered by research/abi-audit.
"""
from pathlib import Path
import os
import re
import subprocess
import tempfile

repo = Path(__file__).resolve().parents[2]
cross = os.environ.get('CROSS_COMPILE', 'm68k-linux-gnu-')
qemu = os.environ.get('QEMU_M68K', 'qemu-m68k')
flags = ['-m68000', '-msoft-float', '-msep-data', '-fomit-frame-pointer',
         '-fshort-enums', '-fno-store-merging', '-Os', '-fno-builtin',
         '-ffreestanding', '-I' + str(repo / 'os3k')]
source = (repo / 'os3k/os3k_abi_bridge.c').read_text()
entries = re.findall(r'extern uintptr_t _OS3K_(\w+)\(([^;]*)\);', source)
includes = '#include "os3k.h"\n'
mocks = includes + 'uintptr_t mock_result;\n'
checks = []
for name, params in entries:
    args = []
    tests = []
    if params != 'void':
        for index, param in enumerate(params.split(','), 1):
            kind, arg = param.strip().rsplit(' ', 1)
            args.append(f'({kind}){index}')
            tests.append(f'(uintptr_t){arg} != {index}')
    condition = ' || '.join(tests) or '0'
    mocks += f'''uintptr_t _OS3K_{name}({params}) {{
    uintptr_t result = ({condition}) ? 0xdeadbeef : mock_result;
    __asm__ volatile ("move.l #0x76543210,%%a0" ::: "a0");
    return result;
}}
'''
    checks.append(f'    if ((uintptr_t){name}({", ".join(args)}) != mock_result) return {len(checks)+1};')
test = includes + 'extern uintptr_t mock_result;\nint test(void) {\n'
for result in ['0', '0x12345678']:
    test += f'    mock_result = {result};\n' + '\n'.join(checks) + '\n'
test += '    return 0;\n}\n'
start = '''.text
.globl _start
_start:
    lea _GLOBAL_OFFSET_TABLE_,%a5
    jsr test
    move.l %d0,%d1
    moveq #1,%d0
    trap #0
.section .note.GNU-stack,"",@progbits
'''
def run(args):
    subprocess.run([str(a) for a in args], check=True)
with tempfile.TemporaryDirectory(prefix='os3k-bridges-') as tmp:
    out = Path(tmp)
    for name, content in [('mocks.c', mocks), ('test.c', test), ('start.S', start)]:
        (out / name).write_text(content)
    run(['make', '-C', repo / 'os3k'])
    run([cross + 'gcc', '-m68000', '-c', out / 'start.S', '-o', out / 'start.o'])
    run([cross + 'gcc', *flags, '-nostdlib', '-static', '-Wl,--build-id=none', '-Wl,--gc-sections',
         out / 'start.o', out / 'test.c', out / 'mocks.c',
         repo / 'os3k/os3k_abi_bridge.o', '-o', out / 'test.elf'])
    run([qemu, '-cpu', 'm68000', out / 'test.elf'])
print(f'PASS {len(entries)} production ABI bridges: arguments, NULL and non-NULL D0 results')
