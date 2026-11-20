-- Calls existing firmware parser/handler in disposable emulated RAM.
-- No USB transport, programming, erase, or installation finalization is invoked.
local cpu = machine.devices[":maincpu"]
local mem = cpu.spaces.program
local code, stack, input, output, length = 0x38000, 0x3A000, 0x38100, 0x38200, 0x38300
local originalFloor, append, originalFree

local function call(command, argument)
    local packet = {command, (argument >> 24) & 255, (argument >> 16) & 255,
                    (argument >> 8) & 255, argument & 255, 0, 0}
    local sum = 0
    for i, b in ipairs(packet) do mem:write_u8(input+i-1,b); sum = (sum+b) & 255 end
    mem:write_u8(input+7,sum)
    mem:write_u32(stack,input)
    mem:write_u32(stack+4,8)
    mem:write_u32(stack+8,output)
    mem:write_u32(stack+12,length)
    mem:write_u16(code,0x4EB9)
    mem:write_u32(code+2,0x4C0C3A)
    mem:write_u16(code+6,0x60FE)
    cpu.state.SR.value = 0x2700
    cpu.state.SP.value = stack
    cpu.state.PC.value = code
end

local function reply()
    assert(cpu.state.PC.value == code+6 or cpu.state.PC.value == code+8,
           string.format("handler failed to return PC=%08X",cpu.state.PC.value))
    assert(mem:read_u32(length) == 8, "response length")
    return mem:read_u8(output), mem:read_u32(output+1), mem:read_u16(output+5)
end

at(530,function()
    expect_text("hello, world")
    originalFloor = mem:read_u32(0x354E)
    call(0x1A,0)
end)
at(532,function()
    local response, free, ram = reply()
    assert(response == 0x58)
    originalFree = free
    append = applet_end
    assert(free == expected_end - append)
    print(string.format("FREE flash=%08X ram_units=%04X ram_floor=%08X append=%08X",free,ram,originalFloor,append))
    -- A real RAM allocator boundary is not a flash allocator boundary.
    mem:write_u32(0x354E,originalFloor-0x10000)
    call(0x1A,0)
end)
at(534,function()
    local response, free, ram = reply()
    assert(response == 0x58 and free == originalFree, "RAM floor changed flash accounting")
    print(string.format("RAM_FLOOR_CHANGED flash=%08X ram_units=%04X",free,ram))
    mem:write_u32(0x354E,originalFloor)
    -- 512-KiB request fits stock flash but overlaps a 256-KiB archive tail.
    call(0x06,0x80000)
end)
at(800,function()
    local response, value = reply()
    assert(response == expected_response,string.format("begin reply=%02X expected=%02X",response,expected_response))
    local write = mem:read_u32(0x44E)
    if response == 0x46 then assert(write == append) else assert(write == 0) end
    print(string.format("BEGIN size=00080000 response=%02X value=%08X write_pointer=%08X",response,value,write))
    print("PASS " .. test_case)
end)
run()
