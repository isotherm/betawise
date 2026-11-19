-- Shared AS3000 automation. check.sh prepends the case name.
local machine = manager.machine
local events = {}

local function at(frame, action)
    assert(not events[frame], "duplicate frame " .. frame)
    events[frame] = action
end

local function text(frame, characters)
    at(frame, function() machine.natkeyboard:post(characters) end)
end

local function field(col, mask)
    for _, input in pairs(machine.ioport.ports[":COL." .. col].fields) do
        if input.mask == mask then return input end
    end
    error("missing AS3000 key")
end

local function tap(frame, key)
    at(frame, function() key:set_value(1) end)
    at(frame + 12, function() key:clear_value() end)
end

local function chord(frame, modifier, key)
    at(frame, function() modifier:set_value(1) end)
    tap(frame + 8, key)
    at(frame + 30, function() modifier:clear_value() end)
end

local function lcd()
    local rows = {}
    for _, tag in ipairs({":ks0066_0", ":ks0066_1"}) do
        local index = assert(machine.devices[tag].items["0/m_ddram"], "missing LCD DDRAM")
        local ddram = emu.item(index)
        -- AS3000 3.01 translates text through its ROM table at 0x4ea62c.
        -- These ASCII glyphs do not use their ASCII byte in controller DDRAM.
        local ascii = {[0]="g", [1]="y", [2]="p", [3]="q", [0xda]="\\", [0xd5]="~"}
        local function decode(bytes)
            return (bytes:gsub(".", function(c) return ascii[c:byte()] or c end))
        end
        rows[#rows + 1] = decode(ddram:read_block(0, 40))
        rows[#rows + 1] = decode(ddram:read_block(0x40, 40))
    end
    return table.concat(rows, "\n")
end

local function expect_text(expected)
    local screen = lcd()
    assert(screen:find(expected, 1, true), "missing LCD text " .. expected .. "\n" .. screen)
end

local function pass()
    machine.video:snapshot()
    print("PASS " .. test_case)
end

local function run()
    local frame = 0
    local failed = false
    emu.register_frame_done(function()
        frame = frame + 1
        if not failed and events[frame] then
            local ok, err = pcall(events[frame])
            if not ok then
                failed = true
                print("LUA ERROR: " .. err)
                machine.video:snapshot()
            end
        end
    end)
end

-- Fresh NVRAM invokes the backup battery recovery prompt. Main Return
-- selects an applet; keypad Enter is the Applets key on this driver.
text(60, "y")
text(200, "\r")
text(450, "\r")
