
local machine=manager.machine
local cpu=machine.devices[":maincpu"]
local m=cpu.spaces.program
local code,stack,copy,result=0x38000,0x3d000,0x39000,0x38200
local start,count=0x1200,2
local last=start+count*0x48
local current,ids,table,cache,user=0x35ec,0xe92,0xfd2,0x34d2,0x3e83
local helper,finish,branch=0x5e5fe2,0x5e61b0,0x5e607e
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
