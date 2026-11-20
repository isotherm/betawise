#!/usr/bin/env python3
"""Execute production console/display code against original ROMs in MAME."""
from pathlib import Path
import hashlib, json, os, subprocess, sys
here=Path(__file__).resolve().parent
repo=here.parents[1]
workspace=repo.parent
out=Path(sys.argv[1] if len(sys.argv)>1 else '/tmp/betawise-console-rom').resolve()
out.mkdir(parents=True,exist_ok=True)
def run(args,**kw):subprocess.run(list(map(str,args)),check=True,**kw)
run(['make','-C',repo/'os3k','-j4'],stdout=subprocess.DEVNULL)
run(['m68k-linux-gnu-gcc','-m68000','-msoft-float','-msep-data','-fomit-frame-pointer','-fshort-enums','-fno-store-merging','-Os','-fno-builtin','-ffunction-sections','-fdata-sections','-I'+str(repo/'os3k'),'-c',here/'rom-probe.c','-o',out/'probe.o'])
(out/'probe.lds').write_text('SECTIONS { . = 0x30000; .text : { *(.text*) *(.rodata*) } . = 0x34000; .data : { *(.got.plt) *(.got) *(.data*) *(.bss*) } }')
run(['m68k-linux-gnu-ld','--gc-sections','-e','console_probe','-T',out/'probe.lds',out/'probe.o',repo/'os3k/libos3k.a','-o',out/'probe.elf'])
run(['m68k-linux-gnu-objcopy','-O','binary',out/'probe.elf',out/'probe.bin'])
symbols={l.split()[2]:int(l.split()[0],16) for l in subprocess.check_output(['m68k-linux-gnu-nm',str(out/'probe.elf')],text=True).splitlines() if len(l.split())==3}
assert len((out/'probe.bin').read_bytes())<0x5000
inventory=json.loads((repo/'research/abi-audit/rom-inventory.json').read_text())
for neo,name in [(False,'as3000'),(True,'neo_3_21')]:
    rom=next(r for r in inventory if r['name']==name)
    assert hashlib.sha256((workspace/rom['path']).read_bytes()).hexdigest()==rom['sha256']
    folder=out/name;folder.mkdir(exist_ok=True)
    if neo:
        romdir=out/'roms/asneo';romdir.mkdir(parents=True,exist_ok=True)
        for filename in ['os3kneorom.os3kos','smallos3kneorom.os3kos']:
            p=romdir/filename
            if not p.exists():p.symlink_to(workspace/'Ghidra/Alphasmart Neo/SmartApplets'/filename)
    script='''local machine=manager.machine;local cpu=machine.devices[":maincpu"];local m=cpu.spaces.program
local frame=0
emu.register_frame_done(function()
 frame=frame+1
 if frame==180 then
  local f=assert(io.open('probe.bin','rb'));local s=f:read('*a');f:close()
  for i=1,#s do m:write_u8(0x30000+i-1,s:byte(i))end
  m:write_u32(0x35000,0);m:write_u16(0x37f00,0x60fe);m:write_u32(0x3d000,0x37f00)
  cpu.state.A5.value=@GOT@;cpu.state.SR.value=0x2700;cpu.state.SP.value=0x3d000;cpu.state.PC.value=@ENTRY@
 elseif frame==240 then
  local pc=cpu.state.PC.value;local result=m:read_u32(0x35000)
  assert((pc==0x37f00 or pc==0x37f02) and result==0x70617373,string.format('console failed PC=%x stage=%x',pc,result))
  machine.video:snapshot();print('PASS production terminal ROM integration');machine:exit()
 end
end)
'''.replace('@GOT@',str(symbols['_GLOBAL_OFFSET_TABLE_'])).replace('@ENTRY@',str(symbols['console_probe']))
    lua=folder/'probe.lua';lua.write_text(script)
    args=[workspace/'mame'/('neodiag' if neo else 'mame'),'asneo' if neo else 'asma3k','-rompath',out/'roms' if neo else workspace/'mame/roms','-nvram_directory',folder/'nvram','-cfg_directory',folder/'cfg','-snapshot_directory',folder/'snap','-video','none','-sound','none','-nothrottle','-seconds_to_run','8','-autoboot_delay','0','-autoboot_script',lua]
    with (folder/'mame.log').open('w') as log:run(args,cwd=out,env={**os.environ,'SDL_VIDEODRIVER':'dummy'},stdout=log,stderr=subprocess.STDOUT)
    text=(folder/'mame.log').read_text()
    assert 'PASS production terminal ROM integration' in text and 'LUA ERROR' not in text,text
    print(name+': PASS')
print('Evidence:',out)
