-- Fresh flash order: AlphaWord, UCBLogo, MicroPython EXTRA, NeoFontTerminal.
-- Exercises native keyboard events and the Neo font renderer's cursor.
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
local function field(row, mask)
    for _, f in pairs(machine.ioport.ports[":ROW" .. row].fields) do
        if f.mask == mask then return f end
    end
    error("missing Neo key")
end
local function named(name)
    for row = 0, 15 do
        for _, f in pairs(machine.ioport.ports[":ROW" .. row].fields) do
            if f.name:sub(1, #name + 2) == name .. " [" then return f end
        end
    end
    error(name)
end
local function tap(frame, key)
    events[frame] = function() key:set_value(1) end
    events[frame + 20] = function() key:clear_value() end
end
local function keys(frame, names)
    for i, name in ipairs(names) do tap(frame + (i - 1) * 40, named(name)) end
end
tap(150, field(15, 1))
tap(360, named("Enter"))
tap(740, field(6, 0x10))
tap(850, named("Down"))
tap(910, named("Down"))
tap(980, named("Enter"))
keys(1100, {"A", "Equals", "1", "2", "3", "4"})
local x, y
events[1360] = function()
    x, y = memory:read_u16(0x5c72), memory:read_u16(0x5c74)
    capture()
end
tap(1400, named("Backspace"))
events[1480] = function()
    assert(memory:read_u16(0x5c72) == x - 6, "Backspace did not move Neo text cursor left")
    assert(memory:read_u16(0x5c74) == y, "Backspace moved to a different row")
    capture()
end
tap(1520, named("Enter"))
keys(1600, {"A", "Enter"})
events[1800] = function()
    capture()
end
keys(1900, {"A"})
tap(2000, named("Backspace"))
events[2070] = function()
    assert(memory:read_u16(0x5c72) == 24, "deleting to empty reprinted the prompt")
    capture()
end
tap(2120, named("Backspace"))
events[2190] = function()
    assert(memory:read_u16(0x5c72) == 24, "Backspace on empty input changed the prompt")
    capture()
end
-- Delete a character from the middle, then evaluate the edited assignment.
keys(2250, {"A", "Equals", "1", "2", "3", "4"})
tap(2540, named("Left"))
tap(2620, named("Backspace"))
tap(2700, named("End"))
tap(2780, named("Enter"))
keys(2860, {"A", "Enter"})
events[3070] = function()
    capture()
end
keys(3100, {"A", "Equals", "9"})
events[3260] = function() named("Left Ctrl"):set_value(1) end
tap(3280, named("U"))
events[3320] = function() named("Left Ctrl"):clear_value() end
events[3380] = function()
    assert(memory:read_u16(0x5c72) == 24, "Ctrl+U did not clear the draft")
    capture()
end
keys(3450, {"A", "Enter"})
events[3650] = function()
    capture()
    print("PASS Neo cursor assertions; screen assertions pending offline reader")
end
-- Arithmetic and wrapping/scrolling smoke checks, formerly screenshot-only.
local function type_text(start, text)
    for i = 1, #text do
        local ch = text:sub(i, i)
        local shift = ch:match("%u") ~= nil
        local key = ch:upper()
        if ch == "*" then key, shift = "8", true end
        if ch == "(" then key, shift = "9", true end
        if ch == ")" then key, shift = "0", true end
        if ch == '"' then key, shift = "Quote", true end
        local f = start + (i-1)*40
        events[f] = function()
            if shift then named("Left Shift"):set_value(1) end
            named(key):set_value(1)
        end
        events[f+20] = function()
            named(key):clear_value()
            named("Left Shift"):clear_value()
        end
    end
end
type_text(3750, "6*7")
tap(3910, named("Enter"))
events[4050] = capture
type_text(4150, 'len("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ")')
events[6550] = capture -- wrapped input before evaluation
tap(6630, named("Enter"))
events[6800] = capture
-- Repeated results must remain visible as the display scrolls.
keys(6900, {"1", "Enter", "1", "Enter", "1", "Enter", "1", "Enter"})
events[7300] = capture
events[7301] = function() machine:exit() end
local frame = 0
emu.register_frame_done(function()
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
    frame = frame + 1
    if events[frame] then
        local ok, err = pcall(events[frame])
        if not ok then print("FAIL Neo readline Backspace: " .. err); capture(); machine:exit() end
    end
end)
