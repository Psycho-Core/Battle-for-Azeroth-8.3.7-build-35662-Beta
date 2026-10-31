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

H.version = "0.3.0"
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

-- ---------------------------------------------------------------------------
-- Blueprint codes (doc 06 sec.3)
--
--   PCH1-<kind>-<faction>-<payload>-<checksum>
--
-- kind is R, B or E; faction is 0, 1 or 2; the payload is base62 text; the checksum is
-- four base62 characters, the CRC32 of the payload. The payload carries no hyphen, so a
-- plain split on "-" is safe.
--
-- There is no blueprint opcode yet - import and export are phase 4 - so this checks a code
-- against the format and nothing else. It is here rather than in the menu because the
-- format is the thing that has to be right before a single code is written.
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

local BLUEPRINT_KINDS = { R = true, B = true, E = true }

function H.ValidateBlueprint(code)
    local kind, faction, payload, checksum =
        string.match(code or "", "^PCH1%-(%a)%-(%d)%-(%w+)%-(%w%w%w%w)$")

    if not kind then
        return false, "not a PCH1 code (PCH1-<kind>-<faction>-<payload>-<checksum>)"
    end
    if not BLUEPRINT_KINDS[kind] then
        return false, "kind '" .. kind .. "' is not R, B or E"
    end
    if faction ~= "0" and faction ~= "1" and faction ~= "2" then
        return false, "faction '" .. faction .. "' is not 0, 1 or 2"
    end
    if H.Checksum(payload) ~= checksum then
        return false, "checksum is " .. H.Checksum(payload) .. ", code says " .. checksum
    end

    return true, kind, faction, payload
end

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
        local decorId, x, y, z, o, scale, indoor =
            string.match(rest, "^(%d+)%s+(%S+)%s+(%S+)%s+(%S+)%s+(%S+)%s+(%S+)%s+(%d+)$")
        if not decorId then
            Print("usage: /housing place <decorId> <x> <y> <z> <o> <scale> <indoor>")
        else
            Send(string.format("C:PLACE|%s|%.3f|%.3f|%.3f|%.3f|%.3f|%s",
                decorId, tonumber(x), tonumber(y), tonumber(z), tonumber(o), tonumber(scale), indoor))
        end
    elseif command == "move" then
        local id, x, y, z, o, scale =
            string.match(rest, "^(%d+)%s+(%S+)%s+(%S+)%s+(%S+)%s+(%S+)%s+(%S+)$")
        if not id then
            Print("usage: /housing move <placementId> <x> <y> <z> <o> <scale>")
        else
            Send(string.format("C:MOVE|%s|%.3f|%.3f|%.3f|%.3f|%.3f",
                id, tonumber(x), tonumber(y), tonumber(z), tonumber(o), tonumber(scale)))
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
        local ok, reason, faction, payload = H.ValidateBlueprint(rest)
        if ok then
            Print("blueprint ok: kind " .. reason .. ", faction " .. faction ..
                ", payload " .. #payload .. " chars")
        else
            Print("|cffff5555" .. reason .. "|r")
        end
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
