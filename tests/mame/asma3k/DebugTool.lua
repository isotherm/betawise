local cpu = machine.devices[":maincpu"]
local memory = cpu.spaces.program
local ram_base, scratch, address_slot, cursor_slot

local ctrl = field(12, 0x80)
local shift = field(14, 0x01)
local key_g = field(13, 0x02)
local key_i = field(0, 0x08)
local backspace = field(9, 0x01)
local equals = field(0, 0x10)
local enter = field(9, 0x40)

local function expect_address(address)
    local actual = memory:read_u32(address_slot) + memory:read_u16(cursor_slot)
    assert(actual == address, string.format("cursor at %08x, expected %08x", actual, address))
end

local function scratch_jump(frame, offset)
    chord(frame, ctrl, key_g)
    text(frame + 50, offset)
    -- The MAME natural keyboard table does not map '+', so press Shift+=.
    chord(frame + 140, shift, equals)
    text(frame + 200, "\r")
end

local function install_routine(code)
    for index, byte in ipairs(code) do
        memory:write_u8(scratch + 0x40 + index - 1, byte)
    end
end

local function invoke_routine(code)
    at(535, function() install_routine(code) end)
    chord(550, ctrl, key_i)
    text(680, "\r")
    tap(750, backspace)
    at(800, function() machine.natkeyboard:post(string.format("0x%x\r", scratch + 0x40)) end)
end

at(530, function()
    expect_text("0000000 ")
    ram_base = cpu.state.A5.value
    assert(ram_base ~= 0 and ram_base < 0x40000, "unexpected applet A5")
    -- Current OS3K ELF files tag RAM symbols with bit 31.
    scratch = ram_base + (assert(symbols.g_scratch) & 0x7fffffff)
    address_slot = ram_base + (assert(symbols.g_pAddress) & 0x7fffffff)
    cursor_slot = ram_base + (assert(symbols.g_cursor) & 0x7fffffff)
end)

if test_case == "memory" then
    chord(550, ctrl, key_g)
    text(600, "400\r")
    at(790, function() expect_address(0x400); expect_text("0000400 ") end)
    scratch_jump(850, "20")
    at(1120, function() expect_address(scratch + 0x20) end)
    text(1150, "a5")
    text(1270, "\tHi")
    at(1410, function()
        assert(memory:read_u8(scratch + 0x20) == 0xa5)
        assert(memory:read_u8(scratch + 0x21) == string.byte("H"))
        assert(memory:read_u8(scratch + 0x22) == string.byte("i"))
        -- The applet RAM base need not align scratch to an eight-byte dump
        -- row. Join just the hex fields so this also checks a split sequence.
        local hex = ""
        for line in lcd():gmatch("[^\n]+") do hex = hex .. line:sub(9, 32) end
        assert(hex:find("A5 48 69", 1, true), "missing edited hex bytes\n" .. lcd())
    end)
    scratch_jump(1470, "20")
    text(1740, "\t")
    at(1790, function()
        machine.natkeyboard:post(string.format("%08x", scratch + 0x30))
    end)
    at(1990, function() assert(memory:read_u32(scratch + 0x20) == scratch + 0x30) end)
    scratch_jump(2030, "20")
    at(2300, function() expect_address(scratch + 0x20) end)
    at(2340, function() shift:set_value(1) end)
    chord(2350, ctrl, key_g)
    at(2390, function() shift:clear_value() end)
    at(2480, function() expect_address(scratch + 0x30) end)
    tap(2510, backspace)
    at(2600, function() expect_address(scratch + 0x20) end)
    tap(2630, field(4, 0x20)) -- Clear File
    at(2720, function()
        for offset = 0, 255 do assert(memory:read_u8(scratch + offset) == 0) end
        pass()
    end)
elseif test_case == "invoke" then
    -- Pack the six arguments into nibbles so an argument-order error is visible.
    local code = {0x20, 0x2f, 0, 4} -- move.l 4(sp),d0
    for arg = 2, 6 do
        for _, byte in ipairs({0xe9, 0x88, 0x80, 0xaf, 0, arg * 4}) do
            code[#code + 1] = byte
        end
    end
    code[#code + 1] = 0x4e
    code[#code + 1] = 0x75
    invoke_routine(code)
    for arg = 1, 6 do
        local frame = 1130 + (arg - 1) * 285
        text(frame, tostring(arg))
        text(frame + 65, "\r")
        text(frame + 130, tostring(arg) .. "\r")
    end
    text(2920, "c")
    text(2980, "\r")
    at(3100, function() expect_text("Return value = 0x00123456"); pass() end)
elseif test_case == "syscall" then
    chord(550, ctrl, key_i)
    text(680, "\r")
    text(760, "0\r") -- The function editor starts with '!', so this is !0.
    text(935, "c")
    text(1080, "\r")
    at(1190, function() expect_text("Return value = 0x00000001"); pass() end)
elseif test_case == "string" then
    -- Return the first byte of the C string at the first argument.
    invoke_routine({0x20, 0x6f, 0, 4, 0x70, 0, 0x10, 0x10, 0x4e, 0x75})
    text(1130, "1")
    text(1195, "\r")
    tap(1260, backspace)
    tap(1310, backspace)
    text(1380, "$ABC\r")
    text(1620, "c")
    chord(1680, shift, enter)
    at(1810, function()
        expect_text(" to call *0x")
        assert(not lcd():find("Return value", 1, true))
    end)
    text(1850, " ")
    at(1950, function() expect_text("Return value = 0x00000041"); pass() end)
elseif test_case == "handler" then
    -- With fresh flash, the cartridge is the first applet at 0x408000.
    local handler = 0x408000 + assert(symbols.BusErrorHandler) - assert(symbols.__header)
    -- Build the 68000 bus-error stack frame and jump to the actual handler.
    local code = string.char(
        0x41, 0xfa, 0x00, 0x10, -- lea resume(pc),a0
        0x2f, 0x08,             -- move.l a0,-(sp)
        0x40, 0xe7,             -- move.w sr,-(sp)
        0x42, 0xa7, 0x42, 0xa7, -- clr.l -(sp) twice
        0x4e, 0xf9              -- jmp handler
    ) .. string.pack(">I", handler) .. string.char(
        0x20, 0x3c, 0x24, 0x68, 0xac, 0xe0, -- resume: move.l #$2468ace0,d0
        0x4e, 0x75                         -- rts
    )
    invoke_routine({code:byte(1, #code)})
    text(1160, "c")
    text(1330, "\r")
    at(1450, function()
        expect_text("Return value = 0x2468ACE0")
        local flag = ram_base + (assert(symbols.g_busError) & 0x7fffffff)
        assert(memory:read_u8(flag) == 1, "handler did not set the bus-error flag")
        pass()
    end)
end

run()
