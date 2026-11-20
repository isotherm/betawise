-- Read-only ST7565 save items. Never sample guest data ports: reads advance them.
local lcd = {cache={}}
local states={}
local fields={'m_start_line','m_display_on','m_adc','m_com_reverse','m_reverse','m_all_points','m_reset'}
for i=0,1 do
    local device=assert(manager.machine.devices[":lcd"..i], "Neo LCD controller missing")
    local state={ram=emu.item((assert(device.items['0/m_ddram'], "LCD RAM missing")))}
    assert(state.ram.size==1 and state.ram.count==1188, "unsupported Neo LCD RAM geometry")
    for _,name in ipairs(fields) do
        state[name]=emu.item((assert(device.items['0/'..name], "LCD state missing: "..name)))
    end
    states[i+1]=state
end
function lcd.refresh()
    local parts={}
    for i,state in ipairs(states) do
        local c={ram=state.ram:read_block(0,1188)}
        local flags={}
        for _,name in ipairs(fields) do c[name]=state[name]:read(0);flags[#flags+1]=c[name] end
        lcd.cache[i]=c
        parts[#parts+1]=c.ram..string.char(table.unpack(flags))
    end
    return table.concat(parts)
end
function lcd.black(x,y)
    local c=lcd.cache[x//132+1];local col=x%132
    if c.m_display_on==0 or c.m_reset~=0 then return false end
    if c.m_all_points~=0 then return true end
    if c.m_adc~=0 then col=131-col end
    local scan=c.m_com_reverse~=0 and 63-y or y
    local line=(scan+c.m_start_line)&63
    return (((c.ram:byte((line//8)*132+col+1)>>(line&7))&1) ~= c.m_reverse)
end
function lcd.frame()
    return {lcd.cache[1],lcd.cache[2]}
end
function lcd.save(path,samples)
    local f=assert(io.open(path,'w'))
    f:write('{"format":"neo-lcd-v1","samples":[')
    for n,sample in ipairs(samples) do
        if n>1 then f:write(',') end
        f:write('{"controllers":[')
        for i,c in ipairs(sample) do
            if i>1 then f:write(',') end
            local hex=c.ram:gsub('.',function(v) return string.format('%02x',v:byte()) end)
            f:write('{"ram":"',hex,'"')
            for _,name in ipairs(fields) do f:write(',"',name,'":',tostring(c[name])) end
            f:write('}')
        end
        f:write(']}')
    end
    f:write(']}\n');f:close()
end
return lcd
