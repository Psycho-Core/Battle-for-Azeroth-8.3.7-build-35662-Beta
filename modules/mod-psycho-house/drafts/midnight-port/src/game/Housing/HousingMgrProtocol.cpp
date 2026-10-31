/*
 * Copyright (C) 2022 BfaCore Reforged
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * BFAH addon protocol handling. Split out of HousingMgr.cpp so that the state machine
 * and the wire format can be read separately.
 *
 * Framing rules live in docs/player-housing/04-addon-protocol.md. Every message is
 * treated as hostile: fields are parsed, ids are resolved, and nothing is trusted.
 */

#include "HousingMgr.h"

#include "DatabaseEnv.h"
#include "Log.h"
#include "ObjectAccessor.h"
#include "Player.h"
#include "StringFormat.h"
#include "Timer.h"
#include "WorldSession.h"

#include <algorithm>

namespace Housing
{

void HousingMgr::SendChunk(Player* player, std::string const& payload)
{
    PlayerState* state = GetPlayerState(player);
    if (state)
    {
        HousingAddon::Send(player, state->ChunkMessageId, payload);
        return;
    }

    uint8 throwaway = 0;
    HousingAddon::Send(player, throwaway, payload);
}

void HousingMgr::SendLine(Player* player, std::string const& payload)
{
    SendChunk(player, payload);
}

void HousingMgr::SendResult(Player* player, char const* op, HousingResult result, uint64 extra)
{
    std::string payload = Trinity::StringFormat("S:RESULT|{}|{}", op, uint32(result));

    if (extra)
        payload += Trinity::StringFormat("|{}", extra);

    SendChunk(player, payload);
}

void HousingMgr::HandleAddonMessage(Player* player, std::string const& message)
{
    if (!player || !player->GetSession())
        return;

    PlayerState* state = GetPlayerState(player);
    if (!state)
        return;

    if (!_enabled)
    {
        SendResult(player, "MODULE", Disabled);
        return;
    }

    // Rate limiting first: everything below this point costs at least a lookup.
    uint32 const now = getMSTime();
    if (!state->RateWindowStart || (now - state->RateWindowStart) > kRateLimitWindowMs)
    {
        state->RateWindowStart = now;
        state->RateCount = 0;
    }

    if (++state->RateCount > _rateLimit)
    {
        SendResult(player, "RATE", RateLimited);
        return;
    }

    std::vector<std::string> const fields = HousingAddon::Split(message);
    if (fields.empty() || fields[0].size() < 3)
        return;

    std::string const& head = fields[0];
    if (head[0] != 'C' || head[1] != ':')
        return;

    std::string const op = head.substr(2);
    uint32 const accountId = player->GetSession()->GetAccountId();

    // ---- handshake -------------------------------------------------------

    if (op == "HELLO")
    {
        HandleHello(player, *state);
        return;
    }

    if (op == "PING")
    {
        SendChunk(player, Trinity::StringFormat("S:PONG|{}", getMSTime()));
        return;
    }

    if (op == "HOME")
    {
        HousingResult const result = TeleportHome(player);

        uint32 remaining = 0;
        if (result == Cooldown && state->LastHomeUse)
        {
            uint32 const elapsed = getMSTime() - state->LastHomeUse;
            uint32 const total = _homeCooldown * 1000;
            remaining = elapsed < total ? (total - elapsed) / 1000 : 0;
        }

        SendChunk(player, Trinity::StringFormat("S:HOME|{}|{}", uint32(result), remaining));
        return;
    }

    if (op == "LEAVE")
    {
        SendResult(player, "LEAVE", TeleportToTownCenter(player));
        return;
    }

    if (op == "NEIGHBORHOOD")
    {
        HandleNeighborhoodInfo(player, *state);
        return;
    }

    if (op == "PLOTS")
    {
        HandlePlotList(player, *state);
        return;
    }

    if (op == "PLOT")
    {
        uint32 index = 0;
        if (fields.size() < 2 || !HousingAddon::ParseUInt(fields[1], index) || index >= kPlotCount)
        {
            SendResult(player, "PLOT", InvalidPlot);
            return;
        }

        HandlePlotInfo(player, *state, uint8(index));
        return;
    }

    if (op == "BUY")
    {
        uint32 index = 0;
        if (fields.size() < 2 || !HousingAddon::ParseUInt(fields[1], index) || index >= kPlotCount)
        {
            SendResult(player, "BUY", InvalidPlot);
            return;
        }

        SendResult(player, "BUY", BuyPlot(player, FindNeighborhoodFor(player), uint8(index)));
        return;
    }

    if (op == "SELL")
    {
        SendResult(player, "SELL", SellPlot(player));
        return;
    }

    if (op == "TIER")
    {
        uint32 tier = 0;
        if (fields.size() < 2 || !HousingAddon::ParseUInt(fields[1], tier) || tier > kMaxTier)
        {
            SendResult(player, "TIER", TierMax);
            return;
        }

        SendResult(player, "TIER", UpgradeTier(player, uint8(tier)));
        return;
    }

    if (op == "NAME")
    {
        if (fields.size() < 2)
        {
            SendResult(player, "NAME", InvalidPlot);
            return;
        }

        SendResult(player, "NAME", RenameHouse(player, fields[1]));
        return;
    }

    if (op == "LOCK")
    {
        uint32 value = 0;
        if (fields.size() < 2 || !HousingAddon::ParseUInt(fields[1], value))
        {
            SendResult(player, "LOCK", InvalidPlot);
            return;
        }

        SendResult(player, "LOCK", SetLocked(player, value != 0));
        return;
    }

    if (op == "ENTER")
    {
        uint32 index = 0;
        if (fields.size() < 2 || !HousingAddon::ParseUInt(fields[1], index) || index >= kPlotCount)
        {
            SendResult(player, "ENTER", InvalidPlot);
            return;
        }

        SendResult(player, "ENTER", TeleportToPlot(player, uint8(index)));
        return;
    }

    if (op == "INTERIOR")
    {
        uint32 value = 0;
        if (fields.size() < 2 || !HousingAddon::ParseUInt(fields[1], value))
        {
            SendResult(player, "INTERIOR", InvalidPlot);
            return;
        }

        SendResult(player, "INTERIOR", EnterHouse(player, value != 0));
        return;
    }

    if (op == "DIR")
    {
        uint32 page = 0;
        HousingAddon::ParseUInt(fields.size() > 1 ? fields[1] : std::string("0"), page);
        HandleDirectory(player, *state, page);
        return;
    }

    if (op == "VAREA")
    {
        HandleAmenityList(player, *state);
        return;
    }

    if (op == "GUESTLIST")
    {
        HandleGuestList(player, *state);
        return;
    }

    if (op == "GUESTADD")
    {
        if (fields.size() < 3)
        {
            SendResult(player, "GUESTADD", InvalidPlot);
            return;
        }

        HousingPlot* plot = GetPlotForEdit(player);
        if (!plot || !plot->IsOwned())
        {
            SendResult(player, "GUESTADD", NoHouse);
            return;
        }

        if (plot->GetOwnerAccount() != accountId)
        {
            SendResult(player, "GUESTADD", NotOwner);
            return;
        }

        uint32 flags = 0;
        if (!HousingAddon::ParseUInt(fields[2], flags))
        {
            SendResult(player, "GUESTADD", InvalidPlot);
            return;
        }

        Player* guest = ObjectAccessor::FindPlayerByName(fields[1]);
        if (!guest)
        {
            SendResult(player, "GUESTADD", NotFound);
            return;
        }

        AddGuest(plot->GetHousingId(), guest->GetSession()->GetAccountId(), uint8(flags & 0x07));
        SendResult(player, "GUESTADD", Ok);
        return;
    }

    if (op == "GUESTDEL")
    {
        if (fields.size() < 2)
        {
            SendResult(player, "GUESTDEL", InvalidPlot);
            return;
        }

        HousingPlot* plot = GetPlotForEdit(player);
        if (!plot || !plot->IsOwned())
        {
            SendResult(player, "GUESTDEL", NoHouse);
            return;
        }

        if (plot->GetOwnerAccount() != accountId)
        {
            SendResult(player, "GUESTDEL", NotOwner);
            return;
        }

        Player* guest = ObjectAccessor::FindPlayerByName(fields[1]);
        if (!guest)
        {
            SendResult(player, "GUESTDEL", NotFound);
            return;
        }

        RemoveGuest(plot->GetHousingId(), guest->GetSession()->GetAccountId());
        SendResult(player, "GUESTDEL", Ok);
        return;
    }

    // ---- decor -----------------------------------------------------------

    if (op == "DECOR")
    {
        uint32 page = 0;
        std::string category = fields.size() > 1 ? fields[1] : "";
        HousingAddon::ParseUInt(fields.size() > 2 ? fields[2] : std::string("0"), page);
        HandleDecorList(player, *state, category, page);
        return;
    }

    if (op == "PLACED")
    {
        HandlePlacedList(player, *state);
        return;
    }

    if (op == "BUYDECOR")
    {
        uint32 decorId = 0;
        if (fields.size() < 2 || !HousingAddon::ParseUInt(fields[1], decorId))
        {
            SendChunk(player, "S:BUYRESULT|" + std::to_string(uint32(InvalidDecor)) + "|0");
            return;
        }

        HousingResult const result = BuyDecor(player, decorId);
        SendChunk(player, Trinity::StringFormat("S:BUYRESULT|{}|{}", uint32(result), decorId));
        return;
    }

    if (op == "PLACE")
    {
        uint32 decorId = 0;
        float x = 0.0f, y = 0.0f, z = 0.0f, o = 0.0f, scale = 1.0f;
        uint32 indoor = 0;

        if (fields.size() < 8
            || !HousingAddon::ParseUInt(fields[1], decorId)
            || !HousingAddon::ParseFloat(fields[2], x) || !HousingAddon::ParseFloat(fields[3], y)
            || !HousingAddon::ParseFloat(fields[4], z) || !HousingAddon::ParseFloat(fields[5], o)
            || !HousingAddon::ParseFloat(fields[6], scale) || !HousingAddon::ParseUInt(fields[7], indoor))
        {
            SendResult(player, "PLACE", InvalidDecor);
            return;
        }

        uint64 placementId = 0;
        HousingResult const result = PlaceDecor(player, decorId, x, y, z, o, scale, indoor != 0, placementId);
        SendResult(player, "PLACE", result, result == Ok ? placementId : 0);
        return;
    }

    if (op == "MOVE")
    {
        uint32 placementId = 0;
        float x = 0.0f, y = 0.0f, z = 0.0f, o = 0.0f, scale = 1.0f;

        if (fields.size() < 7
            || !HousingAddon::ParseUInt(fields[1], placementId)
            || !HousingAddon::ParseFloat(fields[2], x) || !HousingAddon::ParseFloat(fields[3], y)
            || !HousingAddon::ParseFloat(fields[4], z) || !HousingAddon::ParseFloat(fields[5], o)
            || !HousingAddon::ParseFloat(fields[6], scale))
        {
            SendResult(player, "MOVE", NotFound);
            return;
        }

        SendResult(player, "MOVE", MoveDecor(player, placementId, x, y, z, o, scale));
        return;
    }

    if (op == "REMOVE")
    {
        uint32 placementId = 0;
        if (fields.size() < 2 || !HousingAddon::ParseUInt(fields[1], placementId))
        {
            SendResult(player, "REMOVE", NotFound);
            return;
        }

        SendResult(player, "REMOVE", RemoveDecor(player, placementId));
        return;
    }

    // Unknown opcode: drop silently. Echoing an error would turn the channel into an
    // amplifier for anything that can send addon messages.
}

// -----------------------------------------------------------------------------
// Read handlers
// -----------------------------------------------------------------------------

void HousingMgr::HandleHello(Player* player, PlayerState& state)
{
    SendChunk(player, Trinity::StringFormat("S:HELLO|{}|{}|1|{}",
        kAddonProtocolVersion, GetModuleVersion(), state.HasOwnedPlot ? 1 : 0));
}

void HousingMgr::HandleNeighborhoodInfo(Player* player, PlayerState& state)
{
    HousingNeighborhood* neighborhood = FindNeighborhoodFor(player);
    if (!neighborhood)
    {
        SendResult(player, "NEIGHBORHOOD", NotInNeighborhood);
        return;
    }

    state.NeighborhoodId = neighborhood->GetId();

    SendChunk(player, Trinity::StringFormat("S:NBR|{}|{}|{}|{}|{}|{}|{}",
        neighborhood->GetId(), uint32(neighborhood->GetType()), neighborhood->GetName(),
        uint32(neighborhood->GetSlot()), neighborhood->GetOwnedCount(), uint32(kPlotCount),
        uint32(neighborhood->GetFaction())));
}

void HousingMgr::HandlePlotList(Player* player, PlayerState& state)
{
    HousingNeighborhood* neighborhood = GetNeighborhood(state.NeighborhoodId);
    if (!neighborhood)
        neighborhood = FindNeighborhoodFor(player);

    if (!neighborhood)
    {
        SendResult(player, "PLOTS", NotInNeighborhood);
        return;
    }

    uint32 const accountId = player->GetSession()->GetAccountId();
    std::string payload;

    for (auto const& plot : neighborhood->GetPlots())
    {
        uint32 const tier = plot->GetHouse() ? plot->GetHouse()->GetTier() : 0;

        payload += Trinity::StringFormat("S:PLOT|{}|{}|{}|{}|{};",
            uint32(plot->GetIndex()), plot->GetBiomeId(), uint32(plot->GetStateFor(accountId)),
            tier, plot->GetOwnerName());
    }

    SendChunk(player, payload);
}

void HousingMgr::HandlePlotInfo(Player* player, PlayerState& state, uint8 plotIndex)
{
    HousingNeighborhood* neighborhood = GetNeighborhood(state.NeighborhoodId);
    if (!neighborhood)
        neighborhood = FindNeighborhoodFor(player);

    if (!neighborhood)
    {
        SendResult(player, "PLOT", NotInNeighborhood);
        return;
    }

    HousingPlot* plot = neighborhood->GetPlot(plotIndex);
    if (!plot)
    {
        SendResult(player, "PLOT", InvalidPlot);
        return;
    }

    uint32 const accountId = player->GetSession()->GetAccountId();
    HousingHouse* house = plot->GetHouse();

    SendChunk(player, Trinity::StringFormat("S:PLOTINFO|{}|{}|{}|{}|{}|{}|{}|{}|{}|{}|{}|{}|{}",
        uint32(plotIndex), plot->GetBiomeId(), plot->GetPrice(), uint32(plot->GetSizeClass()),
        uint32(plot->GetStateFor(accountId)), uint32(house ? house->GetTier() : 0),
        plot->GetOwnerName(), house ? house->GetName() : std::string(),
        uint32(house && house->IsLocked() ? 1 : 0),
        house ? house->GetBudget(true) : 0, house ? house->GetBudget(false) : 0,
        house ? house->GetUsedBudget(true) : 0, house ? house->GetUsedBudget(false) : 0));
}

void HousingMgr::HandleDirectory(Player* player, PlayerState& state, uint32 page)
{
    HousingNeighborhood* neighborhood = GetNeighborhood(state.NeighborhoodId);
    if (!neighborhood)
    {
        SendResult(player, "DIR", NotInNeighborhood);
        return;
    }

    std::vector<HousingPlot*> owned;
    for (auto const& plot : neighborhood->GetPlots())
        if (plot->IsOwned())
            owned.push_back(plot.get());

    uint32 const start = page * kDirectoryPageSize;
    uint32 const end = std::min<uint32>(start + kDirectoryPageSize, uint32(owned.size()));

    std::string payload;
    for (uint32 i = start; i < end; ++i)
    {
        HousingPlot* plot = owned[i];
        uint32 const tier = plot->GetHouse() ? plot->GetHouse()->GetTier() : 0;
        uint32 const locked = plot->GetHouse() && plot->GetHouse()->IsLocked() ? 1 : 0;

        payload += Trinity::StringFormat("S:DIRENTRY|{}|{}|{}|{};",
            uint32(plot->GetIndex()), plot->GetOwnerName(), tier, locked);
    }

    payload += Trinity::StringFormat("S:DIREND|{}|{};", page, end < owned.size() ? 1 : 0);
    SendChunk(player, payload);
}

void HousingMgr::HandleDecorList(Player* player, PlayerState& state, std::string const& category, uint32 page)
{
    (void)state;

    uint32 const accountId = player->GetSession()->GetAccountId();
    LoadCollection(accountId);

    uint8 const factionBit = player->GetTeamId() == TEAM_ALLIANCE ? 0x01 : 0x02;

    std::vector<DecorInfo const*> visible;
    for (auto const& entry : _decor)
    {
        DecorInfo const& info = entry.second;

        if (info.Flags & DecorFlagHidden)
            continue;
        if (!(info.FactionMask & factionBit))
            continue;
        if (info.SourceType == SourceGm)
            continue;
        if (!category.empty() && info.Category != category)
            continue;
        if (info.SourceType == SourceVendor && info.Price == 0 && info.Currency == 0)
            continue;

        visible.push_back(&info);
    }

    std::sort(visible.begin(), visible.end(),
        [](DecorInfo const* left, DecorInfo const* right)
        {
            if (left->Category != right->Category)
                return left->Category < right->Category;
            return left->Id < right->Id;
        });

    uint32 const start = page * kCataloguePageSize;
    uint32 const end = std::min<uint32>(start + kCataloguePageSize, uint32(visible.size()));

    std::string payload;
    for (uint32 i = start; i < end; ++i)
    {
        DecorInfo const* info = visible[i];

        payload += Trinity::StringFormat("S:DECORENTRY|{}|{}|{}|{}|{}|{}|{}|{};",
            info->Id, info->Name, uint32(info->BudgetCost), uint32(info->Placement),
            uint32(info->Surface), HasUnlocked(accountId, info->Id) ? 1u : 0u,
            info->Price, info->Currency);
    }

    payload += Trinity::StringFormat("S:DECOREND|{}|{};", page, end < visible.size() ? 1 : 0);
    SendChunk(player, payload);
}

void HousingMgr::HandlePlacedList(Player* player, PlayerState& state)
{
    (void)state;

    HousingPlot* plot = GetPlotForEdit(player);
    if (!plot || !plot->GetHouse())
    {
        SendResult(player, "PLACED", NoHouse);
        return;
    }

    HousingHouse* house = plot->GetHouse();
    std::string payload;

    for (DecorPlacement const& placement : house->GetPlacements())
    {
        payload += Trinity::StringFormat("S:PLACEDENTRY|{}|{}|{}|{}|{}|{}|{}|{};",
            placement.Id, placement.DecorId, placement.X, placement.Y, placement.Z,
            placement.O, placement.Scale, uint32(placement.Indoor ? 1 : 0));
    }

    payload += "S:PLACEDEND;";
    SendChunk(player, payload);
}

void HousingMgr::HandleGuestList(Player* player, PlayerState& state)
{
    (void)state;

    HousingPlot* plot = GetPlotForEdit(player);
    if (!plot || !plot->IsOwned())
    {
        SendResult(player, "GUESTLIST", NoHouse);
        return;
    }

    std::map<uint32, uint8> const& guests = LoadGuests(plot->GetHousingId());
    std::string payload;

    for (auto const& entry : guests)
    {
        // Guests are stored by account; the addon shows a character name, so resolve one.
        std::string name = Trinity::StringFormat("<account {}>", entry.first);

        if (QueryResult result = CharacterDatabase.PQuery(
            "SELECT `name` FROM `characters` WHERE `account` = {} ORDER BY `guid` LIMIT 1", entry.first))
            name = result->Fetch()[0].GetString();

        payload += Trinity::StringFormat("S:GUEST|{}|{};", name, uint32(entry.second));
    }

    payload += "S:GUESTEND;";
    SendChunk(player, payload);
}

void HousingMgr::HandleAmenityList(Player* player, PlayerState& state)
{
    (void)player;
    (void)state;

    std::string payload;

    for (AmenityInfo const& amenity : _amenityList)
    {
        payload += Trinity::StringFormat("S:VAREA|{}|{}|{}|{}|{};",
            uint32(amenity.Type), amenity.Name, amenity.Pos.m_positionX, amenity.Pos.m_positionY,
            amenity.BiomeId);
    }

    payload += "S:VAREAEND;";
    SendChunk(player, payload);
}

} // namespace Housing
