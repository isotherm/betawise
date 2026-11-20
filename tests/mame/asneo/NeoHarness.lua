-- Native Neo keyboard and firmware cursor capture helpers. Neo 3.21 only.
local machine = manager.machine
local memory = machine.devices[":maincpu"].spaces.program
local events = {}
local capture_count, capture_tick = 0, nil
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
local function capture()
    capture_count = capture_count + 1
    saved_cpu, saved_ram = {}, {}
    assert((cpu.state.SR.value & 0x2000) ~= 0, "capture requires supervisor context")
    for _, name in ipairs(register_names) do saved_cpu[name] = cpu.state[name].value end
    for i=0,4095 do saved_ram[i] = memory:read_u8(scratch+i) end
    saved_mode = memory:read_u8(0x48be) -- Neo 3.21 cursor mode (test-only address)
    print(string.format("CAPTURE %d %d %d mode=%x", capture_count,
        memory:read_u16(0x5c72), memory:read_u16(0x5c74), saved_mode))
    cursor_mode(0x0c)
    capture_tick = 0
end

local function at(frame, fn)
    assert(not events[frame], "duplicate event at " .. frame)
    events[frame] = fn
end
local function named(name)
    for row=0,15 do
        for _, f in pairs(machine.ioport.ports[":ROW"..row].fields) do
            if f.name:sub(1, #name+2) == name.." [" then return f end
        end
    end
    error("missing Neo key: "..name)
end
local function press(frame, name, modifier, hold)
    local k, mod = named(name), modifier and named(modifier)
    at(frame, function() if mod then mod:set_value(1) end; k:set_value(1) end)
    at(frame+(hold or 6), function() k:clear_value(); if mod then mod:clear_value() end end)
end
local t = 1100
local function key(name, modifier, hold)
    press(t, name, modifier, hold)
    t = t+math.max(24, (hold or 6)+12)
end
local punctuation = {
    [' ']='Space', ['=']='Equals', ['-']='Minus', ['.']='Period', [',']='Comma',
    [';']='Semicolon', ["'"]='Quote', ['[']='Left bracket', [']']='Right bracket',
    ['/']='Slash', ['\\']='Backslash', ['`']='Grave', ['\r']='Enter', ['\t']='Tab'
}
local shifted = {['"']='Quote', [':']='Semicolon', ['(']='9', [')']='0',
    ['+']='Equals', ['_']='Minus', ['*']='8', ['<']='Comma', ['>']='Period',
    ['!']='1', ['@']='2', ['%']='5', ['{']='Left bracket', ['}']='Right bracket'}
local function type_text(text)
    for i=1,#text do
        local ch = text:sub(i,i)
        local mod = (ch:match('%u') or shifted[ch]) and 'Left Shift' or nil
        key(shifted[ch] or punctuation[ch] or ch:upper(), mod)
    end
end
local function wait(frames) t=t+frames end
local function checkpoint()
    wait(60)
    at(t, capture)
    t=t+24
end
local function boot(applet)
    press(150,'Y'); press(360,'Enter'); press(740,'Applets')
    press(850,'Down')
    if applet == 'micropython' then press(910,'Down') end
    press(980,'Enter')
end
local function finish()
    at(t+30, function() print('PASS Neo scenario completed'); machine:exit() end)
end
local function run()
    local frame, failed = 0, false
    emu.register_frame_done(function()
        if failed then return end
        local ok, err = pcall(function()
            if capture_tick then
                capture_tick = capture_tick + 1
                -- Two frames allow the firmware call and LCD redraw to complete (~33 ms).
                if capture_tick == 2 then returned() end
                if capture_tick == 3 or capture_tick == 4 then
                    local path = string.format("checkpoint-%02d-%03d.png", capture_count, capture_tick)
                    local err = machine.screens[":screen"]:snapshot(path)
                    assert(not err, tostring(err))
                end
                if capture_tick == 5 then cursor_mode(saved_mode) end
                if capture_tick == 7 then
                    returned()
                    assert(memory:read_u8(0x48be) == saved_mode, "cursor mode not restored")
                    for i=0,4095 do memory:write_u8(scratch+i, saved_ram[i]) end
                    -- Restore the interrupted guest exactly, after restoring the real cursor.
                    for _, name in ipairs(register_names) do cpu.state[name].value = saved_cpu[name] end
                    capture_tick = nil
                end
                return
            end
            frame=frame+1
            if events[frame] then events[frame]() end
        end)
        if not ok then failed=true; print('FAIL Neo scenario: '..tostring(err)); machine:exit() end
    end)
end
