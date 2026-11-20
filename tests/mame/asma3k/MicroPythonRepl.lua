-- Focused readline integration checks, shared by every ROM profile.
local frame = 530
local function check(expected)
    local ok, err = pcall(expect_text, expected)
    if not ok then manager.machine:exit(); error(err) end
end
local function enter(command, expected)
    text(frame, command .. "\r")
    frame = frame + #command * 12 + 180
    if expected then at(frame, function() check(expected) end) end
    frame = frame + 20
end
local function key(input)
    tap(frame, input)
    frame = frame + 40
end
local function expect(expected)
    at(frame, function() check(expected) end)
    frame = frame + 20
end
if variant == "extra" then
    enter('import micropython; events=[]; micropython.schedule(events.append,7); micropython.schedule(events.append,8)')
    enter('assert events==[7,8]; print("SCHEDULER",1)', 'SCHEDULER 1')
end

text(frame, 'print("BACKSPACE",123)X')
frame = frame + 360
key(field(9, 0x01)) -- Backspace: remove the stray X.
enter('', 'BACKSPACE 123')
text(frame, 'x')
frame = frame + 80
key(field(9, 0x01))
key(field(9, 0x01))
at(frame, function()
    local screen = lcd()
    assert(not screen:find('>>> >>>', 1, true), 'Backspace reprinted the prompt')
end)
frame = frame + 20
text(frame, 'x')
frame = frame + 80
key(field(4, 0x08)) -- Home, then Ctrl+D deletes the last remaining character.
chord(frame, field(12, 0x80), field(10, 0x04))
frame = frame + 100
at(frame, function() assert(not lcd():find('>>> >>>', 1, true), 'delete-to-empty reprinted the prompt') end)
frame = frame + 20
enter('print("DELETE",1)', 'DELETE 1')
text(frame, 'discard this draft')
frame = frame + 300
chord(frame, field(12, 0x80), field(15, 0x08)) -- Ctrl+U clears to beginning.
frame = frame + 100
enter('print("CTRLU",1)', 'CTRLU 1')
text(frame, 'pint("CTRLF",1)')
frame = frame + 250
key(field(4, 0x08)) -- Home, then Ctrl+F moves past the p.
chord(frame, field(12, 0x80), field(13, 0x04))
frame = frame + 100
enter('r', 'CTRLF 1')
text(frame, 'print("CTRLK",1)junk')
frame = frame + 320
for i=1,4 do key(field(5, 0x80)) end
chord(frame, field(12, 0x80), field(0, 0x04)) -- Ctrl+K removes the tail.
frame = frame + 100
enter('', 'CTRLK 1')
enter('history=0')
enter('history+=1; print("HISTORY",history)', 'HISTORY 1')
key(field(7, 0x80)) -- Up: recall and execute previous command.
enter('', 'HISTORY 2')
chord(frame, field(12, 0x80), field(3, 0x08)) -- Ctrl+P recalls history.
frame = frame + 80
chord(frame, field(12, 0x80), field(15, 0x80)) -- Ctrl+N returns to new input.
frame = frame + 80
chord(frame, field(12, 0x80), field(3, 0x08))
frame = frame + 80
enter('', 'HISTORY 3')
enter('completionvalue=37')
text(frame, 'print("COMPLETE",completi')
frame = frame + 320
key(field(12, 1)) -- Complete a user-defined name.
enter(')', 'COMPLETE 37')
enter('word="abc"')
text(frame, 'print("METHOD",word.upp')
frame = frame + 320
key(field(12, 1)) -- Complete an attribute.
enter('())', 'METHOD ABC')
text(frame, 'print("EDIT",12)')
frame = frame + 240
key(field(5, 0x80)) -- Left twice, insert in the middle.
key(field(5, 0x80))
enter('0', 'EDIT 102')
text(frame, 'if True:\r')
frame = frame + 200
key(field(12, 1)) -- Four-space indent after newline.
enter('print("TAB",1)\r', 'TAB 1')
-- Exercise collection and recall after many temporary allocations.
enter('for i in range(900):\r t=list(range(12))\r', nil)
frame = frame + 200
enter('assert completionvalue==37; print("ALIVE",1)', 'ALIVE 1')
text(frame, 'while True:\r pass\r\r')
frame = frame + 450
chord(frame, field(12, 0x80), field(10, 0x40))
frame = frame + 150
expect('KeyboardInterrupt') -- Check full decoded LCD text, not a substitute marker.
enter('assert completionvalue==37; print("INTERRUPT",1)', 'INTERRUPT 1')
if variant == "extra" then
    enter('f=getattr(micropython,"kbd"+chr(95)+"intr"); print("HOOK",1)', 'HOOK 1')
    text(frame, 'while True:\r f(24)\r\r')
    frame = frame + 450
    chord(frame, field(12, 0x80), field(11, 0x40)) -- Assigned Ctrl+X.
    frame = frame + 150
    expect('KeyboardInterrupt')
    text(frame, 'while True:\r f(120)\r\r')
    frame = frame + 450
    key(field(11, 0x40)) -- Assigned plain x.
    frame = frame + 150
    expect('KeyboardInterrupt')
end

chord(frame, field(12, 0x80), field(10, 0x04))
frame = frame + 120
expect('MicroPython')
enter('try:\r completionvalue\rexcept NameError:\r print("RESET",1)\r', 'RESET 1')
-- The adapter must preserve upstream raw and paste modes.
chord(frame, field(12, 0x80), field(12, 0x04)) -- Ctrl+A: raw REPL.
frame = frame + 100
expect('raw REPL')
text(frame, 'print("RAW",1)')
frame = frame + 250
chord(frame, field(12, 0x80), field(10, 0x04)) -- Ctrl+D executes raw source.
frame = frame + 150
expect('RAW 1')
chord(frame, field(12, 0x80), field(13, 0x80)) -- Ctrl+B: friendly REPL.
frame = frame + 100
expect('MicroPython')
chord(frame, field(12, 0x80), field(10, 0x08)) -- Ctrl+E: paste mode.
frame = frame + 100
expect('paste mode')
text(frame, 'print("PASTE",1)\r')
frame = frame + 300
chord(frame, field(12, 0x80), field(10, 0x04))
frame = frame + 150
expect('PASTE 1')
chord(frame, field(12, 0x80), field(10, 0x08))
frame = frame + 100
text(frame, 'unfinished paste')
frame = frame + 260
chord(frame, field(12, 0x80), field(10, 0x40)) -- Cancel paste.
frame = frame + 100
enter('print("FRIENDLY",1)', 'FRIENDLY 1')
-- Terminal controls must pass through Python output, not appear as text.
enter('print(chr(27),"[2J",chr(27),"[HVT100",sep="")')
at(frame, function() assert(lcd():sub(1,5)=='VT100', 'clear/home escape was not interpreted') end)
frame = frame + 20
enter('print(chr(27),"[2;5HAT",sep="")')
at(frame, function()
    local row2 = lcd():match("^[^\n]*\n([^\n]*)")
    assert(row2:sub(5,6)=='AT', 'absolute cursor position failed')
end)
frame = frame + 20
enter('print(chr(27),"[2;5H",chr(27),"[K",sep="")')
at(frame, function()
    local row2 = lcd():match("^[^\n]*\n([^\n]*)")
    assert(row2:sub(5,6)=='  ', 'erase line failed')
    pass(); manager.machine:exit()
end)
run()
