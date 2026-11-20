-- Shared AS3000 automation. check.sh prepends the case name.
local machine = manager.machine
local events, actions = {}, {}
local adaptive = test_case:match("^micropython") ~= nil
local function enqueue(kind, data)
    data = data or {}; data.kind = kind; actions[#actions+1] = data
end

local function at(frame, action)
    if adaptive then enqueue("check", {fn=action}); return end
    assert(not events[frame], "duplicate frame " .. frame)
    events[frame] = action
end

-- Track submitted source across chunks (underscore workarounds split commands).
-- A single Return within a compound statement acknowledges continuation; the
-- terminating blank line must reach the top-level prompt before the next input.
local compound, draft_line = false, ""
local function text(frame, characters, busy)
    if adaptive then
        if frame < 530 then
            events[frame] = function() machine.natkeyboard:post(characters) end
        else
            for ch in characters:gmatch(".") do
                if ch=="\r" then
                    if draft_line=="" then compound=false
                    elseif draft_line:match(":%s*$") then compound=true end
                    draft_line=""
                else draft_line=draft_line..ch end
            end
            enqueue("text", {chars=characters, busy=busy or false, continuation=compound})
        end
    else
        at(frame, function() machine.natkeyboard:post(characters) end)
    end
end

local key_ids = {}
local function field(col, mask)
    for _, input in pairs(machine.ioport.ports[":COL." .. col].fields) do
        if input.mask == mask then key_ids[input] = col .. ":" .. mask; return input end
    end
    error("missing AS3000 key")
end

local function tap(frame, key)
    if adaptive then enqueue("tap", {key=key}); return end
    at(frame, function() key:set_value(1) end)
    at(frame + 12, function() key:clear_value() end)
end

local function chord(frame, modifier, key)
    if adaptive then
        if key_ids[modifier]=="12:128" and (key_ids[key]=="10:64" or key_ids[key]=="10:4") then
            compound, draft_line=false,""
        end
        enqueue("chord", {key=key, mod=modifier, id=key_ids[key]}); return
    end
    at(frame, function() modifier:set_value(1) end)
    tap(frame + 8, key)
    at(frame + 30, function() modifier:clear_value() end)
end

local function lcd()
    local rows = {}
    for _, tag in ipairs({":ks0066_0", ":ks0066_1"}) do
        local index = assert(machine.devices[tag].items["0/m_ddram"], "missing LCD DDRAM")
        local ddram = emu.item(index)
        -- AS3000 3.01 translates text through its ROM table at 0x4ea62c.
        -- These ASCII glyphs do not use their ASCII byte in controller DDRAM.
        local ascii = {[0]="g", [1]="y", [2]="p", [3]="q", [0xda]="\\", [0xd5]="~"}
        local function decode(bytes)
            return (bytes:gsub(".", function(c) return ascii[c:byte()] or c end))
        end
        rows[#rows + 1] = decode(ddram:read_block(0, 40))
        rows[#rows + 1] = decode(ddram:read_block(0x40, 40))
    end
    return table.concat(rows, "\n")
end

local function expect_text(expected)
    local screen = lcd()
    if adaptive and expected == "KeyboardInterrupt" then
        assert(screen:match(">>>%s*$"), "interpreter has not returned to its prompt")
    end
    assert(screen:find(expected, 1, true), "missing LCD text " .. expected .. "\n" .. screen)
end

local function pass()
    machine.video:snapshot()
    print("PASS " .. test_case)
end

-- Completion is observed in controller RAM, never inferred from a sleep.
-- Only MicroPython cases use ordered actions; other applet fixtures retain their
-- explicit CPU/RAM event schedules. Their frame arguments remain compatible.
local function run_ready()
    local mode = "friendly"
    local function input_ready(screen, item)
        if item and item.busy then return not screen:match(">>>%s*$") end
        if item and not item.chars:match("\r$") then return true end
        if mode == "paste" then return screen:match("===%s*$") ~= nil end
        if mode == "raw" then return screen:match(">%s*$") ~= nil end
        local continuation = item and item.continuation
        local pattern=continuation and "^%.%.%.%s*$" or "^>>>%s*$"
        for row in (screen.."\n"):gmatch("([^\n]*)\n") do
            if row:match(pattern) then return true end
        end
        return false
    end
    local frame, index, started, previous, stable, booted = 0, 1, nil, nil, 0, false
    local failed = false
    local function fail(message)
        failed = true
        print("LUA ERROR: " .. message)
        local f=io.open("failure.lcd.txt", "w")
        if f then f:write(message .. "\n" .. lcd()); f:close() end
        machine.video:snapshot()
        machine:exit()
    end
    emu.register_frame_done(function()
        if failed then return end
        frame=frame+1
        local ok, err=pcall(function()
            if events[frame] then events[frame]() end
            local current=lcd()
            stable=current==previous and stable+1 or 1; previous=current
            if frame < 530 then return end
            if not booted then
                if not machine.natkeyboard.empty or not input_ready(current) or stable<3
                    or not current:find("MicroPython",1,true) then
                    assert(frame<1500, "MicroPython boot readiness timeout")
                    return
                end
                booted=true
            end
            local item=actions[index]; assert(item, "scenario ended without PASS")
            if not started then
                started=frame; stable=0
                if item.kind=="text" then machine.natkeyboard:post(item.chars)
                elseif item.kind=="tap" then item.key:set_value(1)
                elseif item.kind=="chord" then
                    item.mod:set_value(1)
                    if item.id=="12:4" then mode="raw"
                    elseif item.id=="13:128" then mode="friendly"
                    elseif item.id=="10:8" then mode="paste"
                    elseif mode=="paste" and (item.id=="10:4" or item.id=="10:64") then mode="friendly" end
                end
            end
            local age=frame-started
            local function advance()
                print(string.format("READY stage=%d kind=%s frames=%d", index,item.kind,age))
                index=index+1; started=nil
            end
            if item.kind=="text" then
                if machine.natkeyboard.empty then
                    item.drained=item.drained or frame
                    if stable>=3 and input_ready(current, item) then advance() end
                    assert(frame-item.drained<600, "command completion timeout")
                end
                assert(age < #item.chars*25+600, "natural keyboard queue timeout")
            elseif item.kind=="tap" then
                if age==12 then item.key:clear_value() end
                if age>=14 and stable>=3 then advance() end
            elseif item.kind=="chord" then
                if age==2 then item.key:set_value(1) end
                if age==14 then item.key:clear_value() end
                if age==16 then item.mod:clear_value() end
                if age>=16 and stable>=3 then advance() end
            elseif item.kind=="check" and stable>=3 then
                local matched, why=pcall(item.fn)
                if matched then advance() else item.why=tostring(why) end
            end
            if item.kind~="text" then
                assert(age<600, "readiness timeout: "..tostring(item.why or item.kind))
            end
        end)
        if not ok then fail(tostring(err)) end
    end)
end

local function run()
    if adaptive then run_ready(); return end
    local frame = 0
    local failed = false
    emu.register_frame_done(function()
        frame = frame + 1
        if not failed and events[frame] then
            local ok, err = pcall(events[frame])
            if not ok then
                failed = true
                print("LUA ERROR: " .. err)
                machine.video:snapshot()
            end
        end
    end)
end

-- Fresh NVRAM invokes the backup battery recovery prompt. Main Return
-- selects an applet; keypad Enter is the Applets key on this driver.
text(60, "y")
text(200, "\r")
text(450, "\r")
