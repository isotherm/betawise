#!/usr/bin/env python3
"""Build reproducible battery-dialog/power syscall probes for AS3000 3.01 and Neo 3.21."""
from pathlib import Path
import subprocess,sys
here=Path(__file__).resolve().parent
repo=here.parents[2]
out=Path(sys.argv[1]).resolve();out.mkdir(parents=True,exist_ok=True)
flags=['-m68000','-msoft-float','-msep-data','-fomit-frame-pointer','-fshort-enums','-fno-store-merging','-Os','-fno-builtin','-ffunction-sections','-Wall','-Wextra','-Werror',f'-I{repo/"os3k"}']
def run(a):subprocess.run(list(map(str,a)),check=True)
run(['m68k-linux-gnu-gcc',*flags,'-c',repo/'os3k/syscall.c','-o',out/'battery-dialog-stubs.o'])
for name,code,data,stack in [('as',0x38000,0x39000,0x3d000),('neo',0x70000,0x71000,0x78000)]:
 stem=out/('battery-dialog-'+name)
 run(['m68k-linux-gnu-gcc',*flags,f'-DPROBE_BUFFER={data}','-c',here/'battery-dialog-probe.c','-o',stem.with_suffix('.o')])
 run(['m68k-linux-gnu-gcc',*flags,'-c',repo/'os3k/os3k_abi_bridge.c','-o',out/'dialog-wrapper.o'])
 run(['m68k-linux-gnu-ld','-e','dialog_probe',f'-Ttext={code:#x}',stem.with_suffix('.o'),out/'battery-dialog-stubs.o',out/'dialog-wrapper.o','-o',stem.with_suffix('.elf')])
 run(['m68k-linux-gnu-objcopy','-O','binary',stem.with_suffix('.elf'),stem.with_suffix('.bin')])
 symbols={l.split()[2]:int(l.split()[0],16)for l in subprocess.check_output(['m68k-linux-gnu-nm',stem.with_suffix('.elf')],text=True).splitlines()if len(l.split())==3}
 script=(here/'battery-dialog-probe.lua.in').read_text()
 values={'NAME':name,'NEO':str(name=='neo').lower(),'CODE':str(code),'DATA':str(data),'STACK':str(stack),'FIRST':str(530 if name=='as'else 120),'BYTES':','.join(map(str,stem.with_suffix('.bin').read_bytes()))}
 values.update({k:str(v)for k,v in symbols.items()})
 for k,v in values.items():script=script.replace('@'+k+'@',v)
 assert '@'not in script,'unexpanded placeholder'
 stem.with_suffix('.lua').write_text(script)
