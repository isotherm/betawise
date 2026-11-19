-- OS3K facility exclusions and retained stream/filesystem placeholders.
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

statement('for name in ("select","asyncio",chr(95)+"asyncio",chr(95)+"thread","multiprocessing","subprocess","socket","ssl"):\r try:\r  exec("import "+name)\r except ImportError:\r  pass\r else:\r  raise AssertionError(name)\r if name=="ssl": print("ABSENT",1)\r', "ABSENT 1")
statement('import sys; assert sys.stdout.write("out")==3; assert sys.stderr.write("err")==3; assert sys.stdout.buffer.write(b"bin")==3; assert sys.stderr.buffer.write(b"bin")==3; print("STREAMS",1)', "STREAMS 1")
statement('try:\r input()\rexcept OSError as error:\r assert error.args[0]==95; print("INPUT",1)\r', "INPUT 1")
statement('try:\r sys.stdin.read(1)\rexcept OSError as error:\r assert error.args[0]==95; print("STDIN",1)\r', "STDIN 1")
statement('try:\r sys.stdin.buffer.read(1)\rexcept OSError as error:\r assert error.args[0]==95; print("STDIN",2)\r', "STDIN 2")
statement('import io,os; assert not hasattr(os,"system"); assert io.StringIO("ok").read()=="ok"; assert io.BytesIO(b"ok").read()==b"ok"; print("MEMORY",1)', "MEMORY 1")
statement('try:\r open("missing")\rexcept OSError as error:\r assert error.args[0]==95; print("FILES",1)\r', "FILES 1")
statement('try:\r io.open("missing")\rexcept OSError as error:\r assert error.args[0]==95; print("FILES",2)\r', "FILES 2")
at(frame, function() pass(); manager.machine:exit() end)
run()
