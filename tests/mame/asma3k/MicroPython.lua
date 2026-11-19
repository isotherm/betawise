local function check(frame, expected)
    at(frame, function()
        local ok, err = pcall(expect_text, expected)
        if not ok then
            manager.machine:exit()
            error(err)
        end
    end)
end

check(530, ">>> ")
text(540, "print(6*7)\r")
check(655, "42")
text(670, "x=93\r")
text(770, "x//2\r")
check(875, "46")
text(890, "x%2\r")
check(990, "\n1 ")
text(1000, "7-9")
tap(1060, field(9, 1))
text(1090, "5\r")
check(1170, "\n2 ")
text(1190, "1//0\r")
check(1290, "ZeroDivisionError")
text(1300, "if x:\r")
check(1390, "... ")
text(1400, " print(1234)\r\r")
check(1580, "1234")
text(1590, "for i in range(900):\r")
text(1800, " t=list(range(12))\r\r")
text(2240, "x-7\r")
check(2360, "\n86 ")
text(2380, "i\r")
check(2490, "\n899 ")
-- Keep the closing quote and parenthesis beyond the old input limit.
text(2500, "len(\"" .. string.rep("a", 55) .. "\")")
check(3260, "\naaaa")
text(3280, "\r")
check(3500, "\n55 ")
text(3520, "f=1.25\r")
text(3660, "4.0-f*2\r")
check(3820, "\n1.5 ")
text(3840, "divmod(7.5,2.0)\r")
check(4050, "(3.0, 1.5)")
text(4070, "6.25**0.5\r")
check(4230, "\n2.5 ")
text(4250, "round(2.5),round(3.5)\r")
check(4520, "(2, 4)")
text(4540, "1.25e-2\r")
check(4690, "\n0.0125 ")
-- Exercise each optional module through the firmware-driven REPL. A marker
-- only appears after its assertions succeed (and differs from the typed text).
local frame = 4710
local function module_check(command, expected)
    -- MAME declares underscore on both Equals and Minus, but the firmware
    -- produces it only on Shift+Minus. Type that key explicitly.
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
    frame = frame + 102
    check(frame, expected)
    frame = frame + 20
end

module_check('import gc; gc.collect(); assert gc.mem_free()>0; assert gc.mem_alloc()>0; print("MODULE",1)', "MODULE 1")
module_check('import micropython; assert micropython.const(42)==42; micropython.opt_level(1); assert micropython.opt_level()==1; micropython.opt_level(0); print("MODULE",2)', "MODULE 2")
module_check('import sys; assert sys.platform=="os3k"; assert sys.implementation[0]=="micropython"; assert sys.byteorder=="big"; assert isinstance(sys.modules,dict); print("MODULE",3)', "MODULE 3")
module_check('import array; a=array.array("h",[1,-2,300]); a.append(7); a[0]=42; assert list(a)==[42,-2,300,7]; print("MODULE",4)', "MODULE 4")
module_check('import struct; b=struct.pack(">hH",-2,300); assert b==b"\\xff\\xfe\\x01,"; assert struct.unpack(">hH",b)==(-2,300); assert struct.calcsize(">hH")==4; print("MODULE",5)', "MODULE 5")
module_check('import collections; P=collections.namedtuple("Point",["x","y"]); p=P(3,4); assert p.x==3; assert p.y==4; assert p[1]==4; assert tuple(p)==(3,4); print("MODULE",6)', "MODULE 6")
-- Imported modules, array storage, and dynamically created namedtuple types
-- must remain usable after allocation pressure and an explicit collection.
text(frame, "for i in range(900):\r")
frame = frame + 280
text(frame, " t=list(range(12))\r\r")
frame = frame + 500
module_check('gc.collect(); assert a[2]==300; assert p.x==3; assert struct.unpack(">h",struct.pack(">h",-7))[0]==-7; print("MODULE",7)', "MODULE 7")
module_check('d=collections.deque((),3); d.append(1); d.appendleft(2); assert list(d)==[2,1]; assert d[0]==2; assert d.pop()==1; assert d.popleft()==2; print("EXTRA",1)', "EXTRA 1")
module_check('o=collections.OrderedDict(); o["b"]=2; o["a"]=1; o["b"]=3; assert list(o)==["b","a"]; assert o["b"]==3; print("EXTRA",2)', "EXTRA 2")
module_check('micropython.mem_info(); micropython.qstr_info(); assert micropython.stack_use()>0; print("EXTRA",3)', "EXTRA 3")
module_check('buf=bytearray(b"abcd"); view=memoryview(buf); view[1]=98; view[2:4]=b"XY"; assert buf==b"abXY"; assert bytes(view[1:3])==b"bX"; print("EXTRA",4)', "EXTRA 4")
module_check('struct.pack_into(">h",buf,1,-7); assert struct.unpack_from(">h",view,1)==(-7,); print("EXTRA",5)', "EXTRA 5")
module_check('import json; obj={"a":[1,True,None],"b":1.25}; encoded=json.dumps(obj); assert json.loads(encoded)==obj; print("EXTRA",6)', "EXTRA 6")
module_check('try:\r json.loads("{")\rexcept ValueError:\r print("INVALID",1)\r', "INVALID 1")
module_check('import binascii; assert binascii.hexlify(b"abc")==b"616263"; assert binascii.unhexlify(b"616263")==b"abc"; print("EXTRA",7)', "EXTRA 7")
module_check('assert binascii.b2a_base64(b"hi")==b"aGk=\\n"; assert binascii.a2b_base64(b"aGk=")==b"hi"; print("EXTRA",8)', "EXTRA 8")
module_check('import re; pattern=re.compile("([a-z]*)([0-9]*)"); match=pattern.match("abc123"); assert match.groups()==("abc","123"); assert match.span(1)==(0,3); print("EXTRA",9)', "EXTRA 9")
module_check('assert re.search("[0-9]*$","abc123").group(0)=="123"; assert re.sub("[0-9]","x","a1b2")=="axbx"; assert re.compile(",").split("a,b")==["a","b"]; print("EXTRA",10)', "EXTRA 10")
text(frame, "for i in range(900):\r")
frame = frame + 280
text(frame, " t=list(range(12))\r\r")
frame = frame + 500
module_check('gc.collect(); assert list(o)==["b","a"]; assert bytes(view)==bytes(buf); assert json.loads(encoded)==obj; assert pattern.match("abc123").group(1)=="abc"; print("EXTRA",11)', "EXTRA 11")
module_check('import io; stream=io.StringIO(); json.dump(obj,stream); stream.seek(0); assert json.load(stream)==obj; print("STREAM",1)', "STREAM 1")
module_check('try:\r open("test")\rexcept OSError as error:\r assert error.args[0]==95; print("FILES",1)\r', "FILES 1")
at(frame, function()
    pass()
    manager.machine:exit()
end)

run()
