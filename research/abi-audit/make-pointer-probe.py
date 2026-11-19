from pathlib import Path
import sys
output=Path(sys.argv[1]);output.mkdir(parents=True,exist_ok=True)
prefix='''local machine=manager.machine
local cpu=machine.devices[":maincpu"]
local m=cpu.spaces.program
local code,stack,a,b,search=0x38000,0x3d000,0x38100,0x38200,0x38300
local function put(addr,s) for i=1,#s do m:write_u8(addr+i-1,s:byte(i)) end;m:write_u8(addr+#s,0) end
local function prep() put(a,"abcba");put(b,"bc");m:write_u32(search,2);m:write_u32(search+4,b);m:write_u32(search+8,5);m:write_u32(search+12,a);m:write_u8(search+16,1) end
local cases={
{0xa070,{a,98},a+1,true},{0xa070,{a,0},0,true},{0xa070,{a,120},0,true},
{0xa074,{a,a+4,98},a+1,false},{0xa074,{a,a+4,120},0,false},
{0xa078,{search},a+1,false},{0xa078,{search},0,false,function()m:write_u32(search,0)end},
{0xa358,{a,98,5},a+1,false},{0xa358,{a,120,5},0,false},{0xa358,{a,97,0},0,false},
{0xa360,{b,a,5},b,false},{0xa360,{b,a,0},b,false},
{0xa364,{a+1,a,4},a+1,true},{0xa364,{a,a+1,4},a,false},{0xa364,{b,a,0},b,true},
{0xa368,{b,90,3},b,false},{0xa368,{b,90,0},b,true},
{0xa380,{a,b},a,false},{0xa384,{a,98},a+1,false},{0xa384,{a,120},0,false},{0xa384,{a,0},a+5,false},
{0xa38c,{b,a},b,false},{0xa394,{a,b,1},a,false},{0xa394,{a,b,0},a,false},
{0xa39c,{b,a,4},b,false},{0xa39c,{b,a,0},b,false},
{0xa3a0,{a,98},a+3,true},{0xa3a0,{a,120},0,true},{0xa3a0,{a,0},a+5,true},
{0xa3a4,{a,b},a+1,false},{0xa3a4,{a,b},a,false,function()put(b,"")end},{0xa3a4,{a,b},0,false,function()put(b,"z")end},
{0xa1c4,{0xcb,0x38400,0x38404,0x38408,0x3840c},a,false,function()
 for o=0,0x47 do m:write_u8(search+o,0)end
 m:write_u32(CLIP_GLOBAL,search);m:write_u32(search,a);m:write_u32(search+4,5);m:write_u32(search+12,20);m:write_u32(search+20,2)
 end},
{0xa1f0,{0x101,b,0},search+0x34,false,function()
 for o=0,0x47 do m:write_u8(search+o,0)end
 m:write_u32(CLIP_GLOBAL,search);m:write_u32(search,a);put(search+0x34,"test");m:write_u32(CURRENT,0);m:write_u16(IDS,0);m:write_u32(TABLE,search);m:write_u8(TABLE+4,1);m:write_u16(search+0x44,0x101)
 end},
{0xa220,{},a,false,function() m:write_u32(CLIP_GLOBAL,search);m:write_u32(search,a)end},
}
local frame,index=0,0
local failed=false
local regs={"D2","D3","D4","D5","D6","D7","A2","A3","A4","A5","A6"}
emu.register_frame_done(function()
 if failed then return end
 frame=frame+1
 if frame<180 then return end
 local phase=(frame-180)%6
 if phase==0 then
 index=index+1;if index>#cases then print("PASS ABI pointer probe "..#cases.." cases");machine:exit();return end
 prep();local p=cases[index];if p[5]then p[5]()end
 for j,r in ipairs(regs)do cpu.state[r].value=0x12340000+j*0x100 end
 cpu.state.A0.value=0x77777770;cpu.state.D0.value=0x55555550
 m:write_u16(code,p[1]);m:write_u16(code+2,0x60fe);m:write_u32(stack,code+2)
 for j,v in ipairs(p[2])do m:write_u32(stack+j*4,v)end
 cpu.state.SR.value=0x2700;cpu.state.SP.value=stack;cpu.state.PC.value=code
 elseif phase==4 and index<=#cases then
 local ok,err=pcall(function()
 local p=cases[index];local d0,a0=cpu.state.D0.value,cpu.state.A0.value
 assert(cpu.state.PC.value==code+2 or cpu.state.PC.value==code+4,"no return")
 assert(cpu.state.SP.value==stack+4,"stack changed")
 assert(d0==p[3],string.format("%x D0 %x != %x",p[1],d0,p[3]))
 assert((a0==p[3])==p[4],string.format("%x unexpected A0 %x",p[1],a0))
 local function bytes(addr,n)local t={};for k=0,n-1 do t[#t+1]=string.char(m:read_u8(addr+k))end;return table.concat(t)end
 if p[1]==0xa1c4 then assert(m:read_u32(0x38400)==a and m:read_u32(0x38404)==5 and m:read_u32(0x38408)==20 and m:read_u32(0x3840c)==2,'file mirrors')end
 if p[1]==0xa1f0 then assert(bytes(b,5)=="test\0",'name copy')end
 if p[1]==0xa360 and p[2][3]==5 then assert(bytes(b,5)=='abcba','memcpy output')end
 if p[1]==0xa364 and p[2][3]==4 then assert(bytes(a,5)==(p[2][1]==a and 'bcbaa' or 'aabcb'),'memmove output')end
 if p[1]==0xa368 and p[2][3]==3 then assert(bytes(b,3)=='ZZZ','memset output')end

 for j,r in ipairs(regs)do assert(cpu.state[r].value==0x12340000+j*0x100,r.." clobbered")end
 print(string.format("PASS %04x D0=%08x A0=%08x saved-registers/SP",p[1],d0,a0))
 end)
 if not ok then failed=true;print("FAIL ABI "..tostring(err));machine:exit()end
 end
end)
'''
for name,glob in [('as3000',0x344a),('neo_3_21',0x34da)]:
 (output/(name+'.lua')).write_text(prefix.replace('CLIP_GLOBAL',str(glob)).replace('CURRENT',str(0x355c if name=='as3000' else 0x35ec)).replace('IDS',str(0xe02 if name=='as3000' else 0xe92)).replace('TABLE',str(0xf42 if name=='as3000' else 0xfd2)))
