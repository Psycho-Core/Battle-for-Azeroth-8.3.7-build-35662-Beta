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

#include "HousingNeighborhood.h"
#include "DatabaseEnv.h"
#include "Log.h"

namespace Housing
{

HousingNeighborhood::HousingNeighborhood(uint32 id, uint16 slot, uint32 mapId)
    : _id(id), _slot(slot), _mapId(mapId), _type(NeighborhoodPublic), _faction(FactionAlliance),
    _guildId(0), _ownerAccount(0), _created(0)
{
}

HousingNeighborhood::~HousingNeighborhood()
{
}

void HousingNeighborhood::SetInfo(uint8 type, uint8 faction, std::string const& name, uint64 guildId,
    uint32 ownerAccount, uint32 created)
{
    _type = type;
    _faction = faction;
    _name = name;
    _guildId = guildId;
    _ownerAccount = ownerAccount;
    _created = created;
}

void HousingNeighborhood::InitPlots(std::vector<PlotTemplate> const& templates)
{
    _plots.clear();

    for (PlotTemplate const& tmpl : templates)
    {
        if (tmpl.MapId != _mapId)
            continue;

        _plots.push_back(std::unique_ptr<HousingPlot>(new HousingPlot(this, tmpl)));
    }

    if (_plots.size() != kPlotCount)
        TC_LOG_ERROR("housing", "Neighbourhood " UI64FMTD " initialised with " UI64FMTD
            " plots, expected %u", uint64(_id), uint64(_plots.size()), uint32(kPlotCount));
}

HousingPlot* HousingNeighborhood::GetPlot(uint8 index)
{
    for (auto& plot : _plots)
        if (plot->GetIndex() == index)
            return plot.get();

    return nullptr;
}

HousingPlot const* HousingNeighborhood::GetPlot(uint8 index) const
{
    for (auto const& plot : _plots)
        if (plot->GetIndex() == index)
            return plot.get();

    return nullptr;
}

uint32 HousingNeighborhood::GetOwnedCount() const
{
    uint32 owned = 0;
    for (auto const& plot : _plots)
        if (plot->IsOwned())
            ++owned;

    return owned;
}

HousingPlot* HousingNeighborhood::FindFreePlot() const
{
    for (auto const& plot : _plots)
        if (!plot->IsOwned() && plot->GetTemplate().HasSpawn)
            return plot.get();

    return nullptr;
}

HousingPlot* HousingNeighborhood::FindPlotOfAccount(uint32 accountId) const
{
    if (!accountId)
        return nullptr;

    for (auto const& plot : _plots)
        if (plot->IsOwned() && plot->GetOwnerAccount() == accountId)
            return plot.get();

    return nullptr;
}

bool HousingNeighborhood::IsMember(uint32 accountId) const
{
    return _members.find(accountId) != _members.end();
}

void HousingNeighborhood::AddMember(uint32 accountId, uint8 rank)
{
    if (!accountId)
        return;

    _members[accountId] = rank;

    CharacterDatabase.PExecute(
        "REPLACE INTO `character_housing_neighbor_member` (`neighborhoodId`, `accountId`, `rank`, `joined`) "
        "VALUES ({}, {}, {}, UNIX_TIMESTAMP())", _id, accountId, uint32(rank));
}

void HousingNeighborhood::RemoveMember(uint32 accountId)
{
    if (_members.erase(accountId))
        CharacterDatabase.PExecute(
            "DELETE FROM `character_housing_neighbor_member` WHERE `neighborhoodId` = {} AND `accountId` = {}",
            _id, accountId);
}

uint8 HousingNeighborhood::GetRank(uint32 accountId) const
{
    auto const itr = _members.find(accountId);
    return itr != _members.end() ? itr->second : 0;
}

void HousingNeighborhood::LoadMembers()
{
    _members.clear();

    QueryResult result = CharacterDatabase.PQuery(
        "SELECT `accountId`, `rank` FROM `character_housing_neighbor_member` WHERE `neighborhoodId` = {}", _id);

    if (!result)
        return;

    do
    {
        Field* fields = result->Fetch();
        _members[fields[0].GetUInt32()] = fields[1].GetUInt8();
    }
    while (result->NextRow());
}

} // namespace Housing
