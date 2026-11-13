--[[
    wow_env.lua - enough of the 8.3.7 client to run the addon outside a client.

    This is a test instrument, not a shim shipped to players: it builds a fresh global table,
    fills in the parts of the client API the addon touches, and runs HousingUI/Core.lua and
    Menu.lua inside it. Lua 5.1 is the version 8.3.7 uses, so there is no version drift to
    argue about - `bit`, `unpack` and the rest of 5.1 are what the addon expects.

    Two rules keep it honest:

      * an unknown global read is recorded, not invented. A misspelled API call therefore
        fails the way it would in a client ("attempt to call global 'Foo'"), and the list of
        names that came back nil is printed so a typo shows up as a name rather than a shrug.
      * a frame is a table with the methods the addon calls. Methods that no addon code has
        needed yet are still provided as no-ops and recorded, so a new frame method is
        visible in the report instead of crashing the run.
--]]

local M = {}

local function Pack(...)
    return { n = select("#", ...), ... }
end

-- ---------------------------------------------------------------------------
-- bit, as 8.3.7 provides it (32-bit, two's complement, unsigned results)
-- ---------------------------------------------------------------------------

local function U32(value)
    value = value % 4294967296
    if value < 0 then
        value = value + 4294967296
    end
    return value
end

local function And(a, b)
    local result, place = 0, 1
    a, b = U32(a), U32(b)
    for _ = 1, 32 do
        local x, y = a % 2, b % 2
        if x == 1 and y == 1 then
            result = result + place
        end
        a, b, place = math.floor(a / 2), math.floor(b / 2), place * 2
    end
    return result
end

local function Or(a, b)
    local result, place = 0, 1
    a, b = U32(a), U32(b)
    for _ = 1, 32 do
        local x, y = a % 2, b % 2
        if x == 1 or y == 1 then
            result = result + place
        end
        a, b, place = math.floor(a / 2), math.floor(b / 2), place * 2
    end
    return result
end

local function Xor(a, b)
    local result, place = 0, 1
    a, b = U32(a), U32(b)
    for _ = 1, 32 do
        local x, y = a % 2, b % 2
        if x ~= y then
            result = result + place
        end
        a, b, place = math.floor(a / 2), math.floor(b / 2), place * 2
    end
    return result
end

local function Rshift(a, n)
    return math.floor(U32(a) / (2 ^ n))
end

local function Lshift(a, n)
    return U32(U32(a) * (2 ^ n))
end

M.bit = {
    band = And,
    bor = Or,
    bxor = Xor,
    bnot = function(a) return 4294967295 - U32(a) end,
    rshift = Rshift,
    lshift = Lshift,
    arshift = function(a, n)
        a = U32(a)
        if a >= 2147483648 then
            a = a - 4294967296
        end
        return math.floor(a / (2 ^ n)) % 4294967296
    end,
    tobit = function(a) return U32(a) end,
    tohex = function(a) return string.format("%08x", U32(a)) end,
}

-- ---------------------------------------------------------------------------
-- Frames
-- ---------------------------------------------------------------------------

local REGIONS = {
    FontString = { "SetText", "GetText", "SetFontObject", "SetJustifyH", "SetJustifyV",
                   "SetTextColor", "SetShadowColor", "SetShadowOffset", "SetWidth",
                   "SetHeight", "SetFont", "SetFormattedText", "SetWordWrap" },
    Texture = { "SetTexture", "SetVertexColor", "SetTexCoord", "SetDesaturated",
                "SetBlendMode", "SetGradientAlpha", "SetGradient", "SetColorTexture" },
}

local function NewMethodSet(kind, state, record)
    local methods = {}

    local function Get(key, fallback) return state[key] or fallback end

    methods.SetText = function(self, text) state.text = text end
    methods.GetText = function(self) return state.text or "" end
    methods.GetNumber = function(self) return tonumber(state.text) or 0 end
    methods.SetFormattedText = function(self, format, ...)
        local ok, text = pcall(string.format, format, ...)
        state.text = ok and text or tostring(format)
    end
    methods.SetFontObject = function(self, font) state.font = font end
    methods.SetJustifyH = function(self, value) state.justifyH = value end
    methods.SetJustifyV = function(self, value) state.justifyV = value end
    methods.SetTextColor = function(self, r, g, b, a) state.colour = { r, g, b, a } end
    methods.SetShadowColor = function() end
    methods.SetShadowOffset = function() end
    methods.SetTexture = function(self, texture) state.texture = texture end
    methods.SetVertexColor = function() end
    methods.SetTexCoord = function() end
    methods.SetDesaturated = function() end
    methods.SetBlendMode = function() end
    methods.SetGradient = function() end
    methods.SetGradientAlpha = function() end
    methods.SetColorTexture = function() end
    methods.SetWordWrap = function(self, value) state.wordWrap = value end
    methods.SetFont = function() end
    methods.SetScale = function(self, value) state.scale = value end
    methods.GetScale = function(self) return state.scale or 1 end
    methods.SetParent = function(self, parent) state.parent = parent end
    methods.GetParent = function(self) return state.parent end
    methods.SetPoint = function(self, point, relative, relativePoint, x, y)
        state.point = { point = point, relative = relative, relativePoint = relativePoint,
                        x = x or 0, y = y or 0 }
        state.points = state.points or {}
        state.points[#state.points + 1] = state.point
    end
    methods.GetPoint = function(self)
        local point = state.point
        if not point then
            return nil
        end
        return point.point, point.relative, point.relativePoint, point.x, point.y
    end
    methods.GetNumPoints = function(self) return state.points and #state.points or 0 end
    methods.ClearAllPoints = function(self) state.point = nil; state.points = {} end
    methods.SetAllPoints = function(self, relative) state.allPoints = relative end
    methods.SetSize = function(self, width, height) state.width, state.height = width, height end
    methods.SetWidth = function(self, width) state.width = width end
    methods.SetHeight = function(self, height) state.height = height end
    methods.GetWidth = function(self) return state.width or 0 end
    methods.GetHeight = function(self) return state.height or 0 end
    methods.GetSize = function(self) return state.width or 0, state.height or 0 end
    methods.GetLeft = function(self) return state.x or 0 end
    methods.GetTop = function(self) return state.y or 0 end
    methods.GetName = function(self) return state.name end
    methods.GetObjectType = function(self) return kind end

    -- Frames
    methods.Show = function(self) state.shown = true end
    methods.Hide = function(self) state.shown = false end
    methods.IsShown = function(self) return state.shown and true or false end
    methods.IsVisible = function(self)
        local node = self
        while node do
            if not node:IsShown() then
                return false
            end
            node = node.GetParent and node:GetParent() or nil
        end
        return true
    end
    methods.SetShown = function(self, value) state.shown = value and true or false end
    methods.Enable = function(self) state.disabled = false end
    methods.Disable = function(self) state.disabled = true end
    methods.IsEnabled = function(self) return not state.disabled end
    methods.EnableMouse = function(self, value) state.mouse = value end
    methods.SetMovable = function(self, value) state.movable = value end
    methods.SetClampedToScreen = function() end
    methods.SetFrameStrata = function(self, value) state.strata = value end
    methods.SetFrameLevel = function(self, value) state.level = value end
    methods.GetFrameLevel = function(self) return state.level or 1 end
    methods.SetToplevel = function() end
    methods.SetUserPlaced = function() end
    methods.StartMoving = function(self) state.moving = true end
    methods.StopMovingOrSizing = function(self) state.moving = false end
    methods.RegisterForDrag = function(self, buttons) state.drag = buttons end
    methods.SetHitRectInsets = function() end
    methods.SetBackdrop = function() end
    methods.SetBackdropColor = function() end
    methods.SetBackdropBorderColor = function() end
    methods.SetAlpha = function(self, value) state.alpha = value end
    methods.GetAlpha = function(self) return state.alpha or 1 end
    methods.SetScript = function(self, event, handler)
        state.scripts[event] = handler
    end
    methods.HookScript = function(self, event, handler)
        local previous = state.scripts[event]
        state.scripts[event] = function(...)
            if previous then previous(...) end
            handler(...)
        end
    end
    methods.GetScript = function(self, event) return state.scripts[event] end
    methods.HasScript = function(self, event) return state.scripts[event] ~= nil end
    methods.RegisterEvent = function(self, event)
        state.events[event] = true
        state.eventList[#state.eventList + 1] = event
    end
    methods.UnregisterEvent = function(self, event) state.events[event] = nil end
    methods.RegisterAllEvents = function() end
    methods.SetID = function(self, id) state.id = id end
    methods.GetID = function(self) return state.id end
    methods.CreateFontString = function(self, name, layer, template)
        return M.NewFrame("FontString", name, self, template)
    end
    methods.CreateTexture = function(self, name, layer, template)
        return M.NewFrame("Texture", name, self, template)
    end
    methods.CreateAnimationGroup = function(self)
        return M.NewFrame("AnimationGroup", nil, self)
    end

    -- Buttons
    methods.SetButtonState = function(self, stateName, locked)
        state.buttonState = stateName
        state.buttonLocked = locked
    end
    methods.GetButtonState = function(self) return state.buttonState or "NORMAL" end
    methods.SetHighlightTexture = function(self, texture) state.highlight = texture end
    methods.SetNormalTexture = function(self, texture) state.normal = texture end
    methods.SetPushedTexture = function(self, texture) state.pushed = texture end
    methods.SetDisabledTexture = function(self, texture) state.disabledTexture = texture end
    methods.GetFontString = function(self)
        state.fontString = state.fontString or M.NewFrame("FontString", nil, self)
        return state.fontString
    end
    methods.Click = function(self)
        local handler = state.scripts.OnClick
        if handler then
            handler(self, "LeftButton")
        end
    end

    -- Edit boxes
    methods.SetAutoFocus = function(self, value) state.autoFocus = value end
    methods.ClearFocus = function(self) state.focus = false end
    methods.SetFocus = function(self) state.focus = true end
    methods.HighlightText = function(self) state.highlighted = true end
    methods.SetMaxLetters = function(self, value) state.maxLetters = value end
    methods.SetNumeric = function(self, value) state.numeric = value end
    methods.SetMultiLine = function(self, value) state.multiLine = value end
    methods.SetTextInsets = function() end

    -- Scroll frames
    methods.SetScrollChild = function(self, child) state.scrollChild = child end
    methods.GetScrollChild = function(self) return state.scrollChild end
    methods.SetVerticalScroll = function(self, value) state.verticalScroll = value end
    methods.GetVerticalScroll = function(self) return state.verticalScroll or 0 end
    methods.SetHorizontalScroll = function(self, value) state.horizontalScroll = value end
    methods.GetVerticalScrollRange = function(self) return state.scrollRange or 0 end
    methods.UpdateScrollChildRect = function() end
    methods.SetClipsChildren = function() end

    -- Status bars and sliders
    methods.SetMinMaxValues = function(self, low, high) state.low, state.high = low, high end
    methods.GetMinMaxValues = function(self) return state.low or 0, state.high or 0 end
    methods.SetValue = function(self, value) state.value = value end
    methods.GetValue = function(self) return state.value or 0 end
    methods.SetValueStep = function(self, step) state.valueStep = step end
    methods.SetStatusBarTexture = function(self, texture) state.barTexture = texture end
    methods.SetStatusBarColor = function() end
    methods.GetStatusBarTexture = function(self) return state.barTexture end

    -- What is left over: a method the client has and this file has not met yet.
    return setmetatable(methods, {
        -- Only a capitalised name can be a method. A lowercase one is a field the addon fills
        -- itself - `row.remove`, `row.label` - and answering it with a function would make a
        -- missing field look like a present one. That mistake is worse than a nil: it turns
        -- "this row has no Remove button" into "this function has no methods".
        __index = function(_, key)
            if type(key) == "string" and key:match("^[A-Z]") then
                record(kind .. ":" .. key)
                return function() end
            end
            return nil
        end,
    })
end

M.frames = {}

function M.NewFrame(kind, name, parent, template)
    local state = {
        name = name,
        parent = parent,
        -- A frame is visible from the moment it is created, as it is in a client. The addon
        -- hides what it does not want seen, and a model where everything started hidden would
        -- make every Hide() look like a no-op.
        shown = true,
        scripts = {},
        events = {},
        eventList = {},
        template = template,
        children = {},
    }

    if kind == "AnimationGroup" then
        state.scripts = {}
    end

    local frame = setmetatable({
        __state = state,
        __kind = kind,
    }, {
        __index = NewMethodSet(kind, state, function(key) M.unknownMethods[key] = true end),
        __tostring = function() return (name or kind) .. "(" .. kind .. ")" end,
    })

    if parent and parent.__state then
        local children = parent.__state.children
        children[#children + 1] = frame
    end

    M.frames[#M.frames + 1] = frame

    if name then
        M.byName[name] = frame
    end

    return frame
end

M.byName = {}
M.unknownMethods = {}
M.unknownGlobals = {}
M.printed = {}
M.sent = {}
M.allSent = {}
M.prefixes = {}
M.now = 1000

function M.BuildEnv()
    local env = {}
    M.env = env
    M.frames, M.byName = {}, {}
    M.unknownMethods, M.unknownGlobals = {}, {}
    M.printed, M.sent, M.prefixes = {}, {}, {}

    env._G = env
    env.bit = M.bit
    env.unpack = unpack
    env.select = select
    env.type = type
    env.tonumber = tonumber
    env.tostring = tostring
    env.pairs = pairs
    env.ipairs = ipairs
    env.next = next
    env.print = print
    env.error = error
    env.assert = assert
    env.pcall = pcall
    env.xpcall = xpcall
    env.setmetatable = setmetatable
    env.getmetatable = getmetatable
    env.rawget = rawget
    env.rawset = rawset
    env.rawequal = rawequal
    env.rawlen = rawlen
    env.unpack = unpack
    env.collectgarbage = function() return 0 end
    env.math = math
    env.string = string
    env.table = table
    env.coroutine = coroutine

    -- Client constants the addon reads.
    env.MAX_PLAYER_LEVEL = 120
    env.GetLocale = function() return "enUS" end
    env.GetTime = function() return M.now end
    env.time = function() return 1767225600 + math.floor(M.now) end
    env.GetServerTime = function() return 1767225600 + math.floor(M.now) end
    env.date = function() return "2026-01-01" end
    env.GetTimePreciseSec = function() return M.now end
    env.UnitName = function(unit) return unit == "player" and "Thordekk" or nil end
    env.UnitFactionGroup = function() return "Horde" end
    env.GetRealmName = function() return "Azeroth" end
    env.IsShiftKeyDown = function() return false end
    env.IsControlKeyDown = function() return false end
    env.GetMouseFocus = function() return nil end
    env.PlaySound = function() return true end
    env.SlashCmdList = {}
    env.ChatFrame_AddMessage = function() end
    env.UISpecialFrames = {}

    env.C_ChatInfo = {
        RegisterAddonMessagePrefix = function(prefix) M.prefixes[#M.prefixes + 1] = prefix end,
        SendAddonMessage = function(prefix, text, channel, target)
            local record = { prefix = prefix, text = text, channel = channel, target = target }
            M.sent[#M.sent + 1] = record
            M.allSent[#M.allSent + 1] = record
            return true
        end,
    }

    env.UIParent = M.NewFrame("Frame", "UIParent", nil)
    env.UIParent.__state.shown = true
    env.Minimap = M.NewFrame("Frame", "Minimap", env.UIParent)
    env.Minimap.__state.shown = true

    local fonts = { "GameFontNormal", "GameFontNormalLarge", "GameFontNormalSmall",
                    "GameFontHighlight", "GameFontHighlightSmall", "GameFontHighlightLarge",
                    "GameFontDisable", "GameFontDisableSmall", "GameFontWhite",
                    "NumberFontNormal", "ChatFontNormal", "GameFontNormalMed3" }
    for _, font in ipairs(fonts) do
        env[font] = { __font = font }
    end

    env.DEFAULT_CHAT_FRAME = {
        AddMessage = function(self, text)
            M.printed[#M.printed + 1] = tostring(text)
        end,
    }

    env.CreateFrame = function(kind, name, parent, template)
        return M.NewFrame(kind or "Frame", name, parent, template)
    end

    env.CreateFont = function(name, height, flags) return { name = name, height = height } end

    setmetatable(env, {
        __index = function(_, key)
            M.unknownGlobals[key] = true
            return nil
        end,
    })

    return env
end

-- ---------------------------------------------------------------------------
-- Driving the addon
-- ---------------------------------------------------------------------------

function M.LoadFile(env, path, name)
    local chunk, err = loadfile(path)
    if not chunk then
        error("cannot load " .. path .. ": " .. tostring(err))
    end
    setfenv(chunk, env)
    local ok, problem = pcall(chunk, name, name)
    if not ok then
        error("error loading " .. path .. ": " .. tostring(problem))
    end
end

-- Fire an event on every frame that registered for it, in creation order.
function M.FireEvent(event, ...)
    local args = Pack(...)
    for _, frame in ipairs(M.frames) do
        local state = frame.__state
        if state.events[event] and state.scripts.OnEvent then
            local ok, err = pcall(state.scripts.OnEvent, frame, event, unpack(args, 1, args.n))
            if not ok then
                error("OnEvent " .. event .. " on " .. tostring(frame) .. ": " .. tostring(err))
            end
        end
    end
end

-- Deliver a server line the way the client would: prefix, text, channel, sender.
function M.Deliver(text, sender)
    M.FireEvent("CHAT_MSG_ADDON", "BFAH", text, "WHISPER", sender or "Server")
end

function M.Slash(list, input)
    local handler = M.env.SlashCmdList[list]
    if not handler then
        error("no slash handler registered for " .. list)
    end
    handler(input or "")
end

-- Show a frame and everything under it, so IsVisible works on children.
function M.Show(frame)
    local state = frame.__state
    local node = frame
    while node and node.__state do
        if node.__state.shown then
            return false
        end
        node = node.__state.parent
    end
    state.shown = true
    return true
end

function M.Walk(frame, visit)
    visit(frame)
    for _, child in ipairs(frame.__state.children or {}) do
        M.Walk(child, visit)
    end
end

return M
