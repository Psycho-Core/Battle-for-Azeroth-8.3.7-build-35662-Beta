--[[
    HousingUI - Menu.lua

    The housing menu. Core.lua is the transport; this is the part with frames on it,
    arranged the way the Midnight housing window is arranged: a tab rail down the left,
    one pane per tab, and the house card at the top of the landing pane.

    Tabs:

        House        the plot card: name, tier, budgets, homestone, lock, sell, level
        Catalogue    everything buyable, filterable, with the currency named
        Placed       what is standing in the house, tilt, pets, placement
        Rooms        exterior shells and interior rooms
        Storage      how many copies of each piece are in store
        Dyes         the server's palette, applied to the selected placement
        Map          every plot on the ring, clickable
        Neighbours   the street, the finder, and moving house
        Travel       homestone, gryphon, arcanostorage, portals
        Endeavors    live task list, the coffer, the four milestone claims
        Blueprints   export a code, import one, restore an archived house
        Access       the two permission matrices, the guest list, the amenities
        Guide        every opcode the transport speaks
        Settings     the six client preferences

    Every button here maps to an opcode Core.lua already speaks. Nothing in this file
    invents state: it draws what the server last said and then asks again.

    Toggle with /housing or /house, or the House button next to the minimap.
--]]

local ADDON_NAME = ...
HousingUI = HousingUI or {}

local H = HousingUI
local Menu = {}
H.Menu = Menu

local GOLD = "|cffffd100"
local DIM = "|cff9d9d9d"
local GOOD = "|cff40c040"
local BAD = "|cffff5555"
local RESET = "|r"

-- Every tab is a place a player can actually do something: no tab is a description of a
-- feature that lives somewhere else. The order follows retail's build window, then the
-- services a house sells, then the reference.
local TABS = {
    { id = "house", label = "House" },
    { id = "catalogue", label = "Catalogue" },
    { id = "placed", label = "Placed" },
    { id = "rooms", label = "Rooms" },
    { id = "storage", label = "Storage" },
    { id = "dye", label = "Dyes" },
    { id = "map", label = "Map" },
    { id = "neighbours", label = "Neighbours" },
    { id = "travel", label = "Travel" },
    { id = "endeavors", label = "Endeavors" },
    { id = "hunts", label = "Hunts" },
    { id = "blueprints", label = "Blueprints" },
    { id = "access", label = "Access" },
    { id = "guide", label = "Guide" },
    { id = "settings", label = "Settings" },
}

local MILESTONES = {
    { 1, "25% of tasks", "the first tier of themed stock becomes buyable" },
    { 2, "50%", "the second tier, plus NPC flavour dialogue" },
    { 3, "75%", "the third tier, plus hub decoration stage two" },
    { 4, "100%", "the fourth tier, the coffer, and the best pieces" },
}

local CURRENCY_NAME = {
    [0] = "gold",
    [3363] = "Community Coupons",
    [33226] = "Tricky Treats",
}

local STATE_NAME = {
    [0] = "unclaimed",
    [1] = "owned by someone else",
    [2] = "yours",
}

-- ---------------------------------------------------------------------------
-- Small helpers
-- ---------------------------------------------------------------------------

local function CurrencyName(id)
    return CURRENCY_NAME[id] or ("currency " .. tostring(id))
end

local function Money(amount)
    if amount == nil then
        return "?"
    end
    if amount >= 10000 then
        return string.format("%d,%03d", math.floor(amount / 1000), amount % 1000) .. "g"
    end
    return tostring(amount) .. "g"
end

local function CountTable(tbl)
    local count = 0
    for _ in pairs(tbl or {}) do
        count = count + 1
    end
    return count
end

local function NewLabel(parent, size, anchor, x, y)
    local label = parent:CreateFontString(nil, "OVERLAY", "GameFontNormal")
    if size then
        label:SetFontObject(size)
    end
    label:SetPoint(anchor or "TOPLEFT", parent, anchor or "TOPLEFT", x or 0, y or 0)
    label:SetJustifyH("LEFT")
    return label
end

local function NewButton(parent, text, width, x, y, onClick)
    local button = CreateFrame("Button", nil, parent, "UIPanelButtonTemplate")
    button:SetSize(width or 110, 22)
    button:SetPoint("TOPLEFT", parent, "TOPLEFT", x or 0, y or 0)
    button:SetText(text)
    if onClick then
        button:SetScript("OnClick", onClick)
    end
    return button
end

-- ---------------------------------------------------------------------------
-- The frame
-- ---------------------------------------------------------------------------

local frame = CreateFrame("Frame", "PsychoHouseMenuFrame", UIParent, "BasicFrameTemplateWithInset")
frame:SetSize(720, 460)
frame:SetPoint("CENTER")
frame:SetMovable(true)
frame:EnableMouse(true)
frame:RegisterForDrag("LeftButton")
frame:SetScript("OnDragStart", frame.StartMoving)
frame:SetScript("OnDragStop", function(self)
    self:StopMovingOrSizing()

    -- Where the window sits is a preference like any other, so it goes in the same table as
    -- the snapping settings rather than being forgotten at logout.
    local point, _, relativePoint, x, y = self:GetPoint()
    H.settings.framePoint = { point, relativePoint, math.floor(x + 0.5), math.floor(y + 0.5) }
    H.SaveSettings()
end)
frame:Hide()

frame:SetScale(H.settings.frameScale or 1.0)

if H.settings.framePoint then
    local stored = H.settings.framePoint
    frame:ClearAllPoints()
    frame:SetPoint(stored[1] or "CENTER", UIParent, stored[2] or "CENTER", stored[3] or 0, stored[4] or 0)
end

local title = frame:CreateFontString(nil, "OVERLAY", "GameFontNormalLarge")
title:SetPoint("TOPLEFT", frame, "TOPLEFT", 16, -14)
title:SetText("Housing")

local subtitle = frame:CreateFontString(nil, "OVERLAY", "GameFontDisableSmall")
subtitle:SetPoint("LEFT", title, "RIGHT", 10, 0)
subtitle:SetText("mod-psycho-house")

local close = CreateFrame("Button", nil, frame, "UIPanelCloseButton")
close:SetPoint("TOPRIGHT", frame, "TOPRIGHT", -4, -4)

local rail = CreateFrame("Frame", nil, frame)
rail:SetPoint("TOPLEFT", frame, "TOPLEFT", 12, -44)
rail:SetSize(132, 380)

local pane = CreateFrame("Frame", nil, frame)
pane:SetPoint("TOPLEFT", rail, "TOPRIGHT", 12, 0)
pane:SetPoint("BOTTOMRIGHT", frame, "BOTTOMRIGHT", -14, 42)

local footer = frame:CreateFontString(nil, "OVERLAY", "GameFontDisableSmall")
footer:SetPoint("BOTTOMLEFT", frame, "BOTTOMLEFT", 16, 14)
footer:SetText("|cff9d9d9d/housing <cmd> does everything this menu does|r")

-- ---------------------------------------------------------------------------
-- Tab rail
-- ---------------------------------------------------------------------------

Menu.tabButtons = {}
Menu.current = "house"

local function ShowTab(id)

    Menu.current = id

    for key, button in pairs(Menu.tabButtons) do
        if key == id then
            button:SetButtonState("PUSHED", true)
        else
            button:SetButtonState("NORMAL", false)
        end
    end

    for key, region in pairs(Menu.panes or {}) do
        if key == id then
            region:Show()
        else
            region:Hide()
        end
    end

    if Menu.onShow and Menu.onShow[id] then
        Menu.onShow[id]()
    end
end

Menu.ShowTab = ShowTab

for index, tab in ipairs(TABS) do
    local button = CreateFrame("Button", nil, rail, "UIPanelButtonTemplate")
    button:SetSize(128, 21)
    button:SetPoint("TOPLEFT", rail, "TOPLEFT", 0, -((index - 1) * 25))
    button:SetText(tab.label)
    button:SetScript("OnClick", function()
        ShowTab(tab.id)
    end)
    Menu.tabButtons[tab.id] = button
end

Menu.panes = {}

local function NewPane(id)
    local region = CreateFrame("Frame", nil, pane)
    region:SetAllPoints(pane)
    region:Hide()
    Menu.panes[id] = region
    return region
end

-- ---------------------------------------------------------------------------
-- House
-- ---------------------------------------------------------------------------

local housePane = NewPane("house")

local houseName = NewLabel(housePane, "GameFontNormalHuge", "TOPLEFT", 0, 0)
houseName:SetText("No neighbourhood yet")

local houseLine = NewLabel(housePane, nil, "TOPLEFT", 0, -32)
local tierLine = NewLabel(housePane, nil, "TOPLEFT", 0, -54)
local budgetLine = NewLabel(housePane, nil, "TOPLEFT", 0, -76)
local lockLine = NewLabel(housePane, nil, "TOPLEFT", 0, -98)

local renameBox = CreateFrame("EditBox", nil, housePane, "InputBoxTemplate")
renameBox:SetSize(220, 20)
renameBox:SetPoint("TOPLEFT", housePane, "TOPLEFT", 8, -130)
renameBox:SetAutoFocus(false)
renameBox:SetMaxLetters(64)
renameBox:SetText("")

local renameLabel = NewLabel(housePane, nil, "TOPLEFT", 240, -130)
renameLabel:SetText(DIM .. "new house name" .. RESET)

NewButton(housePane, "Rename", 90, 0, -160, function()
    local text = renameBox:GetText()
    if text and text ~= "" then
        H.Send("C:NAME|" .. text)
        renameBox:SetText("")
    end
end)

NewButton(housePane, "Homestone", 110, 100, -160, function()
    H.Send("C:HOME")
end)

NewButton(housePane, "Leave house", 110, 220, -160, function()
    H.Send("C:LEAVE")
end)

NewButton(housePane, "Lock / unlock", 120, 0, -192, function()
    if H.plotInfo and H.plotInfo.locked then
        H.Send("C:LOCK|0")
    else
        H.Send("C:LOCK|1")
    end
end)

NewButton(housePane, "Sell house", 100, 130, -192, function()
    -- The refund is Housing.Sell.RefundPercent and the archive row is written either way, so
    -- a sale inside Housing.Evict.GraceDays can be undone from the Blueprints tab.
    H.Send("C:SELL")
end)

NewButton(housePane, "Spend XP", 100, 236, -192, function()
    H.Send("C:LEVELUP")
end)

NewButton(housePane, "Refresh", 90, 342, -192, function()
    H.Send("C:NEIGHBORHOOD")
    H.Send("C:PLOTS")
    if H.plotIndex then
        H.Send("C:PLOT|" .. H.plotIndex)
    end
end)

local tierLabel = NewLabel(housePane, nil, "TOPLEFT", 0, -226)
tierLabel:SetText("Tier upgrades cost gold and raise both budgets:")

Menu.tierButtons = {}

for tier = 2, 5 do
    local button = NewButton(housePane, "Tier " .. tier, 90, (tier - 2) * 96, -250, function()
        H.Send("C:TIER|" .. tier)
    end)
    Menu.tierButtons[tier] = button
end

-- Hearthlight Armillary: five presets. The buttons light the one the plot reports, so the
-- window and the world never disagree about which ambience is standing.

local ARMILLARY_PRESETS = {
    { 0, "Default" },
    { 1, "Natural" },
    { 2, "Unnatural" },
    { 3, "Evening" },
    { 4, "Midnight" },
}

local armillaryHeader = NewLabel(housePane, nil, "TOPLEFT", 0, -282)
armillaryHeader:SetText(GOLD .. "Hearthlight Armillary|r")

local armillaryLine = NewLabel(housePane, "GameFontHighlightSmall", "TOPLEFT", 0, -330)
armillaryLine:SetText("preset 0 - as authored")

Menu.armillaryButtons = {}

for index, preset in ipairs(ARMILLARY_PRESETS) do
    local button = NewButton(housePane, preset[2], 96, (index - 1) * 104, -304, function()
        H.Send("C:ARMILLARY|" .. preset[1])
    end)
    Menu.armillaryButtons[preset[1]] = button
end

local function RefreshArmillary()
    local preset = (H.plotInfo and H.plotInfo.armillary) or 0

    for id, button in pairs(Menu.armillaryButtons) do
        button:SetButtonState(id == preset and "PUSHED" or "NORMAL", id == preset)
    end

    local names = { "as authored", "as authored, sky clear", "lit pieces",
        "lit pieces, sky clear", "lit pieces, sky natural" }
    armillaryLine:SetText(string.format("preset %d - %s", preset, names[preset + 1] or "unknown"))
end

Menu.RefreshArmillary = RefreshArmillary

-- ---------------------------------------------------------------------------
-- Access
-- ---------------------------------------------------------------------------

local accessPane = NewPane("access")

-- Amenities: the hub's fixed points - stage, mailbox, bulletin board, the rest. They are
-- server data (C:VAREA), not addon guesses, because the module moves them with the map.

local amenityHeader = NewLabel(accessPane, nil, "TOPLEFT", 0, 0)
amenityHeader:SetText(GOLD .. "Amenities in this hub|r")

Menu.amenityRows = {}

local function BuildAmenities()

    local sorted = {}

    for _, amenity in ipairs(H.amenities or {}) do
        sorted[#sorted + 1] = amenity
    end

    for index, row in ipairs(Menu.amenityRows) do
        row:Hide()
    end

    for index = 1, math.min(#sorted, 6) do
        local amenity = sorted[index]
        local row = Menu.amenityRows[index]

        if not row then
            row = NewLabel(accessPane, "GameFontHighlightSmall", "TOPLEFT", 0, -22 - ((index - 1) * 16))
            row:SetWidth(520)
            Menu.amenityRows[index] = row
        end

        row:SetText(string.format("%s%s|r  type %s, at %.0f / %.0f",
            DIM, tostring(amenity.name), tostring(amenity.kind),
            amenity.x or 0, amenity.y or 0))
        row:Show()
    end
end

Menu.onShow = Menu.onShow or {}
Menu.onShow.house = function()
    H.Send("C:NEIGHBORHOOD")
    if H.plotIndex then
        H.Send("C:PLOT|" .. H.plotIndex)
    end
end

-- The access tab carries what used to crowd the house tab: the hub's fixed points and the
-- yard/interior permission matrices. Both are lists that grow, so they get a pane each.
Menu.onShow.access = function()
    -- Through the table, not the local name: the function is declared further down this file
    -- and a closure made here would capture a global that is still nil.
    if Menu.RefreshPermissions then
        Menu.RefreshPermissions()
    end
    H.amenities = {}
    H.guests = {}
    H.Send("C:VAREA")
    H.Send("C:GUESTLIST")
    if Menu.BuildGuests then
        Menu.BuildGuests()
    end
    if H.plotIndex then
        H.Send("C:PLOT|" .. H.plotIndex)
    end
end

-- ---------------------------------------------------------------------------
-- Permissions
--
-- Retail's model: the yard and the interior are separate, each with its own audiences. The
-- buttons are the six bits the server stores, drawn from the masks the server sent rather
-- than from anything remembered here.
-- ---------------------------------------------------------------------------

local permHeader = NewLabel(accessPane, "GameFontNormalSmall", "TOPLEFT", 0, -140)
permHeader:SetText("Who may come in")

local permRows = {}
local permState = { plot = 0, house = 0 }

local PERM_SPACES = {
    { id = "plot", space = 0, label = "Yard" },
    { id = "house", space = 1, label = "Inside" },
}

for spaceIndex, entry in ipairs(PERM_SPACES) do
    local label = NewLabel(accessPane, "GameFontDisableSmall", "TOPLEFT", 0, -164 - ((spaceIndex - 1) * 46))
    label:SetText(entry.label)
    label:SetWidth(50)

    permRows[entry.id] = {}

    for audienceIndex, audience in ipairs(H.AUDIENCES) do
        local button = NewButton(accessPane, audience.label, 78,
            52 + ((audienceIndex - 1) * 82), -162 - ((spaceIndex - 1) * 46), nil)

        button:SetScript("OnClick", function()
            -- Flip one bit with arithmetic rather than the bit library: the masks are small
            -- and the addon already avoids depending on anything outside the default UI.
            local mask = permState[entry.id] or 0
            local rest = mask % (audience.bit * 2)

            if rest >= audience.bit then
                mask = mask - audience.bit
            else
                mask = mask + audience.bit
            end

            H.Send(string.format("C:ACCESS|%d|%d", entry.space, mask))
            H.Send("C:PLOT|" .. tostring(H.plotIndex or 0))
        end)

        permRows[entry.id][audience.bit] = button
    end
end

local function RefreshPermissions()
    local plot = H.plotInfo
    if not plot then
        return
    end

    permState.plot = plot.plotMask or 0
    permState.house = plot.houseMask or 0

    for _, space in ipairs(PERM_SPACES) do
        local mask = permState[space.id] or 0

        for audienceIndex, audience in ipairs(H.AUDIENCES) do
            local button = permRows[space.id][audience.bit]

            if math.floor(mask / audience.bit) % 2 == 1 then
                button:SetButtonState("PUSHED", true)
            else
                button:SetButtonState("NORMAL", false)
            end
        end
    end
end

Menu.RefreshPermissions = RefreshPermissions

-- ---------------------------------------------------------------------------
-- Guests
--
-- The guest list is the exception the masks cannot override: a name on it may stand on the
-- yard and come inside whatever the six audiences say, which is how "book club at eight"
-- works. Flags are the same six audience bits, stored per name.
-- ---------------------------------------------------------------------------

local guestHeader = NewLabel(accessPane, "GameFontNormalSmall", "TOPLEFT", 0, -256)
guestHeader:SetText(GOLD .. "Guest list" .. RESET)

local guestBox = CreateFrame("EditBox", nil, accessPane, "InputBoxTemplate")
guestBox:SetSize(180, 20)
guestBox:SetPoint("TOPLEFT", accessPane, "TOPLEFT", 8, -282)
guestBox:SetAutoFocus(false)
guestBox:SetMaxLetters(64)
guestBox:SetText("")

local guestFlagsBox = CreateFrame("EditBox", nil, accessPane, "InputBoxTemplate")
guestFlagsBox:SetSize(40, 20)
guestFlagsBox:SetPoint("TOPLEFT", accessPane, "TOPLEFT", 196, -282)
guestFlagsBox:SetAutoFocus(false)
guestFlagsBox:SetNumeric(true)
guestFlagsBox:SetText("63")

NewButton(accessPane, "Add guest", 90, 244, -280, function()
    local name = guestBox:GetText() or ""

    if name == "" then
        H.Print("type the character name first")
        return
    end

    H.Send(string.format("C:GUESTADD|%s|%s", name, guestFlagsBox:GetText() or "63"))
    guestBox:SetText("")
end)

NewButton(accessPane, "Refresh list", 100, 338, -280, function()
    H.guests = {}
    H.Send("C:GUESTLIST")
end)

Menu.guestRows = {}

local function BuildGuests()

    local names = {}

    for name, flags in pairs(H.guests or {}) do
        names[#names + 1] = { name = name, flags = tonumber(flags) or 0 }
    end

    table.sort(names, function(left, right) return tostring(left.name) < tostring(right.name) end)

    guestHeader:SetText(string.format("%sGuest list: %d name(s), and a name here is always " ..
        "admitted%s", GOLD, #names, RESET))

    for _, row in ipairs(Menu.guestRows) do
        row:Hide()
    end

    if #names == 0 then
        local empty = Menu.guestEmpty

        if not empty then
            empty = NewLabel(accessPane, "GameFontDisableSmall", "TOPLEFT", 0, -312)
            empty:SetWidth(540)
            Menu.guestEmpty = empty
        end

        empty:SetText("nobody on the list - the six audience bits decide who may visit")
        empty:Show()
        return
    end

    if Menu.guestEmpty then
        Menu.guestEmpty:Hide()
    end

    for index = 1, math.min(#names, 3) do
        local entry = names[index]
        local row = Menu.guestRows[index]

        if not row then
            row = CreateFrame("Frame", nil, accessPane)
            row:SetSize(320, 22)
            row.label = row:CreateFontString(nil, "OVERLAY", "GameFontHighlightSmall")
            row.label:SetPoint("LEFT", row, "LEFT", 0, 0)
            row.label:SetWidth(240)
            row.label:SetJustifyH("LEFT")
            row.remove = NewButton(row, "Remove", 70, 246, 0, nil)
            Menu.guestRows[index] = row
        end

        row:SetPoint("TOPLEFT", accessPane, "TOPLEFT", 0, -312 - ((index - 1) * 24))
        row.label:SetText(string.format("%s  %sflags %d%s", tostring(entry.name), DIM, entry.flags, RESET))
        row.remove:SetScript("OnClick", function()
            H.Send("C:GUESTDEL|" .. tostring(entry.name))
            H.guests[tostring(entry.name)] = nil
            BuildGuests()
        end)
        row:Show()
    end
end

Menu.BuildGuests = BuildGuests

Menu.RefreshPermissions = RefreshPermissions

-- ---------------------------------------------------------------------------
-- Guests
--
-- The guest list is the exception the masks cannot override: a name on it may stand on the
-- yard and come inside whatever the six audiences say, which is how "book club at eight"
-- works. Flags are the same six audience bits, stored per name.
-- ---------------------------------------------------------------------------

local function RefreshHouse()
    local nbr = H.neighbourhood
    if nbr then
        houseName:SetText(nbr.name or "Neighbourhood")
        houseLine:SetText(string.format("slot %s, faction %s, %s%d/%d plots claimed%s",
            tostring(nbr.slot), tostring(nbr.faction), DIM, nbr.owned or 0, nbr.total or 0, RESET))
    end

    local plot = H.plotInfo
    if plot then
        tierLine:SetText(string.format("plot %s - tier %s, %s%s%s",
            tostring(plot.index), tostring(plot.tier),
            DIM, STATE_NAME[plot.state] or ("state " .. tostring(plot.state)), RESET))
        budgetLine:SetText(string.format("budget: %s%s/%s indoor|r, %s%s/%s outdoor|r",
            GOOD, tostring(plot.usedIndoor or 0), tostring(plot.budgetIndoor or 0),
            GOOD, tostring(plot.usedOutdoor or 0), tostring(plot.budgetOutdoor or 0)))
        lockLine:SetText(plot.locked and (BAD .. "locked to guests" .. RESET)
            or (DIM .. "open to guests" .. RESET))
    end
end

Menu.RefreshHouse = RefreshHouse

-- ---------------------------------------------------------------------------
-- Catalogue
-- ---------------------------------------------------------------------------

local cataloguePane = NewPane("catalogue")

local filterBox = CreateFrame("EditBox", nil, cataloguePane, "InputBoxTemplate")
filterBox:SetSize(180, 20)
filterBox:SetPoint("TOPLEFT", cataloguePane, "TOPLEFT", 8, -4)
filterBox:SetAutoFocus(false)
filterBox:SetScript("OnEscapePressed", function(self) self:ClearFocus() end)

local filterLabel = NewLabel(cataloguePane, nil, "TOPLEFT", 196, -6)
filterLabel:SetText(DIM .. "filter by name|r")

NewButton(cataloguePane, "Reload", 90, 300, -2, function()
    H.catalogue = {}
    H.Send("C:DECOR|")
end)

local catalogueScroll = CreateFrame("ScrollFrame", "PsychoHouseCatalogueScroll", cataloguePane, "UIPanelScrollFrameTemplate")
catalogueScroll:SetPoint("TOPLEFT", cataloguePane, "TOPLEFT", 0, -30)
catalogueScroll:SetPoint("BOTTOMRIGHT", cataloguePane, "BOTTOMRIGHT", -28, 0)

local catalogueContent = CreateFrame("Frame", nil, catalogueScroll)
catalogueContent:SetSize(520, 1)
catalogueScroll:SetScrollChild(catalogueContent)

Menu.catalogueRows = {}

local function BuildCatalogue()

    local filter = string.lower(filterBox:GetText() or "")
    local sorted = {}
    local lastHeading = nil

    for _, entry in pairs(H.catalogue or {}) do
        if filter == "" or string.find(string.lower(entry.name or ""), filter, 1, true) then
            sorted[#sorted + 1] = entry
        end
    end

    -- Category, then subcategory, then name: the same order the server pages in, so a page
    -- boundary never splits a subcategory heading off the pieces under it.
    table.sort(sorted, function(left, right)
        if (left.category or "") ~= (right.category or "") then
            return (left.category or "") < (right.category or "")
        end
        if (left.subcategory or "") ~= (right.subcategory or "") then
            return (left.subcategory or "") < (right.subcategory or "")
        end
        return (left.name or "") < (right.name or "")
    end)

    for _, row in ipairs(Menu.catalogueRows) do
        row:Hide()
    end

    for index = 1, math.min(#sorted, 40) do
        local entry = sorted[index]
        local row = Menu.catalogueRows[index]

        if not row then
            row = CreateFrame("Frame", nil, catalogueContent)
            row:SetSize(500, 22)
            row.name = row:CreateFontString(nil, "OVERLAY", "GameFontNormal")
            row.name:SetPoint("LEFT", row, "LEFT", 0, 0)
            row.name:SetWidth(300)
            row.name:SetJustifyH("LEFT")
            row.price = row:CreateFontString(nil, "OVERLAY", "GameFontHighlightSmall")
            row.price:SetPoint("LEFT", row.name, "RIGHT", 8, 0)
            row.buy = NewButton(row, "Buy", 60, 400, 0, nil)
            row:EnableMouse(true)
            Menu.catalogueRows[index] = row
        end

        row:SetPoint("TOPLEFT", catalogueContent, "TOPLEFT", 0, -((index - 1) * 24))

        -- A row is also the selection: the Placed tab drops whatever is picked here in front
        -- of the player, which is the only placement gesture that does not need coordinates.
        row:SetScript("OnMouseUp", function()
            Menu.selectedDecorId = entry.id
            H.Print("selected " .. tostring(entry.name) .. " - the Placed tab can drop it in front of the character")
        end)

        local heading = entry.subcategory and entry.subcategory ~= "" and entry.subcategory or nil
        if heading and heading ~= lastHeading then
            row.name:SetText(string.format("%s%s|r", GOLD, heading))
            lastHeading = heading
        else
            row.name:SetText((entry.unlocked and GOOD or "") .. (entry.name or "?") .. RESET)
        end

        if entry.unlocked then
            row.price:SetText(DIM .. "owned" .. RESET)
            row.buy:Hide()
        else
            row.price:SetText(string.format("%s %s", Money(entry.price), CurrencyName(entry.currency)))
            row.buy:Show()
            row.buy:SetScript("OnClick", function()
                H.Send("C:BUYDECOR|" .. tostring(entry.id))
            end)
        end

        row:Show()
    end

    catalogueContent:SetHeight(math.max(1, math.min(#sorted, 40) * 24))
end

Menu.onShow.catalogue = function()
    if CountTable(H.catalogue) == 0 then
        H.Send("C:DECOR|")
    else
        BuildCatalogue()
    end
end

filterBox:SetScript("OnTextChanged", function()
    BuildCatalogue()
end)

-- ---------------------------------------------------------------------------
-- Placed
-- ---------------------------------------------------------------------------

local placedPane = NewPane("placed")

local placedHeader = NewLabel(placedPane, "GameFontNormalLarge", "TOPLEFT", 0, 0)
placedHeader:SetText("Standing in this house")

-- ---------------------------------------------------------------------------
-- Controls
--
-- Four rows above the list, in the order a piece is worked on: pick it, tilt it, give it a
-- pet, put the next one down, or type exact numbers. The list starts below all of them, so
-- nothing is drawn over anything else.
-- ---------------------------------------------------------------------------

NewButton(placedPane, "Remove selected", 140, 0, -26, function()
    if Menu.selectedPlacement then
        H.Send("C:REMOVE|" .. tostring(Menu.selectedPlacement))
    else
        H.Print("select a placement in the list first")
    end
end)

-- Tilt: advanced mode's two extra axes. The buttons step the selected piece by the rotation
-- step the player set on the Settings tab, so the two preferences agree; the boxes take a
-- typed angle in degrees for the cases a step cannot reach.
local tiltPitchBox = CreateFrame("EditBox", nil, placedPane, "InputBoxTemplate")
tiltPitchBox:SetSize(46, 20)
tiltPitchBox:SetPoint("TOPLEFT", placedPane, "TOPLEFT", 150, -28)
tiltPitchBox:SetAutoFocus(false)
tiltPitchBox:SetNumeric(true)
tiltPitchBox:SetText("0")

local tiltRollBox = CreateFrame("EditBox", nil, placedPane, "InputBoxTemplate")
tiltRollBox:SetSize(46, 20)
tiltRollBox:SetPoint("TOPLEFT", placedPane, "TOPLEFT", 202, -28)
tiltRollBox:SetAutoFocus(false)
tiltRollBox:SetNumeric(true)
tiltRollBox:SetText("0")

local tiltLabel = NewLabel(placedPane, "GameFontDisableSmall", "TOPLEFT", 254, -32)
tiltLabel:SetText("pitch / roll, degrees")

local function ApplyTilt()
    local entry = Menu.selectedPlacement and H.placed[Menu.selectedPlacement]
    if not entry then
        H.Print("select a placement in the list first")
        return
    end

    local pitch = math.rad(tonumber(tiltPitchBox:GetText()) or 0)
    local roll = math.rad(tonumber(tiltRollBox:GetText()) or 0)

    H.Send(string.format("C:MOVE|%s|%.3f|%.3f|%.3f|%.3f|%.3f|%.4f|%.4f",
        tostring(entry.id), entry.x or 0, entry.y or 0, entry.z or 0, entry.o or 0,
        entry.scale or 1, pitch, roll))
end

NewButton(placedPane, "Apply", 60, 372, -26, ApplyTilt)

local function NudgeTilt(pitchStep, rollStep)
    local entry = Menu.selectedPlacement and H.placed[Menu.selectedPlacement]
    if not entry then
        H.Print("select a placement in the list first")
        return
    end

    local pitch = (entry.pitch or 0) + pitchStep
    local roll = (entry.roll or 0) + rollStep

    tiltPitchBox:SetText(string.format("%.0f", math.deg(pitch)))
    tiltRollBox:SetText(string.format("%.0f", math.deg(roll)))

    H.Send(string.format("C:MOVE|%s|%.3f|%.3f|%.3f|%.3f|%.3f|%.4f|%.4f",
        tostring(entry.id), entry.x or 0, entry.y or 0, entry.z or 0, entry.o or 0,
        entry.scale or 1, pitch, roll))
end

NewButton(placedPane, "Pitch -", 56, 440, -26, function() NudgeTilt(-H.settings.angle, 0) end)

-- Pets. The server refuses unless the companion is actually out, so the buttons say so
-- rather than failing silently; the piece has to be a bed, which the server also checks.
local function AssignPet(mode)
    local entry = Menu.selectedPlacement and H.placed[Menu.selectedPlacement]
    if not entry then
        H.Print("select a placement in the list first")
        return
    end

    H.Send(string.format("C:PET|%s|%d", tostring(entry.id), mode))
end

NewButton(placedPane, "Pitch +", 56, 440, -52, function() NudgeTilt(H.settings.angle, 0) end)
NewButton(placedPane, "Pet sleeps", 80, 150, -52, function() AssignPet(1) end)
NewButton(placedPane, "Pet roams", 80, 234, -52, function() AssignPet(2) end)
NewButton(placedPane, "Pet off", 60, 318, -52, function() AssignPet(0) end)

-- Placement without typing six coordinates: the server takes the player's own position and
-- facing, walks forward the given distance, drops the piece on the ground and reports the id
-- back. The coordinates never leave the server, so no client can claim a spot it is not
-- standing near.
local placeMetresBox = CreateFrame("EditBox", nil, placedPane, "InputBoxTemplate")
placeMetresBox:SetSize(40, 20)
placeMetresBox:SetPoint("TOPLEFT", placedPane, "TOPLEFT", 154, -80)
placeMetresBox:SetAutoFocus(false)
placeMetresBox:SetNumeric(true)
placeMetresBox:SetText("2")

local placeIndoorBox = CreateFrame("EditBox", nil, placedPane, "InputBoxTemplate")
placeIndoorBox:SetSize(28, 20)
placeIndoorBox:SetPoint("TOPLEFT", placedPane, "TOPLEFT", 200, -80)
placeIndoorBox:SetAutoFocus(false)
placeIndoorBox:SetNumeric(true)
placeIndoorBox:SetText("0")

local placeLabel = NewLabel(placedPane, "GameFontDisableSmall", "TOPLEFT", 234, -84)
placeLabel:SetText("metres ahead, 0 outdoor / 1 indoor")

NewButton(placedPane, "Place in front of me", 150, 0, -78, function()
    if not Menu.selectedDecorId then
        H.Print("pick a piece in the Catalogue first")
        return
    end

    H.Send(string.format("C:PLACEAT|%s|%s|%s", tostring(Menu.selectedDecorId),
        placeMetresBox:GetText() or "2", placeIndoorBox:GetText() or "0"))
end)

NewButton(placedPane, "Build mode", 90, 420, -78, function()
    H.buildMode = not H.buildMode
    H.Send(string.format("C:BUILDMODE|%d", H.buildMode and 1 or 0))
    H.Print("build mode " .. (H.buildMode and "on" or "off"))
end)

-- Advanced mode: retail lets a player type the position, the facing and the scale. The
-- server takes the same five numbers plus the indoor flag and the two tilt angles.
local advancedBoxes = {}
local ADVANCED = {
    { key = "x", width = 52, x = 0, default = "0" },
    { key = "y", width = 52, x = 56, default = "0" },
    { key = "z", width = 52, x = 112, default = "0" },
    { key = "o", width = 46, x = 168, default = "0" },
    { key = "s", width = 40, x = 218, default = "1" },
}

for _, spec in ipairs(ADVANCED) do
    local box = CreateFrame("EditBox", nil, placedPane, "InputBoxTemplate")
    box:SetSize(spec.width, 20)
    box:SetPoint("TOPLEFT", placedPane, "TOPLEFT", spec.x, -106)
    box:SetAutoFocus(false)
    box:SetText(spec.default)
    advancedBoxes[spec.key] = box
end

local advancedLabel = NewLabel(placedPane, "GameFontDisableSmall", "TOPLEFT", 264, -110)
advancedLabel:SetText("x, y, z, facing, scale")

NewButton(placedPane, "Place exactly", 110, 420, -104, function()
    if not Menu.selectedDecorId then
        H.Print("pick a piece in the Catalogue first")
        return
    end

    H.Send(string.format("C:PLACE|%s|%s|%s|%s|%s|%s|0",
        tostring(Menu.selectedDecorId),
        advancedBoxes.x:GetText() or "0", advancedBoxes.y:GetText() or "0",
        advancedBoxes.z:GetText() or "0", advancedBoxes.o:GetText() or "0",
        advancedBoxes.s:GetText() or "1"))
end)

local placedScroll = CreateFrame("ScrollFrame", "PsychoHousePlacedScroll", placedPane, "UIPanelScrollFrameTemplate")
placedScroll:SetPoint("TOPLEFT", placedPane, "TOPLEFT", 0, -136)
placedScroll:SetPoint("BOTTOMRIGHT", placedPane, "BOTTOMRIGHT", -28, 0)

local placedContent = CreateFrame("Frame", nil, placedScroll)
placedContent:SetSize(520, 1)
placedScroll:SetScrollChild(placedContent)

Menu.placedRows = {}

local function BuildPlaced()

    local sorted = {}

    for _, entry in pairs(H.placed or {}) do
        sorted[#sorted + 1] = entry
    end

    table.sort(sorted, function(left, right)
        return left.id < right.id
    end)

    for _, row in ipairs(Menu.placedRows) do
        row:Hide()
    end

    for index = 1, math.min(#sorted, 40) do
        local entry = sorted[index]
        local row = Menu.placedRows[index]

        if not row then
            row = CreateFrame("Button", nil, placedContent)
            row:SetSize(500, 22)
            row.label = row:CreateFontString(nil, "OVERLAY", "GameFontNormal")
            row.label:SetPoint("LEFT", row, "LEFT", 0, 0)
            row.label:SetWidth(420)
            row.label:SetJustifyH("LEFT")
            row:SetHighlightTexture("Interface\\QuestFrame\\UI-QuestTitleHighlight", "ADD")
            Menu.placedRows[index] = row
        end

        local catalogue = H.catalogue[entry.decorId]
        local name = catalogue and catalogue.name or ("decor " .. tostring(entry.decorId))

        row:SetPoint("TOPLEFT", placedContent, "TOPLEFT", 0, -((index - 1) * 24))
        local tilt = ""
        if (entry.pitch or 0) ~= 0 or (entry.roll or 0) ~= 0 then
            tilt = string.format(DIM .. ", tilt %.0f/%.0f deg" .. RESET,
                math.deg(entry.pitch or 0), math.deg(entry.roll or 0))
        end

        if (entry.petEntry or 0) ~= 0 then
            tilt = tilt .. string.format(DIM .. ", pet %d %s" .. RESET, entry.petEntry,
                entry.petRoaming and "roaming" or "asleep")
        end

        row.label:SetText(string.format("%s%s|r  %s%s, x %.1f y %.1f|r%s",
            entry.indoor and "" or DIM, name,
            entry.indoor and GOOD or DIM, entry.indoor and "indoor" or "outdoor",
            entry.x or 0, entry.y or 0, tilt))

        row:SetScript("OnClick", function()
            Menu.selectedPlacement = entry.id
            placedHeader:SetText("Selected placement " .. tostring(entry.id))

            if Menu.PlacementSelected then
                Menu.PlacementSelected(entry.id)
            end
        end)

        row:Show()
    end

    placedContent:SetHeight(math.max(1, math.min(#sorted, 40) * 24))
end

Menu.onShow.placed = function()
    H.placed = {}
    H.Send("C:PLACED")
end

H.OnPlacedEnd = function()
    BuildPlaced()
end

-- ---------------------------------------------------------------------------
-- Neighbours
-- ---------------------------------------------------------------------------

local neighboursPane = NewPane("neighbours")

local neighboursHeader = NewLabel(neighboursPane, "GameFontNormalLarge", "TOPLEFT", 0, 0)
neighboursHeader:SetText("The street")

local neighboursIntro = NewLabel(neighboursPane, "GameFontDisableSmall", "TOPLEFT", 0, -24)
neighboursIntro:SetWidth(540)
neighboursIntro:SetText("Every plot here, and every public neighbourhood a plot can be bought in.")

NewButton(neighboursPane, "Refresh", 80, 0, -46, function()
    H.plots = {}
    H.finder = {}
    H.Send("C:PLOTS")
    H.Send("C:FINDER")
end)

NewButton(neighboursPane, "Directory", 90, 86, -46, function()
    H.Send("C:DIR|0")
end)

NewButton(neighboursPane, "Found a street", 110, 182, -46, function()
    H.Send("C:FOUND")
end)

NewButton(neighboursPane, "Favour", 70, 298, -46, function()
    H.Send("C:FAVOR")
end)

NewButton(neighboursPane, "Move house here", 120, 0, -72, function()
    if not H.plotIndex then
        H.Print("pick a plot on the Map tab first, or buy one")
        return
    end

    H.Send("C:MOVEHOUSE|" .. tostring(H.plotIndex))
end)

NewButton(neighboursPane, "Reserve free plot", 130, 126, -72, function()
    if not H.plotIndex then
        H.Print("pick a plot on the Map tab first")
        return
    end

    H.Send("C:RESERVE|" .. tostring(H.plotIndex))
end)

NewButton(neighboursPane, "Leave the street", 120, 262, -72, function()
    H.Send("C:LEAVENB")
end)

local neighboursScroll = CreateFrame("ScrollFrame", "PsychoHouseNeighboursScroll", neighboursPane, "UIPanelScrollFrameTemplate")
neighboursScroll:SetPoint("TOPLEFT", neighboursPane, "TOPLEFT", 0, -98)
neighboursScroll:SetPoint("BOTTOMRIGHT", neighboursPane, "BOTTOMRIGHT", -28, 0)

local neighboursContent = CreateFrame("Frame", nil, neighboursScroll)
neighboursContent:SetSize(520, 1)
neighboursScroll:SetScrollChild(neighboursContent)

Menu.plotRows = {}

local function BuildPlots()

    local sorted = {}

    for _, plot in pairs(H.plots or {}) do
        sorted[#sorted + 1] = plot
    end

    table.sort(sorted, function(left, right) return left.index < right.index end)

    for _, row in ipairs(Menu.plotRows) do
        row:Hide()
    end

    local y = 0

    for index = 1, math.min(#sorted, 40) do
        local plot = sorted[index]
        local row = Menu.plotRows[index]

        if not row then
            row = CreateFrame("Frame", nil, neighboursContent)
            row:SetSize(500, 22)
            row.label = row:CreateFontString(nil, "OVERLAY", "GameFontNormal")
            row.label:SetPoint("LEFT", row, "LEFT", 0, 0)
            row.label:SetWidth(310)
            row.label:SetJustifyH("LEFT")
            row.buy = NewButton(row, "Buy", 50, 330, 0, nil)
            row.visit = NewButton(row, "Enter", 60, 386, 0, nil)
            Menu.plotRows[index] = row
        end

        row:SetPoint("TOPLEFT", neighboursContent, "TOPLEFT", 0, -y)
        row.label:SetText(string.format("plot %d - %s (tier %d)",
            plot.index, plot.owner ~= nil and plot.owner ~= "" and plot.owner or DIM .. "empty" .. RESET,
            plot.tier or 0))

        row.buy:SetScript("OnClick", function()
            if plot.state == 0 or (plot.owner == nil or plot.owner == "") then
                H.Send("C:BUY|" .. tostring(plot.index))
            else
                H.Print("plot " .. tostring(plot.index) .. " is taken")
            end
        end)

        if plot.owner ~= nil and plot.owner ~= "" then
            row.visit:Show()
            row.visit:SetScript("OnClick", function()
                H.Send("C:ENTER|" .. tostring(plot.index))
            end)
        else
            row.visit:Hide()
        end

        row:Show()
        y = y + 24
    end

    -- The finder list follows the plots in the same scroll: a street with a free plot is a
    -- place to move to, and the two lists answer the same question.
    local finder = {}

    for _, entry in ipairs(H.finder or {}) do
        finder[#finder + 1] = entry
    end

    if #finder > 0 then
        local heading = Menu.finderHeading

        if not heading then
            heading = CreateFrame("Frame", nil, neighboursContent)
            heading:SetSize(500, 22)
            heading.text = heading:CreateFontString(nil, "OVERLAY", "GameFontNormalSmall")
            heading.text:SetPoint("LEFT", heading, "LEFT", 0, 0)
            Menu.finderHeading = heading
        end

        heading:SetPoint("TOPLEFT", neighboursContent, "TOPLEFT", 0, -y)
        heading.text:SetText(GOLD .. "Public neighbourhoods with room" .. RESET)
        heading:Show()
        y = y + 26
    end

    for index, entry in ipairs(finder) do
        local row = Menu.finderRows[index]

        if not row then
            row = CreateFrame("Frame", nil, neighboursContent)
            row:SetSize(500, 22)
            row.label = row:CreateFontString(nil, "OVERLAY", "GameFontNormal")
            row.label:SetPoint("LEFT", row, "LEFT", 0, 0)
            row.label:SetWidth(310)
            row.label:SetJustifyH("LEFT")
            row.join = NewButton(row, "Join", 50, 330, 0, nil)
            row.visit = NewButton(row, "Visit", 60, 386, 0, nil)
            Menu.finderRows[index] = row
        end

        row:SetPoint("TOPLEFT", neighboursContent, "TOPLEFT", 0, -y)
        row.label:SetText(string.format("%s (%d of %d plots held)",
            tostring(entry.name), entry.owned or 0, entry.total or 0))

        row.join:SetScript("OnClick", function()
            H.Send("C:JOIN|" .. tostring(entry.id))
        end)

        row.visit:SetScript("OnClick", function()
            H.Send("C:VISIT|" .. tostring(entry.id) .. "|0")
        end)

        row:Show()
        y = y + 24
    end

    for index = #finder + 1, #Menu.finderRows do
        Menu.finderRows[index]:Hide()
    end

    neighboursContent:SetHeight(math.max(1, y))
end

Menu.finderRows = {}

Menu.onShow.neighbours = function()
    H.plots = {}
    H.finder = {}
    H.Send("C:PLOTS")
    H.Send("C:FINDER")
end

-- ---------------------------------------------------------------------------
-- Endeavors
-- ---------------------------------------------------------------------------

local endeavorPane = NewPane("endeavors")

local endeavorHeader = NewLabel(endeavorPane, "GameFontNormalLarge", "TOPLEFT", 0, 0)
endeavorHeader:SetText("Endeavor")

local endeavorIntro = NewLabel(endeavorPane, nil, "TOPLEFT", 0, -24)
endeavorIntro:SetWidth(540)
endeavorIntro:SetText("The street works on one Endeavor at a time. Every task that finishes " ..
    "moves the neighbourhood towards the four milestones, and the milestones are what unlock " ..
    "vendor stock - so this tab is the ladder the catalogue's gated rows climb.")

local endeavorName = NewLabel(endeavorPane, "GameFontHighlight", "TOPLEFT", 0, -76)
endeavorName:SetWidth(540)

local endeavorBar = CreateFrame("StatusBar", nil, endeavorPane)
endeavorBar:SetSize(420, 18)
endeavorBar:SetPoint("TOPLEFT", endeavorPane, "TOPLEFT", 0, -100)
endeavorBar:SetStatusBarTexture("Interface\\TargetFrame\\UI-StatusBar")
endeavorBar:SetMinMaxValues(0, 100)
endeavorBar:SetValue(0)

local endeavorBarText = endeavorBar:CreateFontString(nil, "OVERLAY", "GameFontHighlightSmall")
endeavorBarText:SetPoint("CENTER")
endeavorBarText:SetText("no Endeavor running")

local endeavorDone = NewLabel(endeavorPane, "GameFontDisableSmall", "TOPLEFT", 0, -122)
endeavorDone:SetWidth(540)

local endeavorScroll = CreateFrame("ScrollFrame", "PsychoHouseEndeavorScroll", endeavorPane, "UIPanelScrollFrameTemplate")
endeavorScroll:SetPoint("TOPLEFT", endeavorPane, "TOPLEFT", 0, -146)
endeavorScroll:SetPoint("BOTTOMRIGHT", endeavorPane, "BOTTOMRIGHT", -28, 78)

local endeavorContent = CreateFrame("Frame", nil, endeavorScroll)
endeavorContent:SetSize(520, 1)
endeavorScroll:SetScrollChild(endeavorContent)

Menu.taskRows = {}

local function BuildTasks()

    for _, row in ipairs(Menu.taskRows) do
        row:Hide()
    end

    local tasks = {}

    for _, task in ipairs(H.tasks or {}) do
        tasks[#tasks + 1] = task
    end

    for index = 1, math.min(#tasks, 40) do
        local task = tasks[index]
        local row = Menu.taskRows[index]

        if not row then
            row = CreateFrame("Frame", nil, endeavorContent)
            row:SetSize(500, 20)
            row.text = row:CreateFontString(nil, "OVERLAY", "GameFontHighlightSmall")
            row.text:SetPoint("LEFT", row, "LEFT", 0, 0)
            row.text:SetWidth(500)
            row.text:SetJustifyH("LEFT")
            Menu.taskRows[index] = row
        end

        local progress = task.progress or 0
        local required = math.max(1, task.required or 1)
        local mark = progress >= required and GOOD .. "done|r" or string.format("%d/%d", progress, required)

        row:SetPoint("TOPLEFT", endeavorContent, "TOPLEFT", 0, -((index - 1) * 22))
        row.text:SetText(string.format("%s  %s%s|r", mark, DIM, task.text or ("task " .. tostring(task.id))))
        row:Show()
    end

    endeavorContent:SetHeight(math.max(1, math.min(#tasks, 40) * 22))
end

local function RefreshEndeavor()

    local endeavor = H.endeavor or {}
    local total = #(H.tasks or {})
    local done = endeavor.done or 0

    if not endeavor.id or endeavor.id == 0 then
        endeavorName:SetText(DIM .. "no Endeavor running in this neighbourhood" .. RESET)
        endeavorBar:SetValue(0)
        endeavorBarText:SetText("nothing to work on")
        endeavorDone:SetText(DIM .. "an Endeavor starts on the next rotation, or when a " ..
            "private neighbourhood's leader picks one from the Steward" .. RESET)
        BuildTasks()
        return
    end

    local percent = total > 0 and math.floor((done / total) * 100) or 0

    endeavorName:SetText(string.format("%s%s|r  %s%s", GOLD, tostring(endeavor.name or "Endeavor"),
        DIM, endeavor.milestone and ("milestone " .. tostring(endeavor.milestone) .. "/4") or "" ))

    endeavorBar:SetValue(percent)
    endeavorBarText:SetText(string.format("%d of %d tasks, %d%%", done, total, percent))

    if (endeavor.ends or 0) > 0 then
        local left = math.max(0, (endeavor.ends or 0) - (os.time and os.time() or 0))
        endeavorDone:SetText(string.format("%s%d task(s) left, %d day(s) until the rotation gives way%s",
            DIM, math.max(0, total - done), math.floor(left / 86400), RESET))
    else
        endeavorDone:SetText(string.format("%s%d task(s) left%s", DIM, math.max(0, total - done), RESET))
    end

    BuildTasks()
end

Menu.RefreshEndeavor = RefreshEndeavor

NewButton(endeavorPane, "Refresh", 80, 0, -300, function()
    H.tasks = {}
    H.Send("C:ENDEAVOR")
end)

-- The Coffer: the one task a player turns in by hand, and the four milestone claims. Retail
-- pays each milestone once per warband, which is what the server's claim row enforces.
NewButton(endeavorPane, "Open the coffer", 120, 88, -300, function()
    H.Send("C:DELIVER")
end)

Menu.claimButtons = {}

for milestone = 1, 4 do
    local index = milestone
    local button = NewButton(endeavorPane, "Claim " .. index, 64, 216 + ((index - 1) * 70), -300, function()
        H.Send("C:CLAIM|" .. index)
    end)
    Menu.claimButtons[index] = button
end

local milestoneNote = NewLabel(endeavorPane, "GameFontDisableSmall", "TOPLEFT", 0, -330)
milestoneNote:SetWidth(540)
milestoneNote:SetText("Milestones are 25%, 50%, 75% and 100% of the task list. A rotation is " ..
    "driven by the server; a private street's leader picks the next one at the Steward.")

Menu.onShow.endeavors = function()
    H.tasks = {}
    H.Send("C:ENDEAVOR")
end

-- ---------------------------------------------------------------------------
-- Hunts
--
-- The ten riddle treasure hunts: one line of text, a cache hidden somewhere around the
-- town centre, and the piece the cache holds. Taking a riddle is a quest; opening the
-- cache is the objective. The tab lists every hunt the realm has bound, so a hunt whose
-- cache entry nobody has authored says so rather than vanishing.
-- ---------------------------------------------------------------------------

local huntPane = NewPane("hunts")

local huntHeader = NewLabel(huntPane, "GameFontNormalLarge", "TOPLEFT", 0, 0)
huntHeader:SetText("Riddle hunts")

local huntIntro = NewLabel(huntPane, nil, "TOPLEFT", 0, -24)
huntIntro:SetWidth(540)
huntIntro:SetText("Ten caches are hidden around the town centre, each one behind a line of " ..
    "verse. Take a riddle here, walk the ring until the cache answers, and open it for the " ..
    "piece it holds. A cache is only open to the player holding its riddle.")

local huntSummary = NewLabel(huntPane, "GameFontHighlightSmall", "TOPLEFT", 0, -80)

NewButton(huntPane, "Refresh", 80, 0, -100, function()
    H.hunts = {}
    H.Send("C:HUNTS")
end)

local huntScroll = CreateFrame("ScrollFrame", "PsychoHouseHuntScroll", huntPane, "UIPanelScrollFrameTemplate")
huntScroll:SetPoint("TOPLEFT", huntPane, "TOPLEFT", 0, -130)
huntScroll:SetPoint("BOTTOMRIGHT", huntPane, "BOTTOMRIGHT", -28, 0)

local huntContent = CreateFrame("Frame", nil, huntScroll)
huntContent:SetSize(520, 1)
huntScroll:SetScrollChild(huntContent)

Menu.huntRows = {}

local HUNT_STATE = {
    [0] = "not taken",
    [1] = "looking",
    [2] = "found",
}

local function BuildHunts()

    for _, row in ipairs(Menu.huntRows) do
        row:Hide()
    end

    table.sort(H.hunts or {}, function(left, right) return left.id < right.id end)

    local found = 0

    for _, hunt in ipairs(H.hunts or {}) do
        if (hunt.state or 0) == 2 then
            found = found + 1
        end
    end

    huntSummary:SetText(string.format("%s%d hunt(s), %d found%s", DIM,
        #(H.hunts or {}), found, RESET))

    for index = 1, math.min(#(H.hunts or {}), 40) do
        local hunt = H.hunts[index]
        local row = Menu.huntRows[index]

        if not row then
            row = CreateFrame("Frame", nil, huntContent)
            row:SetSize(500, 54)
            row.name = row:CreateFontString(nil, "OVERLAY", "GameFontNormal")
            row.name:SetPoint("TOPLEFT", row, "TOPLEFT", 0, 0)
            row.name:SetWidth(380)
            row.name:SetJustifyH("LEFT")
            row.riddle = row:CreateFontString(nil, "OVERLAY", "GameFontHighlightSmall")
            row.riddle:SetPoint("TOPLEFT", row, "TOPLEFT", 0, -18)
            row.riddle:SetWidth(380)
            row.riddle:SetJustifyH("LEFT")
            row.state = row:CreateFontString(nil, "OVERLAY", "GameFontDisableSmall")
            row.state:SetPoint("TOPLEFT", row, "TOPLEFT", 0, -38)
            row.state:SetWidth(380)
            row.state:SetJustifyH("LEFT")
            row.take = NewButton(row, "Take", 70, 400, -4, nil)
            Menu.huntRows[index] = row
        end

        local state = hunt.state or 0
        local reward = hunt.reward or "unbound"

        row:SetPoint("TOPLEFT", huntContent, "TOPLEFT", 0, -((index - 1) * 58))
        row.name:SetText(string.format("%s%s|r  %s(%s)%s", GOLD, hunt.name or ("hunt " .. tostring(hunt.id)),
            DIM, reward, RESET))
        row.riddle:SetText("\"" .. tostring(hunt.riddle or "") .. "\"")
        row.state:SetText(string.format("%s%s%s", state == 2 and GOOD or DIM,
            HUNT_STATE[state] or "unknown", RESET))

        if state == 0 then
            row.take:Enable()
        else
            row.take:Disable()
        end

        row.take:SetScript("OnClick", function()
            H.Send("C:HUNT|" .. tostring(hunt.id))
        end)

        row:Show()
    end

    huntContent:SetHeight(math.max(1, math.min(#(H.hunts or {}), 40) * 58))
end

Menu.BuildHunts = BuildHunts

Menu.onShow.hunts = function()
    H.hunts = {}
    H.Send("C:HUNTS")
end

-- ---------------------------------------------------------------------------
-- Dyes
-- ---------------------------------------------------------------------------

local dyePane = NewPane("dye")

local dyeHeader = NewLabel(dyePane, "GameFontNormalLarge", "TOPLEFT", 0, 0)
dyeHeader:SetText("Fabric dye")

local dyeIntro = NewLabel(dyePane, nil, "TOPLEFT", 0, -24)
dyeIntro:SetWidth(540)
dyeIntro:SetText("A dye recolours one placed piece, not the room. The swatches come from the " ..
    "server's own housing_dye table, so a realm that adds a seventh colour gets a seventh " ..
    "button without a new addon.")

local dyeTarget = NewLabel(dyePane, "GameFontHighlightSmall", "TOPLEFT", 0, -64)
dyeTarget:SetWidth(540)
dyeTarget:SetText(DIM .. "select a placement in the Placed tab first" .. RESET)

Menu.dyeButtons = {}

-- One button per colour the server sent. Rebuilt rather than created once, because the
-- palette is server data and can be shorter than the last realm the player visited.
local function BuildDyeButtons()
    local colours = {}

    for _, dye in pairs(H.dyes or {}) do
        colours[#colours + 1] = dye
    end

    table.sort(colours, function(left, right) return (left.id or 0) < (right.id or 0) end)

    for _, button in ipairs(Menu.dyeButtons) do
        button:Hide()
    end

    for index, dye in ipairs(colours) do
        local button = Menu.dyeButtons[index]

        if not button then
            button = NewButton(dyePane, dye.name or "?", 130, 0, 0, nil)
            Menu.dyeButtons[index] = button
        end

        button:SetText(dye.name or ("colour " .. tostring(dye.id)))
        button:SetPoint("TOPLEFT", dyePane, "TOPLEFT", 0, -90 - ((index - 1) * 28))
        button:SetScript("OnClick", function()
            if not Menu.selectedPlacement then
                H.Print("select a placement in the Placed tab first")
                return
            end

            -- Group 0 asks the piece which dye group it keeps this colour in; the server
            -- answers NotFound when the piece has no variant row for the colour.
            H.Send(string.format("C:DYE|%s|%d|0", tostring(Menu.selectedPlacement), dye.id))
        end)
        button:Show()
    end

    if #colours == 0 then
        dyeTarget:SetText(BAD .. "the server sent no colours - housing_dye is empty on this realm" .. RESET)
    end
end

Menu.BuildDyeButtons = BuildDyeButtons

NewButton(dyePane, "Strip dye", 110, 150, -90, function()
    if not Menu.selectedPlacement then
        H.Print("select a placement in the Placed tab first")
        return
    end

    -- Colour 0 is the strip: the server clears the variant and respawns the original piece.
    H.Send(string.format("C:DYE|%s|0|0", tostring(Menu.selectedPlacement)))
end)

NewButton(dyePane, "Refresh", 90, 270, -90, function()
    H.dyes = {}
    H.dyesReady = false
    H.Send("C:DYES")
end)

Menu.PlacementSelected = function()
    dyeTarget:SetText(DIM .. "placement " .. tostring(Menu.selectedPlacement) .. RESET)
end

Menu.onShow["dye"] = function()
    if not Menu.selectedPlacement then
        H.Print("pick a placement in the Placed tab to dye it")
    else
        dyeTarget:SetText(DIM .. "placement " .. tostring(Menu.selectedPlacement) .. RESET)
    end

    if not H.dyesReady then
        H.dyes = {}
        H.Send("C:DYES")
    else
        BuildDyeButtons()
    end
end

-- ---------------------------------------------------------------------------
-- Map
-- ---------------------------------------------------------------------------

local mapPane = NewPane("map")

local mapHeader = NewLabel(mapPane, "GameFontNormalLarge", "TOPLEFT", 0, 0)
mapHeader:SetText("The neighbourhood")

local mapIntro = NewLabel(mapPane, nil, "TOPLEFT", 0, -26)
mapIntro:SetWidth(540)
mapIntro:SetText("Every plot on the ring, who holds it, and whether its door is open to visitors. " ..
    "The town centre is the dot in the middle; the gryphon lands beside it.")

local mapScroll = CreateFrame("ScrollFrame", "PsychoHouseMapScroll", mapPane, "UIPanelScrollFrameTemplate")
mapScroll:SetPoint("TOPLEFT", mapPane, "TOPLEFT", 0, -52)
mapScroll:SetPoint("BOTTOMRIGHT", mapPane, "BOTTOMRIGHT", -28, 44)

local mapContent = CreateFrame("Frame", nil, mapScroll)
mapContent:SetSize(520, 1)
mapScroll:SetScrollChild(mapContent)

Menu.mapRows = {}

local function BuildMap()

    for _, row in ipairs(Menu.mapRows) do
        row:Hide()
    end

    local sorted = {}
    for _, plot in ipairs(H.plots or {}) do
        sorted[#sorted + 1] = plot
    end
    table.sort(sorted, function(left, right) return left.index < right.index end)

    local y = 0

    for index, plot in ipairs(sorted) do
        local row = Menu.mapRows[index]
        if not row then
            row = CreateFrame("Button", nil, mapContent)
            row:SetSize(500, 20)
            row.text = row:CreateFontString(nil, "OVERLAY", "GameFontHighlightSmall")
            row.text:SetPoint("LEFT", row, "LEFT", 0, 0)
            row:SetScript("OnClick", function(self)
                H.plotIndex = self.plotIndex
                H.Send("C:PLOT|" .. tostring(self.plotIndex))
                ShowTab("house")
            end)
            row:SetHighlightTexture("Interface\\QuestFrame\\UI-QuestTitleHighlight", "ADD")
            Menu.mapRows[index] = row
        end

        row.plotIndex = plot.index
        row:SetPoint("TOPLEFT", mapContent, "TOPLEFT", 0, -y)
        row:Show()

        local owner = plot.state == 2 and "yours" or (plot.owner ~= "" and plot.owner or "unclaimed")

        row.text:SetText(string.format("%sPlot %d|r - %s - %s",
            plot.state == 2 and GOOD or DIM, plot.index, owner,
            plot.locked and "locked" or "open"))

        y = y + 22
    end

    mapContent:SetHeight(math.max(y, 1))
end

NewButton(mapPane, "Refresh", 90, 430, -24, function()
    H.plots = {}
    H.Send("C:PLOTS")
end)

Menu.onShow = Menu.onShow or {}
Menu.onShow["map"] = function()
    H.plots = {}
    H.Send("C:PLOTS")
    BuildMap()
end

-- ---------------------------------------------------------------------------
-- Blueprints
-- ---------------------------------------------------------------------------

local blueprintPane = NewPane("blueprints")

local blueprintHeader = NewLabel(blueprintPane, "GameFontNormalLarge", "TOPLEFT", 0, 0)
blueprintHeader:SetText("Blueprint codes")

local blueprintIntro = NewLabel(blueprintPane, nil, "TOPLEFT", 0, -24)
blueprintIntro:SetWidth(540)
blueprintIntro:SetText("A blueprint is one line of text: PHP1 followed by a base62 payload of " ..
    "every piece in the house, its position, facing, scale and tilt. Export writes one and " ..
    "shows it here; paste one in and Import asks the server to rebuild it. The code is " ..
    "portable - it survives a database wipe and can be pasted to another player - and the " ..
    "server refuses a code whose checksum does not match, so a truncated paste is rejected " ..
    "rather than half-built.")

local blueprintBox = CreateFrame("EditBox", nil, blueprintPane, "InputBoxTemplate")
blueprintBox:SetSize(520, 20)
blueprintBox:SetPoint("TOPLEFT", blueprintPane, "TOPLEFT", 8, -104)
blueprintBox:SetAutoFocus(false)
blueprintBox:SetMaxLetters(20000)
blueprintBox:SetText("")

local blueprintResult = NewLabel(blueprintPane, nil, "TOPLEFT", 0, -142)
blueprintResult:SetWidth(540)
blueprintResult:SetText(DIM .. "no code loaded" .. RESET)

local blueprintKind = 0
local blueprintKindLabel = NewLabel(blueprintPane, "GameFontHighlightSmall", "TOPLEFT", 250, -172)
blueprintKindLabel:SetText("kind: build")

local function RefreshKindLabel()
    local names = { "build", "room", "exterior" }
    blueprintKindLabel:SetText("kind: " .. (names[blueprintKind + 1] or "build"))
end

NewButton(blueprintPane, "Kind <", 70, 0, -170, function()
    blueprintKind = (blueprintKind + 2) % 3
    RefreshKindLabel()
end)

NewButton(blueprintPane, "Kind >", 70, 76, -170, function()
    blueprintKind = (blueprintKind + 1) % 3
    RefreshKindLabel()
end)

NewButton(blueprintPane, "Export", 90, 156, -170, function()
    H.Send(string.format("C:BPEXP|%d|%s", blueprintKind, H.plotInfo and H.plotInfo.name or "build"))
end)

NewButton(blueprintPane, "Import", 90, 252, -170, function()
    local code = blueprintBox:GetText() or ""
    local ok, reason = H.ValidateBlueprint(code)

    if not ok then
        blueprintResult:SetText(BAD .. reason .. RESET)
        return
    end

    blueprintResult:SetText(DIM .. "asking the server to rebuild this code" .. RESET)
    H.Send("C:BPIMP|" .. code)
end)

NewButton(blueprintPane, "Check", 80, 0, -200, function()
    local ok, reason, payload = H.ValidateBlueprint(blueprintBox:GetText() or "")

    if ok then
        blueprintResult:SetText(GOOD .. "the envelope is right: " .. #payload ..
            " characters of payload, and the server decides the rest" .. RESET)
    else
        blueprintResult:SetText(BAD .. tostring(reason) .. RESET)
    end
end)

NewButton(blueprintPane, "Copy", 80, 86, -200, function()
    local code = blueprintBox:GetText() or ""

    if code == "" then
        blueprintResult:SetText(BAD .. "nothing to copy" .. RESET)
        return
    end

    -- No clipboard API in 8.3.7: the box is selectable, which is the whole mechanism. Put the
    -- code in the chat edit box as well, where Ctrl-A Ctrl-C works.
    if ChatFrame1EditBox then
        ChatFrame1EditBox:Show()
        ChatFrame1EditBox:SetText(code)
        ChatFrame1EditBox:HighlightText()
    end

    blueprintResult:SetText(DIM .. "the code is in the chat box, selected - Ctrl-C to copy" .. RESET)
end)

NewButton(blueprintPane, "Restore house", 120, 172, -200, function()
    blueprintResult:SetText(DIM .. "asking the server to unpack the archived house" .. RESET)
    H.Send("C:RESTORE")
end)

NewButton(blueprintPane, "Clear", 70, 298, -200, function()
    blueprintBox:SetText("")
    blueprintResult:SetText(DIM .. "no code loaded" .. RESET)
end)

local blueprintNote = NewLabel(blueprintPane, "GameFontDisableSmall", "TOPLEFT", 0, -234)
blueprintNote:SetWidth(540)
blueprintNote:SetText("Kind: 0 build (every piece), 1 room (the interior only), 2 exterior " ..
    "(the yard only). Restore is the regret window from the House tab: a house sold inside " ..
    "Housing.Evict.GraceDays comes back, one sold after it does not.")

RefreshKindLabel()

Menu.onShow.blueprints = function()
    if H.lastCode and H.lastCode ~= "" then
        blueprintBox:SetText(H.lastCode)
    end
end

-- ---------------------------------------------------------------------------
-- Guide
-- ---------------------------------------------------------------------------

local guidePane = NewPane("guide")

local guideHeader = NewLabel(guidePane, "GameFontNormalLarge", "TOPLEFT", 0, 0)
guideHeader:SetText("What this menu sends")

local GUIDE_LINES = {
    "|cffffd100House|r      C:NEIGHBORHOOD, C:PLOT <n>, C:HOME, C:LEAVE, C:NAME <text>,",
    "            C:LOCK <0|1>, C:TIER <2-5>",
    "|cffffd100Catalogue|r  C:DECOR [category] [page] [subcategory] -> S:DECORENTRY per decor, S:DECOREND at the end",
    "|cffffd100Buy|r        C:BUYDECOR <id> -> S:BUYRESULT",
    "|cffffd100Placed|r     C:PLACED -> S:PLACEDENTRY per placement, S:PLACEDEND",
    "|cffffd100Neighbours|r C:PLOTS -> S:PLOT per plot; C:DIR <page> -> S:DIRENTRY",
    "|cffffd100Place|r      C:PLACE <decorId> <x> <y> <z> <o> <scale> <indoor> [pitch] [roll]",
    "|cffffd100Move|r       C:MOVE <placementId> <x> <y> <z> <o> <scale> [pitch] [roll]",
    "|cffffd100Remove|r     C:REMOVE <placementId>",
    "|cffffd100Guests|r     C:GUESTLIST, C:GUESTADD <name> <flags>, C:GUESTDEL <name>",
    "|cffffd100Access|r     C:ACCESS <0 yard|1 inside> <mask> -> S:RESULT; the masks come back on S:PLOTINFO",
    "|cffffd100Amenities|r  C:VAREA -> S:VAREA per amenity, S:VAREAEND at the end",
    "|cffffd100Finder|r     C:FINDER -> S:FINDER per open neighbourhood, S:FINDEREND",
    "|cffffd100Visit|r      C:VISIT <neighborhood> <plot>, C:JOIN <neighborhood>, C:RESERVE <plot>",
    "|cffffd100Neighbourhood|r C:FOUND, C:LEAVENB, C:MOVEHOUSE <plot>, C:FAVOR, C:BUY <plot>, C:SELL",
    "|cffffd100Storage|r    C:STORAGE -> S:STORAGE per copy, C:BUYCOPY <decorId> <count>",
    "|cffffd100Shells|r     C:SHELLS / C:SHELL <id> -> S:SHELL per exterior",
    "|cffffd100Rooms|r      C:ROOMS / C:ROOM <id> -> S:ROOM per room",
    "|cffffd100Progression|r C:LEVELUP, C:ENDEAVOR, C:DELIVER, C:CLAIM <milestone>",
    "|cffffd100Devices|r    C:ARCANE (weekly Sphere), C:ARMILLARY [0-4] (level 6, five presets)",
    "|cffffd100Travel|r     C:GRYPHON (gold + cooldown), C:PORTALS, C:GOPORTAL <id>",
    "|cffffd100Dyes|r      C:DYES -> S:DYE per colour the server has; C:DYE <placement> <colour> [group], 0 strips it",
    "|cffffd100Blueprints|r C:BPIMP <code>, C:BPEXP -> S:BPCODE, C:RESTORE",
    "|cffffd100Place at|r   C:PLACEAT <decorId> <metres> [indoor], C:BUILDMODE <0|1> [decorId]",
}

for index, line in ipairs(GUIDE_LINES) do
    local label = NewLabel(guidePane, "GameFontHighlightSmall", "TOPLEFT", 0, -30 - ((index - 1) * 20))
    label:SetWidth(540)
    label:SetText(line)
end

local guideNote = NewLabel(guidePane, "GameFontDisableSmall", "TOPLEFT", 0, -256)
guideNote:SetWidth(540)
guideNote:SetText("Every one of these has a /housing equivalent. The menu exists so nobody has to " ..
    "type them, not so the transport is hidden.")

-- ---------------------------------------------------------------------------
-- Settings
--
-- The client half of the configuration. These are the values the transport uses before
-- anything is sent, so they belong to the player and not to the server's conf file, and they
-- are written to HousingUIDB as soon as they change.
-- ---------------------------------------------------------------------------

local settingsPane = NewPane("settings")

local settingsHeader = NewLabel(settingsPane, "GameFontNormalLarge", "TOPLEFT", 0, 0)
settingsHeader:SetText("Client settings")

local settingsNote = NewLabel(settingsPane, "GameFontDisableSmall", "TOPLEFT", 0, -24)
settingsNote:SetWidth(540)
settingsNote:SetText("Saved in HousingUIDB, so they survive a reload and a login. The server's " ..
    "own settings are in mod-psycho-house.conf; .housing config prints them.")

local settingRows = {}
local rowIndex = 0

-- One row: a caption on the left, the current value in the middle, and two buttons. Every row
-- reads its value through the getter it is given, so a refresh never has to know the shape of
-- H.settings.
local function AddSettingRow(caption, getter, onStep)
    local y = -60 - (rowIndex * 30)
    rowIndex = rowIndex + 1

    local label = NewLabel(settingsPane, "GameFontHighlightSmall", "TOPLEFT", 0, y)
    label:SetWidth(220)
    label:SetText(caption)

    local value = NewLabel(settingsPane, "GameFontNormal", "TOPLEFT", 230, y)
    value:SetWidth(150)
    value:SetJustifyH("RIGHT")

    local minus = NewButton(settingsPane, "-", 28, 392, y + 2, function()
        onStep(-1)
        Menu.RefreshSettings()
    end)

    local plus = NewButton(settingsPane, "+", 28, 428, y + 2, function()
        onStep(1)
        Menu.RefreshSettings()
    end)

    settingRows[#settingRows + 1] = { getter = getter, label = value, minus = minus, plus = plus }
end

AddSettingRow("Grid snapping", function()
    return H.settings.snapEnabled and "on" or "off"
end, function()
    H.settings.snapEnabled = not H.settings.snapEnabled
    H.SaveSettings()
end)

AddSettingRow("Grid size", function()
    return string.format("%.2f yd", H.settings.grid)
end, function(direction)
    H.StepSetting("grid", direction)
end)

AddSettingRow("Rotation step", function()
    return string.format("%.1f deg", math.deg(H.settings.angle))
end, function(direction)
    H.StepSetting("angle", direction)
end)

AddSettingRow("Scale step", function()
    return string.format("%.2f", H.settings.scaleStep)
end, function(direction)
    H.StepSetting("scaleStep", direction)
end)

AddSettingRow("Window scale", function()
    return string.format("%.2f", H.settings.frameScale)
end, function(direction)
    H.StepSetting("frameScale", direction)
    frame:SetScale(H.settings.frameScale)
end)

function Menu.RefreshSettings()
    for _, row in ipairs(settingRows) do
        row.label:SetText(row.getter())
    end
end

NewButton(settingsPane, "Reset to defaults", 150, 0, -240, function()
    H.ResetSettings()
    frame:SetScale(H.settings.frameScale)
    Menu.RefreshSettings()
end)

NewButton(settingsPane, "Centre the window", 150, 160, -240, function()
    H.settings.framePoint = nil
    H.SaveSettings()
    frame:ClearAllPoints()
    frame:SetPoint("CENTER")
end)

Menu.onShow = Menu.onShow or {}
Menu.onShow.settings = function()
    Menu.RefreshSettings()
end

-- ---------------------------------------------------------------------------
-- Rooms and shells
--
-- Retail sells room shells and exterior shells from the same window it sells decor. Here the
-- shell is a game object the house spawns, and a room is a shell placed inside it: both are
-- server data (C:SHELLS, C:ROOMS), both are bought with `C:SHELL` and `C:ROOM`.
-- ---------------------------------------------------------------------------

local roomsPane = NewPane("rooms")

local roomsHeader = NewLabel(roomsPane, "GameFontNormalLarge", "TOPLEFT", 0, 0)
roomsHeader:SetText("Shells and rooms")

local roomsIntro = NewLabel(roomsPane, nil, "TOPLEFT", 0, -24)
roomsIntro:SetWidth(540)
roomsIntro:SetText("The exterior shell is the house itself; rooms are placed inside it and " ..
    "come out of the room budget, not the decor budget.")

NewButton(roomsPane, "Refresh", 80, 0, -64, function()
    H.shells = {}
    H.rooms = {}
    H.Send("C:SHELLS")
    H.Send("C:ROOMS")
end)

local roomsOwned = NewLabel(roomsPane, "GameFontHighlightSmall", "TOPLEFT", 92, -68)
roomsOwned:SetWidth(400)
roomsOwned:SetText(DIM .. "rooms owned: -" .. RESET)

local shellScroll = CreateFrame("ScrollFrame", "PsychoHouseShellScroll", roomsPane, "UIPanelScrollFrameTemplate")
shellScroll:SetPoint("TOPLEFT", roomsPane, "TOPLEFT", 0, -92)
shellScroll:SetPoint("BOTTOMRIGHT", roomsPane, "BOTTOMRIGHT", -28, 196)

local shellContent = CreateFrame("Frame", nil, shellScroll)
shellContent:SetSize(520, 1)
shellScroll:SetScrollChild(shellContent)

local roomScroll = CreateFrame("ScrollFrame", "PsychoHouseRoomScroll", roomsPane, "UIPanelScrollFrameTemplate")
roomScroll:SetPoint("TOPLEFT", roomsPane, "TOPLEFT", 0, -212)
roomScroll:SetPoint("BOTTOMRIGHT", roomsPane, "BOTTOMRIGHT", -28, 0)

local roomContent = CreateFrame("Frame", nil, roomScroll)
roomContent:SetSize(520, 1)
roomScroll:SetScrollChild(roomContent)

local shellHeading = NewLabel(roomsPane, nil, "TOPLEFT", 0, -90)

Menu.shellRows = {}
Menu.roomRows = {}

local SHELL_SIZE = { "small", "medium", "large" }

local function BuildShells()

    shellHeading:SetText(GOLD .. "Exterior shells" .. RESET)

    for _, row in ipairs(Menu.shellRows) do
        row:Hide()
    end

    local sorted = {}

    for id, shell in pairs(H.shells or {}) do
        sorted[#sorted + 1] = { id = id, name = shell.name, size = shell.size, level = shell.level }
    end

    table.sort(sorted, function(left, right) return left.id < right.id end)

    for index = 1, math.min(#sorted, 20) do
        local shell = sorted[index]
        local row = Menu.shellRows[index]

        if not row then
            row = CreateFrame("Frame", nil, shellContent)
            row:SetSize(500, 22)
            row.label = row:CreateFontString(nil, "OVERLAY", "GameFontNormal")
            row.label:SetPoint("LEFT", row, "LEFT", 0, 0)
            row.label:SetWidth(330)
            row.label:SetJustifyH("LEFT")
            row.pick = NewButton(row, "Use", 60, 380, 0, nil)
            Menu.shellRows[index] = row
        end

        row:SetPoint("TOPLEFT", shellContent, "TOPLEFT", 0, -((index - 1) * 24))
        row.label:SetText(string.format("%s - %s, level %d", tostring(shell.name),
            SHELL_SIZE[(shell.size or 0) + 1] or "small", shell.level or 1))
        row.pick:SetScript("OnClick", function()
            H.Send("C:SHELL|" .. tostring(shell.id))
        end)
        row:Show()
    end

    shellContent:SetHeight(math.max(1, math.min(#sorted, 20) * 24))
end

local function BuildRooms()

    for _, row in ipairs(Menu.roomRows) do
        row:Hide()
    end

    local sorted = {}

    for _, room in ipairs(H.rooms or {}) do
        sorted[#sorted + 1] = room
    end

    for index = 1, math.min(#sorted, 20) do
        local room = sorted[index]
        local row = Menu.roomRows[index]

        if not row then
            row = CreateFrame("Frame", nil, roomContent)
            row:SetSize(500, 22)
            row.label = row:CreateFontString(nil, "OVERLAY", "GameFontNormal")
            row.label:SetPoint("LEFT", row, "LEFT", 0, 0)
            row.label:SetWidth(330)
            row.label:SetJustifyH("LEFT")
            row.buy = NewButton(row, "Add", 60, 380, 0, nil)
            Menu.roomRows[index] = row
        end

        row:SetPoint("TOPLEFT", roomContent, "TOPLEFT", 0, -((index - 1) * 24))

        -- The fifth field is a HousingResult, not a flag: 0 means the server would sell this
        -- room right now, anything else is the reason it would not. Reading it as a boolean
        -- backwards would hide every room a player can actually buy.
        local result = room.state or 0
        local state = result == 0 and GOOD .. "can add|r"
            or string.format("%sbudget %d, level %d - not yet: result %d%s",
                DIM, room.cost or 0, room.level or 1, result, RESET)

        row.label:SetText(string.format("%s  %s", tostring(room.name), state))

        if result ~= 0 then
            row.buy:Hide()
        else
            row.buy:Show()
            row.buy:SetScript("OnClick", function()
                H.Send("C:ROOM|" .. tostring(room.id))
            end)
        end

        row:Show()
    end

    roomContent:SetHeight(math.max(1, math.min(#sorted, 20) * 24))
end

Menu.onShow.rooms = function()
    H.shells = {}
    H.rooms = {}
    H.Send("C:SHELLS")
    H.Send("C:ROOMS")
end

-- ---------------------------------------------------------------------------
-- Storage
--
-- Retail's rule, and this module's: four chairs are four chairs. The collection row gates
-- permission; the storage row is the physical count. Placing spends one, removing returns
-- one, and BUYCOPY buys another.
-- ---------------------------------------------------------------------------

local storagePane = NewPane("storage")

local storageHeader = NewLabel(storagePane, "GameFontNormalLarge", "TOPLEFT", 0, 0)
storageHeader:SetText("What is in store")

local storageIntro = NewLabel(storagePane, nil, "TOPLEFT", 0, -24)
storageIntro:SetWidth(540)
storageIntro:SetText("Buying a piece gives one copy. Placing it spends the copy; picking it up " ..
    "puts it back. Buy another copy from here rather than from the catalogue when the piece " ..
    "is already known.")

local storageTotal = NewLabel(storagePane, "GameFontHighlightSmall", "TOPLEFT", 0, -64)

NewButton(storagePane, "Refresh", 80, 0, -84, function()
    H.storage = {}
    H.Send("C:STORAGE")
end)

local storageScroll = CreateFrame("ScrollFrame", "PsychoHouseStorageScroll", storagePane, "UIPanelScrollFrameTemplate")
storageScroll:SetPoint("TOPLEFT", storagePane, "TOPLEFT", 0, -112)
storageScroll:SetPoint("BOTTOMRIGHT", storagePane, "BOTTOMRIGHT", -28, 0)

local storageContent = CreateFrame("Frame", nil, storageScroll)
storageContent:SetSize(520, 1)
storageScroll:SetScrollChild(storageContent)

Menu.storageRows = {}

local function BuildStorage()

    for _, row in ipairs(Menu.storageRows) do
        row:Hide()
    end

    local sorted = {}
    local total = 0

    for decorId, count in pairs(H.storage or {}) do
        sorted[#sorted + 1] = { id = decorId, count = count }
        total = total + count
    end

    table.sort(sorted, function(left, right) return left.id < right.id end)

    storageTotal:SetText(string.format("%s%d piece(s) in store%s", DIM, total, RESET))

    for index = 1, math.min(#sorted, 40) do
        local entry = sorted[index]
        local row = Menu.storageRows[index]

        if not row then
            row = CreateFrame("Frame", nil, storageContent)
            row:SetSize(500, 22)
            row.label = row:CreateFontString(nil, "OVERLAY", "GameFontNormal")
            row.label:SetPoint("LEFT", row, "LEFT", 0, 0)
            row.label:SetWidth(330)
            row.label:SetJustifyH("LEFT")
            row.buy = NewButton(row, "Buy one", 70, 370, 0, nil)
            Menu.storageRows[index] = row
        end

        local catalogue = H.catalogue[entry.id]
        local name = catalogue and catalogue.name or ("decor " .. tostring(entry.id))

        row:SetPoint("TOPLEFT", storageContent, "TOPLEFT", 0, -((index - 1) * 24))
        row.label:SetText(string.format("%s  %sx%d%s", name, GOLD, entry.count, RESET))
        row.buy:SetScript("OnClick", function()
            H.Send("C:BUYCOPY|" .. tostring(entry.id) .. "|1")
        end)
        row:Show()
    end

    storageContent:SetHeight(math.max(1, math.min(#sorted, 40) * 24))
end

Menu.onShow.storage = function()
    H.storage = {}
    H.Send("C:STORAGE")
end

-- ---------------------------------------------------------------------------
-- Travel
--
-- Every way out of the house, and the two devices that are not decor: the arcanostorage
-- and the gryphon. All of them are server-authoritative - the button asks, the server
-- decides about cooldowns, gold and level.
-- ---------------------------------------------------------------------------

local travelPane = NewPane("travel")

local travelHeader = NewLabel(travelPane, "GameFontNormalLarge", "TOPLEFT", 0, 0)
travelHeader:SetText("Getting there")

local travelIntro = NewLabel(travelPane, nil, "TOPLEFT", 0, -24)
travelIntro:SetWidth(540)
travelIntro:SetText("The homestone, the door, the gryphon and the portals. Each one has its " ..
    "own cost or cooldown, and the server is the one that knows it.")

NewButton(travelPane, "Homestone", 90, 0, -64, function() H.Send("C:HOME") end)
NewButton(travelPane, "Step inside", 100, 96, -64, function() H.Send("C:INTERIOR|1") end)
NewButton(travelPane, "Step outside", 100, 202, -64, function() H.Send("C:INTERIOR|0") end)
NewButton(travelPane, "Back to town", 110, 308, -64, function() H.Send("C:LEAVE") end)

NewButton(travelPane, "Gryphon", 90, 0, -92, function() H.Send("C:GRYPHON") end)
NewButton(travelPane, "Arcanostorage", 120, 96, -92, function() H.Send("C:ARCANE") end)
NewButton(travelPane, "Portals", 90, 222, -92, function()
    H.portals = {}
    H.Send("C:PORTALS")
end)

local portalHeading = NewLabel(travelPane, nil, "TOPLEFT", 0, -122)
portalHeading:SetText(GOLD .. "Portal destinations" .. RESET)

local portalScroll = CreateFrame("ScrollFrame", "PsychoHousePortalScroll", travelPane, "UIPanelScrollFrameTemplate")
portalScroll:SetPoint("TOPLEFT", travelPane, "TOPLEFT", 0, -144)
portalScroll:SetPoint("BOTTOMRIGHT", travelPane, "BOTTOMRIGHT", -28, 0)

local portalContent = CreateFrame("Frame", nil, portalScroll)
portalContent:SetSize(520, 1)
portalScroll:SetScrollChild(portalContent)

Menu.portalRows = {}

local function BuildPortals()

    for _, row in ipairs(Menu.portalRows) do
        row:Hide()
    end

    local sorted = {}

    for id, portal in pairs(H.portals or {}) do
        sorted[#sorted + 1] = { id = id, name = portal.name, level = portal.level }
    end

    table.sort(sorted, function(left, right) return left.id < right.id end)

    for index = 1, math.min(#sorted, 20) do
        local portal = sorted[index]
        local row = Menu.portalRows[index]

        if not row then
            row = CreateFrame("Frame", nil, portalContent)
            row:SetSize(500, 22)
            row.label = row:CreateFontString(nil, "OVERLAY", "GameFontNormal")
            row.label:SetPoint("LEFT", row, "LEFT", 0, 0)
            row.label:SetWidth(330)
            row.label:SetJustifyH("LEFT")
            row.go = NewButton(row, "Go", 60, 380, 0, nil)
            Menu.portalRows[index] = row
        end

        row:SetPoint("TOPLEFT", portalContent, "TOPLEFT", 0, -((index - 1) * 24))
        row.label:SetText(string.format("%s  %shouse level %d%s", tostring(portal.name), DIM,
            portal.level or 1, RESET))
        row.go:SetScript("OnClick", function()
            H.Send("C:GOPORTAL|" .. tostring(portal.id))
        end)
        row:Show()
    end

    portalContent:SetHeight(math.max(1, math.min(#sorted, 20) * 24))
end

Menu.onShow.travel = function()
    H.portals = {}
    H.Send("C:PORTALS")
end

-- ---------------------------------------------------------------------------
-- Launcher button
-- ---------------------------------------------------------------------------

local launcher = CreateFrame("Button", "PsychoHouseLauncher", UIParent, "UIPanelButtonTemplate")
launcher:SetSize(64, 22)
launcher:SetPoint("TOPRIGHT", Minimap, "BOTTOMRIGHT", -6, -6)
launcher:SetText("House")
launcher:SetScript("OnClick", function()
    if frame:IsShown() then
        frame:Hide()
    else
        frame:Show()
        ShowTab(Menu.current)
    end
end)

-- ---------------------------------------------------------------------------
-- Wiring
-- ---------------------------------------------------------------------------

H.plotInfo = H.plotInfo

local function StorePlotInfo(fields)
    H.plotInfo = {
        index = tonumber(fields[2]),
        biome = tonumber(fields[3]),
        price = tonumber(fields[4]),
        sizeClass = tonumber(fields[5]),
        state = tonumber(fields[6]),
        tier = tonumber(fields[7]),
        owner = fields[8],
        name = fields[9],
        locked = fields[10] == "1",
        budgetIndoor = tonumber(fields[11]),
        budgetOutdoor = tonumber(fields[12]),
        usedIndoor = tonumber(fields[13]),
        usedOutdoor = tonumber(fields[14]),
        plotMask = tonumber(fields[15]) or 0,
        houseMask = tonumber(fields[16]) or 0,
        armillary = tonumber(fields[17]) or 0,
    }

    H.plotIndex = H.plotInfo.index

    if frame:IsShown() then
        if H.plotInfo.name and H.plotInfo.name ~= "" then
            houseName:SetText(H.plotInfo.name)
        end
        RefreshHouse()
        RefreshArmillary()
    end
end

local previousPlotInfo = H.handlers["PLOTINFO"]
H.handlers["PLOTINFO"] = function(fields)
    if previousPlotInfo then
        previousPlotInfo(fields)
    end
    StorePlotInfo(fields)

    -- A mask that changed at the Steward or in another window arrives with the plot, not with
    -- its own opcode, so the buttons follow the same line the card does.
    if Menu.RefreshPermissions then
        Menu.RefreshPermissions()
    end
end

H.amenities = H.amenities or {}

local previousVarea = H.handlers["VAREA"]
H.handlers["VAREA"] = function(fields)
    if previousVarea then
        previousVarea(fields)
    end
    H.amenities[#H.amenities + 1] = {
        kind = tonumber(fields[2]),
        name = fields[3],
        x = tonumber(fields[4]),
        y = tonumber(fields[5]),
        biome = tonumber(fields[6]),
    }
end

local previousVareaEnd = H.handlers["VAREAEND"]
H.handlers["VAREAEND"] = function(fields)
    if previousVareaEnd then
        previousVareaEnd(fields)
    end
    if frame:IsShown() then
        BuildAmenities()
    end
end

local previousNbr = H.handlers["NBR"]
H.handlers["NBR"] = function(fields)
    if previousNbr then
        previousNbr(fields)
    end
    if frame:IsShown() then
        RefreshHouse()
    end
end

local previousDecorEnd = H.handlers["DECOREND"]
H.handlers["DECOREND"] = function(fields)
    if previousDecorEnd then
        previousDecorEnd(fields)
    end
    if Menu.current == "catalogue" then
        BuildCatalogue()
    end
end

local previousPlacedEnd = H.handlers["PLACEDEND"]
H.handlers["PLACEDEND"] = function(fields)
    if previousPlacedEnd then
        previousPlacedEnd(fields)
    end
    if H.OnPlacedEnd then
        H.OnPlacedEnd()
    end
end

local previousPlotList = H.handlers["PLOT"]
H.handlers["PLOT"] = function(fields)
    if previousPlotList then
        previousPlotList(fields)
    end
    if Menu.current == "neighbours" then
        BuildPlots()
    end
end

-- ---------------------------------------------------------------------------
-- Redraws
--
-- Each of these chains to whatever handler Core.lua installed, so the state is always
-- updated first and the pane only redraws. A pane that is not showing is not touched.
-- ---------------------------------------------------------------------------

local previousShellEnd = H.handlers["SHELLEND"]
H.handlers["SHELLEND"] = function(fields)
    if previousShellEnd then
        previousShellEnd(fields)
    end
    if Menu.current == "rooms" then
        BuildShells()
    end
end

local previousRooms = H.handlers["ROOMS"]
H.handlers["ROOMS"] = function(fields)
    if previousRooms then
        previousRooms(fields)
    end
    roomsOwned:SetText(string.format("%srooms owned: %s%s", DIM,
        tostring(H.roomsOwned or 0), RESET))
    if Menu.current == "rooms" then
        BuildRooms()
    end
end

local previousRoom = H.handlers["ROOM"]
H.handlers["ROOM"] = function(fields)
    if previousRoom then
        previousRoom(fields)
    end
    if Menu.current == "rooms" then
        BuildRooms()
    end
end

local previousStorageEnd = H.handlers["STORAGEEND"]
H.handlers["STORAGEEND"] = function(fields)
    if previousStorageEnd then
        previousStorageEnd(fields)
    end
    if Menu.current == "storage" then
        BuildStorage()
    end
end

local previousPortalEnd = H.handlers["PORTALEND"]
H.handlers["PORTALEND"] = function(fields)
    if previousPortalEnd then
        previousPortalEnd(fields)
    end
    if Menu.current == "travel" then
        BuildPortals()
    end
end

local previousTaskEnd = H.handlers["ENDTASKEND"]
H.handlers["ENDTASKEND"] = function(fields)
    if previousTaskEnd then
        previousTaskEnd(fields)
    end
    if Menu.current == "endeavors" then
        RefreshEndeavor()
    end
end

local previousHuntEnd = H.handlers["HUNTEND"]
H.handlers["HUNTEND"] = function(fields)
    if previousHuntEnd then
        previousHuntEnd(fields)
    end
    if Menu.current == "hunts" then
        BuildHunts()
    end
end

local previousBpCode = H.handlers["BPCODE"]
H.handlers["BPCODE"] = function(fields)
    if previousBpCode then
        previousBpCode(fields)
    end
    blueprintBox:SetText(fields[2] or "")
    blueprintResult:SetText(GOOD .. "exported - " .. string.len(fields[2] or "") ..
        " characters, kind " .. tostring(blueprintKind) .. RESET)
end

local previousDyeEnd = H.handlers["DYEEND"]
H.handlers["DYEEND"] = function(fields)
    if previousDyeEnd then
        previousDyeEnd(fields)
    end
    if Menu.current == "dye" then
        BuildDyeButtons()
    end
end

local previousGuest = H.handlers["GUEST"]
H.handlers["GUEST"] = function(fields)
    if previousGuest then
        previousGuest(fields)
    end
    H.guests = H.guests or {}
    H.guests[fields[2]] = tonumber(fields[3]) or 0
    if Menu.current == "access" then
        BuildGuests()
    end
end

local previousGuestEnd = H.handlers["GUESTEND"]
H.handlers["GUESTEND"] = function(fields)
    if previousGuestEnd then
        previousGuestEnd(fields)
    end
    if Menu.current == "access" then
        BuildGuests()
    end
end

local previousResult = H.handlers["RESULT"]
H.handlers["RESULT"] = function(fields)
    if previousResult then
        previousResult(fields)
    end
    if fields[2] == "DYE" and Menu.current == "dye" and Menu.selectedPlacement then
        dyeTarget:SetText(DIM .. "placement " .. tostring(Menu.selectedPlacement) .. RESET)
    end
end

SLASH_HOUSINGUI1 = "/house"
SlashCmdList["HOUSINGUI"] = function()
    if frame:IsShown() then
        frame:Hide()
    else
        frame:Show()
        ShowTab(Menu.current)
    end
end

print("|cff66c2ffHousing|r menu loaded - /house opens the housing window")
