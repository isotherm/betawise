#!/usr/bin/env python3
"""Test the shared libos3k jump implementation on a 68000 under QEMU."""
from pathlib import Path
import os
import subprocess
import tempfile

here = Path(__file__).resolve().parent
repo = here.parents[1]
cross = os.environ.get('CROSS_COMPILE', 'm68k-linux-gnu-')
qemu = os.environ.get('QEMU_M68K', 'qemu-m68k')
subprocess.run(['make', '-C', str(repo / 'os3k')], check=True)
with tempfile.TemporaryDirectory(prefix='os3k-setjmp-') as directory:
    image = Path(directory) / 'test.elf'
    start = Path(directory) / 'start.o'
    subprocess.run([cross + 'gcc', '-m68000', '-c',
                    str(here / 'setjmp-registers.S'), '-o', str(start)], check=True)
    for optimization in ('-O0', '-Os', '-O2'):
        subprocess.run([
            cross + 'gcc', '-m68000', '-msoft-float', '-msep-data',
            '-fomit-frame-pointer', '-fshort-enums', optimization,
            '-ffreestanding', '-nostdlib', '-static', '-I' + str(repo / 'os3k'),
            '-Wl,--build-id=none', str(start),
            str(here / 'setjmp.c'), str(repo / 'os3k/libos3k.a'),
            '-o', str(image),
        ], check=True)
        subprocess.run([qemu, '-cpu', 'm68000', str(image)], check=True)
        print(f'PASS shared setjmp {optimization}: zero/positive/negative returns, nested unwinding, SP/PC and callee-saved registers')
