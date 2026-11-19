#!/usr/bin/env python3
"""Run raw syscall characterization in disposable MAME state; original ROMs required."""
from pathlib import Path
import sys,json,subprocess,os,hashlib
here=Path(__file__).resolve().parent;repo=here.parents[2];workspace=repo.parent
sys.path.insert(0, str(here.parent / 'coverage'))
from syscall_coverage import instrument
out=Path(sys.argv[1]).resolve();out.mkdir(parents=True,exist_ok=True)
roms=out/'roms/asneo';roms.mkdir(parents=True,exist_ok=True)
for name in ['os3kneorom.os3kos','smallos3kneorom.os3kos']:
 p=roms/name
 if not p.exists():p.symlink_to(workspace/'Ghidra/Alphasmart Neo/SmartApplets'/name)
flags=['-m68000','-msoft-float','-msep-data','-fomit-frame-pointer','-fshort-enums','-fno-store-merging','-Os','-fno-builtin','-ffunction-sections','-fdata-sections','-Wall','-Wextra','-Werror',f'-I{repo/"os3k"}']
def run(args):subprocess.run(list(map(str,args)),check=True)
run(['m68k-linux-gnu-gcc',*flags,'-c',repo/'os3k/syscall.c','-o',out/'stubs.o'])
run(['m68k-linux-gnu-gcc',*flags,'-c',repo/'os3k/os3k_abi_bridge.c','-o',out/'bridge.o'])
for r in json.loads((repo/'research/abi-audit/rom-inventory.json').read_text()):
 assert hashlib.sha256((workspace/r['path']).read_bytes()).hexdigest()==r['sha256'],r['name']+' ROM hash differs'
 if r['name']=='neo_3_01':continue
 neo=r['name']=='neo_3_21';folder=out/r['name'];folder.mkdir(exist_ok=True)
 code=0x70400 if neo else 0x36400;data=0x74000 if neo else 0x38000
 run(['m68k-linux-gnu-gcc',*flags,*(['-DPROBE_NEO']if neo else []),f'-DPROBE_DATA={data}','-c',here/'typed-probe.c','-o',folder/'typed.o'])
 (folder/'link.ld').write_text(f'SECTIONS {{ . = {code}; .text : {{ *(.text*) *(.rodata*) }} . = ALIGN(4); .got : {{ _GLOBAL_OFFSET_TABLE_ = .; *(.got*) }} .data : {{ *(.data*) }} .bss : {{ *(.bss*) }} }}')
 run(['m68k-linux-gnu-ld','--gc-sections','-e','typed_probe','-u','typed_form_probe','-T',folder/'link.ld',folder/'typed.o',out/'stubs.o',out/'bridge.o','-o',folder/'typed.elf'])
 run(['m68k-linux-gnu-objcopy','-O','binary',folder/'typed.elf',folder/'typed.bin'])
 blob=(folder/'typed.bin').read_bytes();assert len(blob)<0x1c00,len(blob)
 symbols={z.split()[2]:int(z.split()[0],16)for z in subprocess.check_output(['m68k-linux-gnu-nm',folder/'typed.elf'],text=True).splitlines()if len(z.split())==3}
 script=(here/'probe.lua.in').read_text().replace('@NEO@',str(neo).lower())
 script=script.replace('@TYPED_BYTES@',','.join(map(str,blob))).replace('@TYPED_CODE@',str(code))
 for key,value in symbols.items():script=script.replace('@'+key+'@',str(value))
 assert '@'not in script,'unexpanded placeholder'
 lua=folder/'probe.lua';lua.write_text(instrument(script, neo))
 args=[workspace/'mame'/('neodiag'if neo else'mame'),'asneo'if neo else'asma3k','-rompath',out/'roms'if neo else workspace/'mame/roms','-nvram_directory',folder/'nvram','-cfg_directory',folder/'cfg','-video','none','-sound','none','-nothrottle','-seconds_to_run','32','-autoboot_delay','0','-autoboot_script',lua]
 with(folder/'mame.log').open('w')as log:subprocess.run(list(map(str,args)),cwd=out,env={**os.environ,'SDL_VIDEODRIVER':'dummy'},stdout=log,stderr=subprocess.STDOUT,check=True)
 text=(folder/'mame.log').read_text(errors='replace');assert 'PASS text probe' in text and 'FAIL text'not in text and 'LUA ERROR'not in text,text[-1800:]
 print('PASS',r['name'])
