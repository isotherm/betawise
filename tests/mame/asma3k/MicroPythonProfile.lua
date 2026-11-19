-- Standard upstream ROM profiles, using real applet startup and keyboard input.
local frame = 530
local function statement(command, expected)
    -- MAME's automatic underscore mapping selects Equals, not Shift+Minus.
    local start = 1
    while true do
        local pos = command:find("_", start, true)
        local chunk = command:sub(start, pos and pos - 1 or #command)
        if #chunk > 0 then
            text(frame, chunk)
            frame = frame + #chunk * 12 + 20
        end
        if not pos then break end
        chord(frame, field(14, 1), field(3, 0x10))
        frame = frame + 50
        start = pos + 1
    end
    text(frame, "\r")
    frame = frame + 112
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
    if variant == "basic" or variant == "extra" then
        statement('assert len(b"\\xc3\\xa9".decode())==1; assert next(iter(()),17)==17; print("BASIC",1)', "BASIC 1")
    end
end
if variant == "extra" then
    statement('d=collections.deque((),3); d.append(1); d.appendleft(2); assert list(d)==[2,1]; assert d.pop()==1; print("EXTRA",1)', "EXTRA 1")
    statement('o=collections.OrderedDict(); o["b"]=2; o["a"]=1; assert list(o)==["b","a"]; print("EXTRA",2)', "EXTRA 2")
    statement('buf=bytearray(b"abcd"); view=memoryview(buf); view[1]=98; view[2:4]=b"XY"; assert buf==b"abXY"; print("EXTRA",3)', "EXTRA 3")
    statement('import json,io; data={"a":[1,True,None],"b":1.25}; encoded=json.dumps(data); assert json.loads(encoded)==data; print("EXTRA",4)', "EXTRA 4")
    statement('stream=io.StringIO(); json.dump(data,stream); stream.seek(0); assert json.load(stream)==data; print("EXTRA",5)', "EXTRA 5")
    statement('import binascii; assert binascii.hexlify(b"abc")==b"616263"; assert binascii.a2b_base64(b"aGk=")==b"hi"; print("EXTRA",6)', "EXTRA 6")
    statement('assert binascii.crc32(b"c")==112844655; print("EXTRA",7)', "EXTRA 7")
    statement('import re; pattern=re.compile("([a-z]*)([0-9]*)"); assert pattern.match("abc123").group(1)=="abc"; assert re.sub("[0-9]","x","a1b2")=="axbx"; print("EXTRA",8)', "EXTRA 8")
    statement('import heapq; heap=[3,1,2]; heapq.heapify(heap); assert heapq.heappop(heap)==1; print("EXTRA",9)', "EXTRA 9")
    statement('import hashlib; assert hashlib.sha256(b"abc").digest().hex()=="ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad"; print("EXTRA",10)', "EXTRA 10")
    statement('import random; random.seed(123); value=random.randint(0,9); assert 0<=value<=9; print("EXTRA",11)', "EXTRA 11")
    statement('import micropython; events=[]; micropython.schedule(events.append,7); micropython.schedule(events.append,8); print("EXTRA",12)', "EXTRA 12")
    statement('assert events==[7,8]; micropython.mem_info(); assert micropython.stack_use()>0; print("EXTRA",13)', "EXTRA 13")
    statement('ring=micropython.RingIO(8); assert ring.write(b"abc")==3; assert ring.read(3)==b"abc"; print("EXTRA",14)', "EXTRA 14")
    statement('import framebuf; pixels=bytearray(8); fb=framebuf.FrameBuffer(pixels,8,8,framebuf.MONO_HLSB); fb.pixel(1,2,1); assert fb.pixel(1,2)==1; print("EXTRA",15)', "EXTRA 15")
    statement('import deflate; packed=binascii.unhexlify("789ccb48cdc9c90700062c0215"); assert deflate.DeflateIO(io.BytesIO(packed),deflate.AUTO).read()==b"hello"; print("EXTRA",16)', "EXTRA 16")
    statement('import sys; assert sys.stdout.write("out")==3; assert sys.stderr.write("err")==3; assert sys.stdout.buffer.write(b"bin")==3; assert sys.stderr.buffer.write(b"bin")==3; print("EXTRA",17)', "EXTRA 17")
    statement('import os; assert not hasattr(os,"system"); print("OS",1)', "OS 1")
    statement('for name in ("select","asyncio",chr(95)+"asyncio",chr(95)+"thread","multiprocessing","subprocess","socket","ssl"):\r try:\r  exec("import "+name)\r except ImportError:\r  pass\r else:\r  raise AssertionError(name)\r if name=="ssl": print("DISABLED",1)\r', "DISABLED 1")
    statement('assert abs(math.log2(8)-3)<1e-5; assert math.erf(0)==0; assert math.factorial(5)==120; print("EXTRA",20)', "EXTRA 20")
    statement('try:\r input()\rexcept OSError as error:\r assert error.args[0]==95; print("STUB",1)\r', "STUB 1")
    statement('try:\r sys.stdin.read(1)\rexcept OSError as error:\r assert error.args[0]==95; print("STUB",2)\r', "STUB 2")
    statement('try:\r sys.stdin.buffer.read(1)\rexcept OSError as error:\r assert error.args[0]==95; print("STUB",3)\r', "STUB 3")
    statement('micropython.kbd_intr(-1); micropython.kbd_intr(3); print("KBD",1)', "KBD 1")
    statement('try:\r import uctypes\rexcept ImportError:\r print("STUB",5)\r', "STUB 5")
    statement('try:\r import time\rexcept ImportError:\r print("STUB",6)\r', "STUB 6")
end
text(frame, "for i in range(900):\r")
frame = frame + 280
text(frame, " t=list(range(12))\r\r")
frame = frame + 500
statement('assert x==93; assert i==899; print("STRESS",1)', "STRESS 1")
if variant == "extra" then
    statement('gc.collect(); assert list(o)==["b","a"]; assert bytes(view)==bytes(buf); assert json.loads(encoded)==data; print("EXTRA",21)', "EXTRA 21")
end
-- Tab indentation, draft cancellation, cooperative interrupt and soft reset.
text(frame, "if True:\r")
frame = frame + 200
tap(frame, field(12, 1)) -- Tab
frame = frame + 40
text(frame, 'print("TAB",1)\r\r')
frame = frame + 400
at(frame, function() expect_text("TAB 1") end)
frame = frame + 20
text(frame, "discard this")
frame = frame + 200
chord(frame, field(12, 0x80), field(10, 0x40)) -- Ctrl+C
frame = frame + 80
statement('assert x==93; print("CANCEL",1)', "CANCEL 1")
local loop = 'try:\r while True:\r  pass\rexcept KeyboardInterrupt:\r print("BREAK",1)\r\r'
text(frame, loop)
frame = frame + #loop * 12 + 200
chord(frame, field(12, 0x80), field(10, 0x40))
frame = frame + 150
at(frame, function() expect_text("BREAK 1") end)
frame = frame + 20
statement('assert x==93; print("INTERRUPT",1)', "INTERRUPT 1")
chord(frame, field(12, 0x80), field(10, 0x04)) -- Ctrl+D
frame = frame + 120
at(frame, function() expect_text("MicroPython") end)
frame = frame + 20
statement('try:\r x\rexcept NameError:\r print("RESET",1)\r', "RESET 1")
at(frame, function()
    pass()
    manager.machine:exit()
end)
run()
