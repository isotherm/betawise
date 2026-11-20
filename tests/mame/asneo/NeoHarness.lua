-- Ordered Neo actions, direct LCD RAM predicates, and bounded emulated waits.
-- Python prepends the font, geometry and assertion tables; Neo 3.21 only.
local machine=manager.machine
local memory=machine.devices[":maincpu"].spaces.program
local lcd=dofile(lcd_module)
local actions, events={},{}
local checkpoint_count=0
local draft=''
local applet
local function named(name)
    for row=0,15 do for _,f in pairs(machine.ioport.ports[":ROW"..row].fields) do
        if f.name:sub(1,#name+2)==name.." [" then return f end
    end end
    error("missing Neo key "..name)
end
local function cell(x,y,ch)
    local bits=glyphs[ch];if not bits then return false end
    for dy=0,7 do for dx=0,5 do
        if lcd.black(x+dx,y+dy)~=(bits:sub(dy*6+dx+1,dy*6+dx+1)=='1') then return false end
    end end
    return true
end
local function logo_input(text)
    local tail=text:sub(math.max(1,#text-18));local cursor=(2+#tail)*6
    for row=0,7 do
        local y=row*8;local ok=true
        for yy=y,y+7 do if not lcd.black(cursor,yy) then ok=false;break end end
        if ok and (cell(0,y,'?') or cell(0,y,'>')) and cell(6,y,' ') then
            for i=1,#tail do if not cell((i+1)*6,y,tail:sub(i,i)) then ok=false;break end end
            if ok then for yy=y,y+7 do for x=cursor+1,131 do
                if lcd.black(x,yy) then ok=false;break end
            end end end
            if ok then return true end
        end
    end
    return false
end
local function idle(busy)
    if applet=='logo' then return logo_input(draft) end
    return memory:read_u8(0x48be)==(busy and 0x0c or 0x0f)
end
local function decode(region)
    region=region or {0,0,264,64}
    local lines={}
    for y=region[2],region[2]+region[4]-1,8 do
        local chars={}
        for x=region[1],region[1]+region[3]-1,6 do
            local bits=0
            for dy=0,7 do for dx=0,5 do bits=(bits<<1)|(lcd.black(x+dx,y+dy) and 1 or 0) end end
            local ch=font[bits]
            if not ch then return nil,string.format("unknown/ambiguous glyph at %d,%d",x,y) end
            chars[#chars+1]=ch
        end
        lines[#lines+1]=table.concat(chars):gsub(' +$','')
    end
    return table.concat(lines,'\n'):gsub('^\n+',''):gsub('\n+$','')
end
local function matches(spec)
    if spec.graphics then
        local bits=graphics[spec.graphics]
        for y=0,63 do for x=0,131 do
            if lcd.black(132+x,y)~=(bits:sub(y*132+x+1,y*132+x+1)=='1') then
                return false,'graphics mismatch: '..spec.graphics
            end
        end end
        return true
    end
    local text,err=decode(spec.region);if not text then return false,err end
    if spec.equals~=nil and text~=spec.equals then return false,'exact text mismatch: '..text end
    if spec.suffix and text:sub(-#spec.suffix)~=spec.suffix then return false,'suffix mismatch: '..text end
    for _,line in ipairs(spec.lines or {}) do
        local found=false
        for value in (text..'\n'):gmatch('(.-)\n') do if value==line then found=true;break end end
        if not found then return false,'missing output line '..line..'\n'..text end
    end
    for _,part in ipairs(spec.contains or {}) do if not text:find(part,1,true) then return false,'missing '..part..'\n'..text end end
    for _,part in ipairs(spec.absent or {}) do if text:find(part,1,true) then return false,'unexpected '..part end end
    return true
end
local cpu = machine.devices[":maincpu"]
local saved_cpu, saved_ram, saved_mode
local scratch, code, stack = 0x3f000, 0x3ff00, 0x3fe00
local register_names = {"D0", "D1", "D2", "D3", "D4", "D5", "D6", "D7",
    "A0", "A1", "A2", "A3", "A4", "A5", "A6", "SP", "PC", "SR"}
local function cursor_mode(mode)
    -- Invoke the real firmware SetCursorMode syscall on a temporary test stack.
    -- No framebuffer edits and no changes to the applet binary.
    memory:write_u16(code, 0xa018)
    memory:write_u16(code+2, 0x60fe)
    memory:write_u32(stack, code+2)
    memory:write_u32(stack+4, mode)
    cpu.state.SR.value = 0x2700
    cpu.state.SP.value = stack
    cpu.state.PC.value = code
end
local function returned()
    assert(cpu.state.PC.value == code+2 or cpu.state.PC.value == code+4, string.format("SetCursorMode did not return: PC=%x", cpu.state.PC.value))
end

local function park_cursor()
    saved_cpu,saved_ram={},{}
    assert((cpu.state.SR.value&0x2000)~=0,'capture requires supervisor context')
    for _,name in ipairs(register_names) do saved_cpu[name]=cpu.state[name].value end
    for i=0,4095 do saved_ram[i]=memory:read_u8(scratch+i) end
    saved_mode=memory:read_u8(0x48be)
    cursor_mode(0x0c)
end
local function resume_guest()
    for i=0,4095 do memory:write_u8(scratch+i,saved_ram[i]) end
    for _,name in ipairs(register_names) do cpu.state[name].value=saved_cpu[name] end
end
local function key(name,modifier,hold)
    actions[#actions+1]={kind='key',name=name,modifier=modifier,hold=hold or 3,
        step=math.max(8,(hold or 3)+5)}
end
local punctuation={[' ']='Space',['=']='Equals',['-']='Minus',['.']='Period',[',']='Comma',[';']='Semicolon',["'"]='Quote',['[']='Left bracket',[']']='Right bracket',['/']='Slash',['\\']='Backslash',['`']='Grave',['\r']='Enter',['\t']='Tab'}
local shifted={['"']='Quote',[':']='Semicolon',['(']='9',[')']='0',['+']='Equals',['_']='Minus',['*']='8',['<']='Comma',['>']='Period',['!']='1',['@']='2',['%']='5',['{']='Left bracket',['}']='Right bracket'}
local function type_text(text,busy)
    for i=1,#text do
        local ch=text:sub(i,i)
        key(shifted[ch] or punctuation[ch] or ch:upper(),(ch:match('%u') or shifted[ch]) and 'Left Shift' or nil)
        actions[#actions].char=ch
        actions[#actions].busy=busy and i==#text
    end
end
local function checkpoint(index) actions[#actions+1]={kind='check',index=index} end
local function probe(fn) actions[#actions+1]={kind='probe',fn=fn} end
local function press(frame,name)
    events[frame]=function() named(name):set_value(1) end
    events[frame+6]=function() named(name):clear_value() end
end
local function boot(name)
    applet=name
    press(150,'Y');press(360,'Enter');press(740,'Applets');press(850,'Down')
    if name=='micropython' then press(910,'Down') end
    press(980,'Enter')
end
local function finish() actions[#actions+1]={kind='finish'} end
local function run()
    local frame,index,started,previous,stable=0,1,nil,nil,0
    local capture,failed=nil,false
    local function parity()
        if not verify_rendering then return end
        local screen=machine.screens[":screen"]
        for y=0,63 do for x=0,263 do
            assert(lcd.black(x,y)==((screen:pixel(x,y)&0xffffff)==0x25302a), 'RAM/render mismatch')
        end end
        print('RAM_RENDER_PARITY pixels=16896')
    end
    local function save_failure(message)
        failed=true
        print('FAIL Neo stage '..index..': '..message)
        lcd.save(failure_path,{lcd.frame()})
        local f=io.open(failure_path..'.txt','w')
        if f then f:write(message);f:close() end
        machine.video:snapshot() -- diagnostics only, never an assertion input
        machine:exit()
    end
    emu.register_frame_done(function()
        if failed then return end
        frame=frame+1
        local ok,err=pcall(function()
            local signature=lcd.refresh()
            stable=signature==previous and stable+1 or 1;previous=signature
            local item=actions[index]
            local function advance()
                print(string.format('READY stage=%d kind=%s frames=%d',index,item.kind,frame-started))
                index=index+1;started=nil
            end
            if capture then
                local age=frame-capture.tick
                assert(age<30,'firmware cursor call timeout')
                if capture.phase=='hide' and (cpu.state.PC.value==code+2 or cpu.state.PC.value==code+4) then
                    returned();capture.phase='observe';capture.tick=frame;capture.last=nil;capture.samples={}
                elseif capture.phase=='observe' then
                    if signature~=capture.last then capture.samples={} end
                    capture.last=signature;capture.samples[#capture.samples+1]=lcd.frame()
                    if #capture.samples>=3 then
                        local matched,why=matches(expectations[item.index]);item.why=why
                        capture.matched=matched
                        if matched then
                            lcd.save(string.format('%s/checkpoint-%02d.lcd.json',capture_directory,item.index),capture.samples)
                            parity();checkpoint_count=checkpoint_count+1
                            print('PASS LCD checkpoint '..item.index)
                        end
                        if capture.parked then
                            cursor_mode(saved_mode);capture.phase='restore';capture.tick=frame
                        else
                            capture=nil
                            if matched then advance() else item.retry_signature=signature end
                        end
                    end
                elseif capture.phase=='restore' and (cpu.state.PC.value==code+2 or cpu.state.PC.value==code+4) then
                    returned();assert(memory:read_u8(0x48be)==saved_mode,'cursor mode not restored')
                    resume_guest()
                    local matched=capture.matched;capture=nil
                    if matched then advance() else item.retry_signature=signature end
                end
                return
            end
            if events[frame] then events[frame]() end
            if frame<1100 then return end
            assert(item,'scenario ended without completion')
            if not started then
                started=frame;stable=0
                if item.kind=='key' then
                    item.key=named(item.name);item.mod=item.modifier and named(item.modifier)
                    if item.mod then item.mod:set_value(1) end;item.key:set_value(1)
                    if applet=='logo' then
                        item.next_draft=draft
                        if item.char then item.next_draft=item.char=='\r' and '' or draft..item.char
                        elseif item.name=='Backspace' then item.next_draft=draft:sub(1,-2)
                        elseif item.modifier=='Left Ctrl' and (item.name=='U' or item.name=='C') then item.next_draft='' end
                    end
                end
            end
            local age=frame-started
            assert(age<900,'readiness timeout: '..tostring(item.why or item.kind))
            if item.kind=='key' then
                if age==item.hold then item.key:clear_value();if item.mod then item.mod:clear_value() end end
                if age>=item.step and stable>=3 then
                    local ready=applet=='logo' and logo_input(item.next_draft) or (applet~='logo' and idle(item.busy))
                    if ready then if applet=='logo' then draft=item.next_draft end;advance() end
                end
            elseif item.kind=='probe' and stable>=3 and idle(false) then
                local matched,why=pcall(item.fn)
                if matched then advance() else item.why=tostring(why) end
            elseif item.kind=='check' and stable>=3 and idle(expectations[item.index].busy) then
                if item.retry_signature~=signature then
                    local parked=memory:read_u8(0x48be)~=0x0c
                    capture={phase=parked and 'hide' or 'observe',tick=frame,parked=parked,samples={}}
                    if parked then park_cursor() end
                end
            elseif item.kind=='finish' then
                assert(checkpoint_count==#expectations,'missing LCD assertions')
                print('PASS Neo scenario completed');machine:exit()
            end
        end)
        if not ok then save_failure(tostring(err)) end
    end)
end
