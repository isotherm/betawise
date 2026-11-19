local machine=manager.machine
local cpu=machine.devices[":maincpu"];local m=cpu.spaces.program
local frame=0
local function load(path,addr)local f=assert(io.open(path,"rb"));local s=f:read("*a");f:close();for i=1,#s do m:write_u8(addr+i-1,s:byte(i))end end
emu.register_frame_done(function()
frame=frame+1
if frame==180 then
 load('typed.bin',0x36000);load('bridge.bin',0x37e00)
 m:write_u16(0x37f00,0xa358);m:write_u16(0x37f10,0xa074);m:write_u16(0x37f20,0xa444)
 m:write_u32(0x38100,0x61626362);m:write_u16(0x38104,0x6100)
 for o=0,0x20 do m:write_u8(0x38300+o,0)end
 m:write_u16(0x38000,0x60fe);m:write_u32(0x3d000,0x38000)
 cpu.state.SR.value=0x2700;cpu.state.SP.value=0x3d000;cpu.state.PC.value=0x36000
elseif frame==190 then
 assert(cpu.state.PC.value==0x38000 or cpu.state.PC.value==0x38002,'typed probe did not return')
 assert(m:read_u32(0x39000)==0x38105,'raw memchr did not reproduce wrong A0')
 assert(m:read_u32(0x39004)==0x38101,'D0 bridge failed')
 assert(m:read_u32(0x39008)==12 and m:read_u32(0x3900c)==0,'hidden A1 result failed')
 assert(m:read_u32(0x39010)==0x70617373)
 print('PASS compiled C: raw memchr returns WRONG end pointer; bridge correct; six-byte aggregate via A1 correct')
 -- Repeat aggregate directly to inspect return A0, with a sentinel around result.
 m:write_u32(0x3d000,0x38000);m:write_u32(0x3d004,0x38300);m:write_u32(0x3d008,0x38200);m:write_u32(0x3d00c,10);m:write_u32(0x3d010,0x38400)
 for o=0,9 do m:write_u8(0x38500+o,0xa5)end
 cpu.state.A1.value=0x38500;cpu.state.SR.value=0x2700;cpu.state.SP.value=0x3d000;cpu.state.PC.value=0x37f20
elseif frame==200 then
 assert(cpu.state.A0.value==0x38504,'aggregate A0 convention changed')
 assert(m:read_u8(0x38500)==12 and m:read_u32(0x38502)==0 and m:read_u32(0x38506)==0xa5a5a5a5)
 print('PASS aggregate: writes six bytes through A1; A0=result+4; boundary sentinel preserved')
 machine:exit()
end
end)
