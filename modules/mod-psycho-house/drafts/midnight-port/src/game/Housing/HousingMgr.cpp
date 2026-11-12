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

#include "HousingBlueprint.h"
#include "HousingEndeavor.h"

#include "Chat.h"
#include "Config.h"
#include "DB2Stores.h"
#include "DatabaseEnv.h"
#include "GameObject.h"
#include "Log.h"
#include "Map.h"
#include "MapManager.h"
#include "ObjectAccessor.h"
#include "ObjectMgr.h"
#include "SocialMgr.h"
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
#include <sstream>

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
    _nativeMap(true), _evictDays(0), _graceDays(7), _populationCap(0), _lifecycleTimer(0),
    _gryphonCost(5), _gryphonCooldown(300), _minResidents(0), _dissolveGraceDays(30),
    _arcaneLevel(10), _arcaneCooldownDays(7), _armillaryLevel(6), _guildWings(true),
    _cornerstoneEntry(0),
    _nextPlacementId(1), _nextHousingId(1), _nextNeighborhoodId(1),
    _saveTimer(0), _tick(0)
{
    _usedSlots.assign(kNeighborhoodSlotMax + 1, false);
}

// -----------------------------------------------------------------------------
// Config and loading
// -----------------------------------------------------------------------------

void HousingMgr::LoadConfig()
{
    _configValues.clear();

    // One line per setting, written as the loader reads it, so what `.housing config` prints is
    // what LoadConfig kept after clamping - not a second, hopeful reading of the same file.
    auto note = [this](char const* key, long double value)
    {
        std::ostringstream out;
        out << value;
        _configValues.emplace_back(key, out.str());
    };

    auto noteText = [this](char const* key, std::string const& value)
    {
        _configValues.emplace_back(key, value);
    };

    _enabled = sConfigMgr->GetBoolDefault("Housing.Enable", false);
    note("Housing.Enable", _enabled ? 1 : 0);
    _housingMapId = uint32(sConfigMgr->GetIntDefault("Housing.Map.Id", 0));
    note("Housing.Map.Id", _housingMapId);
    _interiorMapId = uint32(sConfigMgr->GetIntDefault("Housing.Interior.Map", 0));
    note("Housing.Interior.Map", _interiorMapId);
    _saveInterval = uint32(std::max(5, sConfigMgr->GetIntDefault("Housing.SaveInterval", kSaveIntervalSeconds)));
    note("Housing.SaveInterval", _saveInterval);
    _rateLimit = uint32(std::max(1, sConfigMgr->GetIntDefault("Housing.RateLimit.Messages", int32(kRateLimitMessages))));
    note("Housing.RateLimit.Messages", _rateLimit);
    _homeCooldown = uint32(std::max(0, sConfigMgr->GetIntDefault("Housing.Homestone.Cooldown", 900)));
    note("Housing.Homestone.Cooldown", _homeCooldown);
    _refundPercent = uint32(std::min(100, std::max(0, sConfigMgr->GetIntDefault("Housing.Sell.RefundPercent", 50))));
    note("Housing.Sell.RefundPercent", _refundPercent);
    _plotScopedPhasing = sConfigMgr->GetBoolDefault("Housing.Phasing.PlotScoped", true);
    note("Housing.Phasing.PlotScoped", _plotScopedPhasing ? 1 : 0);

    // Eviction is off unless the realm asks for it: a house that vanishes because its owner
    // took a break is the loudest possible bug report.
    _evictDays = uint32(std::max(0, sConfigMgr->GetIntDefault("Housing.Evict.Days", 0)));
    note("Housing.Evict.Days", _evictDays);
    _graceDays = uint32(std::max(0, sConfigMgr->GetIntDefault("Housing.Evict.GraceDays", 7)));
    note("Housing.Evict.GraceDays", _graceDays);
    _populationCap = uint32(std::max(0, sConfigMgr->GetIntDefault("Housing.Neighborhood.PopulationCap", 0)));
    note("Housing.Neighborhood.PopulationCap", _populationCap);

    // The Steward's gryphon. Cost is gold; a zero cost makes it free, a zero cooldown makes it
    // instant, and both are allowed because the two are separate decisions.
    _gryphonCost = uint32(std::max(0, sConfigMgr->GetIntDefault("Housing.Gryphon.Cost", 5)));
    note("Housing.Gryphon.Cost", _gryphonCost);
    _gryphonCooldown = uint32(std::max(0, sConfigMgr->GetIntDefault("Housing.Gryphon.Cooldown", 300)));
    note("Housing.Gryphon.Cooldown", _gryphonCooldown);

    // A dye change costs gold rather than an item. Retail's dyes are crafted and tradeable;
    // an 8.3.7 item id per colour would have to be invented, and the module does not invent
    // ids, so the sink is a price the realm can set. Zero is free, which is the default
    // because a realm that has not thought about the dye economy should not be given one.
    _dyeCost = uint32(std::max(0, sConfigMgr->GetIntDefault("Housing.Dye.Cost", 0)));
    note("Housing.Dye.Cost", _dyeCost);

    // Recovering an archived house costs a share of what the plot cost. The warning letter
    // and the conf file both said "for a fee" long before there was one; this is the fee.
    _restoreFeePercent = uint32(std::min(100, std::max(0,
        sConfigMgr->GetIntDefault("Housing.Restore.FeePercent", 10))));
    note("Housing.Restore.FeePercent", _restoreFeePercent);

    // Dissolving a private or guild neighbourhood nobody lives in any more. 0 residents means
    // the sweep never runs: an empty street might be one somebody is about to move into.
    _minResidents = uint32(std::max(0, sConfigMgr->GetIntDefault("Housing.Neighborhood.MinResidents", 0)));
    note("Housing.Neighborhood.MinResidents", _minResidents);
    _dissolveGraceDays = uint32(std::max(1, sConfigMgr->GetIntDefault("Housing.Neighborhood.DissolveGraceDays", 30)));
    note("Housing.Neighborhood.DissolveGraceDays", _dissolveGraceDays);

    // The two level devices.
    _arcaneLevel = uint32(std::max(1, sConfigMgr->GetIntDefault("Housing.Arcanostorage.Level", 10)));
    note("Housing.Arcanostorage.Level", _arcaneLevel);
    _arcaneCooldownDays = uint32(std::max(1, sConfigMgr->GetIntDefault("Housing.Arcanostorage.CooldownDays", 7)));
    note("Housing.Arcanostorage.CooldownDays", _arcaneCooldownDays);
    _armillaryLevel = uint32(std::max(1, sConfigMgr->GetIntDefault("Housing.Armillary.Level", 6)));
    note("Housing.Armillary.Level", _armillaryLevel);

    _guildWings = sConfigMgr->GetBoolDefault("Housing.Guild.Wings", true);

    // Retail's pet bed caps: 100 inside, 25 outside, per house. 0 turns the cap off, which is
    // what a realm with three bed models is likely to want; the numbers are retail's.
    _petBedIndoorCap = uint32(std::max(0, sConfigMgr->GetIntDefault("Housing.PetBed.IndoorCap", 100)));
    _petBedOutdoorCap = uint32(std::max(0, sConfigMgr->GetIntDefault("Housing.PetBed.OutdoorCap", 25)));
    note("Housing.PetBed.IndoorCap", _petBedIndoorCap);
    note("Housing.PetBed.OutdoorCap", _petBedOutdoorCap);
    note("Housing.Guild.Wings", _guildWings ? 1 : 0);

    // The stone on an empty plot. 0 keeps the plots bare, which is what a realm without a
    // suitable gameobject wants; any entry that exists in gameobject_template gets a stone
    // spawned at every free plot with ScriptName `go_housing_cornerstone` on it.
    _cornerstoneEntry = uint32(std::max(0, sConfigMgr->GetIntDefault("Housing.Plot.CornerstoneEntry", 0)));
    note("Housing.Plot.CornerstoneEntry", _cornerstoneEntry);

    // How long looking at a plot holds it for the person looking. Retail reserves a plot while
    // the purchase window is open; a minute is long enough to walk a yard and short enough that
    // an abandoned reservation does not lock a street up for the evening.
    _reservationSeconds = uint32(std::max(5, sConfigMgr->GetIntDefault("Housing.Plot.ReservationSeconds", 60)));
    note("Housing.Plot.ReservationSeconds", _reservationSeconds);

    // A smuggler sells the other faction's furniture at a mark-up. Retail's smugglers are the
    // only cross-faction shop in the neighbourhood, and the number was a quarter in the code
    // before it was a key.
    _smugglerMarkupPercent = uint32(std::min(500, std::max(0,
        sConfigMgr->GetIntDefault("Housing.Smuggler.MarkupPercent", 25))));
    note("Housing.Smuggler.MarkupPercent", _smugglerMarkupPercent);

    // Endeavor requirements scale with how many people live in the street, as
    // `base * clamp(residents / 10, floor, cap)`. The two numbers are what stops a street of
    // three being handed a raid's worth of kills and a street of forty being trivial.
    _endeavorScaleFloor = std::max(0.1f, sConfigMgr->GetFloatDefault("Housing.Endeavor.ScaleFloor", 0.5f));
    _endeavorScaleCap = std::max(_endeavorScaleFloor, sConfigMgr->GetFloatDefault("Housing.Endeavor.ScaleCap", 3.0f));
    note("Housing.Endeavor.ScaleFloor", _endeavorScaleFloor);
    note("Housing.Endeavor.ScaleCap", _endeavorScaleCap);

    // House XP. Retail pays 10 for learning a piece the house has never held, once per piece;
    // the Endeavor task number is this module's own, because the record does not publish one.
    _xpPerDecor = uint32(std::max(0, sConfigMgr->GetIntDefault("Housing.Xp.PerDecor", 10)));
    _xpPerEndeavorTask = uint32(std::max(0, sConfigMgr->GetIntDefault("Housing.Xp.PerEndeavorTask", 50)));
    note("Housing.Xp.PerDecor", _xpPerDecor);
    note("Housing.Xp.PerEndeavorTask", _xpPerEndeavorTask);

    // How many streets one account may found. Retail's charter neighbourhoods are one per
    // player; a realm that wants to run more sets a number, and 0 lifts the limit entirely.
    _maxFounded = uint32(std::max(0, sConfigMgr->GetIntDefault("Housing.Neighborhood.MaxFounded", 1)));
    note("Housing.Neighborhood.MaxFounded", _maxFounded);

    // doc 01 sec.6: a neighbourhood is founded by a minimum number of players, not characters.
    // Retail publishes the rule and not the number, so 3 is this module's own - the smallest
    // group that reads as more than one person deciding. 1 turns the minimum off.
    _minFounders = uint32(std::min(40, std::max(1,
        sConfigMgr->GetIntDefault("Housing.Neighborhood.MinFounders", 3))));
    note("Housing.Neighborhood.MinFounders", _minFounders);

    std::string const mode = sConfigMgr->GetStringDefault("Housing.Map.Mode", "native");
    _nativeMap = mode != "anchor";
    noteText("Housing.Map.Mode", mode);

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

    note("Housing.Interior.X", _interiorAnchor.m_positionX);
    note("Housing.Interior.Y", _interiorAnchor.m_positionY);
    note("Housing.Interior.Z", _interiorAnchor.m_positionZ);
    note("Housing.Interior.O", _interiorAnchor.m_positionO);
    note("Housing.TownCenter.X", _townCenter.m_positionX);
    note("Housing.TownCenter.Y", _townCenter.m_positionY);
    note("Housing.TownCenter.Z", _townCenter.m_positionZ);
    note("Housing.TownCenter.O", _townCenter.m_positionO);

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
    LoadHunts();
    LoadNamePool();

    // Level ladder, shells, rooms, dye variants, decor creatures and portals. Runs after the
    // plot templates because anchor mode fills in the spawn points the templates are missing.
    LoadProgression();
    sHousingEndeavor->Load();

    TC_LOG_INFO("housing", "Loaded %u biomes, %u plots, %u decor items, %u vendors, %u amenities, "
        "%u levels, %u shells, %u rooms, %u portal(s), %u hunt(s)",
        uint32(_biomes.size()), uint32(_plotTemplates.size()), uint32(_decor.size()),
        uint32(_vendorList.size()), uint32(_amenityList.size()), uint32(_levels.size()),
        uint32(_shells.size()), uint32(_rooms.size()), uint32(_portalList.size()),
        uint32(_huntList.size()));
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

        // X and Y are the authoring decision; Z does not have to be. A row with X and Y and a
        // NULL Z means "here, on the ground", and the ground is read from the map on entry.
        tmpl.HasSpawn = !fields[5].IsNull() && !fields[6].IsNull();
        if (tmpl.HasSpawn)
        {
            tmpl.TerrainZ = fields[7].IsNull();
            tmpl.Spawn = Position(fields[5].GetFloat(), fields[6].GetFloat(),
                tmpl.TerrainZ ? 0.0f : fields[7].GetFloat(),
                fields[8].IsNull() ? 0.0f : fields[8].GetFloat());
        }

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
        "`subcategory`, `source_type`, `source_id`, `price`, `currency`, `faction_mask`, `scale_min`, "
        "`scale_max`, `dye_group`, `flags`, `item_entry`, `night_entry` "
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
        info.Subcategory = fields[7].GetString();
        info.SourceType = fields[8].GetUInt8();
        info.SourceId = fields[9].GetUInt32();
        info.Price = fields[10].GetUInt32();
        info.Currency = fields[11].GetUInt32();
        info.FactionMask = fields[12].GetUInt8();
        info.ScaleMin = fields[13].GetFloat();
        info.ScaleMax = fields[14].GetFloat();
        info.DyeGroup = fields[15].GetUInt8();
        info.Flags = fields[16].GetUInt32();
        info.ItemEntry = fields[17].GetUInt32();
        info.NightEntry = fields[18].GetUInt32();

        // A catalogue row without a spawn is kept: the addon lists it, the collection
        // tracks it, and placing it fails cleanly in SpawnDecorObject. Dropping the row
        // would hide every ported piece whose 8.3.7 stand-in is not chosen yet.
        if (!info.GameObjectEntry || !sObjectMgr->GetGameObjectTemplate(info.GameObjectEntry))
            info.Flags |= DecorFlagUnmapped;

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
        "SELECT `vendor_id`, `decor_id`, `price`, `milestone` FROM `housing_vendor_stock`"))
    {
        do
        {
            Field* fields = result->Fetch();

            VendorStockEntry entry;
            entry.DecorId = fields[1].GetUInt32();
            entry.Price = fields[2].GetUInt32();
            entry.Milestone = fields[3].GetUInt8();
            _vendorStock[fields[0].GetUInt32()].push_back(entry);
        }
        while (result->NextRow());
    }
}

void HousingMgr::LoadAmenities()
{
    _amenities.clear();
    _amenityList.clear();

    // The two entry columns are what the spawner uses, so they have to be here. They were not:
    // the select named ten fields and those are eleven and twelve, which is why every amenity
    // stayed a pin on the map no matter what `bind_housing_content.sql` resolved. A row with
    // no object and no NPC is legal - it is a marker - but it is worth one log line so an
    // operator can see how many of them the realm has.
    uint32 pinsOnly = 0;

    if (QueryResult result = WorldDatabase.Query(
        "SELECT `id`, `biome_id`, `name`, `type`, `x`, `y`, `z`, `o`, `radius`, `flags`, "
        "`gameobject_entry`, `creature_entry` FROM `housing_amenity`"))
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
            info.GameObjectEntry = fields[10].GetUInt32();
            info.CreatureEntry = fields[11].GetUInt32();

            if (!info.GameObjectEntry && !info.CreatureEntry)
                ++pinsOnly;

            _amenities[info.Id] = info;
            _amenityList.push_back(info);
        }
        while (result->NextRow());
    }

    TC_LOG_INFO("housing", "Loaded %u amenities, %u with no object and no NPC (map pins only)",
        uint32(_amenityList.size()), pinsOnly);
}

void HousingMgr::LoadHunts()
{
    _hunts.clear();
    _huntList.clear();

    // The table ships with the module, but an install that updated the character schema and
    // not the world one still has to boot, so a missing table is a log line and no hunts
    // rather than a failed start.
    if (QueryResult result = WorldDatabase.Query(
        "SELECT `id`, `name`, `riddle`, `quest_id`, `reward_decor`, `cache_entry`, `x`, `y`, `z`, "
        "`faction_mask` FROM `housing_hunt`"))
    {
        do
        {
            Field* fields = result->Fetch();

            HuntInfo info;
            info.Id = fields[0].GetUInt32();
            info.Name = fields[1].GetString();
            info.Riddle = fields[2].GetString();
            info.QuestId = fields[3].GetUInt32();
            info.RewardDecor = fields[4].GetUInt32();
            info.CacheEntry = fields[5].GetUInt32();
            info.Pos = Position(fields[6].GetFloat(), fields[7].GetFloat(), fields[8].GetFloat());
            info.HasSpawn = info.Pos.m_positionX != 0.0f || info.Pos.m_positionY != 0.0f;
            info.FactionMask = fields[9].GetUInt8();

            _hunts[info.Id] = info;
            _huntList.push_back(info);
        }
        while (result->NextRow());
    }

    if (!_huntList.empty())
        TC_LOG_INFO("housing", "loaded {} riddle hunt(s)", uint32(_huntList.size()));
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
        "`blueprint`, `name`, `locked`, `shell_id`, `rooms_owned`, `armillary`, `plotMask`, "
        "`houseMask` FROM `character_housing`"))
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

            if (HousingHouse* house = plot->GetHouse())
            {
                house->SetShellId(fields[10].GetUInt8());
                house->SetRoomsOwned(fields[11].GetUInt8());
                house->SetArmillary(fields[12].GetUInt8());
                house->SetAccessMask(SpacePlot, fields[13].GetUInt8());
                house->SetAccessMask(SpaceHouse, fields[14].GetUInt8());
            }
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

    CharacterDatabase.DirectPExecute(
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

    // Every house in the street is parked in its owner's archive before the rows go: a street
    // that is dissolved because nobody has lived there for a month still had houses in it, and
    // the owners get the regret window the sale and eviction paths get.
    for (auto const& plot : neighborhood->GetPlots())
    {
        if (!plot->IsOwned())
            continue;

        ArchiveHouse(plot.get());
        ForceSellPlot(plot.get());
    }

    CharacterDatabase.DirectPExecute("DELETE FROM `character_housing_neighborhood` WHERE `id` = {}", neighborhood->GetId());
    CharacterDatabase.DirectPExecute("DELETE FROM `character_housing_neighbor_member` WHERE `neighborhoodId` = {}",
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

    uint8 const faction = player->GetTeamId() == TEAM_ALLIANCE ? FactionAlliance : FactionHorde;

    // A guild that already has a village is where its members belong, and a village that is
    // full opens a wing rather than sending the member to a public street. A guild with no
    // village is not given one here: that is the guild master's decision, made once with
    // `.housing create guild`, and a street that appears because somebody logged in would be
    // a street nobody asked for.
    if (uint64 const guildId = player->GetGuildId())
    {
        bool hasVillage = false;
        for (auto const& entry : _neighborhoods)
            if (entry.second->GetType() == NeighborhoodGuild && entry.second->GetGuildId() == guildId)
                hasVillage = true;

        if (hasVillage)
            if (HousingNeighborhood* wing = GetOrCreateGuildWing(guildId, faction))
                return wing;
    }

    // Fall back to the emptiest public neighbourhood of the player's faction, then create
    // a new one when every public neighbourhood is full.

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

    return CreateNeighborhood(NeighborhoodPublic, faction, GenerateNeighborhoodName(faction), 0, 0);
}

std::string HousingMgr::GenerateNeighborhoodName(uint8 faction)
{
    auto const itr = _namePool.find(faction);

    if (itr == _namePool.end() || itr->second.empty())
        return Trinity::StringFormat("Founder's Point {}", _neighborhoods.size() + 1);

    // The pool is read in order and the index is the count of neighbourhoods, so two
    // foundations in the same second still get different names.
    return Trinity::StringFormat("{} {}", itr->second[_neighborhoods.size() % itr->second.size()],
        _neighborhoods.size() + 1);
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
    LoadHouseSelection(player->GetSession()->GetAccountId());
    SyncAchievementDecor(player);

    // The introduction is a login breadcrumb in retail: the first quest is waiting before the
    // player has spoken to anyone, and the Steward is where it is turned in. Offering it here
    // is what makes that true - the gossip option stays, because a player who abandoned it
    // needs a second way back in.
    OfferIntroQuest(player, nullptr);

    CheckIntroQuestProgress(player);
}

/*
 * Decor earned by an achievement. This core has no achievement hook, so the check runs at
 * login instead: every piece whose source is an achievement the character already has is
 * granted then. It costs one pass over the catalogue rows that name an achievement, and it
 * is self-healing - a piece added to the catalogue after the achievement was earned arrives
 * on the next login rather than never.
 */
void HousingMgr::SyncAchievementDecor(Player* player)
{
    if (!player)
        return;

    uint32 const accountId = player->GetSession()->GetAccountId();
    uint8 const factionBit = player->GetTeamId() == TEAM_ALLIANCE ? 0x01 : 0x02;
    uint32 granted = 0;

    for (auto const& entry : _decor)
    {
        DecorInfo const& info = entry.second;

        if (info.SourceType != SourceAchievement || !info.SourceId)
            continue;

        if ((info.Flags & DecorFlagHidden) || !(info.FactionMask & factionBit))
            continue;

        if (!player->HasAchieved(info.SourceId) || HasUnlocked(accountId, info.Id))
            continue;

        CharacterDatabase.DirectPExecute(
            "REPLACE INTO `character_housing_collection` (`accountId`, `decorId`, `source_type`, "
            "`source_id`, `acquired`) VALUES ({}, {}, {}, {}, UNIX_TIMESTAMP())",
            accountId, info.Id, uint32(SourceAchievement), info.SourceId);

        _collection[accountId].insert(info.Id);
        AddStorage(accountId, info.Id, 1);
        ++granted;
    }

    if (granted)
        player->GetSession()->SendNotification(
            "|cffe6cc80Housing|r: %u piece(s) of decor unlocked by achievements.", granted);
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
        state->InsideHousingId = 0;
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

    if (!IsHousingMap(player->GetMapId()) && player->GetMapId() != _interiorMapId)
        return;

    // Sanctuary and no-fly are rules of the ground, so they are re-asserted rather than
    // applied once on entry: a player who re-flags inside still stops being a target.
    EnforceBiomeRules(player);

    ResolvePlot(player, *state);

    EnforceAccess(player, *state);
}

void HousingMgr::EnforceAccess(Player* player, PlayerState& state)
{
    if (!player || state.InsidePlot == 0xFF)
        return;

    HousingNeighborhood* neighborhood = GetNeighborhood(state.NeighborhoodId);
    if (!neighborhood)
        return;

    HousingPlot* plot = neighborhood->GetPlot(state.InsidePlot);
    if (!plot || !plot->IsOwned())
        return;

    // Access is re-read every half second rather than cached: a guest whose permission was
    // taken away while standing in the yard is moved out on the next tick, and so is anybody
    // standing in a plot that has just been sold underneath them. The yard test, because this
    // is the yard; the interior has its own mask and its own check when a player walks in.
    if (CheckSpace(player, plot, SpacePlot) >= AccessNeighbor)
        return;

    TC_LOG_INFO("housing", "Ejecting %s from neighbourhood %u plot %u: no access",
        player->GetName().c_str(), neighborhood->GetId(), uint32(plot->GetIndex()));

    if (_plotScopedPhasing)
    {
        PhasingHandler::RemovePhase(player, plot->GetPlotPhase(), true);
        PhasingHandler::RemovePhase(player, plot->GetInteriorPhase(), true);
    }

    ReleaseViewer(plot);

    state.InsidePlot = 0xFF;
    state.InsideInterior = false;
    state.InsideHousingId = 0;

    // Out of somebody's yard, onto the ground the plot belongs to: the biome's flight point
    // when the realm has authored one, the town centre otherwise. An ejection that drops the
    // player at the hub is a long walk back to the plot they were standing on.
    if (state.NeighborhoodId && _townCenter.m_positionX != 0.0f)
        player->TeleportTo(neighborhood->GetMapId(),
            ArrivalPosition(plot ? plot->GetTemplate().BiomeId : 0));
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

    // Somebody still inside the interior keeps the furniture up. Despawning under a player
    // who is standing in the room deletes the object they are looking at, and the interior
    // and the yard are two different maps so the viewer count says nothing about it.
    if (IsHouseOccupied(housingId))
        return;

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

        ApplyNeighborhoodPhase(player, neighborhood);

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

    ApplyNeighborhoodPhase(player, neighborhood);
    ApplyPlotPhase(player, plot);

    // The town centre, the amenities and the plot shells are spawned once per neighbourhood
    // in its own phase, so a visitor sees the same street a resident does.
    SpawnNeighborhoodContent(neighborhood, player->GetMap());

    if (!plot->IsOwned())
        return;

    // Walking onto somebody's plot is a visit, and twenty of them is a milestone.
    state.VisitedPlots.insert(neighborhood->GetId() * 256u + uint32(found));

    ++_houseViewers[plot->GetHousingId()];

    if (HousingHouse* house = plot->GetHouse())
        house->EnsureSpawned(player->GetMap(), plot->GetPlotPhase(), plot->GetOwnerGuid());
}

// -----------------------------------------------------------------------------
// Access
// -----------------------------------------------------------------------------

/*
 * Which audiences a visitor belongs to, as a bitmask. One relationship can be several: a
 * guildmate who lives next door and is in the party is three bits, and any one of them passing
 * the house's mask is enough.
 */
uint8 HousingMgr::AudienceBits(Player* player, HousingPlot* plot)
{
    uint8 bits = AudienceAnyone;

    if (!player || !plot)
        return bits;

    uint32 const accountId = player->GetSession()->GetAccountId();

    if (HousingNeighborhood* neighborhood = plot->GetNeighborhood())
        if (neighborhood->IsMember(accountId))
            bits |= AudienceNeighbors;

    if (GetGuestFlags(plot->GetHousingId(), accountId) & GuestFlagEnter)
        bits |= AudienceGuests;

    if (Player* owner = ObjectAccessor::FindPlayerByLowGUID(plot->GetOwnerGuid().GetCounter()))
    {
        if (owner->GetGuildId() && owner->GetGuildId() == player->GetGuildId())
            bits |= AudienceGuild;

        if (owner->GetGroup() && owner->GetGroup() == player->GetGroup())
            bits |= AudienceParty;

        // The owner is online, so their own friend list can be read as well as the visitor's.
        if (player->GetSocial() && player->GetSocial()->HasFriend(owner->GetGUID()))
            bits |= AudienceFriends;

        if (owner->GetSocial() && owner->GetSocial()->HasFriend(player->GetGUID()))
            bits |= AudienceFriends;
    }
    else if (player->GetSocial() && plot->GetOwnerGuid() && player->GetSocial()->HasFriend(plot->GetOwnerGuid()))
    {
        // The owner is offline and their list cannot be read, so the visitor's list stands in.
        // That is more permissive than retail in one direction; the alternative is no friend
        // audience at all while the owner is away, which is when visitors actually arrive.
        bits |= AudienceFriends;
    }

    return bits;
}

HousingAccess HousingMgr::CheckSpace(Player* player, HousingPlot* plot, HousingSpace space)
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

    uint8 const audience = AudienceBits(player, plot);
    bool const guest = (audience & AudienceGuests) != 0;

    // A guest is allowed wherever the guest list says so, whatever the mask holds: the list is
    // the exception mechanism, not another audience competing with the mask.
    if (guest)
        return AccessGuest;

    HousingHouse const* house = plot->GetHouse();

    // A locked house is a blanket denial: the owner, a guest and a GM get in, nobody else,
    // whatever the mask says. That is what the lock is for.
    if (space == SpaceHouse && house && house->IsLocked())
        return AccessStranger;

    uint8 const mask = house ? house->GetAccessMask(space) : kAudienceAll;

    if (audience & mask)
        return AccessNeighbor;

    return AccessStranger;
}

HousingAccess HousingMgr::CheckAccess(Player* player, HousingPlot* plot)
{
    return CheckSpace(player, plot, SpacePlot);
}

HousingPlot* HousingMgr::GetOwnedPlot(Player* player)
{
    if (!player || !player->GetSession())
        return nullptr;

    uint32 const accountId = player->GetSession()->GetAccountId();
    uint8 const faction = player->GetTeamId() == TEAM_ALLIANCE ? FactionAlliance : FactionHorde;

    // An account can hold one house per faction, so "my house" means the house on my side
    // first. A character of the other faction still gets the other house when that is the
    // only one the account owns - retail lets any warband character live in either.
    for (auto const& entry : _neighborhoods)
        if (entry.second->GetFaction() == faction)
            if (HousingPlot* plot = entry.second->FindPlotOfAccount(accountId))
                return plot;

    for (auto const& entry : _neighborhoods)
        if (HousingPlot* plot = entry.second->FindPlotOfAccount(accountId))
            return plot;

    return nullptr;
}

std::vector<HousingPlot*> HousingMgr::GetOwnedPlots(uint32 accountId)
{
    std::vector<HousingPlot*> owned;

    if (!accountId)
        return owned;

    for (auto const& entry : _neighborhoods)
        for (auto const& plot : entry.second->GetPlots())
            if (plot->IsOwned() && plot->GetOwnerAccount() == accountId)
                owned.push_back(plot.get());

    std::sort(owned.begin(), owned.end(), [](HousingPlot* left, HousingPlot* right)
    {
        return left->GetHousingId() < right->GetHousingId();
    });

    return owned;
}

void HousingMgr::LoadHouseSelection(uint32 accountId)
{
    if (!accountId)
        return;

    // 0 records "asked, nothing chosen", so a player with one house does not re-query the row
    // every time the Homestone is pressed.
    _selectedHouse[accountId] = 0;

    if (QueryResult result = CharacterDatabase.Query(
        "SELECT `id` FROM `character_housing` WHERE `ownerAccount` = {} AND `selected` = 1 "
        "ORDER BY `id` LIMIT 1", accountId))
    {
        _selectedHouse[accountId] = result->Fetch()[0].GetUInt32();
    }
}

HousingPlot* HousingMgr::GetHomePlot(uint32 accountId)
{
    if (!accountId)
        return nullptr;

    auto const selected = _selectedHouse.find(accountId);
    if (selected == _selectedHouse.end())
        LoadHouseSelection(accountId);

    auto const chosen = _selectedHouse.find(accountId);
    if (chosen != _selectedHouse.end() && chosen->second)
        for (auto const& entry : _neighborhoods)
            if (HousingPlot* plot = entry.second->FindPlotOfHousingId(chosen->second))
                if (plot->GetOwnerAccount() == accountId)
                    return plot;

    // No choice yet, or the chosen house has been sold since: the first house the account
    // owns answers instead, which is what the addon shows as selected.
    std::vector<HousingPlot*> owned = GetOwnedPlots(accountId);
    return owned.empty() ? nullptr : owned.front();
}

HousingResult HousingMgr::SelectHouse(Player* player, uint32 housingId)
{
    if (!_enabled)
        return Disabled;

    if (!player || !player->GetSession())
        return InternalError;

    uint32 const accountId = player->GetSession()->GetAccountId();
    HousingPlot* target = nullptr;

    for (auto const& entry : _neighborhoods)
        if (HousingPlot* plot = entry.second->FindPlotOfHousingId(housingId))
        {
            target = plot;
            break;
        }

    if (!target || target->GetOwnerAccount() != accountId)
        return NotOwner;

    // One selection per account: clearing the rest in the same statement means a stale flag
    // cannot survive a house being sold and restored.
    CharacterDatabase.DirectPExecute(
        "UPDATE `character_housing` SET `selected` = (`id` = {}) WHERE `ownerAccount` = {}",
        housingId, accountId);

    _selectedHouse[accountId] = housingId;

    return Ok;
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

    // The population cap counts owned plots, so it is checked here as well as on join: a
    // neighbourhood that filled up after the player joined still has no room for them.
    if (_populationCap && GetNeighborhoodPopulation(neighborhood) >= _populationCap)
        return NeighborhoodFull;

    uint32 const accountId = player->GetSession()->GetAccountId();

    if (neighborhood->GetType() != NeighborhoodPublic && !neighborhood->IsMember(accountId))
        return NotAllowedHere;

    uint64 const cost = GoldToCopper(plot->GetPrice());
    if (!player->HasEnoughMoney(int64(cost)))
        return NotEnoughMoney;

    uint32 const housingId = GenerateHousingId();
    std::string ownerName = player->GetName();
    CharacterDatabase.EscapeString(ownerName);

    // A new house arrives in the small shell of the character's racial kit. Retail's first
    // exterior is free and automatic; without this the plot is grass until somebody runs
    // .housing shell, which is not a thing a player should have to discover.
    ShellInfo const* starter = GetStarterShell(player);
    uint32 const starterShell = starter ? starter->Id : 0;

    CharacterDatabase.DirectPExecute(
        "INSERT INTO `character_housing` (`id`, `neighborhoodId`, `plotIndex`, `ownerAccount`, `ownerGuid`, "
        "`ownerName`, `tier`, `blueprint`, `name`, `locked`, `shell_id`, `budget_indoor`, `budget_outdoor`, "
        "`purchase_time`, `last_edited`) VALUES ({}, {}, {}, {}, {}, '{}', 1, 0, '', 0, {}, 0, 0, {}, {})",
        housingId, neighborhood->GetId(), uint32(plotIndex), accountId, player->GetGUID().GetCounter(),
        ownerName, starterShell, Now(), Now());

    player->ModifyMoney(-int64(cost));

    plot->SetOwner(housingId, accountId, player->GetGUID(), player->GetName(), 1, 0, "", false);

    if (plot->GetHouse() && starterShell)
        plot->GetHouse()->SetShellId(uint8(starterShell));

    plot->GetHouse()->MarkDirty();
    plot->GetHouse()->Flush();

    // The building goes up now rather than at the next restart: the buyer is standing on the
    // plot, and a plot that stays a lawn until a server restart reads as broken.
    if (starterShell)
    {
        if (HousingNeighborhood* owned = plot->GetNeighborhood())
            if (Map* map = sMapMgr->FindMap(owned->GetMapId(), 0))
                SpawnPlotShell(plot, owned, map);
    }

    if (PlayerState* state = GetPlayerState(player))
    {
        state->HasOwnedPlot = true;
        state->OwnedHousingId = housingId;
    }

    TC_LOG_INFO("housing", "Account %u bought plot %u in neighbourhood %u (house " UI64FMTD ") for %u gold",
        accountId, uint32(plotIndex), neighborhood->GetId(), uint64(housingId), plot->GetPrice());

    CheckIntroQuestProgress(player);
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

    // The floorplan goes with the building: rooms are the house, not furniture in it.
    ClearPlacedRooms(housingId);

    CharacterDatabase.DirectPExecute("DELETE FROM `character_housing` WHERE `id` = {}", housingId);
    CharacterDatabase.DirectPExecute("DELETE FROM `character_housing_guest` WHERE `housingId` = {}", housingId);

    _houseViewers.erase(housingId);
    _guestCache.erase(housingId);

    plot->ClearOwner();
}

void HousingMgr::ArchiveHouse(HousingPlot* plot, uint32 accountId)
{
    if (!plot || !plot->IsOwned())
        return;

    if (!accountId)
        accountId = plot->GetOwnerAccount();

    if (!accountId)
        return;

    HousingHouse* house = plot->GetHouse();
    uint32 const housingId = plot->GetHousingId();

    // The placements travel as a build code, which is the same format a blueprint uses, so
    // there is one importer for both. The shell, the rooms and the Armillary preset travel in
    // their own columns: a house that comes back as an empty box is not the house that was
    // taken away.
    std::string packed;
    if (house)
        packed = BlueprintCodec::Encode(house->GetPlacements(), BlueprintBuild,
            plot->GetNeighborhood() && plot->GetNeighborhood()->GetFaction() == FactionHorde
                ? FactionHorde : FactionAlliance);

    std::string safeName = house ? house->GetName() : std::string();
    CharacterDatabase.EscapeString(safeName);

    CharacterDatabase.DirectPExecute(
        "REPLACE INTO `character_housing_archive` (`id`, `accountId`, `plotIndex`, `neighborhoodId`, "
        "`name`, `tier`, `xp`, `shell_id`, `rooms_owned`, `armillary`, `decor`, `price_paid`, `archived`) "
        "VALUES ({}, {}, {}, {}, '{}', {}, {}, {}, {}, {}, '{}', {}, UNIX_TIMESTAMP())",
        housingId, accountId, uint32(plot->GetIndex()),
        plot->GetNeighborhood() ? plot->GetNeighborhood()->GetId() : 0u, safeName,
        uint32(house ? house->GetTier() : 1), GetXpForHousing(housingId),
        uint32(house ? house->GetShellId() : 0), uint32(house ? house->GetRoomsOwned() : 0),
        uint32(house ? house->GetArmillary() : 0), packed, plot->GetPrice());
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
    uint32 const accountId = player->GetSession()->GetAccountId();

    // Park the house before it is destroyed, which is what makes "I sold my house and regret
    // it" recoverable for a week.
    ArchiveHouse(plot, accountId);

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

HousingResult HousingMgr::SetAccess(Player* player, uint8 space, uint8 mask)
{
    if (!_enabled)
        return Disabled;

    if (space > SpaceHouse)
        return NotAllowedHere;

    HousingPlot* plot = GetOwnedPlot(player);
    if (!plot || !plot->GetHouse())
        return NoHouse;

    if (!CanEdit(player, plot))
        return NotOwner;

    // Anyone and Guests are not removable: a house with neither would be a room its own owner
    // cannot describe, and a guest list that does not grant entry is not a guest list.
    mask |= AudienceGuests;

    plot->GetHouse()->SetAccessMask(HousingSpace(space), mask);

    // Applies immediately rather than at the next tick: somebody standing in the yard when the
    // mask closes is moved out on the same half second, which is what retail does.
    if (PlayerState* state = GetPlayerState(player))
        EnforceAccess(player, *state);

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

    if (!player->TeleportTo(neighborhood->GetMapId(), PlotSpawn(plot, player)))
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

    HousingPlot* plot = GetHomePlot(player->GetSession()->GetAccountId());
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

    if (!player->TeleportTo(neighborhood->GetMapId(), PlotSpawn(plot, player)))
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

    // The interior's own mask, not the yard's: a house that is open to neighbours but closed
    // to strangers has to be able to say so.
    if (CheckSpace(player, plot, SpaceHouse) < AccessNeighbor)
        return GuestRestricted;

    if (enter)
    {
        // Phase before teleport: the interior furniture must already be in the right phase
        // when the player lands, otherwise they get one frame of an empty room.
        PhasingHandler::AddPhase(player, plot->GetInteriorPhase(), true);

        if (_plotScopedPhasing)
            PhasingHandler::RemovePhase(player, plot->GetPlotPhase(), true);

        state->InsideInterior = true;
        state->InsideHousingId = plot->GetHousingId();

        if (!player->TeleportTo(_interiorMapId, _interiorAnchor))
        {
            state->InsideInterior = false;
            state->InsideHousingId = 0;
            return InternalError;
        }

        return Ok;
    }

    if (!plot->GetTemplate().HasSpawn)
        return NotAllowedHere;

    PhasingHandler::RemovePhase(player, plot->GetInteriorPhase(), true);
    ApplyPlotPhase(player, plot);

    state->InsideInterior = false;
    state->InsideHousingId = 0;

    if (!player->TeleportTo(neighborhood->GetMapId(), PlotSpawn(plot, player)))
        return InternalError;

    return Ok;
}

// -----------------------------------------------------------------------------
// Decor operations
// -----------------------------------------------------------------------------

HousingResult HousingMgr::PlaceDecor(Player* player, uint32 decorId, float x, float y, float z, float o,
    float scale, bool indoor, uint64& placementId, float pitch, float roll)
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

    // Owner, or a guest the owner gave the edit bit to. Unlocking a piece is not enough:
    // retail requires ownership of the copies that are put down.
    if (!CanEdit(player, plot))
        return NotOwner;

    if (!GetStorageCount(accountId, decorId))
        return StorageEmpty;

    // A plot can refuse outdoor furniture outright - the Outer Banks island is a view, not a
    // garden. The flag has lived in the schema since the first draft; this is what reads it.
    if (!indoor && (plot->GetTemplate().Flags & PlotFlagNoOutdoorDecor))
        return NotAllowedHere;

    PlayerState* state = GetPlayerState(player);
    if (state && !indoor && state->InsidePlot != plot->GetIndex())
        return NotAllowedHere;   // outdoor placement requires standing in the plot

    if (indoor && player->GetMapId() != _interiorMapId)
        return NotAllowedHere;

    HousingHouse* house = plot->GetHouse();
    if (!house)
        return NoHouse;

    // Retail's bed caps, counted per house and per side of the door. A cap of 0 is off, so a
    // realm that only owns three bed models is not held to a hundred.
    if (info->Flags & DecorFlagPetBed)
    {
        uint32 const cap = indoor ? _petBedIndoorCap : _petBedOutdoorCap;

        if (cap)
        {
            uint32 beds = 0;
            for (DecorPlacement const& placement : house->GetPlacements())
            {
                if (placement.Indoor != indoor)
                    continue;

                DecorInfo const* bedInfo = GetDecor(placement.DecorId);
                if (bedInfo && (bedInfo->Flags & DecorFlagPetBed))
                    ++beds;
            }

            if (beds >= cap)
                return NotAllowedHere;
        }
    }

    float const yardRadius = plot->GetTemplate().YardRadius > 0.0f
        ? plot->GetTemplate().YardRadius
        : float(kDefaultYardRadius);

    placementId = GeneratePlacementId();

    // Tilt is clamped rather than refused: a slider that overshoots should stop at the limit,
    // not bounce, and the client's own limit is advisory.
    pitch = std::max(-kMaxTilt, std::min(kMaxTilt, pitch));
    roll = std::max(-kMaxTilt, std::min(kMaxTilt, roll));

    HousingResult const result = house->AddPlacement(placementId, *info, x, y, z, o, scale, indoor,
        plot->GetTemplate().Spawn, yardRadius, pitch, roll);

    if (result != Ok)
        return result;

    // The copy leaves storage only after the placement is accepted.
    ConsumeStorage(accountId, decorId);

    if (DecorPlacement* placement = house->GetPlacementMutable(placementId))
    {
        uint32 const phase = indoor ? plot->GetInteriorPhase() : plot->GetPlotPhase();
        house->SpawnPlacement(*placement, player->GetMap(), phase, plot->GetOwnerGuid());
    }

    CheckIntroQuestProgress(player);
    return Ok;
}

HousingResult HousingMgr::MoveDecor(Player* player, uint64 placementId, float x, float y, float z,
    float o, float scale, float pitch, float roll)
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

    pitch = std::max(-kMaxTilt, std::min(kMaxTilt, pitch));
    roll = std::max(-kMaxTilt, std::min(kMaxTilt, roll));

    HousingResult const result = house->MovePlacement(placementId, x, y, z, o, scale, *info,
        plot->GetTemplate().Spawn, yardRadius, pitch, roll);

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

    if (!CanEdit(player, plot))
        return NotOwner;

    HousingHouse* house = plot->GetHouse();
    if (!house)
        return NoHouse;

    uint32 removedDecorId = 0;
    if (DecorPlacement* placement = house->GetPlacementMutable(placementId))
    {
        removedDecorId = placement->DecorId;
        DespawnDecorObject(*placement, house->IsSpawned() ? house->GetSpawnMap() : player->GetMap());
    }

    HousingResult const result = house->RemovePlacement(placementId);
    if (result == Ok && removedDecorId)
        AddStorage(player->GetSession()->GetAccountId(), removedDecorId, 1);

    return result;
}

uint32 HousingMgr::GetPurchasePrice(Player* player, DecorInfo const& info,
    uint32 vendorId) const
{
    uint32 price = info.Price;

    if (info.SourceType == SourceVendor)
    {
        // At a stall the price is that stall's row. With no stall - the catalogue, a chat
        // command, an addon quote - it is the source vendor's row, which is where the piece
        // is listed. One row per price means the two only differ when two hubs disagree
        // about what a piece costs, and then the stall the player is standing at wins.
        auto const stockItr = _vendorStock.find(vendorId ? vendorId : info.SourceId);
        if (stockItr != _vendorStock.end())
            for (VendorStockEntry const& entry : stockItr->second)
                if (entry.DecorId == info.Id && entry.Price)
                    price = entry.Price;
    }

    // The smuggler's mark-up, applied to the same number the shop would quote.
    if (player && _smugglerMarkupPercent
        && !(info.FactionMask & (player->GetTeamId() == TEAM_ALLIANCE ? 0x01 : 0x02)))
        price += uint32(uint64(price) * _smugglerMarkupPercent / 100);

    return price;
}

bool HousingMgr::IsStockAvailable(Player* player, DecorInfo const& info) const
{
    if (info.SourceType != SourceVendor)
        return true;

    // Piece-level, for the catalogue and the chat command: a piece is reachable if any stall
    // sells it ungated, so the gate only closes when every listing that carries it is gated.
    // Reading the source vendor's rows alone made every gate inert the moment a second vendor
    // stocked the same piece - the ten gated lines draft A ships were all like that.
    bool listed = false;

    for (auto const& stock : _vendorStock)
    {
        for (VendorStockEntry const& entry : stock.second)
        {
            if (entry.DecorId != info.Id)
                continue;

            listed = true;

            if (!entry.Milestone || sHousingEndeavor->GetMilestoneFor(player) >= entry.Milestone)
                return true;
        }
    }

    return !listed;
}

bool HousingMgr::IsStockAvailableAt(Player* player, uint32 vendorId, uint32 decorId) const
{
    auto const stockItr = _vendorStock.find(vendorId);
    if (stockItr == _vendorStock.end())
        return true;

    for (VendorStockEntry const& entry : stockItr->second)
    {
        if (entry.DecorId != decorId)
            continue;

        return !entry.Milestone || sHousingEndeavor->GetMilestoneFor(player) >= entry.Milestone;
    }

    return true;
}

HousingResult HousingMgr::BuyDecor(Player* player, uint32 decorId, uint32 vendorId)
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
    {
        // A smuggler sells the other side's furniture, at a mark-up and one copy at a time.
        // The piece stays masked so the ordinary shops still refuse it; what changes is that
        // the smuggler's own window lists it and this call accepts it.
        bool smuggled = false;

        if (info->SourceType == SourceVendor)
            if (VendorInfo const* vendor = GetVendor(info->SourceId))
                smuggled = vendor->Role == RoleSmuggler;

        if (!smuggled)
            return WrongFaction;
    }

    // Milestone-gated stock: the row carries the milestone, the neighbourhood's progress
    // decides whether it is on the shelf. Hiding it in the window is not enough - the buy
    // path checks again, and when the player is at a stall it checks that stall's row rather
    // than the piece's cheapest listing.
    if (vendorId ? !IsStockAvailableAt(player, vendorId, decorId) : !IsStockAvailable(player, *info))
        return NotAllowedHere;

    // The stock row may override the price the decor row carries, and a smuggler adds a
    // quarter on top of whatever the piece would have cost at home. Both are decided in
    // GetPurchasePrice so that the gossip menu, which quotes it, cannot drift from this.
    uint32 const price = GetPurchasePrice(player, *info, vendorId);

    if (info->Currency != kCurrencyGold)
    {
        if (IsItemCost(info->Currency))
        {
            if (!player->HasItemCount(info->Currency, price))
                return NotEnoughMoney;

            player->DestroyItemCount(info->Currency, price, true);
        }
        else
        {
            if (player->GetCurrency(info->Currency) < price)
                return NotEnoughMoney;

            player->ModifyCurrency(info->Currency, -int32(price));
        }
    }
    else
    {
        uint64 const cost = GoldToCopper(price);
        if (!player->HasEnoughMoney(int64(cost)))
            return NotEnoughMoney;

        player->ModifyMoney(-int64(cost));
    }

    CharacterDatabase.DirectPExecute(
        "REPLACE INTO `character_housing_collection` (`accountId`, `decorId`, `source_type`, `source_id`, `acquired`) "
        "VALUES ({}, {}, {}, {}, UNIX_TIMESTAMP())",
        accountId, decorId, uint32(info->SourceType), info->SourceId);

    _collection[accountId].insert(decorId);

    // One copy in storage, and the first time a piece is learned pays house XP - which is
    // the whole reason a decor collector levels a house.
    AddStorage(accountId, decorId, 1);
    GrantHouseXp(player, _xpPerDecor, true);

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

void HousingMgr::AddGuest(uint32 housingId, uint32 accountId, uint8 flags)
{
    if (!housingId || !accountId)
        return;

    CharacterDatabase.DirectPExecute(
        "REPLACE INTO `character_housing_guest` (`housingId`, `accountId`, `flags`, `added`) "
        "VALUES ({}, {}, {}, UNIX_TIMESTAMP())", housingId, accountId, uint32(flags));

    _guestCache[housingId][accountId] = flags;
}

void HousingMgr::RemoveGuest(uint32 housingId, uint32 accountId)
{
    CharacterDatabase.DirectPExecute(
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

    // Hourly. Eviction reads the database, so it must not run on the save cadence.
    _lifecycleTimer += diff;
    if (_lifecycleTimer >= 3600 * 1000)
    {
        _lifecycleTimer = 0;
        RunLifecycle();
    }

    sHousingEndeavor->Update(diff);

    // Reservations expire on their own; nothing else reads the map, so a sweep here is
    // cheaper than a timer per plot.
    uint32 const now = getMSTime();
    for (auto itr = _reservations.begin(); itr != _reservations.end();)
    {
        if (itr->second.second <= now)
            itr = _reservations.erase(itr);
        else
            ++itr;
    }

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
