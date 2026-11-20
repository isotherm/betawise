local slots={[0]=5035358,[1]=5036070,[2]=5035748,[3]=5035212,[4]=5035014,[5]=5035184,[6]=5035280,[7]=5035336,[8]=5035546,[9]=5035602,[10]=5036646,[11]=5036660,[12]=5035114,[13]=5035486,[14]=5036830,[15]=5037068,[16]=0,[17]=0,[18]=5037596,[19]=0,[20]=0,[21]=0,[22]=0,[23]=0,[24]=0,[25]=0,[26]=0,[27]=5029704,[28]=5029760,[29]=5029722,[30]=5029790,[31]=5029544,[32]=5029964,[33]=5030268,[34]=5031036,[35]=5030792,[36]=5030972,[37]=5038228,[38]=5038192,[39]=5038210,[40]=5037918,[41]=5039218,[42]=5038370,[43]=5037926,[44]=5039464,[45]=5040584,[46]=5038092,[47]=5038158,[48]=5038108,[49]=5038004,[50]=5038020,[51]=5038184,[52]=5038174,[53]=5043820,[54]=5043786,[55]=5043894,[56]=5044658,[57]=5044744,[58]=5044822,[59]=5045158,[60]=5025308,[61]=5026260,[62]=5026522,[63]=5026560,[64]=5025384,[65]=5025510,[66]=5026570,[67]=5026578,[68]=5026602,[69]=5026648,[70]=5025300,[71]=5024464,[72]=5054148,[73]=5054006,[74]=5054420,[75]=5054472,[76]=5053996,[77]=5055404,[78]=5052978,[79]=5045686,[80]=5045998,[81]=5046734,[82]=5046972,[83]=5045924,[84]=5046720,[85]=5046702,[86]=5047010,[87]=5047272,[88]=5040960,[89]=5041038,[90]=5041254,[91]=5040724,[92]=5041700,[93]=5052202,[94]=5052192,[95]=5052236,[96]=5052210,[97]=5052762,[98]=5052720,[99]=5052750,[100]=5116114,[101]=5116256,[102]=5116310,[103]=5117274,[104]=5117398,[105]=5117490,[106]=5117602,[107]=5117656,[108]=5117856,[109]=5117920,[110]=5118178,[111]=5118436,[112]=5118490,[113]=5118654,[114]=5118744,[115]=5119048,[116]=5119056,[117]=5119160,[118]=5122514,[119]=5122854,[120]=5122976,[121]=5124238,[122]=5119952,[123]=5121292,[124]=5121786,[125]=5122238,[126]=5122306,[127]=5122450,[128]=5114220,[129]=5114432,[130]=5114660,[131]=5114762,[132]=5114936,[133]=5115062,[134]=5115118,[135]=5115126,[136]=5115200,[137]=5115210,[138]=5115222,[139]=5115298,[140]=5115336,[141]=5115346,[142]=5115386,[143]=5115486,[144]=5115542,[145]=5115650,[146]=5111642,[147]=5113078,[148]=5113116,[149]=5111530,[150]=5111650,[151]=5015480,[152]=5109348,[153]=5109366,[154]=5109388,[155]=5109406,[156]=5109450,[157]=5004120,[158]=5063620,[159]=5063850,[160]=5063924,[161]=5063738,[162]=5001040,[163]=5001052,[164]=5001254,[165]=5001482,[166]=5001542,[167]=5001836,[168]=5001900,[169]=5002100,[170]=5003136,[171]=5127964,[172]=5128076,[173]=5050062,[174]=5050094,[175]=5064008,[176]=5066338,[177]=5065152,[178]=5067786,[179]=5067944,[180]=5069312,[181]=5069468,[182]=5069562,[183]=5069712,[184]=5065074,[185]=5065688,[186]=5065742,[187]=5069720,[188]=5086888,[189]=5088162,[190]=5089064,[191]=5089220,[192]=5088040,[193]=5089816,[194]=5065324,[195]=0,[196]=0,[197]=0,[198]=0,[199]=0,[200]=0,[201]=0,[202]=0,[203]=5113504,[204]=5113512,[205]=5113636,[206]=5113670,[207]=5102938,[208]=5140804,[209]=5138970,[210]=5114006,[211]=5138834,[212]=5113686,[213]=5138874,[214]=5140946,[215]=5102380,[216]=5102428,[217]=5103080,[218]=5102502,[219]=5100562,[220]=5140984,[221]=5138922,[222]=5100602,[223]=5141024,[224]=5102542,[225]=5105580,[226]=5102570,[227]=5102606,[228]=5103140,[229]=5102628,[230]=5103158,[231]=5102690,[232]=5141034,[233]=5102752,[234]=5102888,[235]=5105624,[236]=5114162}
local machine=manager.machine
local cpu=machine.devices[':maincpu'];local m=cpu.spaces.program
local frame=0
local function str(a,n) local t={} for i=0,(n or 128)-1 do local b=m:read_u8(a+i);if b==0 then break end;t[#t+1]=string.char(b)end;return table.concat(t)end
local function put(a,s)for i=1,#s do m:write_u8(a+i-1,s:byte(i))end;m:write_u8(a+#s,0)end
local function lcd()
 local rows={}
 for _,tag in ipairs({':ks0066_0',':ks0066_1'})do
  local d=emu.item(assert(machine.devices[tag].items['0/m_ddram']))
  local map={[0]='g',[1]='y',[2]='p',[3]='q',[0xda]='\\',[0xd5]='~'}
  for _,o in ipairs({0,0x40})do rows[#rows+1]=(d:read_block(o,40):gsub('.',function(c)return map[c:byte()]or c end))end
 end
 return table.concat(rows,'\n')
end
local function dump(label)
 print('STATE '..label..' enabled='..m:read_u8(0x3df2)..' selected='..m:read_u8(0x3df3)..' pc='..string.format('%x',cpu.state.PC.value)..'\n'..lcd())
 local f=assert(io.open(label..'.ram','wb'));for a=0,0x3ffff do f:write(string.char(m:read_u8(a)))end;f:close()
 f=assert(io.open(label..'.lcd.txt','w'));f:write(lcd());f:close()
end
local function wait(fn,label,budget)
 local untilframe=frame+(budget or 600)
 repeat if fn()then return end;assert(frame<untilframe,'timeout '..label..'\n'..lcd());coroutine.yield()until false
end
local function screen(s)local last,count=nil,0;wait(function()local v=lcd();count=v==last and count+1 or 1;last=v;return count>=3 and v:find(s,1,true)~=nil end,s)end
local function blank()local count=0;wait(function()count=lcd():match('^%s*$') and count+1 or 0;return count>=3 end,'blank AlphaWord editor')end
local function type(s)machine.natkeyboard:post(s);wait(function()return machine.natkeyboard.empty end,'input drained')end
local regs={'D0','D1','D2','D3','D4','D5','D6','D7','A0','A1','A2','A3','A4','A5','A6','SP','PC','SR'}
local saved
local function invoke(addr,args)
 assert(not saved);saved={}
 for _,r in ipairs(regs)do saved[r]=cpu.state[r].value end
 saved.mem={};for i=0,0x7ff do saved.mem[i]=m:read_u8(0x3e000+i)end
 m:write_u16(0x3e000,0x4eb9);m:write_u32(0x3e002,addr);m:write_u16(0x3e006,0x60fe)
 for i,v in ipairs(args or {})do m:write_u32(0x3e700+(i-1)*4,v)end
 cpu.state.SP.value=0x3e700;cpu.state.PC.value=0x3e000
end
local function returned()return cpu.state.PC.value==0x3e006 or cpu.state.PC.value==0x3e008 end
local function finish()
 wait(returned,'routine return');local v=cpu.state.D0.value
 for i=0,0x7ff do m:write_u8(0x3e000+i,saved.mem[i])end
 for _,r in ipairs(regs)do if r~='PC' and r~='SR'then cpu.state[r].value=saved[r]end end
 cpu.state.SR.value=saved.SR;cpu.state.PC.value=saved.PC;saved=nil
 return v
end
local function call(addr,args)invoke(addr,args);return finish()end
local function slot(n,args)return call(assert(slots[n]),args)end
local function files(folder)
 local out={};local a=m:read_u32(0xf42+folder*8);local count=m:read_u8(0xf46+folder*8)
 for i=0,count-1 do local b=a+i*0x48;out[#out+1]={id=m:read_u16(b+0x44),name=str(b+0x34,16),length=m:read_u32(b+4),address=m:read_u32(b),tag=m:read_u16(b+0x46)}end
 return out
end
local function find(name,group,folder)
 for _,f in ipairs(files(folder or 1))do if f.name:lower()==name:lower() and math.floor(f.id/256)==group then return f end end
end
local function coded(s)machine.natkeyboard:post_coded(s);wait(function()return machine.natkeyboard.empty end,'coded input drained')end
local function profile(name,password,num)
 -- Native startup helper marks selection pending without changing existing profiles.
 call(0x4df9ee);assert(m:read_u8(0x3df3)>=128)
 invoke(0x4dfa3e);screen('Select a user');dump('selector-'..num)
 -- Add New User is last, and the previous profile is initially selected.
 print('CHOICE before '..m:read_u8(0x3e2d));local down;for _,f in pairs(machine.ioport.ports[':COL.5'].fields)do if f.mask==2 then down=f end end;assert(down);down:set_value(1);wait(function()return m:read_u8(0x3e2d)==num end,'Add New User choice');down:clear_value();print('CHOICE after '..m:read_u8(0x3e2d))
 type('\r');screen('Enter name:');type(name..'\r');screen('Enter password:');type(password..'\r');screen('Reenter password:');type(password..'\r')
 wait(function()return m:read_u8(0x3df3)==num end,'profile selected');finish()
 assert(str(0x3d7a+(num-1)*30,20)==name);print('PASS named profile '..num..' '..name)
end
local main=coroutine.create(function()
 screen('backup battery?');type('y');screen('enter key to restart');type('\r');wait(function()return str(0x4473,7)~='' and not lcd():find('restart',1,true) and not lcd():find('backup battery?',1,true)end,'initialized boot');screen('AlphaWord');dump('launcher');type('\r');wait(function()return m:read_u32(0x355c)==1 and m:read_u8(0xf46+8)>0 and not lcd():find('AlphaWord',1,true)end,'AlphaWord initialized');blank();dump('boot');type('BASELINE');screen('BASELINE');dump('baseline-text')
 assert(m:read_u8(0x3df2)==0,'default mode enabled')
 print('MASTER '..str(0x4473,7))
 invoke(0x4e00f8)
 screen('Multiple user feature');dump('toggle-screen')
 type(str(0x4473,7)..'\r')
 wait(function()return m:read_u8(0x3df2)==1 end,'enabled flag')
 dump('enabled')
 -- Observe whether toggle returns or enters the selector itself.
 wait(function()return returned() or lcd():find('Add New User',1,true)end,'toggle continuation')
 dump('toggle-continuation')
 if returned()then finish();invoke(0x4dfa3e)else saved=nil end
 screen('Add New User');type('\r');screen('Enter name:');type('Alice\r')
 screen('Enter password:');type('alice\r');screen('Reenter password:');type('alice\r')
 wait(function()return m:read_u8(0x3df3)==1 end,'selected Alice')
 dump('alice-created');wait(function()return m:read_u32(0x355c)==1 and not lcd():find('password',1,true) and not lcd():find('Updater',1,true)end,'AlphaWord activation');screen('BASELINE');dump('alice-sees-group-zero');print('OBSERVED Alice opens existing group-zero BASELINE');call(0x4dfa3e);assert(m:read_u8(0x3df3)==1);print('PASS selector returns immediately when profile already selected');type('ALICEONLY');screen('ALICEONLY');dump('alice-text')

 -- Exercise AlphaWord's native Save UI under profile 1.
 local ctrl
 for _,f in pairs(machine.ioport.ports[':COL.12'].fields)do if f.mask==0x80 then ctrl=f end end
 assert(ctrl);ctrl:set_value(1);type('s');ctrl:clear_value();dump('save-dialog')
 screen('name');type('Shared\r');wait(function()return find('Shared',0)~=nil end,'AlphaWord profile file');dump('alice-saved')
 assert(not find('Shared',1));print('OBSERVED AlphaWord Save renames existing group-zero file under Alice')
 ctrl:set_value(1);type('n');ctrl:clear_value();wait(function()return find('File 1',1)~=nil or lcd():find('keep',1,true)end,'native new file');dump('alice-new-dialog')
 if lcd():find('keep',1,true)then type('y');screen('name');type('OldText\r')end
 wait(function()return find('File 1',1)~=nil end,'native group 1 workspace');blank();type('PRIVATEALICE');screen('PRIVATEALICE');dump('alice-native-group1')
 print('PASS AlphaWord Ctrl-N creates and edits group 1 workspace')
 put(0x3e900,'Shared');local anew=slot(122,{1,0x3e900,0,512});assert(anew>=0x101 and anew<=0x1ff);print('PASS group 1 same-name creation')
 profile('Bobby','bobby',2)
 put(0x3e900,'Shared');local bob=slot(122,{2,0x3e900,0,512});assert(bob==0x201,string.format('Bob create %x',bob))
 put(0x3e900,'shared');local duplicate=slot(122,{2,0x3e900,0,512});assert(duplicate==0xfffffff1,'duplicate accepted')
 put(0x3e920,'BOB CONTENT');assert(slot(102,{bob,0x3e920,11,3})==11)
 assert(slot(125,{bob,0x1234,0})==0)
 assert(slot(126,{0x1234})==bob)
 put(0x3e900,'BobOnly');assert(slot(122,{2,0x3e900,0,512})==0x202);print('PASS group 2 duplicate rejection, cross-group same name, raw write and tag lookup')
 profile('Carol','carol',3)
 put(0x3e900,'Shared');local carol=slot(122,{3,0x3e900,0,512});assert(carol==0x301,string.format('Carol create %x',carol))
 assert(slot(126,{0x1234})==0xfffffff5,'tag lookup leaked other profile')
 print('PASS group 3 create and tag filtering')
 profile('David','david',4)
 put(0x3e900,'Shared');local david=slot(122,{4,0x3e900,0,512});print(string.format('GROUP4 create=%08x',david));assert(david==0xfffffff6,'unexpected group4 result')
 dump('four-profiles');print('PASS characterized group 4 creation rejection')
 -- Explicit IDs are intentionally not an access-control boundary.
 assert(slot(100,{bob,0,0})==0);assert(slot(103,{bob,0x3e940,11})==11);assert(str(0x3e940,11)=='BOB CONTENT');print('PASS explicit cross-profile ID remains accessible')
 -- Reenter normal firmware startup; selection is forced by the native startup helper.
 call(0x4df9ee);invoke(0x4c0068);saved=nil;screen('Select a user');dump('restart-selector')
 local up;for _,f in pairs(machine.ioport.ports[':COL.7'].fields)do if f.mask==0x80 then up=f end end;assert(up)
 while m:read_u8(0x3e2d)>1 do local before=m:read_u8(0x3e2d);up:set_value(1);wait(function()return m:read_u8(0x3e2d)<before end,'previous profile');up:clear_value();coroutine.yield()end
 type('\r');screen('Enter password for Alice');type('wrong\r');screen('incorrect');assert(m:read_u8(0x3df3)>=128);screen('Enter password for Alice');type('alice\r');wait(function()return m:read_u8(0x3df3)==1 end,'Alice login');screen('PRIVATEALICE');dump('alice-returned');print('PASS named profiles persist, wrong password rejected, Alice native text reopened')
 ctrl:set_value(1);type('f');ctrl:clear_value();screen('[F1]');dump('alice-file-selector');print('OBSERVED AlphaWord selector lists group-zero and profile workspaces; no isolation claim')
 local esc;for _,f in pairs(machine.ioport.ports[':COL.4'].fields)do if f.mask==0x80 then esc=f end end;assert(esc);esc:set_value(1);wait(function()return lcd():find('PRIVATEALICE',1,true) and not lcd():find('Press enter',1,true)end,'close file selector');esc:clear_value()
 call(0x4df9ee);invoke(0x4c0068);saved=nil;screen('Select a user')
 local down;for _,f in pairs(machine.ioport.ports[':COL.5'].fields)do if f.mask==2 then down=f end end;assert(down);down:set_value(1);wait(function()return m:read_u8(0x3e2d)==2 end,'Bobby choice');down:clear_value()
 type('\r');screen('Enter password for Bobby');type('bobby\r');wait(function()return m:read_u8(0x3df3)==2 end,'Bobby login');screen('PRIVATEALICE');dump('bobby-sees-alice-text');print('OBSERVED Bobby reopens Alice profile workspace through AlphaWord cached selection')
 invoke(0x4e00f8);screen('Multiple user feature');type(str(0x4473,7)..'\r');wait(function()return m:read_u8(0x3df2)==0 end,'disabled flag');saved=nil
 wait(function()return m:read_u8(0x3df3)>=128 end,'unfiltered mode');assert(str(0x3d7a,20)=='Alice');assert((slot(149,{0,0})&0xff)==0);dump('disabled');print('PASS disable retains profiles and makes profile query return zero')
 print('PASS multi-user qualification complete');machine:exit()
end)
emu.register_frame_done(function()
 frame=frame+1
 if coroutine.status(main)=='dead'then return end
 local ok,err=coroutine.resume(main)
 if not ok then print('FAIL '..tostring(err));dump('failure');machine:exit()end
end)
