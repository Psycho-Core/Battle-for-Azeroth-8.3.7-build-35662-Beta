--[[
    HousingUI - protocol harness for the housing module.

    This is not the shipped interface. It is the transport, the parser and a set of slash
    commands I use to prove the server half of the BFAH protocol before anyone spends time
    on frames. Everything here talks to the server; nothing here decides anything.

    Protocol reference: legacy-prototype/04-addon-protocol.md
--]]

local ADDON_NAME = ...
local PREFIX = "BFAH"
local CHUNK_TIMEOUT = 10

HousingUI = HousingUI or {}
local H = HousingUI

H.version = "0.10.0"
H.log = H.log or {}
H.plots = H.plots or {}
H.neighbourhood = nil
H.placed = H.placed or {}
H.catalogue = H.catalogue or {}
H.lastResult = nil

local chunks = {}

local function Print(message)
    DEFAULT_CHAT_FRAME:AddMessage("|cff66c2ffHousing|r " .. tostring(message))
end

H.Print = Print

local function RegisterPrefix()
    if C_ChatInfo and C_ChatInfo.RegisterAddonMessagePrefix then
        C_ChatInfo.RegisterAddonMessagePrefix(PREFIX)
    elseif RegisterAddonMessagePrefix then
        RegisterAddonMessagePrefix(PREFIX)
    end
end

local function Send(payload)
    if C_ChatInfo and C_ChatInfo.SendAddonMessage then
        C_ChatInfo.SendAddonMessage(PREFIX, payload, "WHISPER", UnitName("player"))
    elseif SendAddonMessage then
        SendAddonMessage(PREFIX, payload, "WHISPER", UnitName("player"))
    end
end

H.Send = Send

-- ---------------------------------------------------------------------------
-- Framing
-- ---------------------------------------------------------------------------

local function SplitList(payload)
    local out = {}
    for field in string.gmatch(payload, "([^|]*)|?") do
        out[#out + 1] = field
    end
    return out
end

H.Split = SplitList

local function HandlePayload(payload)
    -- Reassembly is plain concatenation, so a chunked payload is the same text.
    local records = {}
    for record in string.gmatch(payload .. ";", "([^;]*);") do
        if record ~= "" then
            records[#records + 1] = record
        end
    end

    for _, record in ipairs(records) do
        local fields = SplitList(record)
        local head = fields[1] or ""

        local opcode = string.match(head, "^S:(.+)$")
        if opcode then
            H.log[#H.log + 1] = record
            if #H.log > 400 then
                table.remove(H.log, 1)
            end

            if H.handlers[opcode] then
                H.handlers[opcode](fields)
            else
                Print("unhandled opcode " .. opcode .. ": " .. record)
            end
        end
    end
end

local function OnAddonMessage(prefix, text, channel, sender)
    if prefix ~= PREFIX then
        return
    end

    local frameId, seq, total, fragment = string.match(text, "^S:CHUNK|(%d+)|(%d+)|(%d+)|(.*)$")
    if frameId then
        local buffer = chunks[frameId]
        if not buffer or buffer.total ~= tonumber(total) or buffer.next ~= tonumber(seq) then
            buffer = { total = tonumber(total), next = 1, parts = {}, started = GetTime() }
            chunks[frameId] = buffer
        end

        buffer.parts[#buffer.parts + 1] = fragment
        buffer.next = buffer.next + 1

        if buffer.next > buffer.total then
            local payload = table.concat(buffer.parts)
            chunks[frameId] = nil
            HandlePayload(payload)
        end

        return
    end

    HandlePayload(text)
end

-- ---------------------------------------------------------------------------
-- Opcode handlers
-- ---------------------------------------------------------------------------

H.handlers = {}

H.handlers["HELLO"] = function(fields)
    H.protocol = tonumber(fields[2])
    H.serverVersion = tonumber(fields[3])
    H.hasHouse = fields[5] == "1"

    if H.protocol ~= 3 then
        Print("|cffff5555protocol mismatch|r (server " .. tostring(H.protocol) .. ", addon 3)")
    else
        Print("connected to housing module v" .. tostring(H.serverVersion))
    end
end

H.handlers["PONG"] = function(fields)
    Print("pong " .. tostring(fields[2]))
end

H.handlers["NBR"] = function(fields)
    H.neighbourhood = {
        id = tonumber(fields[2]),
        type = tonumber(fields[3]),
        name = fields[4],
        slot = tonumber(fields[5]),
        owned = tonumber(fields[6]),
        total = tonumber(fields[7]),
        faction = tonumber(fields[8]),
    }

    Print("neighbourhood '" .. H.neighbourhood.name .. "' " .. H.neighbourhood.owned ..
        "/" .. H.neighbourhood.total .. " plots owned, slot " .. H.neighbourhood.slot)
end

H.handlers["PLOT"] = function(fields)
    local index = tonumber(fields[2])
    H.plots[index] = {
        index = index,
        biome = tonumber(fields[3]),
        state = tonumber(fields[4]),
        tier = tonumber(fields[5]),
        owner = fields[6],
    }
end

H.handlers["PLOTINFO"] = function(fields)
    Print(string.format("plot %s: biome %s, price %sg, state %s, tier %s, owner '%s', name '%s', budget %s/%s in, %s/%s out",
        tostring(fields[2]), tostring(fields[3]), tostring(fields[4]), tostring(fields[6]),
        tostring(fields[7]), tostring(fields[8]), tostring(fields[9]),
        tostring(fields[13]), tostring(fields[11]), tostring(fields[14]), tostring(fields[12])))

    -- The two permission masks, kept so the House tab draws what the server holds rather than
    -- its own idea of it.
    H.plotMasks = {
        plot = tonumber(fields[15]) or 0,
        house = tonumber(fields[16]) or 0,
    }

    if H.OnPlotMasks then
        H.OnPlotMasks()
    end
end

-- Audience bits, in the order the server numbers them. Kept here rather than in Menu.lua so
-- one table feeds the print, the checkboxes and the command.
H.AUDIENCES = {
    { bit = 0x01, key = "anyone",    label = "Anyone" },
    { bit = 0x02, key = "neighbors", label = "Neighbours" },
    { bit = 0x04, key = "guild",     label = "Guild" },
    { bit = 0x08, key = "party",     label = "Party" },
    { bit = 0x10, key = "friends",   label = "Friends" },
    { bit = 0x20, key = "guests",    label = "Guests" },
}

function H.AudienceMask(names)
    local mask = 0

    for _, audience in ipairs(H.AUDIENCES) do
        if names[audience.key] then
            mask = mask + audience.bit
        end
    end

    return mask
end

H.handlers["RESULT"] = function(fields)
    H.lastResult = { op = fields[2], code = tonumber(fields[3]), extra = fields[4] }
    Print("|cffffcc00" .. tostring(fields[2]) .. "|r -> " .. tostring(fields[3]) ..
        (fields[4] and (" (" .. fields[4] .. ")") or ""))
end

H.handlers["HOME"] = function(fields)
    if fields[2] ~= "0" then
        Print("homestone failed: " .. tostring(fields[2]) .. ", " .. tostring(fields[3]) .. "s remaining")
    else
        Print("homestone used")
    end
end

H.handlers["DECORENTRY"] = function(fields)
    local id = tonumber(fields[2])
    H.catalogue[id] = {
        id = id, name = fields[3], cost = tonumber(fields[4]), placement = tonumber(fields[5]),
        surface = tonumber(fields[6]), unlocked = fields[7] == "1",
        price = tonumber(fields[8]), currency = tonumber(fields[9]),
        subcategory = fields[10],
    }
end

H.handlers["DECOREND"] = function(fields)
    local count = 0
    for _ in pairs(H.catalogue) do count = count + 1 end
    Print("catalogue page " .. tostring(fields[2]) .. " (" .. count .. " known), more: " .. tostring(fields[3]))
end

H.handlers["PLACEDENTRY"] = function(fields)
    local id = tonumber(fields[2])
    H.placed[id] = {
        id = id, decorId = tonumber(fields[3]),
        x = tonumber(fields[4]), y = tonumber(fields[5]), z = tonumber(fields[6]),
        o = tonumber(fields[7]), scale = tonumber(fields[8]), indoor = fields[9] == "1",
        pitch = tonumber(fields[10]) or 0, roll = tonumber(fields[11]) or 0,
        petEntry = tonumber(fields[12]) or 0, petRoaming = fields[13] == "1",
    }
end

H.handlers["PLACEDEND"] = function()
    local count = 0
    for _ in pairs(H.placed) do count = count + 1 end
    Print("placed decor: " .. count .. " pieces")
end

H.handlers["DIRENTRY"] = function(fields)
    Print("  plot " .. tostring(fields[2]) .. " - " .. tostring(fields[3]) ..
        " (tier " .. tostring(fields[4]) .. (fields[5] == "1" and ", locked" or "") .. ")")
end

H.handlers["DIREND"] = function(fields)
    Print("directory page " .. tostring(fields[2]) .. " done (more: " .. tostring(fields[3]) .. ")")
end

H.handlers["BUYRESULT"] = function(fields)
    Print("buy decor -> " .. tostring(fields[2]) .. " (decor " .. tostring(fields[3]) .. ")")
end

H.handlers["GUEST"] = function(fields)
    Print("  guest " .. tostring(fields[2]) .. " flags " .. tostring(fields[3]))
end

H.handlers["GUESTEND"] = function()
    Print("guest list done")
end

H.handlers["VAREA"] = function(fields)
    Print("  amenity " .. tostring(fields[3]) .. " type " .. tostring(fields[2]) ..
        " at " .. tostring(fields[4]) .. ", " .. tostring(fields[5]))
end

H.handlers["VAREAEND"] = function()
    Print("amenity list done")
end

-- ---- finder, visiting, storage, progression, Endeavors, codes -----------------

H.finder = {}
H.rooms = {}
H.dyesReady = false
H.roomsOwned = 0
H.nightMode = false
H.gryphon = 0
H.storage = {}
H.shells = {}
H.portals = {}
H.endeavor = {}
H.tasks = {}
H.hunts = {}

H.handlers["FINDER"] = function(fields)
    H.finder[#H.finder + 1] = {
        id = tonumber(fields[2]), name = fields[3],
        owned = tonumber(fields[4]), total = tonumber(fields[5]),
    }
end

H.handlers["FINDEREND"] = function()
    Print("public neighbourhoods: " .. #H.finder .. " (/housing visit <id> <plot>)")
end

H.handlers["STORAGE"] = function(fields)
    H.storage[tonumber(fields[2])] = tonumber(fields[3])
end

H.handlers["STORAGEEND"] = function()
    local total = 0
    for _, count in pairs(H.storage) do total = total + count end
    Print("storage: " .. total .. " copies")
end

H.handlers["SHELL"] = function(fields)
    H.shells[tonumber(fields[2])] = {
        name = fields[3], size = tonumber(fields[4]), level = tonumber(fields[5]),
    }
end

H.handlers["SHELLEND"] = function()
    local count = 0
    for _ in pairs(H.shells) do count = count + 1 end
    Print("shells: " .. count .. " (/housing shell <id>)")
end

H.handlers["PORTAL"] = function(fields)
    H.portals[tonumber(fields[2])] = { name = fields[3], level = tonumber(fields[4]) }
end

H.handlers["PORTALEND"] = function()
    local count = 0
    for _ in pairs(H.portals) do count = count + 1 end
    Print("portal destinations: " .. count)
end

H.handlers["ENDEAVOR"] = function(fields)
    H.endeavor = {
        neighborhood = tonumber(fields[2]), id = tonumber(fields[3]),
        milestone = tonumber(fields[4]), done = tonumber(fields[5]),
        name = fields[6], ends = tonumber(fields[7]) or 0,
    }
    H.tasks = {}
end

H.handlers["ENDTASK"] = function(fields)
    H.tasks[#H.tasks + 1] = {
        id = tonumber(fields[2]), progress = tonumber(fields[3]),
        required = tonumber(fields[4]), text = fields[5],
    }
end

H.handlers["ENDTASKEND"] = function()
    local e = H.endeavor
    Print("endeavor " .. tostring(e.id) .. " milestone " .. tostring(e.milestone) ..
        "/4, " .. #H.tasks .. " tasks")
end

H.handlers["HUNT"] = function(fields)
    H.hunts[#H.hunts + 1] = {
        id = tonumber(fields[2]), state = tonumber(fields[3]),
        name = fields[4], riddle = fields[5], reward = fields[6],
        quest = tonumber(fields[7]),
    }
end

H.handlers["HUNTEND"] = function()
    Print("riddle hunts: " .. #H.hunts .. " (/house, Hunts tab)")
end

H.handlers["PLACEAT"] = function(fields)
    if fields[3] == "0" then
        Print("placed in front of you (placement " .. tostring(fields[4]) .. ")")
    else
        Print("|cffff5555could not place: code " .. tostring(fields[3]) .. "|r")
    end
end

H.selectedPlacement = nil

-- ---------------------------------------------------------------------------
-- Grid snapping
--
-- Basic mode is a client-side convenience: the server takes whatever numbers it is given and
-- clamps them to the plot, so snapping costs nothing on the server side and can be turned off
-- without the server knowing.
-- ---------------------------------------------------------------------------

-- Client settings.
--
-- The server has a conf file; this is the client's equivalent, and for the same reason: a
-- preference that is retyped every session is not a preference. Everything here lives in
-- HousingUIDB, which the .toc already declares, and one table holds it so there is one place
-- to read and one place to write.
--
-- There is deliberately no second copy: SnapValue reads H.settings directly rather than a
-- mirrored H.snap table, because two copies of one setting is how a UI ends up showing a
-- grid size the code is not using.
local DEFAULTS = {
    snapEnabled = true,
    grid = 0.5,                 -- yards
    angle = math.pi / 12,       -- 15 degrees
    scaleStep = 0.1,
    frameScale = 1.0,
    framePoint = nil,           -- {"TOPLEFT", x, y} once the window has been moved
}

local LIMITS = {
    grid = { 0.25, 2.0, 0.25 },
    angle = { math.pi / 36, math.pi / 4, math.pi / 36 },
    scaleStep = { 0.05, 0.5, 0.05 },
    frameScale = { 0.8, 1.3, 0.05 },
}

H.settings = {}

function H.LoadSettings()
    HousingUIDB = HousingUIDB or {}
    local stored = HousingUIDB.settings or {}

    for key, value in pairs(DEFAULTS) do
        if stored[key] == nil then
            H.settings[key] = value
        else
            H.settings[key] = stored[key]
        end
    end
end

function H.SaveSettings()
    HousingUIDB = HousingUIDB or {}
    HousingUIDB.settings = H.settings
end

function H.ResetSettings()
    for key, value in pairs(DEFAULTS) do
        H.settings[key] = value
    end
    H.SaveSettings()
end

-- Nudge a bounded setting by one step and clamp it. Returning the new value keeps the menu
-- from having to know the limits.
function H.StepSetting(key, direction)
    local limit = LIMITS[key]
    if not limit then
        return H.settings[key]
    end

    local low, high, step = limit[1], limit[2], limit[3]
    local value = (H.settings[key] or low) + (step * direction)
    value = math.max(low, math.min(high, value))
    H.settings[key] = value
    H.SaveSettings()
    return value
end

H.LoadSettings()

function H.SnapValue(value)
    if not H.settings.snapEnabled then
        return value
    end
    return math.floor(value / H.settings.grid + 0.5) * H.settings.grid
end

function H.SnapAngle(value)
    if not H.settings.snapEnabled then
        return value
    end
    local turn = math.pi * 2
    local wrapped = value % turn
    return math.floor(wrapped / H.settings.angle + 0.5) * H.settings.angle
end

function H.SnapScale(value)
    if not H.settings.snapEnabled then
        return value
    end
    return math.floor(value / H.settings.scaleStep + 0.5) * H.settings.scaleStep
end

-- Tilt is two optional trailing fields, and only sent when the player asked for one: an
-- empty suffix leaves the line exactly as long as it was before the addon knew about tilt.
function H.TiltSuffix(pitch, roll)
    local pitchValue = tonumber(pitch)
    local rollValue = tonumber(roll)

    if not pitchValue and not rollValue then
        return ""
    end

    return string.format("|%.4f|%.4f", pitchValue or 0, rollValue or 0)
end

local TiltSuffix = H.TiltSuffix

H.dyes = H.dyes or {}

H.handlers["DYE"] = function(fields)
    H.dyes[#H.dyes + 1] = { id = tonumber(fields[2]), name = fields[3] }
end

H.handlers["DYEEND"] = function()
    H.dyesReady = true
end

H.handlers["ROOM"] = function(fields)
    H.rooms[#H.rooms + 1] = {
        id = tonumber(fields[2]), name = fields[3], cost = tonumber(fields[4]),
        level = tonumber(fields[5]), state = tonumber(fields[6]),
    }
end

H.handlers["ROOMS"] = function(fields)
    H.roomsOwned = tonumber(fields[2])
end

H.handlers["BPCODE"] = function(fields)
    H.lastCode = fields[2]
    Print("build code: " .. tostring(fields[2]))
end

-- ---------------------------------------------------------------------------
-- Blueprint codes (doc 06 sec.3)
--
--   PHP1:<base62 payload>
--
-- The payload is a byte string: version, kind, faction, uint16 count, then a record per
-- piece, with a checksum byte at the end. The server both makes and reads it
-- (`C:BPEXP` / `C:BPIMP`), so the addon never decodes one - it checks the envelope, shows
-- the length, and lets the server accept or reject the contents. That is the only split
-- that cannot drift: one implementation of the format, on the side that owns the data.
-- ---------------------------------------------------------------------------

local CRC_TABLE = nil

local function BuildCrcTable()
    CRC_TABLE = {}
    for index = 0, 255 do
        local value = index
        for _ = 1, 8 do
            if bit.band(value, 1) == 1 then
                value = bit.bxor(bit.rshift(value, 1), 0xEDB88320)
            else
                value = bit.rshift(value, 1)
            end
        end
        CRC_TABLE[index] = value
    end
end

function H.Crc32(text)
    if not CRC_TABLE then
        BuildCrcTable()
    end

    local crc = 0xFFFFFFFF
    for index = 1, string.len(text) do
        local byte = string.byte(text, index)
        crc = bit.bxor(bit.rshift(crc, 8), CRC_TABLE[bit.band(bit.bxor(crc, byte), 0xFF)])
    end

    crc = bit.bxor(crc, 0xFFFFFFFF)
    return crc % 4294967296
end

local BASE62 = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz"

function H.Base62(value, width)
    local out = ""
    repeat
        local digit = value % 62
        out = string.sub(BASE62, digit + 1, digit + 1) .. out
        value = math.floor(value / 62)
    until value == 0

    while string.len(out) < (width or 0) do
        out = "0" .. out
    end
    return out
end

function H.Checksum(payload)
    return H.Base62(H.Crc32(payload), 4)
end

local BLUEPRINT_KINDS = { "build", "room", "exterior" }

-- Envelope only: the header, the base62 alphabet, and a length that could hold a header and
-- one piece. The server's decoder is the authority on everything past that.
function H.ValidateBlueprint(code)
    local payload = string.match(code or "", "^PHP1%:([0-9A-Za-z]+)$")

    if not payload then
        return false, "not a PHP1 code - the form is PHP1: followed by base62 characters"
    end
    if #payload < 12 then
        return false, "payload is " .. #payload .. " characters, too short to hold a house"
    end
    if #payload > 16384 then
        return false, "payload is " .. #payload .. " characters, longer than any code the server writes"
    end

    return true, "code", "?", payload
end

H.BLUEPRINT_KINDS = BLUEPRINT_KINDS

-- ---------------------------------------------------------------------------
-- Slash commands
-- ---------------------------------------------------------------------------

local function Usage()
    Print("commands: /housing hello|ping|neighbourhood|plots|plot <n>|buy <n>|sell|home|leave|interior <0|1>")
    Print("          /housing dir [page]|decor [category] [page]|buydecor <id>|placed")
    Print("          /housing place <decorId> <x> <y> <z> <o> <scale> <indoor>")
    Print("          /housing move <placementId> <x> <y> <z> <o> <scale>|remove <id>")
    Print("          /housing guests|guest add <name> <flags>|guest del <name>|amenities")
    Print("          /housing bp <code>|crc <text>|raw <payload>|log|clear")
end

SLASH_HOUSING1 = "/housing"
SlashCmdList["HOUSING"] = function(input)
    local command, rest = string.match(input or "", "^(%S*)%s*(.*)$")
    command = string.lower(command or "")

    if command == "" then
        Usage()
    elseif command == "hello" then
        Send("C:HELLO|3")
    elseif command == "ping" then
        Send("C:PING")
    elseif command == "neighbourhood" then
        Send("C:NEIGHBORHOOD")
    elseif command == "plots" then
        H.plots = {}
        Send("C:PLOTS")
    elseif command == "plot" then
        Send("C:PLOT|" .. rest)
    elseif command == "buy" then
        Send("C:BUY|" .. rest)
    elseif command == "sell" then
        Send("C:SELL")
    elseif command == "home" then
        Send("C:HOME")
    elseif command == "leave" then
        Send("C:LEAVE")
    elseif command == "interior" then
        Send("C:INTERIOR|" .. (rest == "" and "1" or rest))
    elseif command == "dir" then
        Send("C:DIR|" .. (rest == "" and "0" or rest))
    elseif command == "decor" then
        H.catalogue = {}
        Send("C:DECOR|" .. rest)
    elseif command == "buydecor" then
        Send("C:BUYDECOR|" .. rest)
    elseif command == "placed" then
        H.placed = {}
        Send("C:PLACED")
    elseif command == "place" then
        local decorId, x, y, z, o, scale, indoor, pitch, roll =
            string.match(rest, "^(%d+)%s+(%S+)%s+(%S+)%s+(%S+)%s+(%S+)%s+(%S+)%s+(%d+)%s*(%S*)%s*(%S*)$")
        if not decorId then
            Print("usage: /housing place <decorId> <x> <y> <z> <o> <scale> <indoor> [pitch] [roll]")
        else
            Send(string.format("C:PLACE|%s|%.3f|%.3f|%.3f|%.3f|%.3f|%s%s",
                decorId, H.SnapValue(tonumber(x)), H.SnapValue(tonumber(y)),
                H.SnapValue(tonumber(z)), H.SnapAngle(tonumber(o)),
                H.SnapScale(tonumber(scale)), indoor, TiltSuffix(pitch, roll)))
        end
    elseif command == "dyes" then
        H.dyes = {}
        Send("C:DYES")
    elseif command == "snap" then
        if rest == "on" or rest == "off" then
            H.settings.snapEnabled = (rest == "on")
            H.SaveSettings()
            Print("grid snapping " .. rest .. ", saved")
        else
            Print("grid snapping is " .. (H.settings.snapEnabled and "on" or "off") ..
                " - /housing snap on | off")
        end
    elseif command == "settings" then
        H.Print(string.format("snapping %s, grid %.2f yd, angle %.1f deg, scale step %.2f, window %.2f",
            H.settings.snapEnabled and "on" or "off", H.settings.grid,
            math.deg(H.settings.angle), H.settings.scaleStep, H.settings.frameScale))
        Print("/house lists the same settings with buttons on the Settings tab")
    elseif command == "move" then
        local id, x, y, z, o, scale, pitch, roll =
            string.match(rest, "^(%d+)%s+(%S+)%s+(%S+)%s+(%S+)%s+(%S+)%s+(%S+)%s*(%S*)%s*(%S*)$")
        if not id then
            Print("usage: /housing move <placementId> <x> <y> <z> <o> <scale> [pitch] [roll]")
        else
            Send(string.format("C:MOVE|%s|%.3f|%.3f|%.3f|%.3f|%.3f%s",
                id, H.SnapValue(tonumber(x)), H.SnapValue(tonumber(y)),
                H.SnapValue(tonumber(z)), H.SnapAngle(tonumber(o)),
                H.SnapScale(tonumber(scale)), TiltSuffix(pitch, roll)))
        end
    elseif command == "remove" then
        Send("C:REMOVE|" .. rest)
    elseif command == "amenities" then
        Send("C:VAREA")
    elseif command == "guests" then
        Send("C:GUESTLIST")
    elseif command == "guest" then
        local sub, name, flags = string.match(rest, "^(%S+)%s+(%S+)%s*(%d*)$")
        if sub == "add" and name then
            Send("C:GUESTADD|" .. name .. "|" .. (flags ~= "" and flags or "1"))
        elseif sub == "del" and name then
            Send("C:GUESTDEL|" .. name)
        else
            Print("usage: /housing guest add <name> <flags> | /housing guest del <name>")
        end
    elseif command == "bp" or command == "blueprint" then
        local ok, reason, payload = H.ValidateBlueprint(rest)
        if ok then
            Print("blueprint envelope ok: " .. #payload .. " characters of payload")
        else
            Print("|cffff5555" .. reason .. "|r")
        end
    elseif command == "finder" then
        H.finder = {}
        Send("C:FINDER")
    elseif command == "visit" then
        local neighborhood, plot = string.match(rest, "^(%d+)%s+(%d+)$")
        if not neighborhood then
            Print("usage: /housing visit <neighbourhood> <plot>")
        else
            Send("C:VISIT|" .. neighborhood .. "|" .. plot)
        end
    elseif command == "found" then
        Send("C:FOUND")
    elseif command == "leavehood" then
        Send("C:LEAVENB")
    elseif command == "movehouse" then
        Send("C:MOVEHOUSE|" .. rest)
    elseif command == "gryphon" then
        Send("C:GRYPHON")
    elseif command == "room" then
        Send("C:ROOM|" .. rest)
    elseif command == "rooms" then
        H.rooms = {}
        Send("C:ROOMS")
    elseif command == "arcane" then
        Send("C:ARCANE")
    elseif command == "armillary" then
        Send("C:ARMILLARY")
    elseif command == "favor" then
        Send("C:FAVOR")
    elseif command == "at" then
        local decorId, metres, indoor = string.match(rest, "^(%d+)%s*(%d*)%s*(%d*)$")
        if not decorId then
            Print("usage: /housing at <decorId> [metres] [indoor]")
        else
            Send("C:PLACEAT|" .. decorId .. "|" .. (metres ~= "" and metres or "2") ..
                "|" .. (indoor ~= "" and indoor or "0"))
        end
    elseif command == "join" then
        Send("C:JOIN|" .. rest)
    elseif command == "reserve" then
        Send("C:RESERVE|" .. rest)
    elseif command == "storage" then
        Send("C:STORAGE")
    elseif command == "buycopy" then
        local decorId, count = string.match(rest, "^(%d+)%s*(%d*)$")
        if not decorId then
            Print("usage: /housing buycopy <decorId> [count]")
        else
            Send("C:BUYCOPY|" .. decorId .. "|" .. (count ~= "" and count or "1"))
        end
    elseif command == "levelup" then
        Send("C:LEVELUP")
    elseif command == "shells" then
        H.shells = {}
        Send("C:SHELLS")
    elseif command == "shell" then
        Send("C:SHELL|" .. rest)
    elseif command == "dye" then
        local placementId, group, colour = string.match(rest, "^(%d+)%s+(%d+)%s+(%d+)$")
        if not placementId then
            Print("usage: /housing dye <placementId> <group> <colour>")
        else
            Send("C:DYE|" .. placementId .. "|" .. group .. "|" .. colour)
        end
    elseif command == "portals" then
        H.portals = {}
        Send("C:PORTALS")
    elseif command == "portal" then
        Send("C:GOPORTAL|" .. rest)
    elseif command == "endeavor" then
        H.tasks = {}
        Send("C:ENDEAVOR")
    elseif command == "deliver" then
        Send("C:DELIVER")
    elseif command == "claim" then
        Send("C:CLAIM|" .. rest)
    elseif command == "export" then
        local kind, name = string.match(rest, "^(%d*)%s*(%S*)$")
        Send("C:BPEXP|" .. (kind ~= "" and kind or "0") .. "|" .. (name ~= "" and name or "build"))
    elseif command == "import" then
        Send("C:BPIMP|" .. rest)
    elseif command == "restore" then
        Send("C:RESTORE")
    elseif command == "build" then
        local on, decorId = string.match(rest, "^(%d*)%s*(%d*)$")
        Send("C:BUILDMODE|" .. (on ~= "" and on or "1") .. "|" .. (decorId ~= "" and decorId or "0"))
    elseif command == "crc" then
        Print("crc32 '" .. rest .. "' = " .. H.Crc32(rest) ..
            " -> " .. H.Base62(H.Crc32(rest), 4))
    elseif command == "raw" then
        Send(rest)
    elseif command == "log" then
        for _, line in ipairs(H.log) do
            Print(line)
        end
        Print(#H.log .. " lines")
    elseif command == "clear" then
        H.log = {}
        Print("log cleared")
    else
        Usage()
    end
end

-- ---------------------------------------------------------------------------
-- Events
-- ---------------------------------------------------------------------------

local frame = CreateFrame("Frame")
frame:RegisterEvent("ADDON_LOADED")
frame:RegisterEvent("CHAT_MSG_ADDON")
frame:RegisterEvent("PLAYER_ENTERING_WORLD")

frame:SetScript("OnEvent", function(_, event, arg1, arg2, arg3, arg4)
    if event == "ADDON_LOADED" and arg1 == ADDON_NAME then
        RegisterPrefix()
        Print("v" .. H.version .. " loaded - /housing hello")
    elseif event == "CHAT_MSG_ADDON" then
        OnAddonMessage(arg1, arg2, arg3, arg4)
    elseif event == "PLAYER_ENTERING_WORLD" then
        Send("C:HELLO|3")
    end
end)

-- Drop stale chunk buffers so a lost message cannot leak memory forever.
local ticker = CreateFrame("Frame")
ticker:SetScript("OnUpdate", function(_, elapsed)
    local now = GetTime()
    for frameId, buffer in pairs(chunks) do
        if (now - buffer.started) > CHUNK_TIMEOUT then
            chunks[frameId] = nil
            Print("dropped incomplete chunk frame " .. tostring(frameId))
        end
    end
end)
