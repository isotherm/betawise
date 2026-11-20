#!/usr/bin/env python3
"""Compile a GCC callback and execute it via the private ROM formatter in MAME."""
from pathlib import Path
import hashlib, json, os, subprocess, sys
here = Path(__file__).resolve().parent
repo = here.parents[2]
workspace = repo.parent
out = Path(sys.argv[1] if len(sys.argv)>1 else '/tmp/os3k-formatter-probe').resolve()
out.mkdir(parents=True, exist_ok=True)
def run(args, **kw):
    return subprocess.run([str(a) for a in args], check=True, **kw)
flags = ['-m68000','-msoft-float','-msep-data','-fomit-frame-pointer','-fshort-enums','-fno-store-merging','-Os','-fno-builtin']
run(['m68k-linux-gnu-gcc', *flags, '-c', here/'formatter-probe.c', '-o', out/'probe.o'])
(out/'probe.lds').write_text('SECTIONS { . = 0x36000; .text : { *(.text*) *(.rodata*) } . = 0x37000; .data : { *(.got.plt) *(.got) *(.data*) *(.bss*) } }')
run(['m68k-linux-gnu-ld','-T',out/'probe.lds','-e','formatter_probe',out/'probe.o','-o',out/'probe.elf'])
run(['m68k-linux-gnu-objcopy','-O','binary',out/'probe.elf',out/'probe.bin'])
symbols = subprocess.check_output(['m68k-linux-gnu-nm',str(out/'probe.elf')],text=True)
entry = int(next(l.split()[0] for l in symbols.splitlines() if l.endswith(' formatter_probe')),16)
got = int(next(l.split()[0] for l in symbols.splitlines() if l.endswith(' _GLOBAL_OFFSET_TABLE_')),16)
counter = int(next(l.split()[0] for l in symbols.splitlines() if l.endswith(' callback_count')),16)
roms = json.loads((repo/'research/abi-audit/rom-inventory.json').read_text())
for neo, name, printf, engine in [(False,'as3000',0x4dd412,0x4dd55a),(True,'neo_3_21',0x435f2e,0x436076)]:
    rom = next(r for r in roms if r['name']==name)
    assert hashlib.sha256((workspace/rom['path']).read_bytes()).hexdigest()==rom['sha256']
    folder=out/name
    folder.mkdir(exist_ok=True)
    if neo:
        romdir=out/'roms/asneo';romdir.mkdir(parents=True,exist_ok=True)
        for filename in ['os3kneorom.os3kos','smallos3kneorom.os3kos']:
            link=romdir/filename
            if not link.exists():link.symlink_to(workspace/'Ghidra/Alphasmart Neo/SmartApplets'/filename)
    script=r'''
local machine=manager.machine;local cpu=machine.devices[":maincpu"];local m=cpu.spaces.program
local frame,index=0,0
local cases={
 {"%d %ld %x %s %c %.1f", "ESC\27[2;5H", "-123 305419896 abcdef ESC\27[2;5H ! 1.5",0},
 {"[%*.*s] %.2f %x", "abcdef", "[     abc] -2.25 55",1}
}
local regs={"D2","D3","D4","D5","D6","D7","A2","A3","A4","A5","A6"}
local function put(a,s)for i=1,#s do m:write_u8(a+i-1,s:byte(i))end;m:write_u8(a+#s,0)end
emu.register_frame_done(function()
 frame=frame+1;if frame<180 then return end
 local phase=(frame-180)%12
 if phase==0 then
  index=index+1;if index>#cases then print('PASS formatter GCC callback 2 cases');machine:exit();return end
  assert(m:read_u16(@PRINTF@+24)==0x4eb9 and m:read_u32(@PRINTF@+26)==@ENGINE@,'unexpected printf call pattern')
  local f=assert(io.open('probe.bin','rb'));local s=f:read('*a');f:close()
  for i=1,#s do m:write_u8(0x36000+i-1,s:byte(i))end
  local c=cases[index];put(0x38000,c[1]);put(0x38100,c[2])
  m:write_u32(0x38ffc,0xa5a5a5a5);m:write_u32(0x39114,0xa5a5a5a5)
  m:write_u16(0x37f00,0x60fe);m:write_u32(0x3d000,0x37f00)
  for j,v in ipairs({@ENGINE@,0x38000,0x38100,c[4]})do m:write_u32(0x3d000+4*j,v)end
  for j,r in ipairs(regs)do cpu.state[r].value=0x12340000+j end
  cpu.state.A5.value=@GOT@
  cpu.state.SR.value=0x2700;cpu.state.SP.value=0x3d000;cpu.state.PC.value=@ENTRY@
 elseif phase==10 then
  assert(cpu.state.PC.value==0x37f00 or cpu.state.PC.value==0x37f02,string.format('probe did not return PC=%x SP=%x A5=%x count=%x',cpu.state.PC.value,cpu.state.SP.value,cpu.state.A5.value,m:read_u32(0x39000)))
  assert(cpu.state.SP.value==0x3d004,'stack mismatch')
  for j,r in ipairs(regs)do assert(cpu.state[r].value==(r=='A5' and @GOT@ or 0x12340000+j),'clobbered '..r)end
  local c=cases[index];local n=m:read_u32(0x39000);local s=''
  for i=0,n-1 do s=s..string.char(m:read_u8(0x39014+i))end
  assert(s==c[3],'wrong output: '..s)
  assert(m:read_u32(0x39004)==#s and m:read_u32(0x39010)==#s,'count mismatch')
  assert(m:read_u32(@COUNTER@)==#s,'GCC global access mismatch')
  assert(m:read_u32(0x39008)==@GOT@,'callback A5 mismatch')
  assert(m:read_u32(0x3900c)==0 and m:read_u8(0x39014+n)==0,'buffer mismatch')
  assert(m:read_u32(0x38ffc)==0xa5a5a5a5 and m:read_u32(0x39114)==0xa5a5a5a5,'sentinel mismatch')
  print('PASS formatter case '..index..': bytes='..n..' registers/stack/context/A5 preserved')
 end
end)
'''
    for key,value in [('PRINTF',printf),('ENGINE',engine),('ENTRY',entry),('GOT',got),('COUNTER',counter)]:script=script.replace('@'+key+'@',str(value))
    lua=folder/'probe.lua';lua.write_text(script)
    args=[workspace/'mame'/('neodiag' if neo else 'mame'),'asneo' if neo else 'asma3k','-rompath',out/'roms' if neo else workspace/'mame/roms','-nvram_directory',folder/'nvram','-cfg_directory',folder/'cfg','-video','none','-sound','none','-nothrottle','-seconds_to_run','12','-autoboot_delay','0','-autoboot_script',lua]
    log=folder/'mame.log'
    with log.open('w') as f:run(args,cwd=out,env={**os.environ,'SDL_VIDEODRIVER':'dummy'},stdout=f,stderr=subprocess.STDOUT)
    text=log.read_text()
    assert 'PASS formatter GCC callback 2 cases' in text and 'LUA ERROR' not in text,text
    print(name+': PASS')
print('Evidence:',out)
