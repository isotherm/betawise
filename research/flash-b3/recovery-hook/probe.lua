-- Test instrumentation only; no flash commands and no screen assertions.
local machine=manager.machine
local cpu=machine.devices[':maincpu']
local mem=cpu.spaces.program
local taps,seen={},{}
local sequence,frame=0,0
local function event(name)
 sequence=sequence+1
 print(string.format('EVENT %d %s PC=%08X',sequence,name,cpu.state.PC.value))
end
taps[#taps+1]=mem:install_write_tap(0x30000,0x30003,'recovery-marker',function(off,data,mask)
 if cpu.state.PC.value>=stub and cpu.state.PC.value<stub+16 then
  if not seen.recovery then seen.recovery=true;event('recovery') end
 end
end)
for _,site in ipairs(sites) do
 local name,address=site[1],site[2]
 taps[#taps+1]=mem:install_read_tap(address,address+1,'trace-'..name,function(off,data,mask)
  local pc=cpu.state.PC.value
  if pc>=address and pc<=address+4 and not seen[name] then seen[name]=true;event(name) end
 end)
end
local function key(name,value)
 for row=0,15 do
  local port=machine.ioport.ports[':ROW'..row]
  if port then for _,field in pairs(port.fields) do
   if field.name:sub(1,#name+2)==name..' [' then
    if value then field:set_value(1) else field:clear_value() end
    return
   end
  end end
 end
 error('missing key '..name)
end
emu.register_frame_done(function()
 frame=frame+1
 if frame==150 then if isneo then key('Y',true) else machine.natkeyboard:post('y') end end
 if frame==156 and isneo then key('Y',false) end
 if frame==360 then if isneo then key('Enter',true) else machine.natkeyboard:post('\r') end end
 if frame==366 and isneo then key('Enter',false) end
 if frame==650 then
  print(string.format('FINAL PC=%08X marker=%08X',cpu.state.PC.value,mem:read_u32(0x30000)))
  for _,name in ipairs({'recovery','candidate','scan','original'}) do print('SEEN '..name..' '..tostring(seen[name] or false)) end
  print('PROBE_COMPLETE');machine:exit()
 end
end)
