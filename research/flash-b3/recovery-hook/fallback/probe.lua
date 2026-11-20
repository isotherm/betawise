local machine=manager.machine
local cpu=machine.devices[':maincpu']
local memory=cpu.spaces.program
local seen,taps={},{}
local function key(name,pressed)
 for row=0,15 do
  local port=machine.ioport.ports[':ROW'..row]
  if port then for _,f in pairs(port.fields) do
   if f.name==name or f.name:sub(1,#name+2)==name..' [' then if pressed then f:set_value(1) else f:clear_value() end;return end
  end end
 end
 error('missing key '..name)
end
local chord={'Right Shift','Comma','Period','Slash'}
if force then for _,name in ipairs(chord) do key(name,true) end end
for _,item in ipairs({{'password',0x4013C0},{'handoff',0x40143E},{'updater',0x401730},{'usb',0x402EC4},{'main',0x410082}}) do
 local name,addr=item[1],item[2]
 taps[#taps+1]=memory:install_read_tap(addr,addr+1,'fallback-'..name,function(off,data,mask)
  if not seen[name] then seen[name]=true;print(string.format('REACHED %s PC=%08X',name,cpu.state.PC.value)) end
 end)
end
local comparisons=0
taps[#taps+1]=memory:install_read_tap(0x401402,0x401403,'password-key-return',function()
 if comparisons<15 then comparisons=comparisons+1;print(string.format('PASSWORD_KEY D0=%08X D7=%08X PC=%08X',cpu.state.D0.value,cpu.state.D7.value,cpu.state.PC.value)) end
end)
local frame=0
emu.register_frame_done(function()
 frame=frame+1
 if force then
  if frame==100 then for _,name in ipairs(chord) do key(name,false) end end
  local letters={'E','R','N','I','E'}
  for i,name in ipairs(letters) do
   if frame==600+(i-1)*120 then key(name,true) end
   if frame==612+(i-1)*120 then key(name,false) end
  end
 end
 if frame==1800 then
  print(string.format('FINAL PC=%08X',cpu.state.PC.value))
  for _,name in ipairs({'password','handoff','updater','usb','main'}) do print('SEEN '..name..' '..tostring(seen[name] or false)) end
  print('FALLBACK_COMPLETE');machine:exit()
 end
end)
