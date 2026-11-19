-- Standard upstream ROM profiles, using real applet startup and keyboard input.
local frame = 530
local function statement(command, expected)
    text(frame, command .. "\r")
    frame = frame + (#command + 1) * 12 + 100
    at(frame, function()
        local ok, err = pcall(expect_text, expected)
        if not ok then
            manager.machine:exit()
            error(err)
        end
    end)
    frame = frame + 20
end

statement('assert 6*7==42; assert 4.0-1.25*2==1.5; print("COMMON",1)', "COMMON 1")
statement('x=93; assert x//2==46; print("COMMON",2)', "COMMON 2")
if variant == "minimal" then
    statement('try:\r import gc\rexcept ImportError:\r print("MINIMAL",1)\r', "MINIMAL 1")
    statement('try:\r bytearray()\rexcept NameError:\r print("MINIMAL",2)\r', "MINIMAL 2")
else
    statement('import gc,array,collections,math; gc.collect(); assert math.sin(0.0)==0.0; assert math.sqrt(6.25)==2.5; print("CORE",1)', "CORE 1")
    statement('a=array.array("h",[1,-2,300]); assert list(a)==[1,-2,300]; assert bytearray(b"abc")[1]==98; assert len({1,1,2})==2; print("CORE",2)', "CORE 2")
    statement('P=collections.namedtuple("Point",["x","y"]); p=P(3,4); assert p.x==3; assert p.y==4; print("CORE",3)', "CORE 3")
    statement('try:\r open("missing")\rexcept OSError:\r print("FILES",1)\r', "FILES 1")
    statement('try:\r import missing\rexcept ImportError:\r print("FILES",2)\r', "FILES 2")
    if variant == "basic" then
        statement('assert len(b"\\xc3\\xa9".decode())==1; assert next(iter(()),17)==17; print("BASIC",1)', "BASIC 1")
    end
end
text(frame, "for i in range(900):\r")
frame = frame + 280
text(frame, " t=list(range(12))\r\r")
frame = frame + 500
statement('assert x==93; assert i==899; print("STRESS",1)', "STRESS 1")
at(frame, function()
    pass()
    manager.machine:exit()
end)
run()
