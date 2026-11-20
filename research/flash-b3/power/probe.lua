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
    -- Mode 5 is installed by the wired UART updater callback setup.
    mem:write_u8(0x420,5)
    -- Freeze a controlled cached measurement; this tests the actual Manager
    -- comparison, not the analogue GPIO-to-voltage calibration.
    mem:write_u8(0x43E3,0)
    mem:write_u32(0x43F0,889) -- 2322281/889 + 887 = 3499 mV
    call(0x06,0x400)
end)
at(550,function()
    local response = reply()
    assert(response == 0x8D,string.format("low battery reply %02X",response))
    assert(mem:read_u32(0x44E) == 0)
    print("PASS serial 3499mV rejects applet begin, response=8D")
    mem:write_u32(0x43F0,888) -- 3502 mV
    call(0x06,0x400)
end)
at(580,function()
    local response = reply()
    assert(response == 0x46,string.format("good battery reply %02X",response))
    assert(mem:read_u32(0x44E) > 0x408000)
    print("PASS serial 3502mV accepts applet begin, response=46")
    print("PASS battery-powered admission gate")
end)
run()
