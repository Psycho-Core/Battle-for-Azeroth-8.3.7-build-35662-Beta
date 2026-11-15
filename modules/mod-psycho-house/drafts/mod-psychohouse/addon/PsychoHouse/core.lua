-- PsychoHouse - the wire.
--
-- One prefix, ten records out, seven records back. Everything the window shows comes from
-- here: nothing is cached between sessions except the last window position, because a house
-- can change while the addon is not looking (a sale, a reload, a piece removed from another
-- client) and a stale object list is worse than no object list.
--
-- Messages are sent as whispers to the player themself. That is the only channel a realm can
-- guarantee is neither moderated nor visible, and the server answers on the same channel.

local ADDON_NAME = "PsychoHouse"
local PREFIX = "PHSH"

PsychoHouse = PsychoHouse or {}
local PH = PsychoHouse

PH.protocol = 1
PH.limit = 0
PH.hasHouse = false
PH.phase = 0
PH.bought = 0
PH.facilities = {}
PH.objects = {}
PH.results = {}

-- The result codes are the module's HouseResult enum, in order. The window shows the sentence
-- rather than the number so a player does not have to read a table to know what happened.
local RESULT_TEXT = {
	[0]  = "done",
	[1]  = "housing is switched off on this realm",
	[2]  = "no house on this character yet",
	[3]  = "you already have a house",
	[4]  = "not enough gold",
	[5]  = "no free phase left on this realm",
	[6]  = "you have to be inside your house",
	[7]  = "no such facility",
	[8]  = "already bought",
	[9]  = "this facility is not for your faction",
	[10] = "level too low",
	[11] = "storage is full",
	[12] = "that did not work",
	[13] = "the house is full - remove something first",
	[14] = "that is not a placeable object",
	[15] = "too far from the house",
	[16] = "no object in that slot",
}

function PH.ResultText(code)
	return RESULT_TEXT[code] or ("unknown result " .. tostring(code))
end

local listeners = {}

function PH.On(event, fn)
	listeners[event] = listeners[event] or {}
	table.insert(listeners[event], fn)
end

local function notify(event, ...)
	for _, fn in ipairs(listeners[event] or {}) do
		fn(...)
	end
end

function PH.Send(message)
	if not C_ChatInfo or not C_ChatInfo.SendAddonMessage then
		return
	end

	C_ChatInfo.SendAddonMessage(PREFIX, message, "WHISPER", UnitName("player"))
end

function PH.Request(what)
	PH.Send(what)
end

-- One record per field group, pipe separated. Split is bounded: a record from a different
-- protocol version could carry more fields than this one expects, and the extra ones are
-- ignored rather than shifting every later parse.
local function fields(text)
	local out = {}
	local current = {}

	-- Written as a loop rather than `gmatch("([^|]*)")`: an empty match between two pipes is a
	-- record too, and gmatch yields one extra empty field at every separator, which shifts
	-- every number one place to the right.
	for index = 1, #text do
		local character = string.sub(text, index, index)

		if character == "|" then
			table.insert(out, table.concat(current))
			current = {}
		else
			table.insert(current, character)
		end
	end

	table.insert(out, table.concat(current))
	return out
end

local function num(text)
	return tonumber(text) or 0
end

local handlers = {}

handlers["S:HELLO"] = function(f)
	PH.protocol = num(f[2])
	PH.limit = num(f[3])
	PH.hasHouse = num(f[4]) == 1
	notify("hello")
end

handlers["S:STATUS"] = function(f)
	PH.hasHouse = num(f[2]) == 1
	PH.phase = num(f[3])
	PH.bought = num(f[4])
	PH.facilityCount = num(f[5])
	PH.objectCount = num(f[6])
	PH.limit = num(f[7])
	PH.price = num(f[8])
	notify("status")
end

handlers["S:FAC"] = function(f)
	table.insert(PH.facilities, {
		id = num(f[2]),
		kind = num(f[3]),
		name = f[4] or "",
		price = num(f[5]),
		owned = num(f[6]) == 1,
		entry = num(f[7]),
		isObject = num(f[8]) == 1,
	})
end

handlers["S:FACEND"] = function(f)
	notify("facilities")
end

handlers["S:OBJ"] = function(f)
	table.insert(PH.objects, {
		slot = num(f[2]),
		entry = num(f[3]),
		x = num(f[4]),
		y = num(f[5]),
		z = num(f[6]),
		o = num(f[7]),
		scale = num(f[8]),
	})
end

handlers["S:OBJEND"] = function(f)
	PH.objectCount = num(f[2])
	PH.limit = num(f[3])
	notify("objects")
end

handlers["S:RESULT"] = function(f)
	local op, code = f[2] or "?", num(f[3])
	PH.results[op] = code
	notify("result", op, code, f[4])
end

handlers["S:PONG"] = function(f)
	notify("pong", num(f[2]))
end

local frame = CreateFrame("Frame")
frame:RegisterEvent("ADDON_LOADED")
frame:RegisterEvent("PLAYER_LOGIN")
frame:RegisterEvent("CHAT_MSG_ADDON")

frame:SetScript("OnEvent", function(self, event, ...)
	if event == "ADDON_LOADED" then
		local loaded = ...
		if loaded == ADDON_NAME then
			PsychoHouseDB = PsychoHouseDB or {}
		end
		return
	end

	if event == "PLAYER_LOGIN" then
		if C_ChatInfo and C_ChatInfo.RegisterAddonMessagePrefix then
			C_ChatInfo.RegisterAddonMessagePrefix(PREFIX)
		end
		PH.Send("C:HELLO")
		return
	end

	local prefix, text, channel, sender = ...
	if prefix ~= PREFIX or sender ~= UnitName("player") or not text then
		return
	end

	local f = fields(text)
	local handler = handlers[f[1]]

	if handler then
		handler(f)
	end
end)

-- Refresh is not automatic: the window asks when it is shown, and again after every action
-- that could have changed the answer. Pulling on a timer would put a whisper on the wire every
-- few seconds for a window that is usually closed.
function PH.Refresh()
	-- Emptied before asking, not after the answer: the two lists arrive as a run of records
	-- and a refresh that started from a stale list would double every row.
	PH.facilities = {}
	PH.objects = {}

	PH.Send("C:STATUS")
	PH.Send("C:FACILITIES")
	PH.Send("C:OBJECTS")
end

function PH.Place(entry)
	PH.Send("C:PLACEHERE|" .. tostring(math.floor(tonumber(entry) or 0)))
end

function PH.BuyFacility(id)
	PH.Send("C:BUYFAC|" .. tostring(math.floor(tonumber(id) or 0)))
end

function PH.Remove(slot)
	PH.Send("C:REMOVE|" .. tostring(math.floor(tonumber(slot) or 0)))
end

function PH.Buy()
	PH.Send("C:BUY")
end

function PH.Sell()
	PH.Send("C:SELL")
end

function PH.GoHome()
	PH.Send("C:HOME")
end

-- Called by every result, so an action refreshes the window without the panel having to know
-- which record changed the state.
PH.On("result", function()
	PH.Refresh()
end)
