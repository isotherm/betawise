#!/usr/bin/env python3
"""Generate isolated MAME tests of the original file-entry resolver.

The comparison-only variant is a diagnostic, NOT a deployable patch: it does
not validate entry alignment. Original firmware files are never modified.
"""
from pathlib import Path
import json
import sys

here = Path(__file__).resolve().parent
workspace = here.parents[2]
out = Path(sys.argv[1]).resolve()
out.mkdir(parents=True, exist_ok=True)
roms = json.loads((here.parent/'abi-audit/rom-inventory.json').read_text())
# Resolver starts/ends and the inverted upper-bound branch, verified in listings.
addresses = {
    'as3000': (0x4e366e, 0x4e383c, 0x4e370a, 0x355c, 0xe02, 0xf42, 0x3442, 0x3df3),
    'neo_3_01': (0x5e5fe2, 0x5e61b0, 0x5e607e, 0x35ec, 0xe92, 0xfd2, 0x34d2, 0x3e83),
    'neo_3_21': (0x43c3e4, 0x43c5b2, 0x43c480, 0x35ec, 0xe92, 0xfd2, 0x34d2, 0x3e83),
}
template = r'''
local machine=manager.machine
local cpu=machine.devices[":maincpu"]
local m=cpu.spaces.program
local code,stack,copy,result=0x38000,0x3d000,0x39000,0x38200
local start,count=0x1200,2
local last=start+count*0x48
local current,ids,table,cache,user=GLOBALS
local helper,finish,branch=ADDRESSES
local tests={
 {"null cache",0,start+0x48,start+0x48},
 {"first valid entry",start,start+0x48,start},
 {"second valid entry",start+0x48,start+0x48,start+0x48},
 {"one-past table",last,last,start+0x48},
 {"above table",last+0x48,last+0x48,start+0x48},
 {"below table",start-0x48,start+0x48,start+0x48},
 {"interior address",start+2,start+0x48,start+2},
}
local frame,index,failed=0,0,false
emu.register_frame_done(function()
 if failed then return end
 frame=frame+1
 if frame<180 then return end
 local phase=(frame-180)%5
 if phase==0 then
  index=index+1
  if index>#tests*3 then print("PASS resolver characterization "..(#tests*3).." cases");machine:exit();return end
  local t=tests[math.floor((index-1)/3)+1]
  local variant=(index-1)%3
  local corrected=variant==1
  m:write_u32(current,0);m:write_u16(ids,0);m:write_u32(table,start)
  m:write_u8(table+4,count);m:write_u8(table+5,0xff);m:write_u16(table+6,0)
  m:write_u8(user,0x80);m:write_u32(cache,t[2])
  for o=0,0x48*4-1 do m:write_u8(start+o,0)end
  m:write_u16(start+0x44,1);m:write_u16(start+0x48+0x44,2)
  m:write_u8(start+0x48+0x23,2)
  for o=0,finish-helper-1 do m:write_u8(copy+o,m:read_u8(helper+o))end
  assert(m:read_u16(copy+branch-helper)==0x6506,'unexpected branch')
  if corrected then m:write_u16(copy+branch-helper,0x6406)end
  if variant==2 then
   assert(m:read_u16(copy+0x6c)==0x4a94,'unexpected cache test')
   m:write_u16(copy+0x6c,0x6036)
  end
  m:write_u16(code,0x4eb9);m:write_u32(code+2,variant>0 and copy or helper);m:write_u16(code+6,0x60fe)
  m:write_u32(stack,0);m:write_u32(stack+4,result);m:write_u32(stack+8,0)
  cpu.state.SR.value=0x2700;cpu.state.SP.value=stack;cpu.state.PC.value=code
 elseif phase==3 then
  local t=tests[math.floor((index-1)/3)+1]
  local variant=(index-1)%3
  local corrected=variant==1
  local ok,err=pcall(function()
   assert(cpu.state.PC.value==code+6 or cpu.state.PC.value==code+8,'no return')
   local actual=cpu.state.D0.value
   local expected=variant==2 and start+0x48 or t[corrected and 4 or 3]
   assert(actual==expected,string.format('%s got %x expected %x',t[1],actual,expected))
   print(string.format('PASS %s %s returned=%x',variant==2 and 'cache-bypass' or (corrected and 'comparison-only' or 'original'),t[1],actual))
  end)
  if not ok then failed=true;print('FAIL resolver '..tostring(err));machine:exit()end
 end
end)
'''
for r in roms:
    a=addresses[r['name']]
    b=(workspace/r['path']).read_bytes()
    assert b[a[2]-r['base']:a[2]-r['base']+2]==bytes.fromhex('6506'),r['name']
    lua=template.replace('GLOBALS',','.join(hex(x) for x in a[3:]))
    lua=lua.replace('ADDRESSES',','.join(hex(x) for x in a[:3]))
    (out/(r['name']+'-resolver.lua')).write_text(lua)
print('Generated resolver probes; comparison-only byte change stays in scratch RAM.')
