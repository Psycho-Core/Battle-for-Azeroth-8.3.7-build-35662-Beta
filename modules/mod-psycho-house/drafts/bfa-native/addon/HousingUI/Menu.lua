--[[
    HousingUI - Menu.lua

    The housing menu. Core.lua is the transport; this is the part with frames on it,
    arranged the way the Midnight housing window is arranged: a tab rail down the left,
    one pane per tab, and the house card at the top of the pane you land on.

    Tabs, in Midnight's order:

        House        the plot card: name, tier, budgets, homestone, lock
        Catalogue    everything buyable, filterable, with the currency named
        Placed       what is standing in the house right now
        Neighbours   the directory: every owned plot, with a visit button
        Endeavors    the four milestones and what each one unlocks
        Blueprints   the PCH1 code format (server side is phase 4)
        Guide        what the menu sends, so the transport is never a mystery

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

local TABS = {
    { id = "house", label = "House" },
    { id = "catalogue", label = "Catalogue" },
    { id = "placed", label = "Placed" },
    { id = "neighbours", label = "Neighbours" },
    { id = "endeavors", label = "Endeavors" },
    { id = "blueprints", label = "Blueprints" },
    { id = "guide", label = "Guide" },
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
frame:SetScript("OnDragStop", frame.StopMovingOrSizing)
frame:Hide()

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
    button:SetSize(128, 24)
    button:SetPoint("TOPLEFT", rail, "TOPLEFT", 0, -((index - 1) * 28))
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

NewButton(housePane, "Refresh", 90, 130, -192, function()
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

-- Amenities: the hub's fixed points - stage, mailbox, bulletin board, the rest. They are
-- server data (C:VAREA), not addon guesses, because the module moves them with the map.

local amenityHeader = NewLabel(housePane, nil, "TOPLEFT", 0, -286)
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
            row = NewLabel(housePane, "GameFontHighlightSmall", "TOPLEFT", 0, -306 - ((index - 1) * 16))
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
    H.amenities = {}
    H.Send("C:VAREA")
    if H.plotIndex then
        H.Send("C:PLOT|" .. H.plotIndex)
    end
end

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

    for _, entry in pairs(H.catalogue or {}) do
        if filter == "" or string.find(string.lower(entry.name or ""), filter, 1, true) then
            sorted[#sorted + 1] = entry
        end
    end

    table.sort(sorted, function(left, right)
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
            Menu.catalogueRows[index] = row
        end

        row:SetPoint("TOPLEFT", catalogueContent, "TOPLEFT", 0, -((index - 1) * 24))
        row.name:SetText((entry.unlocked and GOOD or "") .. (entry.name or "?") .. RESET)

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

local placedScroll = CreateFrame("ScrollFrame", "PsychoHousePlacedScroll", placedPane, "UIPanelScrollFrameTemplate")
placedScroll:SetPoint("TOPLEFT", placedPane, "TOPLEFT", 0, -26)
placedScroll:SetPoint("BOTTOMRIGHT", placedPane, "BOTTOMRIGHT", -28, 30)

local placedContent = CreateFrame("Frame", nil, placedScroll)
placedContent:SetSize(520, 1)
placedScroll:SetScrollChild(placedContent)

NewButton(placedPane, "Remove selected", 140, 0, -4, function()
    if Menu.selectedPlacement then
        H.Send("C:REMOVE|" .. tostring(Menu.selectedPlacement))
    end
end)

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
        row.label:SetText(string.format("%s%s|r  %s%s, x %.1f y %.1f|r",
            entry.indoor and "" or DIM, name,
            entry.indoor and GOOD or DIM, entry.indoor and "indoor" or "outdoor",
            entry.x or 0, entry.y or 0))

        row:SetScript("OnClick", function()
            Menu.selectedPlacement = entry.id
            placedHeader:SetText("Selected placement " .. tostring(entry.id))
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
neighboursHeader:SetText("Plots in this neighbourhood")

NewButton(neighboursPane, "Refresh", 90, 0, -26, function()
    H.plots = {}
    H.Send("C:PLOTS")
end)

NewButton(neighboursPane, "Directory", 90, 100, -26, function()
    H.Send("C:DIR|0")
end)

local neighboursScroll = CreateFrame("ScrollFrame", "PsychoHouseNeighboursScroll", neighboursPane, "UIPanelScrollFrameTemplate")
neighboursScroll:SetPoint("TOPLEFT", neighboursPane, "TOPLEFT", 0, -56)
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

    for index = 1, math.min(#sorted, 40) do
        local plot = sorted[index]
        local row = Menu.plotRows[index]

        if not row then
            row = CreateFrame("Frame", nil, neighboursContent)
            row:SetSize(500, 22)
            row.label = row:CreateFontString(nil, "OVERLAY", "GameFontNormal")
            row.label:SetPoint("LEFT", row, "LEFT", 0, 0)
            row.label:SetWidth(330)
            row.label:SetJustifyH("LEFT")
            row.open = NewButton(row, "Info", 50, 340, 0, nil)
            row.visit = NewButton(row, "Visit", 60, 396, 0, nil)
            Menu.plotRows[index] = row
        end

        row:SetPoint("TOPLEFT", neighboursContent, "TOPLEFT", 0, -((index - 1) * 24))
        row.label:SetText(string.format("plot %d - %s (tier %d)",
            plot.index, plot.owner ~= "" and plot.owner or DIM .. "empty" .. RESET, plot.tier or 0))

        row.open:SetScript("OnClick", function()
            H.plotIndex = plot.index
            H.Send("C:PLOT|" .. tostring(plot.index))
            ShowTab("house")
        end)

        if plot.state == 2 or (plot.owner and plot.owner ~= "") then
            row.visit:Show()
            row.visit:SetScript("OnClick", function()
                H.Send("C:ENTER|" .. tostring(plot.index))
            end)
        else
            row.visit:Hide()
        end

        row:Show()
    end

    neighboursContent:SetHeight(math.max(1, math.min(#sorted, 40) * 24))
end

Menu.onShow.neighbours = function()
    H.plots = {}
    H.Send("C:PLOTS")
end

H.OnPlotList = function()
    BuildPlots()
end

-- ---------------------------------------------------------------------------
-- Endeavors
-- ---------------------------------------------------------------------------

local endeavorPane = NewPane("endeavors")

local endeavorHeader = NewLabel(endeavorPane, "GameFontNormalLarge", "TOPLEFT", 0, 0)
endeavorHeader:SetText("Endeavor milestones")

local endeavorIntro = NewLabel(endeavorPane, nil, "TOPLEFT", 0, -26)
endeavorIntro:SetWidth(540)
endeavorIntro:SetText("Thirty weeks of neighborhood Endeavors unlock the four milestones below. " ..
    "Vendor stock that is gated on a milestone stays hidden until the neighborhood passes it. " ..
    "The tracking table is phase 3, so for now this is the ladder and what it unlocks.")

for index, milestone in ipairs(MILESTONES) do
    local label = NewLabel(endeavorPane, nil, "TOPLEFT", 0, -70 - ((index - 1) * 44))
    label:SetWidth(540)
    label:SetText(string.format("%sMilestone %d|r - %s of the tasks: %s.",
        GOLD, milestone[1], milestone[2], milestone[3]))
end

local endeavorNote = NewLabel(endeavorPane, "GameFontDisableSmall", "TOPLEFT", 0, -262)
endeavorNote:SetWidth(540)
endeavorNote:SetText("Progress shown here is the server's number when the Endeavor opcode lands; " ..
    "until then the four rows above are the committed design in doc 04.")

-- ---------------------------------------------------------------------------
-- Blueprints
-- ---------------------------------------------------------------------------

local blueprintPane = NewPane("blueprints")

local blueprintHeader = NewLabel(blueprintPane, "GameFontNormalLarge", "TOPLEFT", 0, 0)
blueprintHeader:SetText("Blueprint codes")

local blueprintIntro = NewLabel(blueprintPane, nil, "TOPLEFT", 0, -26)
blueprintIntro:SetWidth(540)
blueprintIntro:SetText("A blueprint is a single line: a header, a kind, a faction, the payload " ..
    "and a checksum. Paste one below and press Copy to keep it in SavedVariables; the " ..
    "server side of import and export is phase 4, so the code below is the format, not a " ..
    "working import yet.")

local blueprintSample = NewLabel(blueprintPane, "GameFontHighlightSmall", "TOPLEFT", 0, -92)
blueprintSample:SetWidth(540)
blueprintSample:SetText(GOLD .. "PCH1-ROOM-A-[room ids and offsets]-CSUM" .. RESET)

local blueprintBox = CreateFrame("EditBox", nil, blueprintPane, "InputBoxTemplate")
blueprintBox:SetSize(420, 20)
blueprintBox:SetPoint("TOPLEFT", blueprintPane, "TOPLEFT", 8, -120)
blueprintBox:SetAutoFocus(false)
blueprintBox:SetText("")

NewButton(blueprintPane, "Copy", 80, 0, -150, function()
    HousingUIDB = HousingUIDB or {}
    HousingUIDB.blueprint = blueprintBox:GetText()
    H.Print("blueprint saved to SavedVariables")
end)

NewButton(blueprintPane, "Paste", 80, 90, -150, function()
    HousingUIDB = HousingUIDB or {}
    blueprintBox:SetText(HousingUIDB.blueprint or "")
end)

-- ---------------------------------------------------------------------------
-- Guide
-- ---------------------------------------------------------------------------

local guidePane = NewPane("guide")

local guideHeader = NewLabel(guidePane, "GameFontNormalLarge", "TOPLEFT", 0, 0)
guideHeader:SetText("What this menu sends")

local GUIDE_LINES = {
    "|cffffd100House|r      C:NEIGHBORHOOD, C:PLOT <n>, C:HOME, C:LEAVE, C:NAME <text>,",
    "            C:LOCK <0|1>, C:TIER <2-5>",
    "|cffffd100Catalogue|r  C:DECOR [category] -> S:DECORENTRY per decor, S:DECOREND at the end",
    "|cffffd100Buy|r        C:BUYDECOR <id> -> S:BUYRESULT",
    "|cffffd100Placed|r     C:PLACED -> S:PLACEDENTRY per placement, S:PLACEDEND",
    "|cffffd100Neighbours|r C:PLOTS -> S:PLOT per plot; C:DIR <page> -> S:DIRENTRY",
    "|cffffd100Place|r      C:PLACE <decorId> <x> <y> <z> <o> <scale> <indoor>",
    "|cffffd100Move|r       C:MOVE <placementId> <x> <y> <z> <o> <scale>",
    "|cffffd100Remove|r     C:REMOVE <placementId>",
    "|cffffd100Guests|r     C:GUESTLIST, C:GUESTADD <name> <flags>, C:GUESTDEL <name>",
    "|cffffd100Amenities|r  C:VAREA -> S:VAREA per amenity, S:VAREAEND at the end",
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
    }

    H.plotIndex = H.plotInfo.index

    if frame:IsShown() then
        if H.plotInfo.name and H.plotInfo.name ~= "" then
            houseName:SetText(H.plotInfo.name)
        end
        RefreshHouse()
    end
end

local previousPlotInfo = H.handlers["PLOTINFO"]
H.handlers["PLOTINFO"] = function(fields)
    if previousPlotInfo then
        previousPlotInfo(fields)
    end
    StorePlotInfo(fields)
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
