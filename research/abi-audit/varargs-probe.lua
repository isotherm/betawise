local machine=manager.machine;local cpu=machine.devices[":maincpu"];local m=cpu.spaces.program
local frame,index=0,0
local code,stack,a,b,c=0x38000,0x3d000,0x38100,0x38200,0x38300
local function put(addr,s)for i=1,#s do m:write_u8(addr+i-1,s:byte(i))end;m:write_u8(addr+#s,0)end
local function get(addr)local s="";for i=0,127 do local n=m:read_u8(addr+i);if n==0 then return s end;s=s..string.char(n)end;return s end
local cases={
{0xa378,{a,b,0x12345678,0x12345678,0x12345678},"%d %ld %x"},
{0xa378,{a,b,0x3ff80000,0},"%.1f"},
{0xa378,{a,b,0x11223344,0x55667788,0x55},"%lld %x"},
{0xa344,{a,b,c},"%f","1.5"},
{0xa344,{a,b,c},"%lf","1.5"},
{0xa344,{a,b,c},"%lld","1234567890123"},
}
emu.register_frame_done(function()
frame=frame+1;if frame<180 then return end
local phase=(frame-180)%12
if phase==0 then
 index=index+1;if index>#cases then print('DONE variadic characterization');machine:exit();return end
 local p=cases[index];put(b,p[3]);put(a,p[4]or"");for i=0,15 do m:write_u8(c+i,0xa5)end
 m:write_u16(code,p[1]);m:write_u16(code+2,0x60fe);m:write_u32(stack,code+2)
 for j,v in ipairs(p[2])do m:write_u32(stack+4*j,v)end
 cpu.state.SR.value=0x2700;cpu.state.SP.value=stack;cpu.state.PC.value=code
elseif phase==10 then
 local p=cases[index]
 print(string.format('FORMAT %s PC=%x D0=%x text=%s output=%08x %08x %08x',p[3],cpu.state.PC.value,cpu.state.D0.value,get(a),m:read_u32(c),m:read_u32(c+4),m:read_u32(c+8)))
end
end)
