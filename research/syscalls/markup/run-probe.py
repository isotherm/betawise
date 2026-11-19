#!/usr/bin/env python3
"""Run slots 237–246 characterization against original Neo 3.21 in MAME."""
from pathlib import Path
import sys, os, json, hashlib, subprocess
here=Path(__file__).resolve().parent;repo=here.parents[2];workspace=repo.parent
out=Path(sys.argv[1]).resolve();out.mkdir(parents=True,exist_ok=True)
r=next(r for r in json.loads((repo/'research/abi-audit/rom-inventory.json').read_text())if r['name']=='neo_3_21')
assert hashlib.sha256((workspace/r['path']).read_bytes()).hexdigest()==r['sha256']
roms=out/'roms/asneo';roms.mkdir(parents=True,exist_ok=True)
for name in ['os3kneorom.os3kos','smallos3kneorom.os3kos']:
 p=roms/name
 if not p.exists():p.symlink_to(workspace/'Ghidra/Alphasmart Neo/SmartApplets'/name)
flags=['-m68000','-msoft-float','-msep-data','-fomit-frame-pointer','-fshort-enums','-fno-store-merging','-Os','-fno-builtin','-ffunction-sections','-fdata-sections','-Wall','-Wextra','-Werror',f'-I{repo/"os3k"}']
def run(args):subprocess.run(list(map(str,args)),check=True)
for name,source in [('typed',here/'typed-probe.c'),('stubs',repo/'os3k/syscall.c'),('bridge',repo/'os3k/os3k_abi_bridge.c')]:
 run(['m68k-linux-gnu-gcc',*flags,'-c',source,'-o',out/(name+'.o')])
(out/'link.ld').write_text('SECTIONS { . = 0x70400; .text : { *(.text*) *(.rodata*) } . = ALIGN(4); .got : { _GLOBAL_OFFSET_TABLE_ = .; *(.got*) } .data : { *(.data*) } .bss : { *(.bss*) } }')
run(['m68k-linux-gnu-ld','--gc-sections','-e','typed_probe','-T',out/'link.ld',out/'typed.o',out/'stubs.o',out/'bridge.o','-o',out/'typed.elf'])
run(['m68k-linux-gnu-objcopy','-O','binary',out/'typed.elf',out/'typed.bin'])
blob=(out/'typed.bin').read_bytes();assert len(blob)<0x1c00
symbols={z.split()[2]:int(z.split()[0],16)for z in subprocess.check_output(['m68k-linux-gnu-nm',out/'typed.elf'],text=True).splitlines()if len(z.split())==3}
case="""add(10000+@typed_probe@,{},function()
 local b={@BYTES@};for i,v in ipairs(b)do m:write_u8(0x70400+i-1,v)end;cpu.state.A5.value=@_GLOBAL_OFFSET_TABLE_@
end,function()eq(m:read_u32(0x73c00),0x70617373);print('PASS six typed markup wrappers and callback')end)"""
case=case.replace('@BYTES@',','.join(map(str,blob)))
for key,value in symbols.items():case=case.replace('@'+key+'@',str(value))
assert '@'not in case
script=(here/'probe.lua.in').read_text().replace('@TYPED_CASE@',case)
(out/'probe.lua').write_text(script)
args=[workspace/'mame/neodiag','asneo','-rompath',out/'roms','-nvram_directory',out/'nvram','-cfg_directory',out/'cfg','-video','none','-sound','none','-nothrottle','-seconds_to_run','50','-autoboot_delay','0','-autoboot_script',out/'probe.lua']
with(out/'mame.log').open('w')as log:subprocess.run(list(map(str,args)),cwd=out,env={**os.environ,'SDL_VIDEODRIVER':'dummy'},stdout=log,stderr=subprocess.STDOUT,check=True)
s=(out/'mame.log').read_text();assert 'PASS extended probe' in s and 'FAIL extended'not in s and 'LUA ERROR'not in s,s[-2500:]
print(s[s.index('PASS slot'):])
