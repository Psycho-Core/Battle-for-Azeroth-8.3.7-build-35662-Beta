-- PsychoHouse - the window.
--
-- Three parts, top to bottom: the house itself (buy, sell, travel), the objects standing in it
-- (a scroll list, one button per row to remove it, an entry box to add one where the character
-- is standing), and the facilities the Steward sells. The window never decides anything: every
-- button sends a record and waits for the module's answer, so a realm with the feature switched
-- off behaves exactly as its configuration says.

local PH = PsychoHouse
local ROW_HEIGHT = 18
local VISIBLE_ROWS = 8

local panel = CreateFrame("Frame", "PsychoHousePanel", UIParent, "BasicFrameTemplateWithInset")
panel:SetSize(420, 560)
panel:SetPoint("CENTER")
panel:SetMovable(true)
panel:EnableMouse(true)
panel:RegisterForDrag("LeftButton")
panel:SetScript("OnDragStart", panel.StartMoving)
panel:SetScript("OnDragStop", function(self)
	self:StopMovingOrSizing()
	PsychoHouseDB.point = { self:GetPoint() }
end)
panel.title = panel:CreateFontString(nil, "OVERLAY", "GameFontHighlight")
panel.title:SetPoint("TOP", panel, "TOP", 0, -6)
panel.title:SetText("Island House")
panel:Hide()

if PsychoHouseDB and PsychoHouseDB.point then
	panel:ClearAllPoints()
	panel:SetPoint(unpack(PsychoHouseDB.point))
end

local function line(parent, size)
	local text = parent:CreateFontString(nil, "OVERLAY", "GameFontHighlightSmall")
	text:SetJustifyH("LEFT")
	text:SetWidth(380)
	if size then
		text:SetFontObject(size)
	end
	return text
end

--------------------------------------------------------------------------------------------------
-- Header: phase, counts, price
--------------------------------------------------------------------------------------------------

local header = line(panel)
header:SetPoint("TOPLEFT", 14, -28)
header:SetWidth(390)

local capacity = line(panel)
capacity:SetPoint("TOPLEFT", header, "BOTTOMLEFT", 0, -2)

--------------------------------------------------------------------------------------------------
-- Buttons
--------------------------------------------------------------------------------------------------

local function button(text, width, anchor, offsetX, onClick)
	local button = CreateFrame("Button", nil, panel, "UIPanelButtonTemplate")
	button:SetSize(width or 90, 22)
	button:SetPoint("TOPLEFT", anchor, "TOPLEFT", offsetX, 0)
	button:SetText(text)
	button:SetScript("OnClick", onClick)
	return button
end

local buyButton = button("Buy", 70, capacity, 0, nil)
buyButton:SetPoint("TOPLEFT", capacity, "BOTTOMLEFT", 0, -6)
buyButton:SetScript("OnClick", function() PH.Buy() end)

local sellButton = button("Sell", 70, buyButton, 76, nil)
sellButton:SetScript("OnClick", function() PH.Sell() end)

local homeButton = button("Travel", 70, sellButton, 76, nil)
homeButton:SetScript("OnClick", function() PH.GoHome() end)

local refreshButton = button("Refresh", 70, homeButton, 76, nil)
refreshButton:SetScript("OnClick", function() PH.Refresh() end)

--------------------------------------------------------------------------------------------------
-- Objects
--------------------------------------------------------------------------------------------------

local objectsTitle = line(panel)
objectsTitle:SetPoint("TOPLEFT", buyButton, "BOTTOMLEFT", 0, -10)
objectsTitle:SetText("Objects in your house")

local scroll = CreateFrame("ScrollFrame", "PsychoHouseObjectScroll", panel, "UIPanelScrollFrameTemplate")
scroll:SetSize(360, ROW_HEIGHT * VISIBLE_ROWS)
scroll:SetPoint("TOPLEFT", objectsTitle, "BOTTOMLEFT", 0, -4)

local content = CreateFrame("Frame", nil, scroll)
content:SetSize(340, ROW_HEIGHT * VISIBLE_ROWS)
scroll:SetScrollChild(content)

local rows = {}

local function acquireRow(index)
	if rows[index] then
		return rows[index]
	end

	local row = CreateFrame("Frame", nil, content)
	row:SetSize(340, ROW_HEIGHT)
	row:SetPoint("TOPLEFT", 0, -(index - 1) * ROW_HEIGHT)

	row.label = row:CreateFontString(nil, "OVERLAY", "GameFontHighlightSmall")
	row.label:SetPoint("LEFT", 0, 0)
	row.label:SetWidth(250)
	row.label:SetJustifyH("LEFT")

	row.remove = CreateFrame("Button", nil, row, "UIPanelButtonTemplate")
	row.remove:SetSize(60, 16)
	row.remove:SetPoint("RIGHT", 0, 0)
	row.remove:SetText("Remove")

	rows[index] = row
	return row
end

local function drawObjects()
	for index, row in ipairs(rows) do
		if index > #PH.objects then
			row:Hide()
		end
	end

	for index, object in ipairs(PH.objects) do
		local row = acquireRow(index)
		row.label:SetFormattedText("slot %d  entry %d  scale %.2f", object.slot, object.entry, object.scale or 1)
		row.remove:SetScript("OnClick", function() PH.Remove(object.slot) end)
		row:Show()
	end

	content:SetHeight(math.max(ROW_HEIGHT * VISIBLE_ROWS, ROW_HEIGHT * #PH.objects))
end

local entryBox = CreateFrame("EditBox", "PsychoHouseEntryBox", panel, "InputBoxTemplate")
entryBox:SetSize(110, 20)
entryBox:SetPoint("TOPLEFT", scroll, "BOTTOMLEFT", 8, -8)
entryBox:SetAutoFocus(false)
entryBox:SetNumeric(true)
entryBox:SetScript("OnEnterPressed", function(self)
	PH.Place(self:GetText())
	self:SetText("")
	self:ClearFocus()
end)

local entryHint = line(panel)
entryHint:SetPoint("LEFT", entryBox, "RIGHT", 6, 0)
entryHint:SetText("game object entry")

local placeButton = button("Place here", 90, entryBox, 250, function()
	PH.Place(entryBox:GetText())
	entryBox:SetText("")
end)
placeButton:SetPoint("LEFT", entryBox, "RIGHT", 150, 0)

--------------------------------------------------------------------------------------------------
-- Facilities
--------------------------------------------------------------------------------------------------

local facilitiesTitle = line(panel)
facilitiesTitle:SetPoint("TOPLEFT", entryBox, "BOTTOMLEFT", -8, -12)
facilitiesTitle:SetText("Facilities")

local facilityRows = {}

-- The facility list is short (twelve rows in the shipped catalogue) so it is drawn in place
-- rather than in a scroll frame: the window is tall enough for all of them at once.
local function acquireFacilityRow(index)
	if facilityRows[index] then
		return facilityRows[index]
	end

	local row = CreateFrame("Frame", nil, panel)
	row:SetSize(390, ROW_HEIGHT)
	row:SetPoint("TOPLEFT", facilitiesTitle, "BOTTOMLEFT", 0, -2 - (index - 1) * ROW_HEIGHT)

	row.label = row:CreateFontString(nil, "OVERLAY", "GameFontHighlightSmall")
	row.label:SetPoint("LEFT", 0, 0)
	row.label:SetWidth(290)
	row.label:SetJustifyH("LEFT")

	row.buy = CreateFrame("Button", nil, row, "UIPanelButtonTemplate")
	row.buy:SetSize(70, 16)
	row.buy:SetPoint("RIGHT", 0, 0)
	row.buy:SetText("Buy")

	facilityRows[index] = row
	return row
end

local function drawFacilities()
	for index, row in ipairs(facilityRows) do
		if index > #PH.facilities then
			row:Hide()
		end
	end

	for index, facility in ipairs(PH.facilities) do
		local row = acquireFacilityRow(index)

		row.label:SetFormattedText("%s - %d c", facility.name, facility.price)

		if facility.owned then
			row.buy:Disable()
			row.buy:SetText("Bought")
		else
			row.buy:Enable()
			row.buy:SetText("Buy")
			row.buy:SetScript("OnClick", function() PH.BuyFacility(facility.id) end)
		end

		row:Show()
	end
end

--------------------------------------------------------------------------------------------------
-- State
--------------------------------------------------------------------------------------------------

local function drawHeader()
	if PH.hasHouse then
		header:SetFormattedText("House in phase %d - %d facility purchase(s)", PH.phase or 0, PH.bought or 0)
	else
		header:SetText("No house on this character yet.")
	end

	capacity:SetFormattedText("Objects: %d of %d  (limit set by the realm)",
		PH.objectCount or #PH.objects, PH.limit or 0)
end

PH.On("status", function()
	drawHeader()
end)

PH.On("hello", function()
	drawHeader()

	if PH.protocol ~= PsychoHouse.protocol then
		header:SetText("This window speaks a different protocol than the realm. Update the addon.")
	end
end)

PH.On("objects", function()
	drawObjects()
	drawHeader()
end)

PH.On("facilities", function()
	drawFacilities()
end)

PH.On("result", function(op, code)
	if code ~= 0 then
		DEFAULT_CHAT_FRAME:AddMessage("|cffffd200Island house:|r " .. PH.ResultText(code))
	end
end)

panel:SetScript("OnShow", function()
	PH.Refresh()
end)

--------------------------------------------------------------------------------------------------
-- Slash command and the first draw
--------------------------------------------------------------------------------------------------

SLASH_PSYCHOHOUSE1 = "/psychohouse"
SLASH_PSYCHOHOUSE2 = "/phouse"
SlashCmdList["PSYCHOHOUSE"] = function()
	if panel:IsShown() then
		panel:Hide()
	else
		panel:Show()
	end
end

local boot = CreateFrame("Frame")
boot:RegisterEvent("PLAYER_LOGIN")
boot:SetScript("OnEvent", function()
	drawHeader()
	drawObjects()
	drawFacilities()
end)
