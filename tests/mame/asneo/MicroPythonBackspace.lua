-- Fresh flash order: AlphaWord, UCBLogo, MicroPython EXTRA, NeoFontTerminal.
-- Exercises native keyboard events and the Neo font renderer's cursor.
local machine = manager.machine
local memory = machine.devices[":maincpu"].spaces.program
local events = {}
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
    machine.video:snapshot()
end
tap(1400, named("Backspace"))
events[1480] = function()
    assert(memory:read_u16(0x5c72) == x - 6, "Backspace did not move Neo text cursor left")
    assert(memory:read_u16(0x5c74) == y, "Backspace moved to a different row")
    machine.video:snapshot()
end
tap(1520, named("Enter"))
keys(1600, {"A", "Enter"})
events[1800] = function()
    machine.video:snapshot()
end
keys(1900, {"A"})
tap(2000, named("Backspace"))
events[2070] = function()
    assert(memory:read_u16(0x5c72) == 24, "deleting to empty reprinted the prompt")
    machine.video:snapshot()
end
tap(2120, named("Backspace"))
events[2190] = function()
    assert(memory:read_u16(0x5c72) == 24, "Backspace on empty input changed the prompt")
    machine.video:snapshot()
end
-- Delete a character from the middle, then evaluate the edited assignment.
keys(2250, {"A", "Equals", "1", "2", "3", "4"})
tap(2540, named("Left"))
tap(2620, named("Backspace"))
tap(2700, named("End"))
tap(2780, named("Enter"))
keys(2860, {"A", "Enter"})
events[3070] = function()
    machine.video:snapshot()
end
keys(3100, {"A", "Equals", "9"})
events[3260] = function() named("Left Ctrl"):set_value(1) end
tap(3280, named("U"))
events[3320] = function() named("Left Ctrl"):clear_value() end
events[3380] = function()
    assert(memory:read_u16(0x5c72) == 24, "Ctrl+U did not clear the draft")
    machine.video:snapshot()
end
keys(3450, {"A", "Enter"})
events[3650] = function()
    machine.video:snapshot()
    print("PASS Neo readline Backspace, middle edit and generic Ctrl+U")
    machine:exit()
end
local frame = 0
emu.register_frame_done(function()
    frame = frame + 1
    if events[frame] then
        local ok, err = pcall(events[frame])
        if not ok then print("FAIL Neo readline Backspace: " .. err); machine.video:snapshot(); machine:exit() end
    end
end)
