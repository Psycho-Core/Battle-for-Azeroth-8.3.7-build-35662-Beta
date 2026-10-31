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

#include "HousingPlot.h"
#include "HousingNeighborhood.h"

namespace Housing
{

HousingPlot::HousingPlot(HousingNeighborhood* neighborhood, PlotTemplate const& tmpl)
    : _neighborhood(neighborhood), _template(tmpl), _housingId(0), _ownerAccount(0)
{
}

HousingPlot::~HousingPlot()
{
}

uint16 HousingPlot::GetSlot() const
{
    return _neighborhood ? _neighborhood->GetSlot() : 0;
}

uint32 HousingPlot::GetPlotPhase() const
{
    return PlotPhase(GetSlot(), GetIndex());
}

uint32 HousingPlot::GetInteriorPhase() const
{
    return InteriorPhase(GetSlot(), GetIndex());
}

void HousingPlot::SetOwner(uint32 housingId, uint32 accountId, ObjectGuid guid, std::string const& ownerName,
    uint8 tier, uint32 blueprint, std::string const& houseName, bool locked)
{
    _housingId = housingId;
    _ownerAccount = accountId;
    _ownerGuid = guid;
    _ownerName = ownerName;

    _house.reset(new HousingHouse(housingId, GetIndex()));
    _house->SetTier(tier);
    _house->SetBlueprint(blueprint);
    _house->SetName(houseName);
    _house->SetLocked(locked);
}

void HousingPlot::ClearOwner()
{
    _house.reset();
    _housingId = 0;
    _ownerAccount = 0;
    _ownerGuid.Clear();
    _ownerName.clear();
}

PlotState HousingPlot::GetStateFor(uint32 viewerAccountId) const
{
    if (!IsOwned())
        return PlotAvailable;

    if (viewerAccountId != 0 && viewerAccountId == _ownerAccount)
        return PlotMine;

    HousingHouse const* house = _house.get();
    if (house && house->IsLocked())
        return PlotLocked;

    return PlotOwned;
}

} // namespace Housing
