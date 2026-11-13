--[[
    test_addon.lua - run the shipped addon and check what it does.

    Loaded by tools/housing/check_addon.py, which supplies the addon folder in
    HOUSING_ADDON and a plan of one wire line per server opcode in HOUSING_PLAN, taken from
    doc 15. Every PASS and FAIL line here becomes a check or a finding; the SENT and HANDLED
    lines are compared against the document by the caller.

    Nothing in this file knows what the addon "should" print. It loads it, feeds it the
    protocol, drives the commands and the buttons, and asserts only what can be asserted:
    that it loads, that every documented opcode is dispatched, that every command sends
    something well formed, and that no path raises.
--]]

local ADDON_DIR = assert(os.getenv("HOUSING_ADDON"), "HOUSING_ADDON is not set")
local PLAN_PATH = assert(os.getenv("HOUSING_PLAN"), "HOUSING_PLAN is not set")

local source = debug.getinfo(1, "S").source
local toolsDir = source:match("^@(.*)[/\\][^/\\]*$")
local W = assert(dofile(toolsDir .. "/wow_env.lua"), "wow_env.lua")
dofile(PLAN_PATH)

local passes, failures = 0, 0

local function report(name)
    return name
end

local function pass(name)
    passes = passes + 1
    print("PASS " .. report(name))
end

local function fail(name, detail)
    failures = failures + 1
    print("FAIL " .. report(name) .. (detail and (": " .. tostring(detail)) or ""))
end

local function check(condition, name, detail)
    if condition then
        pass(name)
    else
        fail(name, detail)
    end
    return condition
end

local function Count(tbl)
    local n = 0
    for _ in pairs(tbl or {}) do
        n = n + 1
    end
    return n
end

local function HasText(list, needle)
    for _, text in ipairs(list) do
        if string.find(text, needle, 1, true) then
            return true
        end
    end
    return false
end

-- ---------------------------------------------------------------------------
-- Load
-- ---------------------------------------------------------------------------

local env = W.BuildEnv()
local loaded, loadError = pcall(function()
    W.LoadFile(env, ADDON_DIR .. "/Core.lua", "HousingUI")
    W.LoadFile(env, ADDON_DIR .. "/Menu.lua", "HousingUI")
end)

if not check(loaded, "Core.lua and Menu.lua load in a 5.1 client model", loadError) then
    print(("%d checks passed, %d finding(s)"):format(passes, failures))
    os.exit(1)
end

local H = env.HousingUI

-- Which client opcodes have an optional first argument, from doc 15's brackets.
local OPTIONAL = {}
for _, entry in ipairs(CLIENT or {}) do
    OPTIONAL[entry.op] = entry.optional
end
check(type(H) == "table", "the addon publishes its own table")
check(type(H.Menu) == "table", "Menu.lua attaches the menu to it")
check(H.version == "0.10.1", "the version the .toc declares", H.version)

-- A nil global that the addon probes on purpose (the 8.3.7 fallbacks) is not a typo.
local PROBES = {
    RegisterAddonMessagePrefix = true,
    SendAddonMessage = true,
    GetAddonMetadata = true,
    -- Globals the addon creates itself: `HousingUI = HousingUI or {}` reads before it writes.
    HousingUI = true,
    HousingUIDB = true,
}

local unknown = {}
for name in pairs(W.unknownGlobals) do
    if not PROBES[name] then
        unknown[#unknown + 1] = name
    end
end
table.sort(unknown)
check(#unknown == 0, "no unknown client globals are read", table.concat(unknown, ", "))

-- ---------------------------------------------------------------------------
-- Events
-- ---------------------------------------------------------------------------

local eventError = select(2, pcall(W.FireEvent, "ADDON_LOADED", "HousingUI"))
check(eventError == nil, "ADDON_LOADED runs", eventError)
check(W.prefixes[1] == "BFAH", "the addon message prefix is registered as BFAH", W.prefixes[1])
check(HasText(W.printed, "v0.10.1 loaded"), "the load line names the version")

W.sent = {}
W.FireEvent("PLAYER_ENTERING_WORLD")
check(W.sent[1] and W.sent[1].text == "C:HELLO|3", "logging in sends the handshake",
      W.sent[1] and W.sent[1].text)
check(W.sent[1] and W.sent[1].channel == "WHISPER", "the handshake is a whisper to myself",
      W.sent[1] and W.sent[1].channel)

-- ---------------------------------------------------------------------------
-- Slash commands
-- ---------------------------------------------------------------------------

-- Every command the usage text offers, plus the ones it leaves to the menu. Bare and with
-- arguments: a command that only works when its arguments are well formed is half a command.
local COMMANDS = {
    { "", "" },
    { "hello", "" },
    { "ping", "" },
    { "neighbourhood", "" },
    { "plots", "" },
    { "plot", "3" },
    { "buy", "3" },
    { "sell", "" },
    { "home", "" },
    { "leave", "" },
    { "interior", "1" },
    { "interior", "" },
    { "dir", "" },
    { "dir", "2" },
    { "decor", "" },
    { "decor", "banners 1" },
    { "buydecor", "1500" },
    { "placed", "" },
    { "place", "1500 1 2 3 90 1.0 1 0.1 -0.2" },
    { "place", "1500 1 2 3 90 1.0 1" },
    { "dyes", "" },
    { "snap", "on" },
    { "snap", "off" },
    { "snap", "" },
    { "settings", "" },
    { "move", "44 1 2 3 90 1.0 0.1 -0.2" },
    { "remove", "44" },
    { "amenities", "" },
    { "guests", "" },
    { "guest", "add Alt 7" },
    { "guest", "del Alt" },
    { "bp", "PHP1:0000000000abcd" },
    { "blueprint", "PHP1:zz" },
    { "crc", "123456789" },
    { "finder", "" },
    { "visit", "12 3" },
    { "found", "" },
    { "leavehood", "" },
    { "movehouse", "4" },
    { "gryphon", "" },
    { "room", "4" },
    { "rooms", "" },
    { "roomat", "4 1 -1 0" },
    { "roomremove", "4 1 -1 0" },
    { "arcane", "" },
    { "armillary", "" },
    { "armillary", "2" },
    { "favor", "" },
    { "at", "1500" },
    { "at", "1500 3 1" },
    { "join", "12" },
    { "reserve", "4" },
    { "storage", "" },
    { "buycopy", "1500" },
    { "buycopy", "1500 3" },
    { "levelup", "" },
    { "shells", "" },
    { "shell", "2" },
    { "dye", "44 1 5" },
    { "portals", "" },
    { "portal", "2" },
    { "endeavor", "" },
    { "deliver", "" },
    { "claim", "1" },
    { "export", "" },
    { "export", "1 Hall" },
    { "import", "PHP1:0000000000abcd" },
    { "restore", "" },
    { "build", "" },
    { "build", "0 1500" },
    { "hunts", "" },
    { "hunt", "3" },
    { "tier", "5" },
    { "name", "Stormhold" },
    { "lock", "1" },
    { "enter", "3" },
    { "selecthome", "77" },
    { "pet", "44 2" },
    { "access", "0 63" },
    { "log", "" },
    { "clear", "" },
    { "raw", "C:HELLO|3" },
    { "nonsense", "" },
}

-- The usage text is the promise; the sweep has to keep it.
local usageText = {}
for _, text in ipairs(W.printed) do
    if string.find(text, "commands:", 1, true) then
        usageText[#usageText + 1] = text
    end
end

local coreFile = io.open(ADDON_DIR .. "/Core.lua", "r")
local core = coreFile:read("*a")
coreFile:close()

-- Only the strings inside Usage(), and only the first word of each alternative: the rest of
-- a usage line is its arguments, which are not commands.
local function usageCommands(sourceText)
    local body = sourceText:match("local function Usage%(%)%s*(.-)%s*end%s*SLASH_HOUSING1")
    local words = {}
    if not body then
        return words
    end

    for quoted in body:gmatch('"([^"]*)"') do
        local line = quoted:gsub("^commands:%s*", ""):gsub("^%s+", ""):gsub("^/housing%s*", "")
        for part in line:gmatch("[^|]+") do
            local word = part:match("^%s*(%a+)")
            if word then
                words[word] = true
            end
        end
    end

    return words
end

-- Words that are prose rather than command names.
local NOT_COMMANDS = {
    commands = true, housing = true, usage = true,
    ["and"] = true, ["or"] = true, ["the"] = true, ["a"] = true, ["to"] = true,
}

local usageWords = usageCommands(core)
local swept = {}
for _, entry in ipairs(COMMANDS) do
    swept[entry[1]] = true
end

local unswept = {}
for word in pairs(usageWords) do
    if not NOT_COMMANDS[word] and not swept[word] then
        unswept[#unswept + 1] = word
    end
end
table.sort(unswept)
check(#unswept == 0, "every command the usage text lists is swept", table.concat(unswept, " "))

local commandErrors, malformed, emptyArgs = {}, {}, {}

for _, entry in ipairs(COMMANDS) do
    local label = entry[1] == "" and "(bare /housing)" or ("/housing " .. entry[1])
    W.sent = {}
    local ok, err = pcall(W.Slash, "HOUSING", (entry[1] .. " " .. entry[2]):gsub("%s+$", ""))
    if not ok then
        commandErrors[#commandErrors + 1] = label .. ": " .. tostring(err)
    end

    for _, frame in ipairs(W.sent) do
        local text = frame.text
        if type(text) ~= "string" or not text:match("^C:[A-Z]+") then
            malformed[#malformed + 1] = label .. " sent '" .. tostring(text) .. "'"
        elseif text:match("|$") then
            -- `C:DECOR|` is not a mistake: doc 15 brackets the category as optional and an
            -- empty category is how the wire says "every category". A required field sent
            -- empty is the mistake.
            local op = text:match("^C:([A-Z]+)")
            local optionalFirst = OPTIONAL[op] and text:match("^C:[A-Z]+|$") ~= nil
            if not optionalFirst then
                emptyArgs[#emptyArgs + 1] = label .. " sent '" .. text .. "'"
            end
        end
    end
end

check(#commandErrors == 0, "no slash command raises", table.concat(commandErrors, "; "))
check(#malformed == 0, "every slash command sends a C: line", table.concat(malformed, "; "))
check(#emptyArgs == 0, "no command sends a trailing empty field", table.concat(emptyArgs, "; "))

-- Doc 15 brackets the optional arguments. A command whose first field is required has to
-- answer a bare call with its usage line, not with an empty field the server will reject.
local REQUIRED_ARGS = {
    { "plot", "PLOT", "/housing plot" },
    { "buy", "BUY", "/housing buy" },
    { "buydecor", "BUYDECOR", "/housing buydecor" },
    { "remove", "REMOVE", "/housing remove" },
    { "movehouse", "MOVEHOUSE", "/housing movehouse" },
    { "room", "ROOM", "/housing room" },
    { "roomat", "ROOMAT", "/housing roomat" },
    { "roomremove", "ROOMREMOVE", "/housing roomremove" },
    { "shell", "SHELL", "/housing shell" },
    { "portal", "GOPORTAL", "/housing portal" },
    { "join", "JOIN", "/housing join" },
    { "reserve", "RESERVE", "/housing reserve" },
    { "claim", "CLAIM", "/housing claim" },
    { "import", "BPIMP", "/housing import" },
    { "visit", "VISIT", "/housing visit" },
    { "dye", "DYE", "/housing dye" },
    { "place", "PLACE", "/housing place" },
    { "move", "MOVE", "/housing move" },
    { "guest", "GUESTADD", "/housing guest" },
    { "at", "PLACEAT", "/housing at" },
}

local unguarded = {}
for _, case in ipairs(REQUIRED_ARGS) do
    local command, op, usage = case[1], case[2], case[3]
    if OPTIONAL[op] == false then
        W.sent = {}
        W.printed = {}
        pcall(W.Slash, "HOUSING", command)
        if #W.sent > 0 then
            unguarded[#unguarded + 1] = "/housing " .. command .. " sent " .. W.sent[1].text
        elseif not HasText(W.printed, usage) then
            unguarded[#unguarded + 1] = "/housing " .. command .. " said nothing about its usage"
        end
    end
end

check(#unguarded == 0, "a command that needs an argument shows its usage instead of sending",
      table.concat(unguarded, "; "))

-- The window's own buttons send some of them; click through it as well.
check(type(env.SlashCmdList.HOUSINGUI) == "function", "/house is registered")
pcall(W.Slash, "HOUSINGUI", "")
pcall(W.Slash, "HOUSINGUI", "")
do
    local menu = assert(W.byName["PsychoHouseMenuFrame"], "the window frame exists")
    local first = menu:IsShown()
    pcall(W.Slash, "HOUSINGUI", "")
    check(menu:IsShown() ~= first, "/house toggles the window")
end



-- ---------------------------------------------------------------------------
-- Server opcodes
-- ---------------------------------------------------------------------------

local dispatched, unhandled = {}, {}

for _, entry in ipairs(PLAN) do
    W.printed = {}
    local ok, err = pcall(W.Deliver, entry.line)
    if not ok then
        fail("S:" .. entry.op .. " raises when received", err)
    else
        local said = false
        for _, text in ipairs(W.printed) do
            if string.find(text, "unhandled opcode", 1, true) then
                said = true
            end
        end
        if said then
            unhandled[#unhandled + 1] = entry.op
        else
            dispatched[#dispatched + 1] = entry.op
        end
    end
end

check(#unhandled == 0, "every documented server opcode is dispatched",
      "no handler: " .. table.concat(unhandled, ", "))
check(#dispatched == #PLAN, "the plan was delivered whole", #dispatched .. "/" .. #PLAN)
print("HANDLED " .. table.concat(dispatched, " "))

-- Values, not just absence of errors: a line that parses to the wrong numbers is worse than
-- one that fails, because it draws the wrong thing quietly.
W.Deliver("S:PLOTINFO|3|1|250000|2|2|4|Thordekk|Stormhold|0|300|600|120|240|3|5|2")
local info = H.plotInfo or {}
check(info.index == 3 and info.price == 250000 and info.houseMask == 5 and info.armillary == 2,
      "the longest line parses to the documented fields",
      "index " .. tostring(info.index) .. " price " .. tostring(info.price) ..
      " mask " .. tostring(info.houseMask) .. " armillary " .. tostring(info.armillary))

W.Deliver("S:PLACEDENTRY|44|1500|1.5|2.5|0.25|90|1.0|1|0.1|-0.2|311|1")
local piece = H.placed[44]
check(piece and piece.decorId == 1500 and piece.pitch == 0.1 and piece.roll == -0.2
      and piece.petEntry == 311 and piece.petRoaming == true,
      "a placement keeps its tilt and its pet", piece and (piece.petEntry .. "/" .. tostring(piece.petRoaming)))

-- A room on the plan is not a decor placement. They shared a table once, which let a room
-- take the key of a placement and the Placed tab list the wrong thing.
H.roomPlacements = {}
H.placed = {}
W.Deliver("S:PLACEDENTRY|1|1500|0|0|0|0|1.0|1|0|0|0|0")
W.Deliver("S:ROOMPLACED|1|2|-1|0")
check(H.placed[1] ~= nil and H.placed[1].decorId == 1500,
      "a room does not displace placement 1", H.placed[1] and H.placed[1].decorId)
check(#H.roomPlacements == 1 and H.roomPlacements[1].cellX == 2,
      "the room lands in its own list", #H.roomPlacements)

W.Deliver("S:HELLO|3|1041|1|1")
check(H.protocol == 3 and H.hasHouse == true, "the handshake fields land", H.protocol)
W.Deliver("S:HELLO|2|1041|1|1")
check(HasText(W.printed, "protocol mismatch"), "a protocol mismatch is announced")

-- ---------------------------------------------------------------------------
-- Framing: chunks, timeouts, log cap
-- ---------------------------------------------------------------------------

W.printed = {}
W.Deliver("S:CHUNK|9|1|2|S:PONG|")
W.Deliver("S:CHUNK|9|2|2|436")
check(HasText(W.printed, "pong 436"), "a two-chunk payload is reassembled",
      table.concat(W.printed, " / "))

W.printed = {}
W.Deliver("S:CHUNK|9|1|2|S:PONG|")
W.Deliver("S:CHUNK|9|2|2|437")
check(HasText(W.printed, "pong 437"), "the buffer is reused for the next payload")

W.printed = {}
W.Deliver("S:CHUNK|10|1|2|S:PONG|")
W.Deliver("S:CHUNK|10|2|2|438")
check(HasText(W.printed, "pong 438"), "a fresh frame id starts a fresh buffer")

-- A truncated frame must not leak: the ticker drops it after the timeout.
W.Deliver("S:CHUNK|11|1|3|S:PONG|")
W.now = W.now + 11
local ticker
for _, frame in ipairs(W.frames) do
    if frame.__state.scripts.OnUpdate then
        ticker = frame
    end
end
check(ticker ~= nil, "a ticker exists to drop truncated frames")
if ticker then
    W.printed = {}
    ticker.__state.scripts.OnUpdate(ticker, 0.1)
    check(HasText(W.printed, "dropped incomplete chunk frame 11"), "an abandoned chunk is dropped",
          table.concat(W.printed, " / "))
end

-- A frame arriving mid-payload with a different total starts over rather than corrupting.
W.printed = {}
W.Deliver("S:CHUNK|12|1|3|S:PONG|")
W.Deliver("S:CHUNK|12|1|1|S:PONG|450")
check(HasText(W.printed, "pong 450"), "a restarted frame replaces the partial one")

W.log = nil
H.log = {}
for index = 1, 450 do
    W.Deliver("S:PONG|" .. index)
end
check(#H.log == 400, "the log keeps the last 400 lines", #H.log)

-- ---------------------------------------------------------------------------
-- Numbers: CRC32, base62, snapping, settings, the blueprint envelope
-- ---------------------------------------------------------------------------

check(H.Crc32("123456789") == 3421780262, "CRC32 of 123456789 is CBF43926",
      tostring(H.Crc32("123456789")))
check(H.Crc32("") == 0, "CRC32 of an empty string is 0", tostring(H.Crc32("")))
check(H.Crc32("The quick brown fox jumps over the lazy dog") == 0x414FA339,
      "CRC32 matches zlib on the standard sentence",
      string.format("%08x", H.Crc32("The quick brown fox jumps over the lazy dog")))

check(H.Base62(0) == "0", "base62 of zero", H.Base62(0))
check(H.Base62(61) == "z", "base62 of sixty-one is z", H.Base62(61))
check(H.Base62(62) == "10", "base62 carries at sixty-two", H.Base62(62))
check(H.Base62(5, 4) == "0005", "base62 pads to the width asked for", H.Base62(5, 4))
check(H.Checksum("anything") == H.Checksum("anything"), "a checksum is stable")
check(#H.Checksum("anything") >= 4, "a checksum is at least four characters", H.Checksum("anything"))
check(H.Checksum("a") ~= H.Checksum("b"), "different payloads checksum differently")

H.settings.snapEnabled = true
H.settings.grid = 0.5
check(H.SnapValue(1.24) == 1.0, "a yard snaps to the nearest half yard", H.SnapValue(1.24))
check(H.SnapValue(-1.24) == -1.0, "snapping works below zero", H.SnapValue(-1.24))
H.settings.angle = math.pi / 12
check(math.abs(H.SnapAngle(math.rad(97)) - math.rad(90)) < 1e-9, "an angle snaps to fifteen degrees",
      math.deg(H.SnapAngle(math.rad(97))))
check(math.abs(H.SnapAngle(math.rad(358)) - math.rad(0)) < 1e-9, "an angle wraps past north",
      math.deg(H.SnapAngle(math.rad(358))))
H.settings.scaleStep = 0.1
check(math.abs(H.SnapScale(1.04) - 1.0) < 1e-9, "a scale snaps to the step", H.SnapScale(1.04))
H.settings.snapEnabled = false
check(H.SnapValue(1.24) == 1.24, "snapping off leaves the value alone", H.SnapValue(1.24))
H.settings.snapEnabled = true

check(H.TiltSuffix(nil, nil) == "", "no tilt is no suffix", H.TiltSuffix(nil, nil))
check(H.TiltSuffix(0.5, 0) == "|0.5000|0.0000", "a tilt is two four-decimal fields",
      H.TiltSuffix(0.5, 0))
check(H.TiltSuffix(0, nil) == "|0.0000|0.0000", "half a tilt is still a full pair",
      H.TiltSuffix(0, nil))

check(H.AudienceMask({ anyone = true, guests = true }) == 0x21, "the audience mask adds its bits",
      H.AudienceMask({ anyone = true, guests = true }))
check(H.AudienceMask({}) == 0, "an empty audience is no bits", H.AudienceMask({}))

local ok, detail = H.ValidateBlueprint("PHP1:0000000000abcd")
check(ok == true, "a well formed envelope passes", detail)
local bad = {
    { "0000000000abcd", "no prefix" },
    { "PHP1:zzz", "non base62" },
    { "PHP1:0000000000", "short payload" },
    { "PHP1:" .. string.rep("0", 16385), "over-long payload" },
    { "", "empty" },
}
for _, case in ipairs(bad) do
    local accepted, reason = H.ValidateBlueprint(case[1])
    check(accepted == false, "the envelope rejects " .. case[2], reason)
end

-- ---------------------------------------------------------------------------
-- Settings
-- ---------------------------------------------------------------------------

H.settings.frameScale = 1.3
local value = H.StepSetting("frameScale", 1)
check(value == 1.3, "a step past the limit clamps", value)
H.settings.frameScale = 0.8
value = H.StepSetting("frameScale", -1)
check(value == 0.8, "a step below the limit clamps", value)
H.settings.grid = 2.0
H.StepSetting("grid", 1)
check(H.settings.grid == 2.0, "the grid stops at two yards", H.settings.grid)
check(type(env.HousingUIDB) == "table" and env.HousingUIDB.settings ~= nil,
      "settings are written to the saved variable")
check(env.HousingUIDB.settings.grid == 2.0, "the saved copy is the live one")
H.ResetSettings()
check(H.settings.grid == 0.5 and H.settings.snapEnabled == true, "reset restores the defaults",
      tostring(H.settings.grid))
check(H.StepSetting("noSuchSetting", 1) == nil, "stepping an unknown setting does nothing")

-- The window position is a setting too, and it is a table of four values.
H.settings.framePoint = { "TOPLEFT", "TOPLEFT", 100, -80 }
H.SaveSettings()
check(env.HousingUIDB.settings.framePoint[1] == "TOPLEFT" and
      env.HousingUIDB.settings.framePoint[3] == 100, "the window position is saved")

-- ---------------------------------------------------------------------------
-- The window: every tab, every button, every edit box
-- ---------------------------------------------------------------------------

local menu = W.byName["PsychoHouseMenuFrame"]
local panes = H.Menu.panes

local tabs = {}
for id in pairs(panes) do
    tabs[#tabs + 1] = id
end
table.sort(tabs)
check(#tabs == 15, "the window has fifteen tabs", #tabs)

-- Fill everything the tabs draw from, in the documented order.
W.Deliver("S:HELLO|3|1041|1|1")
for _, entry in ipairs(PLAN) do
    pcall(W.Deliver, entry.line)
end
for index = 1, 4 do
    W.Deliver("S:PLOT|" .. index .. "|1|2|4|Thordekk|")
end
W.Deliver("S:PLACEDEND")
W.Deliver("S:DECOREND|0|0")
W.Deliver("S:ROOMS|6")
W.Deliver("S:STORAGE|1500|2")
W.Deliver("S:STORAGEEND")
W.Deliver("S:GUEST|Alt|7")
W.Deliver("S:GUESTEND")
W.Deliver("S:HUNTEND")
W.Deliver("S:ENDTASKEND")
W.Deliver("S:DYEEND")
W.Deliver("S:SHELLEND")
W.Deliver("S:PORTALEND")
W.Deliver("S:HOMESEND|1")
W.Deliver("S:VAREA|1|Forge|10|20|1")
W.Deliver("S:VAREAEND")
W.Deliver("S:RESULT|BUY|0|44")

local clickErrors, editorErrors, clicked, edited = {}, {}, 0, 0

-- Type into every edit box in a pane before clicking: a player who renames a house has typed
-- the name, and a sweep that leaves every box empty only ever exercises the empty path.
local function FillBoxes(frame)
    local state = frame.__state
    if frame.__kind == "EditBox" then
        state.text = "7"
    end
    for _, child in ipairs(state.children or {}) do
        FillBoxes(child)
    end
end

local function ClickAll(frame, tabId)
    local state = frame.__state
    if state.scripts and state.scripts.OnClick then
        clicked = clicked + 1
        local ok, err = pcall(state.scripts.OnClick, frame, "LeftButton")
        if not ok then
            local label = (state.text and tostring(state.text)) or state.name or frame.__kind
            clickErrors[#clickErrors + 1] = tabId .. "/" .. label .. ": " .. tostring(err)
        end
    end

    if state.scripts and state.scripts.OnEnterPressed and frame.__kind == "EditBox" then
        edited = edited + 1
        local ok, err = pcall(state.scripts.OnEnterPressed, frame)
        if not ok then
            editorErrors[#editorErrors + 1] = tabId .. "/" .. frame.__kind .. ": " .. tostring(err)
        end
    end

    for _, child in ipairs(state.children or {}) do
        ClickAll(child, tabId)
    end
end

local tabshowErrors = {}
local burstErrors = {}
for _, id in ipairs(tabs) do
    local ok, err = pcall(H.Menu.ShowTab, id)
    if not ok then
        tabshowErrors[#tabshowErrors + 1] = id .. ": " .. tostring(err)
    end

    -- Answer the tab the way the server would: opening a tab asks again, so the burst is
    -- replayed after every ShowTab. A sweep that only delivered once would be clicking at
    -- lists the tab itself had just cleared.
    local function Deliver(line)
        local sent, err = pcall(W.Deliver, line)
        if not sent then
            burstErrors[#burstErrors + 1] = id .. " " .. line:sub(1, 24) .. ": " .. tostring(err)
        end
    end

    for _, entry in ipairs(PLAN) do
        Deliver(entry.line)
    end
    for index = 1, 4 do
        Deliver("S:PLOT|" .. index .. "|1|2|4|Thordekk|")
    end
    Deliver("S:PLACEDENTRY|44|1500|1.5|2.5|0.25|90|1.0|1|0.1|-0.2|311|1")
    Deliver("S:ROOMPLACED|4|2|-1|0")
    Deliver("S:PLACEDEND")
    Deliver("S:DECOREND|0|0")
    Deliver("S:ROOMS|6")
    Deliver("S:STORAGE|1500|2")
    Deliver("S:STORAGEEND")
    Deliver("S:GUEST|Alt|7")
    Deliver("S:GUESTEND")
    Deliver("S:HUNTEND")
    Deliver("S:ENDTASKEND")
    Deliver("S:DYEEND")
    Deliver("S:SHELLEND")
    Deliver("S:PORTALEND")
    Deliver("S:HOMES|77|3|12|Stormhold|0|2")
    Deliver("S:HOMESEND|1")
    Deliver("S:VAREA|1|Forge|10|20|1")
    Deliver("S:VAREAEND")

    -- A player who has selected a piece before reaching for the pet buttons.
    H.Menu.selectedPlacement = 44
    FillBoxes(panes[id])
    ClickAll(panes[id], id)
end

-- One pane at a time, and the rail is what swaps them.
local tabClickErrors = {}
for _, id in ipairs(tabs) do
    local button = H.Menu.tabButtons[id]
    if not button then
        tabClickErrors[#tabClickErrors + 1] = id .. " has no tab button"
    else
        local ok, err = pcall(button.Click, button)
        if not ok then
            tabClickErrors[#tabClickErrors + 1] = id .. ": " .. tostring(err)
        elseif H.Menu.current ~= id then
            tabClickErrors[#tabClickErrors + 1] = id .. " clicked but the menu shows " .. tostring(H.Menu.current)
        else
            local shown = 0
            for key, region in pairs(panes) do
                if region:IsShown() then
                    shown = shown + 1
                    if key ~= id then
                        tabClickErrors[#tabClickErrors + 1] = id .. " showed " .. key .. " as well"
                    end
                end
            end
            if shown ~= 1 then
                tabClickErrors[#tabClickErrors + 1] = id .. " has " .. shown .. " panes showing"
            end
        end
    end
end

check(#tabClickErrors == 0, "the rail switches one pane at a time", table.concat(tabClickErrors, "; "))
check(#tabshowErrors == 0, "every tab opens", table.concat(tabshowErrors, "; "))
check(#burstErrors == 0, "every tab survives the server answering it",
      table.concat(burstErrors, "; "))
check(#clickErrors == 0, "every button survives a click with data loaded",
      table.concat(clickErrors, "; "))
check(clicked > 60, "the sweep clicked the buttons", clicked .. " buttons")
check(#editorErrors == 0, "every edit box survives typed input with data loaded",
      table.concat(editorErrors, "; "))

-- The same sweep with nothing loaded: a client that opens the window before the server
-- answers must not draw an error.
local emptyErrors = {}
local freshEnv = W.BuildEnv()
pcall(W.LoadFile, freshEnv, ADDON_DIR .. "/Core.lua", "HousingUI")
pcall(W.LoadFile, freshEnv, ADDON_DIR .. "/Menu.lua", "HousingUI")
local freshH = freshEnv.HousingUI
if freshH and freshH.Menu then
    for id in pairs(freshH.Menu.panes) do
        local ok, err = pcall(freshH.Menu.ShowTab, id)
        if not ok then
            emptyErrors[#emptyErrors + 1] = id .. ": " .. tostring(err)
        end
        local ok2, err2 = pcall(ClickAll, freshH.Menu.panes[id], "empty/" .. id)
        if not ok2 then
            emptyErrors[#emptyErrors + 1] = "empty/" .. id .. ": " .. tostring(err2)
        end
    end
else
    emptyErrors[#emptyErrors + 1] = "the addon did not load twice"
end

check(#emptyErrors == 0, "the window opens and clicks with no data from the server",
      table.concat(emptyErrors, "; "))

-- ---------------------------------------------------------------------------
-- The .toc
-- ---------------------------------------------------------------------------

local toc = io.open(ADDON_DIR .. "/HousingUI.toc", "r")
local tocText = toc:read("*a")
toc:close()

check(tocText:match("## Interface:%s*80300"), "the .toc targets 8.3.7 (80300)")
check(tocText:match("## SavedVariables:%s*HousingUIDB") ~= nil, "the .toc declares HousingUIDB")
check(tocText:match("## Version:%s*0%.10%.1") ~= nil, "the .toc version matches the code")

local declared = {}
for line in tocText:gmatch("[^\r\n]+") do
    local file = line:match("^(%S+%.lua)$")
    if file then
        declared[#declared + 1] = file
    end
end
check(#declared >= 2 and declared[1] == "Core.lua" and declared[2] == "Menu.lua",
      "the .toc loads Core before Menu", table.concat(declared, ", "))

local handle = io.open(ADDON_DIR .. "/Bindings.xml", "r")
local bindings = handle:read("*a")
handle:close()
check(bindings:match('name="CLICK PsychoHouseToggleButton:LeftButton"') ~= nil,
      "the binding names the button the code creates")
check(W.byName["PsychoHouseToggleButton"] ~= nil, "that button exists")
check(W.byName["PsychoHouseLauncher"] ~= nil, "the minimap launcher exists")
check(env.BINDING_HEADER_PSYCHOHOUSE == "PsychoHouse", "the binding header is named")

-- Every opcode the run emitted, from slash commands and from the window's own buttons.
local sentOps, sentList = {}, {}
for _, frame in ipairs(W.allSent) do
    local op = type(frame.text) == "string" and frame.text:match("^C:([A-Z]+)")
    if op and not sentOps[op] then
        sentOps[op] = true
        sentList[#sentList + 1] = op
    end
end
table.sort(sentList)
print("SENT " .. table.concat(sentList, " "))

print(("%d checks passed, %d finding(s)"):format(passes, failures))
os.exit(failures == 0 and 0 or 1)
