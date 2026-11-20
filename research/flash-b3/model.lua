-- Prepended by run.py with the shared AS3000 keyboard harness and ELF symbols.
-- A scoped CUI fixture, not a physical flash or board-wiring qualification.
local cpu = machine.devices[":maincpu"]
local mem = cpu.spaces.program
local armed = false
local mode, pending, pendingAddress = "array", nil, nil
local bytes, busy, ready = {}, 0, 0x80
local programCount, eraseCount, commands = 0, 0, 0
local ram, pool, savedCsa, savedSr
local taps = {}

local function byte_read(address, original)
    if mode == "id" then return (address & 1) == 0 and 0x89 or device_id end
    if mode == "status" then
        if busy > 0 then busy = busy - 1; return 0 end
        return ready
    end
    return bytes[address] or original
end

local function byte_write(address, value)
    commands = commands + 1
    assert(cpu.state.PC.value < 0x40000, "flash command executed outside RAM")
    assert((cpu.state.SR.value & 0x700) == 0x700, "interrupts not masked")
    assert((mem:read_u16(0xfffff110) & 0x8000) == 0, "CSA still read-only")
    if pending == "program" then
        assert(address == pendingAddress and address >= pool and address < pool + 0x10000)
        programCount = programCount + 1
        if address ~= pool + 0x402 then bytes[address] = (bytes[address] or 0xFF) & value end
        ready = address == pool + 0x400 and 0x98 or 0x80
        busy = address == pool + 0x401 and 5 or 2
        mode, pending = "status", nil
    elseif pending == "erase" then
        assert(value == 0xD0 and address == pendingAddress and address == pool)
        eraseCount = eraseCount + 1
        for a = pool, pool + 0xFFFF do bytes[a] = 0xFF end
        ready, busy, mode, pending = 0x80, 2, "status", nil
    elseif value == 0x40 then pending, pendingAddress = "program", address
    elseif value == 0x20 then pending, pendingAddress = "erase", address
    elseif value == 0x90 then mode = "id"
    elseif value == 0x70 then mode = "status"
    elseif value == 0x50 then ready = 0x80
    elseif value == 0xFF then
        assert(busy == 0, "Read Array issued before ready")
        mode = "array"
    else error(string.format("unknown CUI byte %02X", value)) end
end

at(530, function()
    ram = cpu.state.A5.value
    assert(ram > 0x400 and ram + symbols.g_probeArm % 0x80000000 < 0x40000)
    local function address(name) return ram + (symbols[name] & 0x7FFFFFFF) end
    local worker = 0x408000 + symbols._FlashWorker
    assert(worker >= 0x408000 and worker < 0x4C0000 and (worker & 1) == 0,
           "worker source blob not in aligned ROM")
    -- The probe-only driver authorizes this virtual fixture region. It is
    -- not allocated inside the applet image and does not exist on hardware.
    pool = 0x410000
    -- This MAME build's EZ328 csa_w replaces the whole word on byte writes.
    -- Boot's byte access leaves 8080 rather than the board's 8007 setup.
    -- Normalize only in the explicit hardware fixture; stock MAME must refuse.
    if emulate then mem:write_u16(0xfffff110, 0x8007) end
    savedCsa, savedSr = mem:read_u16(0xfffff110), cpu.state.SR.value
    if emulate then
        taps[1] = mem:install_read_tap(0x400000, 0x4FFFFF, "B3_reads", function(offset, data, mask)
            if mode ~= "array" then assert(cpu.state.PC.value < 0x40000, "flash fetch while CUI non-array") end
            local high, low = (data >> 8) & 255, data & 255
            if (mask & 0xFF00) ~= 0 then high = byte_read(offset, high) end
            if (mask & 0xFF) ~= 0 then low = byte_read(offset + 1, low) end
            return (high << 8) | low
        end)
        taps[2] = mem:install_write_tap(0x400000, 0x4FFFFF, "B3_writes", function(offset, data, mask)
            if (mask & 0xFF00) ~= 0 then byte_write(offset, (data >> 8) & 255) end
            if (mask & 0xFF) ~= 0 then byte_write(offset + 1, data & 255) end
        end)
    end
    mem:write_u32(address("g_probeArm"), 0xB3A50001)
    armed = true
end)
text(550, "p")
at(750, function()
    assert(armed)
    local function address(name) return ram + (symbols[name] & 0x7FFFFFFF) end
    assert(mem:read_u32(address("g_probeDone")) == 1, "probe did not return")
    local expected = {0,0,257,0,-3,-2,-2,-2,0,65536,0,-4,0x98,-5,1,-6,0,0xE000,0x2000,0,0xF0000,0x2000}
    if not emulate or device_id ~= 0xD2 and device_id ~= 0xD3 then expected = {-1} end
    for i, value in ipairs(expected) do
        local actual = mem:read_u32(address("g_results") + (i-1)*4)
        assert(actual == (value & 0xFFFFFFFF), string.format("result[%d]=%08X expected %08X",i-1,actual,value & 0xFFFFFFFF))
    end
    assert(mem:read_u16(0xfffff110) == savedCsa, "CSA not restored")
    assert((cpu.state.SR.value & 0x700) == (savedSr & 0x700), "interrupt mask not restored")
    assert(mode == "array" and busy == 0, "chip not back in array mode")
    if emulate and (device_id == 0xD2 or device_id == 0xD3) then
        assert(programCount == 260 and eraseCount == 1, "unexpected number of programming/erase operations")
    else assert(programCount == 0 and eraseCount == 0, "unsupported chip was modified") end
    print("PASS " .. test_case .. " commands=" .. commands)
    machine.video:snapshot()
end)
run()
