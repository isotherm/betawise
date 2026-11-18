-- Run in MAME with a fresh NEO flash seeded with AlphaWord and UCBLogo.
local machine = manager.machine
local memory = machine.devices[":maincpu"].spaces.program
local events = {}

local function find_key(row, mask)
    for _, field in pairs(machine.ioport.ports[":ROW" .. row].fields) do
        if field.mask == mask then return field end
    end
    error("Missing NEO keyboard field")
end

local function named_key(name)
    for row = 0, 15 do
        for _, field in pairs(machine.ioport.ports[":ROW" .. row].fields) do
            if field.name:sub(1, #name + 2) == name .. " [" then
                return field
            end
        end
    end
    error("Missing NEO key " .. name)
end

local function tap(frame, key)
    events[frame] = function() key:set_value(1) end
    events[frame + 20] = function() key:clear_value() end
end

local enter = named_key("Enter")
tap(150, find_key(15, 1))                    -- accept backup battery prompt
tap(360, enter)                             -- restart
tap(740, find_key(6, 0x10))                  -- SmartApplets
tap(850, find_key(5, 2))                     -- select UCBLogo
tap(950, enter)

local function type_keys(frame, keys)
    for index, name in ipairs(keys) do
        local at = frame + 40 * (index - 1)
        if name == "Colon" then
            local shift = named_key("Left Shift")
            events[at] = function() shift:set_value(1) end
            tap(at + 5, named_key("Semicolon"))
            events[at + 30] = function() shift:clear_value() end
        else
            tap(at, named_key(name))
        end
    end
end

type_keys(1100, {"R", "E", "P", "E", "A", "T", "Space", "4", "Space",
                 "Left bracket", "F", "D", "Space", "2", "0", "Space",
                 "R", "T", "Space", "9", "0", "Right bracket", "Enter"})
type_keys(2160, {"P", "R", "I", "N", "T", "Space", "4", "2", "Enter"})
type_keys(2610, {"C", "S", "Enter"})
type_keys(2780, {"C", "S", "Enter"})
type_keys(2940, {"H", "T", "Enter"})
type_keys(3100, {"R", "T", "Space", "9", "0", "Enter"})
type_keys(3380, {"S", "T", "Enter"})

type_keys(3600, {"C", "S", "Enter"})
type_keys(3740, {"T", "O", "Space", "Q", "Space", "Colon", "N", "Enter"})
type_keys(4100, {"R", "E", "P", "E", "A", "T", "Space", "4", "Space",
                 "Left bracket", "F", "D", "Space", "Colon", "N", "Space",
                 "R", "T", "Space", "9", "0", "Right bracket", "Enter"})
type_keys(5060, {"E", "N", "D", "Enter"})
type_keys(5260, {"Q", "Space", "2", "0", "Enter"})

type_keys(5520, {"C", "T", "Enter"})
type_keys(5680, {"C", "L", "E", "A", "R", "T", "E", "X", "T", "Enter"})

type_keys(6160, {"E", "R", "A", "S", "E", "Space", "Left bracket", "Q", "Right bracket", "Enter"})
type_keys(6600, {"Q", "Space", "2", "0", "Enter"})
type_keys(6840, {"T", "O", "Space", "Q", "Enter"})
type_keys(7080, {"F", "D", "Space", "3", "0", "Enter"})
type_keys(7360, {"E", "N", "D", "Enter"})
type_keys(7580, {"Q", "Enter"})

local snapshots = {
    [1050] = true, [2050] = true, [2510] = true, [2740] = true,
    [2910] = true, [3070] = true, [3350] = true, [3510] = true,
    [4070] = true, [5230] = true, [5460] = true,
    [5640] = true, [6080] = true,
    [6820] = true, [7700] = true,
}

local reported = 0
local function report_heap()
    for address = 0x4000, 0x80000 - 136, 2 do
        if memory:read_u32(address) == 0x4c4f474f then
            local capacity = memory:read_u32(address + 12)
            local node_size = memory:read_u32(address + 16)
            local count = memory:read_u32(address + 20)
            if capacity >= 16 * 1024 and capacity <= 64 * 1024
                and node_size <= 128 and count > 0 and count <= 4 then
                for index = reported, count - 1 do
                    local base = address + 24 + index * 28
                    local fields = {}
                    for i = 0, 6 do fields[i + 1] = memory:read_u32(base + i * 4) end
                    print(("Logo sample=%d heap=%d node=%d used=%d peak=%d rejected=%d "
                        .. "slots=%d occupied=%d free=%d retained=%d"):format(
                        index, capacity, node_size, table.unpack(fields)))
                end
                reported = count
                return
            end
        end
    end
    print("Logo heap statistics not found")
end

local frame = 0
emu.register_frame_done(function()
    frame = frame + 1
    if events[frame] then events[frame]() end
    if snapshots[frame] then
        machine.video:snapshot()
        if frame > 1050 then report_heap() end
    end
end)
