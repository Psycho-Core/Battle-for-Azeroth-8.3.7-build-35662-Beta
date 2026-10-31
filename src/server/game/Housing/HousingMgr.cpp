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

#include "HousingMgr.h"

#include "Chat.h"
#include "Config.h"
#include "DB2Stores.h"
#include "DatabaseEnv.h"
#include "GameObject.h"
#include "Log.h"
#include "Map.h"
#include "ObjectAccessor.h"
#include "ObjectMgr.h"
#include "PhasingHandler.h"
#include "Player.h"
#include "SharedDefines.h"
#include "StringFormat.h"
#include "Timer.h"
#include "World.h"
#include "WorldSession.h"

#include <algorithm>
#include <cmath>
#include <ctime>

namespace
{
    uint32 Now()
    {
        return uint32(std::time(nullptr));
    }

    uint64 GoldToCopper(uint32 gold)
    {
        return uint64(gold) * GOLD;
    }
}

namespace Housing
{

HousingMgr* HousingMgr::instance()
{
    static HousingMgr instance;
    return &instance;
}

HousingMgr::HousingMgr()
    : _enabled(false), _housingMapId(0), _interiorMapId(0), _saveInterval(kSaveIntervalSeconds),
    _rateLimit(kRateLimitMessages), _homeCooldown(900), _refundPercent(50), _plotScopedPhasing(true),
    _nativeMap(true), _nextPlacementId(1), _nextHousingId(1), _nextNeighborhoodId(1),
    _saveTimer(0), _tick(0)
{
    _usedSlots.assign(kNeighborhoodSlotMax + 1, false);
}

// -----------------------------------------------------------------------------
// Config and loading
// -----------------------------------------------------------------------------

void HousingMgr::LoadConfig()
{
    _enabled = sConfigMgr->GetBoolDefault("Housing.Enable", false);
    _housingMapId = uint32(sConfigMgr->GetIntDefault("Housing.Map.Id", 0));
    _interiorMapId = uint32(sConfigMgr->GetIntDefault("Housing.Interior.Map", 0));
    _saveInterval = uint32(std::max(5, sConfigMgr->GetIntDefault("Housing.SaveInterval", kSaveIntervalSeconds)));
    _rateLimit = uint32(std::max(1, sConfigMgr->GetIntDefault("Housing.RateLimit.Messages", int32(kRateLimitMessages))));
    _homeCooldown = uint32(std::max(0, sConfigMgr->GetIntDefault("Housing.Homestone.Cooldown", 900)));
    _refundPercent = uint32(std::min(100, std::max(0, sConfigMgr->GetIntDefault("Housing.Sell.RefundPercent", 50))));
    _plotScopedPhasing = sConfigMgr->GetBoolDefault("Housing.Phasing.PlotScoped", true);

    std::string const mode = sConfigMgr->GetStringDefault("Housing.Map.Mode", "native");
    _nativeMap = mode != "anchor";

    _interiorAnchor = Position(
        sConfigMgr->GetFloatDefault("Housing.Interior.X", 0.0f),
        sConfigMgr->GetFloatDefault("Housing.Interior.Y", 0.0f),
        sConfigMgr->GetFloatDefault("Housing.Interior.Z", 0.0f),
        sConfigMgr->GetFloatDefault("Housing.Interior.O", 0.0f));

    _townCenter = Position(
        sConfigMgr->GetFloatDefault("Housing.TownCenter.X", 0.0f),
        sConfigMgr->GetFloatDefault("Housing.TownCenter.Y", 0.0f),
        sConfigMgr->GetFloatDefault("Housing.TownCenter.Z", 0.0f),
        sConfigMgr->GetFloatDefault("Housing.TownCenter.O", 0.0f));

    if (!_enabled)
        return;

    if (!_housingMapId)
    {
        TC_LOG_ERROR("housing", "Housing.Enable is set but Housing.Map.Id is 0 - housing disabled");
        _enabled = false;
        return;
    }

    if (!sMapStore.LookupEntry(_housingMapId))
    {
        TC_LOG_ERROR("housing", "Housing.Map.Id %u does not exist in Map.db2 - housing disabled", _housingMapId);
        _enabled = false;
        return;
    }

    if (!_interiorMapId)
        _interiorMapId = _housingMapId;

    TC_LOG_INFO("housing", "Housing enabled: map %u (interior %u, %s mode)",
        _housingMapId, _interiorMapId, _nativeMap ? "native" : "anchor");
}

void HousingMgr::LoadHousingData()
{
    if (!_enabled)
        return;

    LoadBiomes();
    LoadPlotTemplates();
    LoadDecor();
    LoadVendors();
    LoadAmenities();
    LoadNamePool();

    TC_LOG_INFO("housing", "Loaded %u biomes, %u plots, %u decor items, %u vendors, %u amenities",
        uint32(_biomes.size()), uint32(_plotTemplates.size()), uint32(_decor.size()),
        uint32(_vendorList.size()), uint32(_amenityList.size()));
}

void HousingMgr::LoadBiomes()
{
    _biomes.clear();

    QueryResult result = WorldDatabase.Query(
        "SELECT `id`, `name`, `description`, `faction`, `zone_id`, `ui_map_id`, `map_id`, "
        "`flight_x`, `flight_y`, `flight_z`, `flight_o`, `center_x`, `center_y`, `flags` FROM `housing_biome`");

    if (!result)
    {
        TC_LOG_ERROR("housing", "housing_biome is empty - no plot can be placed");
        return;
    }

    do
    {
        Field* fields = result->Fetch();

        BiomeInfo info;
        info.Id = fields[0].GetUInt32();
        info.Name = fields[1].GetString();
        info.Description = fields[2].GetString();
        info.Faction = fields[3].GetUInt8();
        info.ZoneId = fields[4].GetUInt32();
        info.UiMapId = fields[5].GetUInt32();
        info.MapId = fields[6].GetUInt32();
        info.FlightPoint = Position(fields[7].GetFloat(), fields[8].GetFloat(), fields[9].GetFloat(), fields[10].GetFloat());
        info.CenterX = fields[11].GetFloat();
        info.CenterY = fields[12].GetFloat();
        info.Flags = fields[13].GetUInt32();

        _biomes[info.Id] = info;
    }
    while (result->NextRow());
}

void HousingMgr::LoadPlotTemplates()
{
    _plotTemplates.clear();
    _plotTemplatesByMap.clear();

    QueryResult result = WorldDatabase.Query(
        "SELECT `map_id`, `plot_index`, `biome_id`, `size_class`, `price`, `spawn_x`, `spawn_y`, "
        "`spawn_z`, `spawn_o`, `yard_radius`, `flags`, `comment` FROM `housing_plot`");

    if (!result)
    {
        TC_LOG_ERROR("housing", "housing_plot is empty - nothing can be bought");
        return;
    }

    do
    {
        Field* fields = result->Fetch();

        PlotTemplate tmpl;
        tmpl.MapId = fields[0].GetUInt32();
        tmpl.Index = fields[1].GetUInt8();
        tmpl.BiomeId = fields[2].GetUInt32();
        tmpl.SizeClass = fields[3].GetUInt8();
        tmpl.Price = fields[4].GetUInt32();

        tmpl.HasSpawn = !fields[5].IsNull() && !fields[6].IsNull() && !fields[7].IsNull();
        if (tmpl.HasSpawn)
            tmpl.Spawn = Position(fields[5].GetFloat(), fields[6].GetFloat(), fields[7].GetFloat(),
                fields[8].IsNull() ? 0.0f : fields[8].GetFloat());

        tmpl.YardRadius = fields[9].IsNull() ? 0.0f : fields[9].GetFloat();
        tmpl.Flags = fields[10].GetUInt32();
        tmpl.Comment = fields[11].IsNull() ? "" : fields[11].GetString();

        if (tmpl.Index >= kPlotCount)
        {
            TC_LOG_ERROR("housing", "housing_plot row for map %u has plot_index %u, max is %u - skipped",
                tmpl.MapId, uint32(tmpl.Index), uint32(kPlotCount - 1));
            continue;
        }

        _plotTemplates[PlotKey(tmpl.MapId, tmpl.Index)] = tmpl;
        _plotTemplatesByMap[tmpl.MapId].push_back(tmpl);
    }
    while (result->NextRow());

    for (auto& entry : _plotTemplatesByMap)
    {
        std::sort(entry.second.begin(), entry.second.end(),
            [](PlotTemplate const& left, PlotTemplate const& right) { return left.Index < right.Index; });
    }
}

void HousingMgr::LoadDecor()
{
    _decor.clear();

    QueryResult result = WorldDatabase.Query(
        "SELECT `id`, `name`, `gameobject_entry`, `budget_cost`, `surface`, `placement`, `category`, "
        "`source_type`, `source_id`, `price`, `currency`, `faction_mask`, `scale_min`, `scale_max`, `flags` "
        "FROM `housing_decor`");

    if (!result)
    {
        TC_LOG_ERROR("housing", "housing_decor is empty - nothing can be placed");
        return;
    }

    do
    {
        Field* fields = result->Fetch();

        DecorInfo info;
        info.Id = fields[0].GetUInt32();
        info.Name = fields[1].GetString();
        info.GameObjectEntry = fields[2].GetUInt32();
        info.BudgetCost = fields[3].GetUInt8();
        info.Surface = fields[4].GetUInt8();
        info.Placement = fields[5].GetUInt8();
        info.Category = fields[6].GetString();
        info.SourceType = fields[7].GetUInt8();
        info.SourceId = fields[8].GetUInt32();
        info.Price = fields[9].GetUInt32();
        info.Currency = fields[10].GetUInt32();
        info.FactionMask = fields[11].GetUInt8();
        info.ScaleMin = fields[12].GetFloat();
        info.ScaleMax = fields[13].GetFloat();
        info.Flags = fields[14].GetUInt32();

        if (!info.GameObjectEntry || !sObjectMgr->GetGameObjectTemplate(info.GameObjectEntry))
        {
            TC_LOG_ERROR("housing", "Decor %u (%s) points at missing gameobject entry %u - skipped",
                info.Id, info.Name.c_str(), info.GameObjectEntry);
            continue;
        }

        if (info.ScaleMin <= 0.0f)
            info.ScaleMin = 0.5f;
        if (info.ScaleMax < info.ScaleMin)
            info.ScaleMax = info.ScaleMin;

        _decor[info.Id] = info;
    }
    while (result->NextRow());
}

void HousingMgr::LoadVendors()
{
    _vendors.clear();
    _vendorStock.clear();
    _vendorList.clear();

    if (QueryResult result = WorldDatabase.Query(
        "SELECT `id`, `name`, `biome_id`, `role`, `creature_entry`, `x`, `y`, `z`, `o`, `retail_name`, `comment` "
        "FROM `housing_vendor`"))
    {
        do
        {
            Field* fields = result->Fetch();

            VendorInfo info;
            info.Id = fields[0].GetUInt32();
            info.Name = fields[1].GetString();
            info.BiomeId = fields[2].GetUInt32();
            info.Role = fields[3].GetUInt8();
            info.CreatureEntry = fields[4].GetUInt32();
            info.Pos = Position(fields[5].GetFloat(), fields[6].GetFloat(), fields[7].GetFloat(), fields[8].GetFloat());
            info.RetailName = fields[9].GetString();
            info.Comment = fields[10].IsNull() ? "" : fields[10].GetString();
            info.HasSpawn = info.CreatureEntry != 0;

            _vendors[info.Id] = info;
            _vendorList.push_back(info);
        }
        while (result->NextRow());
    }

    if (QueryResult result = WorldDatabase.Query(
        "SELECT `vendor_id`, `decor_id`, `price` FROM `housing_vendor_stock`"))
    {
        do
        {
            Field* fields = result->Fetch();

            VendorStockEntry entry;
            entry.DecorId = fields[1].GetUInt32();
            entry.Price = fields[2].GetUInt32();
            _vendorStock[fields[0].GetUInt32()].push_back(entry);
        }
        while (result->NextRow());
    }
}

void HousingMgr::LoadAmenities()
{
    _amenities.clear();
    _amenityList.clear();

    if (QueryResult result = WorldDatabase.Query(
        "SELECT `id`, `biome_id`, `name`, `type`, `x`, `y`, `z`, `o`, `radius`, `flags` FROM `housing_amenity`"))
    {
        do
        {
            Field* fields = result->Fetch();

            AmenityInfo info;
            info.Id = fields[0].GetUInt32();
            info.BiomeId = fields[1].GetUInt32();
            info.Name = fields[2].GetString();
            info.Type = fields[3].GetUInt8();
            info.Pos = Position(fields[4].GetFloat(), fields[5].GetFloat(), fields[6].GetFloat(), fields[7].GetFloat());
            info.Radius = fields[8].GetFloat();
            info.Flags = fields[9].GetUInt32();

            _amenities[info.Id] = info;
            _amenityList.push_back(info);
        }
        while (result->NextRow());
    }
}

void HousingMgr::LoadNamePool()
{
    _namePool.clear();

    if (QueryResult result = WorldDatabase.Query("SELECT `faction`, `name` FROM `housing_neighborhood_name` ORDER BY `id`"))
    {
        do
        {
            Field* fields = result->Fetch();
            _namePool[fields[0].GetUInt32()].push_back(fields[1].GetString());
        }
        while (result->NextRow());
    }
}

void HousingMgr::LoadNeighborhoods()
{
    _neighborhoods.clear();
    _usedSlots.assign(kNeighborhoodSlotMax + 1, false);
    _houseViewers.clear();
    _guestCache.clear();

    if (!_enabled)
        return;

    if (QueryResult result = CharacterDatabase.Query(
        "SELECT `id`, `type`, `name`, `faction`, `map_id`, `slot`, `guildId`, `ownerAccount`, `created` "
        "FROM `character_housing_neighborhood`"))
    {
        do
        {
            Field* fields = result->Fetch();

            uint32 const id = fields[0].GetUInt32();
            uint8 const type = fields[1].GetUInt8();
            std::string const name = fields[2].GetString();
            uint8 const faction = fields[3].GetUInt8();
            uint32 const mapId = fields[4].GetUInt32();
            uint16 const slot = fields[5].GetUInt16();
            uint64 const guildId = fields[6].GetUInt64();
            uint32 const ownerAccount = fields[7].GetUInt32();
            uint32 const created = fields[8].GetUInt32();

            if (slot > kNeighborhoodSlotMax || _usedSlots[slot])
            {
                TC_LOG_ERROR("housing", "Neighbourhood " UI64FMTD " has an invalid or duplicate slot %u - skipped",
                    uint64(id), uint32(slot));
                continue;
            }

            std::unique_ptr<HousingNeighborhood> neighborhood(new HousingNeighborhood(id, slot, mapId));
            neighborhood->SetInfo(type, faction, name, guildId, ownerAccount, created);
            neighborhood->InitPlots(GetPlotTemplates(mapId));
            neighborhood->LoadMembers();

            _usedSlots[slot] = true;
            _neighborhoods[id] = std::move(neighborhood);
        }
        while (result->NextRow());
    }

    if (QueryResult result = CharacterDatabase.Query(
        "SELECT `id`, `neighborhoodId`, `plotIndex`, `ownerAccount`, `ownerGuid`, `ownerName`, `tier`, "
        "`blueprint`, `name`, `locked` FROM `character_housing`"))
    {
        do
        {
            Field* fields = result->Fetch();

            uint32 const housingId = fields[0].GetUInt32();
            uint32 const neighborhoodId = fields[1].GetUInt32();
            uint8 const plotIndex = fields[2].GetUInt8();

            auto const itr = _neighborhoods.find(neighborhoodId);
            if (itr == _neighborhoods.end())
            {
                TC_LOG_ERROR("housing", "House " UI64FMTD " references missing neighbourhood %u - skipped",
                    uint64(housingId), neighborhoodId);
                continue;
            }

            HousingPlot* plot = itr->second->GetPlot(plotIndex);
            if (!plot)
            {
                TC_LOG_ERROR("housing", "House " UI64FMTD " references missing plot %u in neighbourhood %u - skipped",
                    uint64(housingId), uint32(plotIndex), neighborhoodId);
                continue;
            }

            plot->SetOwner(housingId, fields[3].GetUInt32(), ObjectGuid(fields[4].GetUInt64()),
                fields[5].GetString(), fields[6].GetUInt8(), fields[7].GetUInt32(),
                fields[8].GetString(), fields[9].GetUInt8() != 0);
        }
        while (result->NextRow());
    }

    if (QueryResult result = CharacterDatabase.Query("SELECT MAX(`id`) FROM `character_housing_decor`"))
    {
        Field* fields = result->Fetch();
        _nextPlacementId = fields[0].IsNull() ? 1 : fields[0].GetUInt64() + 1;
    }

    if (QueryResult result = CharacterDatabase.Query("SELECT MAX(`id`) FROM `character_housing_neighborhood`"))
    {
        Field* fields = result->Fetch();
        _nextNeighborhoodId = fields[0].IsNull() ? 1 : fields[0].GetUInt32() + 1;
    }

    if (QueryResult result = CharacterDatabase.Query("SELECT MAX(`id`) FROM `character_housing`"))
    {
        Field* fields = result->Fetch();
        _nextHousingId = fields[0].IsNull() ? 1 : fields[0].GetUInt32() + 1;
    }

    TC_LOG_INFO("housing", "Loaded %u neighbourhoods, %u owned plots", uint32(_neighborhoods.size()),
        uint32(std::count_if(_neighborhoods.begin(), _neighborhoods.end(),
            [](std::pair<uint32 const, std::unique_ptr<HousingNeighborhood>> const& entry)
            {
                return entry.second->GetOwnedCount() != 0;
            })));
}

void HousingMgr::Unload()
{
    SaveAll();

    _neighborhoods.clear();
    _usedSlots.assign(kNeighborhoodSlotMax + 1, false);
    _playerStates.clear();
    _collection.clear();
    _guestCache.clear();
    _collectionLoaded.clear();
    _houseViewers.clear();
    _nextNeighborhoodId = 1;
    _nextHousingId = 1;
    _nextPlacementId = 1;
}

// -----------------------------------------------------------------------------
// Lookups
// -----------------------------------------------------------------------------

uint64 HousingMgr::PlotKey(uint32 mapId, uint8 index)
{
    return (uint64(mapId) << 8) | uint64(index);
}

BiomeInfo const* HousingMgr::GetBiome(uint32 id) const
{
    auto const itr = _biomes.find(id);
    return itr != _biomes.end() ? &itr->second : nullptr;
}

PlotTemplate const* HousingMgr::GetPlotTemplate(uint32 mapId, uint8 index) const
{
    auto const itr = _plotTemplates.find(PlotKey(mapId, index));
    return itr != _plotTemplates.end() ? &itr->second : nullptr;
}

DecorInfo const* HousingMgr::GetDecor(uint32 id) const
{
    auto const itr = _decor.find(id);
    return itr != _decor.end() ? &itr->second : nullptr;
}

VendorInfo const* HousingMgr::GetVendor(uint32 id) const
{
    auto const itr = _vendors.find(id);
    return itr != _vendors.end() ? &itr->second : nullptr;
}

AmenityInfo const* HousingMgr::GetAmenity(uint32 id) const
{
    auto const itr = _amenities.find(id);
    return itr != _amenities.end() ? &itr->second : nullptr;
}

std::vector<PlotTemplate> const& HousingMgr::GetPlotTemplates(uint32 mapId) const
{
    static std::vector<PlotTemplate> const empty;

    auto const itr = _plotTemplatesByMap.find(mapId);
    return itr != _plotTemplatesByMap.end() ? itr->second : empty;
}

std::vector<VendorStockEntry> const& HousingMgr::GetVendorStock(uint32 vendorId) const
{
    static std::vector<VendorStockEntry> const empty;

    auto const itr = _vendorStock.find(vendorId);
    return itr != _vendorStock.end() ? itr->second : empty;
}

bool HousingMgr::IsHousingMap(uint32 mapId) const
{
    return _enabled && (mapId == _housingMapId || mapId == _interiorMapId);
}

// -----------------------------------------------------------------------------
// Neighbourhoods
// -----------------------------------------------------------------------------

HousingNeighborhood* HousingMgr::GetNeighborhood(uint32 id)
{
    auto const itr = _neighborhoods.find(id);
    return itr != _neighborhoods.end() ? itr->second.get() : nullptr;
}

HousingNeighborhood* HousingMgr::GetNeighborhoodBySlot(uint16 slot)
{
    for (auto const& entry : _neighborhoods)
        if (entry.second->GetSlot() == slot)
            return entry.second.get();

    return nullptr;
}

uint16 HousingMgr::AllocateSlot()
{
    for (uint16 slot = 0; slot <= kNeighborhoodSlotMax; ++slot)
    {
        if (!_usedSlots[slot])
        {
            _usedSlots[slot] = true;
            return slot;
        }
    }

    return uint16(kNeighborhoodSlotMax + 1);   // no free slot
}

void HousingMgr::ReleaseSlot(uint16 slot)
{
    if (slot <= kNeighborhoodSlotMax)
        _usedSlots[slot] = false;
}

HousingNeighborhood* HousingMgr::CreateNeighborhood(uint8 type, uint8 faction, std::string const& name,
    uint64 guildId, uint32 ownerAccount)
{
    if (!_enabled)
        return nullptr;

    uint16 const slot = AllocateSlot();
    if (slot > kNeighborhoodSlotMax)
    {
        TC_LOG_ERROR("housing", "No free neighbourhood slot - cannot create '%s'", name.c_str());
        return nullptr;
    }

    uint32 const id = GenerateNeighborhoodId();

    std::string escapedName = name;
    CharacterDatabase.EscapeString(escapedName);

    CharacterDatabase.PExecute(
        "INSERT INTO `character_housing_neighborhood` (`id`, `type`, `name`, `faction`, `map_id`, `slot`, "
        "`guildId`, `ownerAccount`, `created`) VALUES ({}, {}, '{}', {}, {}, {}, {}, {}, UNIX_TIMESTAMP())",
        id, uint32(type), escapedName, uint32(faction), _housingMapId, uint32(slot), guildId, ownerAccount);

    std::unique_ptr<HousingNeighborhood> neighborhood(new HousingNeighborhood(id, slot, _housingMapId));
    neighborhood->SetInfo(type, faction, name, guildId, ownerAccount, Now());
    neighborhood->InitPlots(GetPlotTemplates(_housingMapId));

    // Guild and private neighbourhoods are membership based: the founder becomes leader.
    if (type != NeighborhoodPublic && ownerAccount)
        neighborhood->AddMember(ownerAccount, 2);

    HousingNeighborhood* raw = neighborhood.get();
    _neighborhoods[id] = std::move(neighborhood);

    TC_LOG_INFO("housing", "Created %s neighbourhood %u ('%s') in slot %u",
        type == NeighborhoodPublic ? "public" : (type == NeighborhoodGuild ? "guild" : "private"),
        id, name.c_str(), uint32(slot));

    return raw;
}

void HousingMgr::DissolveNeighborhood(HousingNeighborhood* neighborhood)
{
    if (!neighborhood)
        return;

    for (auto const& plot : neighborhood->GetPlots())
    {
        if (!plot->IsOwned())
            continue;

        ForceSellPlot(plot.get());
    }

    CharacterDatabase.PExecute("DELETE FROM `character_housing_neighborhood` WHERE `id` = {}", neighborhood->GetId());
    CharacterDatabase.PExecute("DELETE FROM `character_housing_neighbor_member` WHERE `neighborhoodId` = {}",
        neighborhood->GetId());

    ReleaseSlot(neighborhood->GetSlot());
    _neighborhoods.erase(neighborhood->GetId());
}

HousingNeighborhood* HousingMgr::FindNeighborhoodFor(Player* player)
{
    if (!player || !player->GetSession())
        return nullptr;

    PlayerState* state = GetPlayerState(player);
    if (state && state->NeighborhoodId)
        if (HousingNeighborhood* current = GetNeighborhood(state->NeighborhoodId))
            return current;

    if (HousingPlot* owned = GetOwnedPlot(player))
        for (auto const& entry : _neighborhoods)
            if (entry.second->FindPlotOfAccount(player->GetSession()->GetAccountId()) == owned)
                return entry.second.get();

    // Fall back to the emptiest public neighbourhood of the player's faction, then create
    // a new one when every public neighbourhood is full.
    uint8 const faction = player->GetTeamId() == TEAM_ALLIANCE ? FactionAlliance : FactionHorde;

    HousingNeighborhood* best = nullptr;
    for (auto const& entry : _neighborhoods)
    {
        HousingNeighborhood* candidate = entry.second.get();
        if (candidate->GetType() != NeighborhoodPublic || candidate->GetFaction() != faction)
            continue;

        if (!candidate->FindFreePlot())
            continue;

        if (!best || candidate->GetOwnedCount() < best->GetOwnedCount())
            best = candidate;
    }

    if (best)
        return best;

    auto const itr = _namePool.find(faction);
    std::string const name = (itr != _namePool.end() && !itr->second.empty())
        ? Trinity::StringFormat("{} {}", itr->second[_neighborhoods.size() % itr->second.size()], _neighborhoods.size() + 1)
        : Trinity::StringFormat("Founder's Point {}", _neighborhoods.size() + 1);

    return CreateNeighborhood(NeighborhoodPublic, faction, name, 0, 0);
}

// -----------------------------------------------------------------------------
// Player lifecycle
// -----------------------------------------------------------------------------

PlayerState* HousingMgr::GetPlayerState(Player* player)
{
    if (!player)
        return nullptr;

    uint64 const key = player->GetGUID().GetCounter();

    auto itr = _playerStates.find(key);
    if (itr == _playerStates.end())
        return nullptr;

    return &itr->second;
}

void HousingMgr::OnPlayerLogin(Player* player)
{
    if (!player)
        return;

    uint64 const key = player->GetGUID().GetCounter();
    _playerStates[key] = PlayerState();

    if (!_enabled)
        return;

    PlayerState& state = _playerStates[key];

    if (HousingPlot* plot = GetOwnedPlot(player))
    {
        state.HasOwnedPlot = true;
        state.OwnedHousingId = plot->GetHousingId();
    }

    LoadCollection(player->GetSession()->GetAccountId());
}

void HousingMgr::OnPlayerLogout(Player* player)
{
    if (!player)
        return;

    if (PlayerState* state = GetPlayerState(player))
        ClearPlotPhases(player, *state);

    _playerStates.erase(player->GetGUID().GetCounter());
}

void HousingMgr::OnPlayerZoneChange(Player* player)
{
    if (!player || !_enabled)
        return;

    PlayerState* state = GetPlayerState(player);
    if (!state)
        return;

    // Any map change drops the phases, including a hop between two housing maps: the
    // target neighbourhood may be a different slot and the old phases would leak.
    ClearPlotPhases(player, *state);

    if (!IsHousingMap(player->GetMapId()))
    {
        state->NeighborhoodId = 0;
        state->InsideInterior = false;
    }
}

void HousingMgr::OnPlayerUpdate(Player* player, uint32 diff)
{
    if (!_enabled || !player || !player->IsInWorld())
        return;

    PlayerState* state = GetPlayerState(player);
    if (!state)
        return;

    state->TickAccum += diff;
    if (state->TickAccum < 500)
        return;

    state->TickAccum = 0;

    if (!IsHousingMap(player->GetMapId()))
        return;

    ResolvePlot(player, *state);
}

void HousingMgr::ClearPlotPhases(Player* player, PlayerState& state)
{
    if (!player)
        return;

    if (HousingNeighborhood* neighborhood = GetNeighborhood(state.NeighborhoodId))
    {
        if (state.InsidePlot != 0xFF)
        {
            HousingPlot* plot = neighborhood->GetPlot(state.InsidePlot);
            if (_plotScopedPhasing)
            {
                PhasingHandler::RemovePhase(player, plot ? plot->GetPlotPhase() : PlotPhase(neighborhood->GetSlot(), state.InsidePlot), true);
                PhasingHandler::RemovePhase(player, plot ? plot->GetInteriorPhase() : InteriorPhase(neighborhood->GetSlot(), state.InsidePlot), true);
            }

            if (plot)
                ReleaseViewer(plot);
        }
    }

    state.InsidePlot = 0xFF;
    state.InsideInterior = false;
}

void HousingMgr::ApplyPlotPhase(Player* player, HousingPlot* plot)
{
    if (!player || !plot || !_plotScopedPhasing)
        return;

    PhasingHandler::AddPhase(player, plot->GetPlotPhase(), true);
}

void HousingMgr::ReleaseViewer(HousingPlot* plot)
{
    if (!plot || !plot->IsOwned())
        return;

    uint32 const housingId = plot->GetHousingId();
    auto itr = _houseViewers.find(housingId);
    if (itr == _houseViewers.end())
        return;

    if (itr->second > 1)
    {
        --itr->second;
        return;
    }

    _houseViewers.erase(itr);

    if (HousingHouse* house = plot->GetHouse())
        house->DespawnAll();
}

void HousingMgr::ResolvePlot(Player* player, PlayerState& state)
{
    // Interior: the player only gets here through EnterHouse, so all that is left is to
    // make sure the furniture exists on the map they landed on.
    if (player->GetMapId() == _interiorMapId)
    {
        HousingNeighborhood* neighborhood = GetNeighborhood(state.NeighborhoodId);
        if (!neighborhood || state.InsidePlot == 0xFF)
            return;

        HousingPlot* plot = neighborhood->GetPlot(state.InsidePlot);
        if (!plot || !plot->IsOwned())
            return;

        if (HousingHouse* house = plot->GetHouse())
            house->EnsureSpawned(player->GetMap(), plot->GetInteriorPhase(),
                player->GetGUID());

        state.InsideInterior = true;
        return;
    }

    HousingNeighborhood* neighborhood = GetNeighborhood(state.NeighborhoodId);
    if (!neighborhood)
        return;

    uint32 const accountId = player->GetSession()->GetAccountId();
    uint8 found = 0xFF;

    for (auto const& plot : neighborhood->GetPlots())
    {
        PlotTemplate const& tmpl = plot->GetTemplate();
        if (!tmpl.HasSpawn)
            continue;

        float const limit = (tmpl.YardRadius > 0.0f ? tmpl.YardRadius : kDefaultYardRadius) * 1.35f;
        float const dx = player->GetPositionX() - tmpl.Spawn.m_positionX;
        float const dy = player->GetPositionY() - tmpl.Spawn.m_positionY;

        if ((dx * dx + dy * dy) <= (limit * limit))
        {
            found = plot->GetIndex();
            break;
        }
    }

    if (found == state.InsidePlot)
        return;

    if (state.InsidePlot != 0xFF)
    {
        if (HousingPlot* previous = neighborhood->GetPlot(state.InsidePlot))
        {
            if (_plotScopedPhasing)
                PhasingHandler::RemovePhase(player, previous->GetPlotPhase(), true);

            ReleaseViewer(previous);
        }
    }

    state.InsidePlot = found;

    if (found == 0xFF)
        return;

    HousingPlot* plot = neighborhood->GetPlot(found);
    if (!plot)
        return;

    bool const allowed = !plot->IsOwned()
        || CheckAccess(player, plot) >= AccessNeighbor
        || plot->GetOwnerAccount() == accountId;

    if (!allowed)
    {
        state.InsidePlot = 0xFF;
        return;
    }

    ApplyPlotPhase(player, plot);

    if (!plot->IsOwned())
        return;

    ++_houseViewers[plot->GetHousingId()];

    if (HousingHouse* house = plot->GetHouse())
        house->EnsureSpawned(player->GetMap(), plot->GetPlotPhase(), plot->GetOwnerGuid());
}

// -----------------------------------------------------------------------------
// Access
// -----------------------------------------------------------------------------

HousingAccess HousingMgr::CheckAccess(Player* player, HousingPlot* plot)
{
    if (!player || !plot)
        return AccessNone;

    if (player->IsGameMaster())
        return AccessGm;

    if (!plot->IsOwned())
        return AccessStranger;

    uint32 const accountId = player->GetSession()->GetAccountId();

    if (accountId == plot->GetOwnerAccount())
        return AccessOwner;

    if (GetGuestFlags(plot->GetHousingId(), accountId) & 0x01)
        return AccessGuest;

    if (HousingNeighborhood* neighborhood = plot->GetNeighborhood())
        if (neighborhood->IsMember(accountId))
            return AccessNeighbor;

    if (HousingHouse const* house = plot->GetHouse())
        if (!house->IsLocked())
            return AccessNeighbor;

    return AccessStranger;
}

HousingPlot* HousingMgr::GetOwnedPlot(Player* player)
{
    if (!player || !player->GetSession())
        return nullptr;

    uint32 const accountId = player->GetSession()->GetAccountId();

    for (auto const& entry : _neighborhoods)
        if (HousingPlot* plot = entry.second->FindPlotOfAccount(accountId))
            return plot;

    return nullptr;
}

HousingPlot* HousingMgr::GetPlotForEdit(Player* player)
{
    if (!player || !player->GetSession())
        return nullptr;

    PlayerState* state = GetPlayerState(player);

    if (state && state->NeighborhoodId)
    {
        if (HousingNeighborhood* neighborhood = GetNeighborhood(state->NeighborhoodId))
        {
            if (state->InsidePlot != 0xFF)
                if (HousingPlot* plot = neighborhood->GetPlot(state->InsidePlot))
                    if (CheckAccess(player, plot) >= AccessOwner)
                        return plot;
        }
    }

    return GetOwnedPlot(player);
}

// -----------------------------------------------------------------------------
// Decor collection
// -----------------------------------------------------------------------------

void HousingMgr::LoadCollection(uint32 accountId)
{
    if (!accountId || _collectionLoaded.count(accountId))
        return;

    _collectionLoaded.insert(accountId);

    QueryResult result = CharacterDatabase.PQuery(
        "SELECT `decorId` FROM `character_housing_collection` WHERE `accountId` = {}", accountId);

    if (!result)
        return;

    std::set<uint32>& unlocked = _collection[accountId];

    do
    {
        unlocked.insert(result->Fetch()[0].GetUInt32());
    }
    while (result->NextRow());
}

bool HousingMgr::HasUnlocked(uint32 accountId, uint32 decorId)
{
    LoadCollection(accountId);

    auto const itr = _collection.find(accountId);
    return itr != _collection.end() && itr->second.count(decorId) != 0;
}

// -----------------------------------------------------------------------------
// Plot operations
// -----------------------------------------------------------------------------

HousingResult HousingMgr::BuyPlot(Player* player, HousingNeighborhood* neighborhood, uint8 plotIndex)
{
    if (!_enabled)
        return Disabled;

    if (!player || !player->GetSession() || !neighborhood)
        return NotFound;

    HousingPlot* plot = neighborhood->GetPlot(plotIndex);
    if (!plot)
        return InvalidPlot;

    if (plot->IsOwned())
        return PlotTaken;

    // A plot without an authored spawn point cannot be entered, so it cannot be sold
    // either - the player would pay for a plot the server has to teleport them out of.
    if (!plot->GetTemplate().HasSpawn)
        return InvalidPlot;

    uint8 const faction = player->GetTeamId() == TEAM_ALLIANCE ? FactionAlliance : FactionHorde;
    if (neighborhood->GetFaction() != faction)
        return WrongFaction;

    // One house per account. Retail allows one per faction per warband; with a single
    // faction implemented, account-wide is the same rule.
    if (GetOwnedPlot(player))
        return TooManyHouses;

    uint32 const accountId = player->GetSession()->GetAccountId();

    if (neighborhood->GetType() != NeighborhoodPublic && !neighborhood->IsMember(accountId))
        return NotAllowedHere;

    uint64 const cost = GoldToCopper(plot->GetPrice());
    if (!player->HasEnoughMoney(int64(cost)))
        return NotEnoughMoney;

    uint32 const housingId = GenerateHousingId();
    std::string ownerName = player->GetName();
    CharacterDatabase.EscapeString(ownerName);

    CharacterDatabase.PExecute(
        "INSERT INTO `character_housing` (`id`, `neighborhoodId`, `plotIndex`, `ownerAccount`, `ownerGuid`, "
        "`ownerName`, `tier`, `blueprint`, `name`, `locked`, `budget_indoor`, `budget_outdoor`, "
        "`purchase_time`, `last_edited`) VALUES ({}, {}, {}, {}, {}, '{}', 1, 0, '', 0, 0, 0, {}, {})",
        housingId, neighborhood->GetId(), uint32(plotIndex), accountId, player->GetGUID().GetCounter(),
        ownerName, Now(), Now());

    player->ModifyMoney(-int64(cost));

    plot->SetOwner(housingId, accountId, player->GetGUID(), player->GetName(), 1, 0, "", false);
    plot->GetHouse()->MarkDirty();
    plot->GetHouse()->Flush();

    if (PlayerState* state = GetPlayerState(player))
    {
        state->HasOwnedPlot = true;
        state->OwnedHousingId = housingId;
    }

    TC_LOG_INFO("housing", "Account %u bought plot %u in neighbourhood %u (house " UI64FMTD ") for %u gold",
        accountId, uint32(plotIndex), neighborhood->GetId(), uint64(housingId), plot->GetPrice());

    return Ok;
}

void HousingMgr::ForceSellPlot(HousingPlot* plot)
{
    if (!plot || !plot->IsOwned())
        return;

    uint32 const housingId = plot->GetHousingId();

    if (HousingHouse* house = plot->GetHouse())
    {
        house->DespawnAll();
        house->DeleteAllDecor();
    }

    CharacterDatabase.PExecute("DELETE FROM `character_housing` WHERE `id` = {}", housingId);
    CharacterDatabase.PExecute("DELETE FROM `character_housing_guest` WHERE `housingId` = {}", housingId);

    _houseViewers.erase(housingId);
    _guestCache.erase(housingId);

    plot->ClearOwner();
}

HousingResult HousingMgr::SellPlot(Player* player)
{
    if (!_enabled)
        return Disabled;

    HousingPlot* plot = GetPlotForEdit(player);
    if (!plot || !plot->IsOwned())
        return NoHouse;

    if (CheckAccess(player, plot) < AccessOwner)
        return NotOwner;

    uint32 const refund = plot->GetPrice() * _refundPercent / 100;
    uint32 const housingId = plot->GetHousingId();

    ForceSellPlot(plot);

    if (refund)
        player->ModifyMoney(int64(GoldToCopper(refund)));

    // The player may be standing in the plot they just sold: drop the phases so they do
    // not keep watching a house that no longer exists.
    if (PlayerState* state = GetPlayerState(player))
    {
        ClearPlotPhases(player, *state);
        state->HasOwnedPlot = false;
        state->OwnedHousingId = 0;
    }

    TC_LOG_INFO("housing", "House " UI64FMTD " sold by account %u, refund %u gold",
        uint64(housingId), player->GetSession()->GetAccountId(), refund);

    return Ok;
}

HousingResult HousingMgr::UpgradeTier(Player* player, uint8 tier)
{
    if (!_enabled)
        return Disabled;

    if (tier < 1 || tier > kMaxTier)
        return TierMax;

    HousingPlot* plot = GetPlotForEdit(player);
    if (!plot || !plot->IsOwned())
        return NoHouse;

    HousingHouse* house = plot->GetHouse();
    if (!house)
        return NoHouse;

    if (tier <= house->GetTier())
        return TierTooLow;

    uint64 const cost = GoldToCopper(TierUnlockCost(tier));
    if (!player->HasEnoughMoney(int64(cost)))
        return NotEnoughMoney;

    player->ModifyMoney(-int64(cost));

    house->SetTier(tier);
    house->Flush();

    return Ok;
}

HousingResult HousingMgr::RenameHouse(Player* player, std::string const& name)
{
    if (!_enabled)
        return Disabled;

    std::string const clean = HousingAddon::SanitiseText(name, 64);
    if (clean.empty())
        return InvalidPlot;

    HousingPlot* plot = GetPlotForEdit(player);
    if (!plot || !plot->IsOwned())
        return NoHouse;

    if (CheckAccess(player, plot) < AccessOwner)
        return NotOwner;

    plot->GetHouse()->SetName(clean);
    plot->GetHouse()->Flush();

    return Ok;
}

HousingResult HousingMgr::SetLocked(Player* player, bool locked)
{
    if (!_enabled)
        return Disabled;

    HousingPlot* plot = GetPlotForEdit(player);
    if (!plot || !plot->IsOwned())
        return NoHouse;

    if (CheckAccess(player, plot) < AccessOwner)
        return NotOwner;

    plot->GetHouse()->SetLocked(locked);
    plot->GetHouse()->Flush();

    return Ok;
}

// -----------------------------------------------------------------------------
// Travel
// -----------------------------------------------------------------------------

HousingResult HousingMgr::TeleportToPlot(Player* player, uint8 plotIndex)
{
    if (!_enabled)
        return Disabled;

    PlayerState* state = GetPlayerState(player);
    if (!state)
        return InternalError;

    HousingNeighborhood* neighborhood = FindNeighborhoodFor(player);
    if (!neighborhood)
        return NotInNeighborhood;

    HousingPlot* plot = neighborhood->GetPlot(plotIndex);
    if (!plot)
        return InvalidPlot;

    if (!plot->GetTemplate().HasSpawn)
        return NotAllowedHere;

    if (plot->IsOwned() && CheckAccess(player, plot) < AccessNeighbor)
        return GuestRestricted;

    ClearPlotPhases(player, *state);

    state->NeighborhoodId = neighborhood->GetId();
    state->InsidePlot = plotIndex;

    ApplyPlotPhase(player, plot);

    if (!player->TeleportTo(neighborhood->GetMapId(), plot->GetTemplate().Spawn))
    {
        state->InsidePlot = 0xFF;
        return InternalError;
    }

    return Ok;
}

HousingResult HousingMgr::TeleportHome(Player* player)
{
    if (!_enabled)
        return Disabled;

    PlayerState* state = GetPlayerState(player);
    if (!state)
        return InternalError;

    HousingPlot* plot = GetOwnedPlot(player);
    if (!plot)
        return NoHouse;

    uint32 const now = getMSTime();
    uint32 const cooldownMs = _homeCooldown * 1000;

    if (_homeCooldown && state->LastHomeUse && (now - state->LastHomeUse) < cooldownMs)
        return Cooldown;

    HousingNeighborhood* neighborhood = plot->GetNeighborhood();
    if (!neighborhood || !plot->GetTemplate().HasSpawn)
        return NotAllowedHere;

    ClearPlotPhases(player, *state);

    state->NeighborhoodId = neighborhood->GetId();
    state->InsidePlot = plot->GetIndex();

    ApplyPlotPhase(player, plot);

    if (!player->TeleportTo(neighborhood->GetMapId(), plot->GetTemplate().Spawn))
        return InternalError;

    state->LastHomeUse = now;

    return Ok;
}

HousingResult HousingMgr::TeleportToNeighborhood(Player* player, HousingNeighborhood* neighborhood)
{
    if (!_enabled)
        return Disabled;

    if (!player || !neighborhood)
        return NotInNeighborhood;

    if (!_townCenter.m_positionX && !_townCenter.m_positionY)
        return NotAllowedHere;   // town center not authored on this map yet

    PlayerState* state = GetPlayerState(player);
    if (!state)
        return InternalError;

    ClearPlotPhases(player, *state);

    state->NeighborhoodId = neighborhood->GetId();

    if (!player->TeleportTo(neighborhood->GetMapId(), _townCenter))
        return InternalError;

    return Ok;
}

HousingResult HousingMgr::TeleportToTownCenter(Player* player)
{
    HousingNeighborhood* neighborhood = FindNeighborhoodFor(player);
    return TeleportToNeighborhood(player, neighborhood);
}

HousingResult HousingMgr::EnterHouse(Player* player, bool enter)
{
    if (!_enabled)
        return Disabled;

    PlayerState* state = GetPlayerState(player);
    if (!state)
        return InternalError;

    HousingNeighborhood* neighborhood = GetNeighborhood(state->NeighborhoodId);
    if (!neighborhood)
        return NotInNeighborhood;

    if (state->InsidePlot == 0xFF)
        return InvalidPlot;

    HousingPlot* plot = neighborhood->GetPlot(state->InsidePlot);
    if (!plot || !plot->IsOwned())
        return NoHouse;

    if (CheckAccess(player, plot) < AccessGuest)
        return GuestRestricted;

    if (enter)
    {
        // Phase before teleport: the interior furniture must already be in the right phase
        // when the player lands, otherwise they get one frame of an empty room.
        PhasingHandler::AddPhase(player, plot->GetInteriorPhase(), true);

        if (_plotScopedPhasing)
            PhasingHandler::RemovePhase(player, plot->GetPlotPhase(), true);

        state->InsideInterior = true;

        if (!player->TeleportTo(_interiorMapId, _interiorAnchor))
        {
            state->InsideInterior = false;
            return InternalError;
        }

        return Ok;
    }

    if (!plot->GetTemplate().HasSpawn)
        return NotAllowedHere;

    PhasingHandler::RemovePhase(player, plot->GetInteriorPhase(), true);
    ApplyPlotPhase(player, plot);

    state->InsideInterior = false;

    if (!player->TeleportTo(neighborhood->GetMapId(), plot->GetTemplate().Spawn))
        return InternalError;

    return Ok;
}

// -----------------------------------------------------------------------------
// Decor operations
// -----------------------------------------------------------------------------

HousingResult HousingMgr::PlaceDecor(Player* player, uint32 decorId, float x, float y, float z, float o,
    float scale, bool indoor, uint64& placementId)
{
    if (!_enabled)
        return Disabled;

    DecorInfo const* info = GetDecor(decorId);
    if (!info)
        return InvalidDecor;

    uint32 const accountId = player->GetSession()->GetAccountId();
    if (!HasUnlocked(accountId, decorId))
        return NotUnlocked;

    HousingPlot* plot = GetPlotForEdit(player);
    if (!plot || !plot->IsOwned())
        return NoHouse;

    if (plot->GetOwnerAccount() != accountId)
        return NotOwner;

    PlayerState* state = GetPlayerState(player);
    if (state && !indoor && state->InsidePlot != plot->GetIndex())
        return NotAllowedHere;   // outdoor placement requires standing in the plot

    if (indoor && player->GetMapId() != _interiorMapId)
        return NotAllowedHere;

    HousingHouse* house = plot->GetHouse();
    if (!house)
        return NoHouse;

    float const yardRadius = plot->GetTemplate().YardRadius > 0.0f
        ? plot->GetTemplate().YardRadius
        : float(kDefaultYardRadius);

    placementId = GeneratePlacementId();

    HousingResult const result = house->AddPlacement(placementId, *info, x, y, z, o, scale, indoor,
        plot->GetTemplate().Spawn, yardRadius);

    if (result != Ok)
        return result;

    if (DecorPlacement* placement = house->GetPlacementMutable(placementId))
    {
        uint32 const phase = indoor ? plot->GetInteriorPhase() : plot->GetPlotPhase();
        house->SpawnPlacement(*placement, player->GetMap(), phase, plot->GetOwnerGuid());
    }

    return Ok;
}

HousingResult HousingMgr::MoveDecor(Player* player, uint64 placementId, float x, float y, float z,
    float o, float scale)
{
    if (!_enabled)
        return Disabled;

    HousingPlot* plot = GetPlotForEdit(player);
    if (!plot || !plot->IsOwned())
        return NoHouse;

    if (plot->GetOwnerAccount() != player->GetSession()->GetAccountId())
        return NotOwner;

    HousingHouse* house = plot->GetHouse();
    if (!house)
        return NoHouse;

    DecorPlacement* existing = house->GetPlacementMutable(placementId);
    if (!existing)
        return NotFound;

    DecorInfo const* info = GetDecor(existing->DecorId);
    if (!info)
        return InvalidDecor;

    float const yardRadius = plot->GetTemplate().YardRadius > 0.0f
        ? plot->GetTemplate().YardRadius
        : float(kDefaultYardRadius);

    bool const indoor = existing->Indoor;

    // Validate before touching the live object: a rejected move must not leave the piece
    // despawned until somebody walks back into the plot.
    HousingResult const checked = house->Validate(*info, x, y, z, scale, indoor,
        plot->GetTemplate().Spawn, yardRadius, placementId);

    if (checked != Ok)
        return checked;

    // Rebuild the object at the new spot: moving a spawned GO is not reliable across all
    // gameobject types, so despawn and respawn instead.
    Map* despawnMap = house->IsSpawned() ? house->GetSpawnMap() : player->GetMap();
    if (!existing->SpawnGuid.IsEmpty())
        DespawnDecorObject(*existing, despawnMap);

    HousingResult const result = house->MovePlacement(placementId, x, y, z, o, scale, *info,
        plot->GetTemplate().Spawn, yardRadius);

    if (result != Ok)
        return result;

    uint32 const phase = indoor ? plot->GetInteriorPhase() : plot->GetPlotPhase();
    house->SpawnPlacement(*existing, player->GetMap(), phase, plot->GetOwnerGuid());

    return Ok;
}

HousingResult HousingMgr::RemoveDecor(Player* player, uint64 placementId)
{
    if (!_enabled)
        return Disabled;

    HousingPlot* plot = GetPlotForEdit(player);
    if (!plot || !plot->IsOwned())
        return NoHouse;

    if (plot->GetOwnerAccount() != player->GetSession()->GetAccountId())
        return NotOwner;

    HousingHouse* house = plot->GetHouse();
    if (!house)
        return NoHouse;

    if (DecorPlacement* placement = house->GetPlacementMutable(placementId))
        DespawnDecorObject(*placement, house->IsSpawned() ? house->GetSpawnMap() : player->GetMap());

    return house->RemovePlacement(placementId);
}

HousingResult HousingMgr::BuyDecor(Player* player, uint32 decorId)
{
    if (!_enabled)
        return Disabled;

    DecorInfo const* info = GetDecor(decorId);
    if (!info)
        return InvalidDecor;

    uint32 const accountId = player->GetSession()->GetAccountId();

    if (HasUnlocked(accountId, decorId))
        return Ok;   // already owned, nothing to charge for

    uint8 const factionBit = player->GetTeamId() == TEAM_ALLIANCE ? 0x01 : 0x02;
    if (!(info->FactionMask & factionBit))
        return WrongFaction;

    // Currency based stock (community coupons, 3363) is Phase 3; refusing is better than
    // silently selling it for free.
    if (info->Currency != 0)
        return NotEnoughMoney;

    uint64 const cost = GoldToCopper(info->Price);
    if (!player->HasEnoughMoney(int64(cost)))
        return NotEnoughMoney;

    player->ModifyMoney(-int64(cost));

    CharacterDatabase.PExecute(
        "REPLACE INTO `character_housing_collection` (`accountId`, `decorId`, `source_type`, `source_id`, `acquired`) "
        "VALUES ({}, {}, {}, {}, UNIX_TIMESTAMP())",
        accountId, decorId, uint32(info->SourceType), info->SourceId);

    _collection[accountId].insert(decorId);

    return Ok;
}

// -----------------------------------------------------------------------------
// Guests
// -----------------------------------------------------------------------------

std::map<uint32, uint8> const& HousingMgr::LoadGuests(uint32 housingId)
{
    static std::map<uint32, uint8> const empty;

    auto itr = _guestCache.find(housingId);
    if (itr != _guestCache.end())
        return itr->second;

    std::map<uint32, uint8>& guests = _guestCache[housingId];

    QueryResult result = CharacterDatabase.PQuery(
        "SELECT `accountId`, `flags` FROM `character_housing_guest` WHERE `housingId` = {}", housingId);

    if (result)
    {
        do
        {
            Field* fields = result->Fetch();
            guests[fields[0].GetUInt32()] = fields[1].GetUInt8();
        }
        while (result->NextRow());
    }

    auto const stored = _guestCache.find(housingId);
    return stored != _guestCache.end() ? stored->second : empty;
}

uint8 HousingMgr::GetGuestFlags(uint32 housingId, uint32 accountId)
{
    if (!housingId || !accountId)
        return 0;

    std::map<uint32, uint8> const& guests = LoadGuests(housingId);
    auto const itr = guests.find(accountId);
    return itr != guests.end() ? itr->second : 0;
}

bool HousingMgr::IsGuest(uint32 housingId, uint32 accountId)
{
    return (GetGuestFlags(housingId, accountId) & 0x01) != 0;
}

void HousingMgr::AddGuest(uint32 housingId, uint32 accountId, uint8 flags)
{
    if (!housingId || !accountId)
        return;

    CharacterDatabase.PExecute(
        "REPLACE INTO `character_housing_guest` (`housingId`, `accountId`, `flags`, `added`) "
        "VALUES ({}, {}, {}, UNIX_TIMESTAMP())", housingId, accountId, uint32(flags));

    _guestCache[housingId][accountId] = flags;
}

void HousingMgr::RemoveGuest(uint32 housingId, uint32 accountId)
{
    CharacterDatabase.PExecute(
        "DELETE FROM `character_housing_guest` WHERE `housingId` = {} AND `accountId` = {}",
        housingId, accountId);

    auto itr = _guestCache.find(housingId);
    if (itr != _guestCache.end())
        itr->second.erase(accountId);
}

// -----------------------------------------------------------------------------
// Maintenance
// -----------------------------------------------------------------------------

void HousingMgr::Update(uint32 diff)
{
    if (!_enabled)
        return;

    _tick += diff;
    _saveTimer += diff;

    if (_saveTimer < std::max(5u, _saveInterval) * 1000)
        return;

    _saveTimer = 0;
    SaveAll();
}

void HousingMgr::SaveAll()
{
    for (auto const& entry : _neighborhoods)
    {
        for (auto const& plot : entry.second->GetPlots())
        {
            if (!plot->IsOwned())
                continue;

            if (HousingHouse* house = plot->GetHouse())
                if (house->IsDirty())
                    house->Flush();
        }
    }
}

std::string HousingMgr::BuildAuditReport()
{
    uint32 plots = 0;
    uint32 unauthored = 0;
    uint32 owned = 0;
    uint32 overBudget = 0;
    uint32 placements = 0;

    std::string report = Trinity::StringFormat("Housing audit: {} neighbourhoods\n", uint32(_neighborhoods.size()));

    for (auto const& entry : _neighborhoods)
    {
        HousingNeighborhood* neighborhood = entry.second.get();
        report += Trinity::StringFormat("  #{} '{}' slot {} type {} faction {} plots {}/{} owned\n",
            neighborhood->GetId(), neighborhood->GetName(), uint32(neighborhood->GetSlot()),
            uint32(neighborhood->GetType()), uint32(neighborhood->GetFaction()),
            neighborhood->GetOwnedCount(), uint32(kPlotCount));

        for (auto const& plot : neighborhood->GetPlots())
        {
            ++plots;

            if (!plot->GetTemplate().HasSpawn)
                ++unauthored;

            if (!plot->IsOwned())
                continue;

            ++owned;

            HousingHouse* house = plot->GetHouse();
            if (!house)
            {
                report += Trinity::StringFormat("    plot {}: owner without house - invariant 3 violated\n",
                    uint32(plot->GetIndex()));
                continue;
            }

            placements += uint32(house->GetPlacements().size());

            if (house->IsOverBudget())
            {
                ++overBudget;
                report += Trinity::StringFormat("    plot {}: over budget (in {}/{}, out {}/{})\n",
                    uint32(plot->GetIndex()), house->GetUsedBudget(true), house->GetBudget(true),
                    house->GetUsedBudget(false), house->GetBudget(false));
            }
        }
    }

    report += Trinity::StringFormat(
        "Totals: {} plots, {} owned, {} without authored spawn, {} houses over budget, {} placed decor\n",
        plots, owned, unauthored, overBudget, placements);

    if (unauthored)
        report += "Note: plots without a spawn point are unbuyable by design (Housing.Map.Mode is "
                  "probably still 'anchor').\n";

    return report;
}

} // namespace Housing
