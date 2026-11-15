--[[
    test_psychohouse.lua - run the mod-psychohouse addon and check what it does.

    Loaded by tools/housing/check_psychohouse_addon.py. It builds the 8.3.7 model in
    wow_env.lua, loads the addon's .toc in order, fires the login events, feeds the window one
    line per record in ADDON-PROTOCOL.md, clicks every button and sets every edit box, and
    asserts what can be asserted without a client: that it loads, that every record is
    dispatched, that each command sends a well formed line, and that nothing raises.

    The server side of the same protocol is a switch in src/mod_psychohouse_addon.cpp. The
    two lists are compared by tools/housing/check_psychohouse.py, so a record added on one
    side and not the other is a finding rather than a silent no-op.
--]]

local ADDON_DIR = assert(os.getenv("PH_ADDON"), "PH_ADDON is not set")

local source = debug.getinfo(1, "S").source
local toolsDir = source:match("^@(.*)[/\\][^/\\]*$")
local W = assert(dofile(toolsDir .. "/wow_env.lua"), "wow_env.lua")

local passes, failures = 0, 0

local function pass(name)
    passes = passes + 1
    print("PASS " .. name)
end

local function fail(name, detail)
    failures = failures + 1
    print("FAIL " .. name .. (detail and (": " .. tostring(detail)) or ""))
end

local function check(condition, name, detail)
    if condition then
        pass(name)
    else
        fail(name, detail)
    end
    return condition
end

local function lastSent()
    local sent = W.sent[#W.sent]
    return sent and sent.text or ""
end

local function sentMatching(prefix)
    local out = {}
    for _, record in ipairs(W.allSent) do
        if record.text:sub(1, #prefix) == prefix then
            out[#out + 1] = record.text
        end
    end
    return out
end

-- The module answers on the player's own whisper channel, so the client sees the player's own
-- name as the sender. Anything else is treated as a foreign message by design.
local function Deliver(text)
    W.Deliver(text, "Thordekk", "PHSH")
end

local function click(frame, label)
    if not frame then
        fail("button " .. label, "frame was never created")
        return
    end
    local handler = frame.__state.scripts.OnClick
    if not handler then
        fail("button " .. label, "has no OnClick")
        return
    end
    local ok, err = pcall(handler, frame, "LeftButton")
    check(ok, "button " .. label .. " click", err)
end

local env = W.BuildEnv()

-- The .toc decides the load order; reading it here rather than hardcoding the two names means
-- a file added to the addon is loaded by the test without editing this file.
local tocPath = ADDON_DIR .. "/PsychoHouse.toc"
local handle = assert(io.open(tocPath, "r"), "cannot open the addon .toc")
local files = {}
for line in handle:lines() do
    line = line:gsub("%s+$", "")
    if line ~= "" and line:sub(1, 1) ~= "#" and line:sub(-4) == ".lua" then
        files[#files + 1] = line
    end
end
handle:close()

check(#files >= 2, "the .toc lists the addon's Lua files", #files)

for _, name in ipairs(files) do
    local ok, err = pcall(W.LoadFile, env, ADDON_DIR .. "/" .. name, "PsychoHouse")
    check(ok, "load " .. name, err)
end

local PH = env.PsychoHouse
check(type(PH) == "table", "the addon declares its root global")

-- Login: the prefix is registered once and the window greets the server.
W.env.PsychoHouseDB = {}
W.FireEvent("ADDON_LOADED", "PsychoHouse")
W.FireEvent("PLAYER_LOGIN")

check(#W.prefixes == 1 and W.prefixes[1] == "PHSH", "the PHSH prefix is registered once", W.prefixes[1])
check(lastSent() == "C:HELLO", "login sends C:HELLO", lastSent())
check(W.sent[1].channel == "WHISPER", "records go out as whispers", W.sent[1].channel)
check(W.sent[1].target == "Thordekk", "records are whispered to the player", W.sent[1].target)

-- The server answers the greeting.
Deliver("S:HELLO|1|20|1")
check(PH.protocol == 1, "S:HELLO carries the protocol version", PH.protocol)
check(PH.limit == 20, "S:HELLO carries the object limit", PH.limit)
check(PH.hasHouse == true, "S:HELLO carries whether a house exists")

-- Status, then a facilities run, then an objects run: the three the window asks for.
Deliver("S:STATUS|1|50001|3|12|2|20|10000000")
check(PH.phase == 50001, "S:STATUS carries the phase", PH.phase)
check(PH.bought == 3, "S:STATUS carries the facility count", PH.bought)
check(PH.objectCount == 2, "S:STATUS carries the object count", PH.objectCount)
check(PH.price == 10000000, "S:STATUS carries the price", PH.price)

PH.facilities = {}
Deliver("S:FAC|1|1|Bank|500000|1|0|0")
Deliver("S:FAC|2|7|Forge|250000|0|0|0")
Deliver("S:FACEND|2")
check(#PH.facilities == 2, "two facility records make two rows", #PH.facilities)
check(PH.facilities[1].name == "Bank", "the facility name survives a pipe split", PH.facilities[1].name)
check(PH.facilities[2].owned == false, "an unowned facility says so", tostring(PH.facilities[2].owned))

PH.objects = {}
Deliver("S:OBJ|0|240|16222.50|16267.00|13.10|1.5700|1.00")
Deliver("S:OBJ|1|2950|16223.50|16268.00|13.10|0.0000|1.50")
Deliver("S:OBJEND|2|20")
check(#PH.objects == 2, "two object records make two rows", #PH.objects)
check(PH.objects[1].entry == 240, "the object entry survives a pipe split", PH.objects[1].entry)
check(math.abs(PH.objects[2].scale - 1.5) < 0.001, "the object scale survives a pipe split")

-- A record from a protocol this addon does not speak: unknown names are ignored, not guessed.
local before = #W.printed
Deliver("S:FROMTHEFUTURE|1|2|3")
check(#W.printed == before, "an unknown record prints nothing and raises nothing")

-- A record under a different prefix is not this addon's business.
local beforeForeign = #PH.objects
W.Deliver("S:OBJ|9|1|0|0|0|0|1", "Server", "BFAH")
check(#PH.objects == beforeForeign, "a record on another prefix is ignored")

-- The window: show it, which asks the server for everything.
local panel = W.byName["PsychoHousePanel"]
check(panel ~= nil, "the window frame is created by name")

if panel then
    W.Show(panel)
    local handler = panel.__state.scripts.OnShow
    check(handler ~= nil, "the window refreshes itself when shown")

    if handler then
        W.sent = {}
        handler(panel)
        check(#sentMatching("C:STATUS") == 1, "showing the window asks for status")
        check(#sentMatching("C:FACILITIES") == 1, "showing the window asks for facilities")
        check(#sentMatching("C:OBJECTS") == 1, "showing the window asks for objects")
    end
end

-- Every button the window draws.
local objects = {}
W.Walk(panel or W.byName["PsychoHousePanel"], function(frame) objects[#objects + 1] = frame end)
check(#objects > 20, "the window builds its rows")

local function findButton(text)
    for _, frame in ipairs(objects) do
        if frame.__kind == "Button" and frame.__state.text == text then
            return frame
        end
    end
    return nil
end

for _, label in ipairs({ "Buy", "Sell", "Travel" }) do
    local frame = findButton(label)
    if check(frame ~= nil, "the window has a " .. label .. " button") then
        W.sent = {}
        click(frame, label)
        check(#W.sent == 1, label .. " sends exactly one record", #W.sent)
    end
end

-- Refresh is the one button that asks for three runs at once, because each run is a separate
-- record family on the wire.
if check(findButton("Refresh") ~= nil, "the window has a Refresh button") then
    W.sent = {}
    click(findButton("Refresh"), "Refresh")
    check(#W.sent == 3, "Refresh asks for status, facilities and objects", #W.sent)
end

W.sent = {}
if findButton("Buy") then
    click(findButton("Buy"), "Buy")
    check(lastSent() == "C:BUY", "Buy sends C:BUY", lastSent())
end

W.sent = {}
if findButton("Travel") then
    click(findButton("Travel"), "Travel")
    check(lastSent() == "C:HOME", "Travel sends C:HOME", lastSent())
end

-- A facility row's Buy button, once the list has been drawn.
W.sent = {}
PH.facilities = {}
Deliver("S:FAC|2|7|Forge|250000|0|0|0")
Deliver("S:FACEND|1")

local facilityBuy
for _, frame in ipairs(objects) do
    if frame.__kind == "Button" and frame.__state.text == "Buy" and frame.__state.parent and
        frame.__state.parent.__state and frame.__state.parent.__state.parent == panel then
        facilityBuy = frame
    end
end

if check(facilityBuy ~= nil, "a facility row draws its own Buy button") then
    W.sent = {}
    click(facilityBuy, "facility Buy")
    check(lastSent() == "C:BUYFAC|2", "the facility button sends C:BUYFAC with its id", lastSent())
end

-- A bought facility disables its button rather than offering a second purchase.
PH.facilities = {}
Deliver("S:FAC|1|1|Bank|500000|1|0|0")
Deliver("S:FACEND|1")

local ownedRow
for _, frame in ipairs(objects) do
    if frame.__kind == "Button" and frame.__state.text == "Bought" then
        ownedRow = frame
    end
end
check(ownedRow ~= nil, "an owned facility shows Bought and is disabled")
if ownedRow then
    check(ownedRow.__state.disabled == true, "the owned facility's button is disabled")
end

-- An object row's Remove button.
PH.objects = {}
Deliver("S:OBJ|4|240|0|0|0|0|1")
Deliver("S:OBJEND|1|20")

W.sent = {}
-- The first one, because rows are reused: a row drawn for an earlier list keeps its old
-- handler until it is drawn again, and the last Remove button on the window may belong to a
-- row that is currently hidden.
local removeButton
for _, frame in ipairs(objects) do
    if frame.__kind == "Button" and frame.__state.text == "Remove" and not removeButton then
        removeButton = frame
    end
end
if check(removeButton ~= nil, "an object row draws a Remove button") then
    click(removeButton, "Remove")
    check(lastSent() == "C:REMOVE|4", "Remove sends C:REMOVE with its slot", lastSent())
end

-- The entry box: what a player types becomes C:PLACEHERE.
local entry = W.byName["PsychoHouseEntryBox"]
if check(entry ~= nil, "the window has an entry box") then
    entry:SetText("2950")
    W.sent = {}
    local handler = entry.__state.scripts.OnEnterPressed
    if check(handler ~= nil, "the entry box accepts Enter") then
        local ok, err = pcall(handler, entry)
        check(ok, "pressing Enter places", err)
        check(lastSent() == "C:PLACEHERE|2950", "Enter sends C:PLACEHERE with the entry", lastSent())
        check(entry:GetText() == "", "the box is cleared after a placement")
    end
end

-- The slash command toggles the window.
check(env.SlashCmdList["PSYCHOHOUSE"] ~= nil, "the slash command is registered")
if env.SlashCmdList["PSYCHOHOUSE"] then
    panel:Hide()
    W.Slash("PSYCHOHOUSE", "")
    check(panel:IsShown() == true, "/psychohouse shows the window", tostring(panel:IsShown()))
    W.Slash("PSYCHOHOUSE", "")
    check(panel:IsShown() == false, "/psychohouse hides it again", tostring(panel:IsShown()))
end

-- A result line becomes a sentence, not a number.
W.printed = {}
Deliver("S:RESULT|PLACE|13")
local printedResult = false
for _, text in ipairs(W.printed) do
    if text:find("full", 1, true) then
        printedResult = true
    end
end
check(printedResult, "a full house prints the sentence for result 13")

-- Every result code the server can send has a sentence here.
for code = 0, 16 do
    check(PH.ResultText(code) ~= nil and PH.ResultText(code) ~= "",
        "result " .. code .. " has a sentence")
end

-- The model records every global the addon read as nil. A name in that list is a typo or an
-- API the model does not provide; it must not be an API the addon needs.
-- The addon's own root global and its saved variable are reads of a global that does not exist
-- yet on the first load; every addon does this and a client answers nil without complaint.
local expected = { PsychoHouse = true, PsychoHouseDB = true }

local missing = {}
for name in pairs(W.unknownGlobals) do
    if not expected[name] then
        missing[#missing + 1] = name
    end
end
table.sort(missing)
check(#missing == 0, "the addon reads no global the model lacks", table.concat(missing, ", "))

print(string.format("SUMMARY %d passed, %d failed", passes, failures))
os.exit(failures == 0 and 0 or 1)
