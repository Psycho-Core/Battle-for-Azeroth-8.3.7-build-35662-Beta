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

#include "HousingHouse.h"
#include "DatabaseEnv.h"
#include "HousingMgr.h"
#include "Log.h"
#include "Map.h"

#include <cmath>
#include <ctime>

namespace Housing
{

HousingHouse::HousingHouse(uint32 id, uint8 plotIndex)
    : _id(id), _plotIndex(plotIndex), _tier(1), _blueprint(0), _locked(false),
    _decorLoaded(false), _spawned(false), _spawnMap(nullptr), _dirty(false), _usedIndoor(0), _usedOutdoor(0)
{
}

HousingHouse::~HousingHouse()
{
}

void HousingHouse::SetTier(uint8 tier)
{
    if (tier < 1)
        tier = 1;
    if (tier > kMaxTier)
        tier = kMaxTier;

    if (_tier == tier)
        return;

    _tier = tier;
    MarkDirty();
}

void HousingHouse::SetBlueprint(uint32 blueprint)
{
    if (_blueprint == blueprint)
        return;

    _blueprint = blueprint;
    MarkDirty();
}

void HousingHouse::SetName(std::string const& name)
{
    _name = name;
    MarkDirty();
}

void HousingHouse::SetLocked(bool locked)
{
    _locked = locked;
    MarkDirty();
}

uint32 HousingHouse::GetUsedBudget(bool indoor) const
{
    uint32 used = 0;
    for (DecorPlacement const& placement : _placements)
    {
        if (placement.Indoor != indoor)
            continue;

        if (DecorInfo const* info = sHousingMgr->GetDecor(placement.DecorId))
            used += info->BudgetCost;
    }

    return used;
}

bool HousingHouse::IsOverBudget() const
{
    return GetUsedBudget(true) > GetBudget(true) || GetUsedBudget(false) > GetBudget(false);
}

void HousingHouse::RecalculateBudget()
{
    _usedIndoor = GetUsedBudget(true);
    _usedOutdoor = GetUsedBudget(false);
}

bool HousingHouse::LoadDecor()
{
    _placements.clear();

    QueryResult result = CharacterDatabase.PQuery(
        "SELECT `id`, `decorId`, `x`, `y`, `z`, `o`, `scale`, `surface`, `indoor`, `placed` "
        "FROM `character_housing_decor` WHERE `housingId` = {}", _id);

    if (!result)
    {
        _decorLoaded = true;
        RecalculateBudget();
        return false;
    }

    do
    {
        Field* fields = result->Fetch();

        DecorPlacement placement;
        placement.Id = fields[0].GetUInt64();
        placement.DecorId = fields[1].GetUInt32();
        placement.X = fields[2].GetFloat();
        placement.Y = fields[3].GetFloat();
        placement.Z = fields[4].GetFloat();
        placement.O = fields[5].GetFloat();
        placement.Scale = fields[6].GetFloat();
        placement.Surface = fields[7].GetUInt8();
        placement.Indoor = fields[8].GetUInt8() != 0;
        placement.Placed = fields[9].GetUInt32();

        if (!sHousingMgr->GetDecor(placement.DecorId))
        {
            TC_LOG_ERROR("housing", "House " UI64FMTD " references unknown decor %u (placement " UI64FMTD "), skipping",
                uint64(_id), placement.DecorId, placement.Id);
            continue;
        }

        _placements.push_back(placement);
    }
    while (result->NextRow());

    _decorLoaded = true;
    RecalculateBudget();
    return true;
}

DecorPlacement const* HousingHouse::GetPlacement(uint64 id) const
{
    for (DecorPlacement const& placement : _placements)
        if (placement.Id == id)
            return &placement;

    return nullptr;
}

DecorPlacement* HousingHouse::GetPlacementMutable(uint64 id)
{
    for (DecorPlacement& placement : _placements)
        if (placement.Id == id)
            return &placement;

    return nullptr;
}

HousingResult HousingHouse::Validate(DecorInfo const& info, float x, float y, float z, float scale,
    bool indoor, Position const& plotSpawn, float yardRadius, uint64 ignorePlacementId) const
{
    if (!std::isfinite(x) || !std::isfinite(y) || !std::isfinite(z) || !std::isfinite(scale))
        return OutOfBounds;

    if (scale < info.ScaleMin || scale > info.ScaleMax)
        return BadScale;

    switch (info.Placement)
    {
        case PlacementIndoor:
            if (!indoor)
                return WrongSurface;
            break;
        case PlacementOutdoor:
            if (indoor)
                return WrongSurface;
            break;
        default:
            break;
    }

    if (!indoor)
    {
        float const dx = x - plotSpawn.m_positionX;
        float const dy = y - plotSpawn.m_positionY;
        float const limit = yardRadius > 0.0f ? yardRadius : kDefaultYardRadius;

        if ((dx * dx + dy * dy) > (limit * limit))
            return OutOfBounds;
    }
    else
    {
        // Interior bounds are authored per shell; until the shells exist I only reject
        // obviously nonsense heights instead of pretending to know the room size.
        if (z < -500.0f || z > 500.0f)
            return OutOfBounds;
    }

    if (_placements.size() >= kMaxPlacedDecor)
        return TooManyDecor;

    uint32 const budget = GetBudget(indoor);
    uint32 used = 0;
    for (DecorPlacement const& placement : _placements)
    {
        if (placement.Indoor != indoor || placement.Id == ignorePlacementId)
            continue;

        if (DecorInfo const* placed = sHousingMgr->GetDecor(placement.DecorId))
            used += placed->BudgetCost;
    }

    if ((used + info.BudgetCost) > budget)
        return BudgetExceeded;

    return Ok;
}

HousingResult HousingHouse::AddPlacement(uint64 placementId, DecorInfo const& info, float x, float y, float z,
    float o, float scale, bool indoor, Position const& plotSpawn, float yardRadius)
{
    HousingResult const result = Validate(info, x, y, z, scale, indoor, plotSpawn, yardRadius);
    if (result != Ok)
        return result;

    CharacterDatabase.PExecute(
        "INSERT INTO `character_housing_decor` "
        "(`id`, `housingId`, `decorId`, `x`, `y`, `z`, `o`, `scale`, `surface`, `indoor`, `placed`) "
        "VALUES ({}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {})",
        placementId, _id, info.Id, x, y, z, o, scale, uint32(info.Surface), uint32(indoor ? 1 : 0),
        uint32(std::time(nullptr)));

    DecorPlacement placement;
    placement.Id = placementId;
    placement.DecorId = info.Id;
    placement.X = x;
    placement.Y = y;
    placement.Z = z;
    placement.O = o;
    placement.Scale = scale;
    placement.Surface = info.Surface;
    placement.Indoor = indoor;
    placement.Placed = uint32(std::time(nullptr));

    _placements.push_back(placement);
    RecalculateBudget();
    MarkDirty();

    return Ok;
}

HousingResult HousingHouse::MovePlacement(uint64 id, float x, float y, float z, float o, float scale,
    DecorInfo const& info, Position const& plotSpawn, float yardRadius)
{
    for (DecorPlacement& placement : _placements)
    {
        if (placement.Id != id)
            continue;

        HousingResult const result = Validate(info, x, y, z, scale, placement.Indoor, plotSpawn,
            yardRadius, id);
        if (result != Ok)
            return result;

        placement.X = x;
        placement.Y = y;
        placement.Z = z;
        placement.O = o;
        placement.Scale = scale;

        CharacterDatabase.PExecute(
            "UPDATE `character_housing_decor` SET `x` = {}, `y` = {}, `z` = {}, `o` = {}, `scale` = {} "
            "WHERE `id` = {} AND `housingId` = {}",
            x, y, z, o, scale, id, _id);

        MarkDirty();
        return Ok;
    }

    return NotFound;
}

HousingResult HousingHouse::RemovePlacement(uint64 id)
{
    for (auto itr = _placements.begin(); itr != _placements.end(); ++itr)
    {
        if (itr->Id != id)
            continue;

        CharacterDatabase.PExecute("DELETE FROM `character_housing_decor` WHERE `id` = {} AND `housingId` = {}",
            id, _id);

        _placements.erase(itr);
        RecalculateBudget();
        MarkDirty();
        return Ok;
    }

    return NotFound;
}

void HousingHouse::SpawnPlacement(DecorPlacement& placement, Map* map, uint32 phaseId, ObjectGuid ownerGuid)
{
    DecorInfo const* info = sHousingMgr->GetDecor(placement.DecorId);
    if (!info)
        return;

    if (!placement.SpawnGuid.IsEmpty())
        return;

    if (GameObject* go = SpawnDecorObject(*info, placement, map, phaseId, ownerGuid))
    {
        TC_LOG_DEBUG("housing", "Spawned decor %u (placement " UI64FMTD ") for house " UI64FMTD,
            info->Id, placement.Id, uint64(_id));
        (void)go;
    }
}

void HousingHouse::EnsureSpawned(Map* map, uint32 phaseId, ObjectGuid ownerGuid)
{
    if (_spawned || !map)
        return;

    if (!_decorLoaded)
        LoadDecor();

    _spawnMap = map;

    for (DecorPlacement& placement : _placements)
        SpawnPlacement(placement, map, phaseId, ownerGuid);

    _spawned = true;
}

void HousingHouse::DespawnAll()
{
    if (!_spawned)
        return;

    for (DecorPlacement& placement : _placements)
        if (!placement.SpawnGuid.IsEmpty())
            DespawnDecorObject(placement, _spawnMap);

    _spawnMap = nullptr;
    _spawned = false;
}

void HousingHouse::Flush()
{
    if (!_dirty)
        return;

    RecalculateBudget();

    CharacterDatabase.EscapeString(_name);

    CharacterDatabase.PExecute(
        "UPDATE `character_housing` SET `tier` = {}, `blueprint` = {}, `name` = '{}', `locked` = {}, "
        "`budget_indoor` = {}, `budget_outdoor` = {}, `last_edited` = {} WHERE `id` = {}",
        uint32(_tier), _blueprint, _name, uint32(_locked ? 1 : 0), _usedIndoor, _usedOutdoor,
        uint32(std::time(nullptr)), _id);

    _dirty = false;
}

void HousingHouse::DeleteAllDecor()
{
    CharacterDatabase.PExecute("DELETE FROM `character_housing_decor` WHERE `housingId` = {}", _id);
    _placements.clear();
    RecalculateBudget();
    MarkDirty();
}

} // namespace Housing
