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

#ifndef HousingPlot_h__
#define HousingPlot_h__

#include "HousingDefines.h"
#include "HousingHouse.h"
#include "ObjectGuid.h"
#include <memory>
#include <string>

namespace Housing
{
    class HousingNeighborhood;

    /// One of the 55 plots in a neighbourhood: a template (from the world DB), an owner
    /// (from the character DB) and, when owned, a house.
    class TC_GAME_API HousingPlot
    {
    public:
        HousingPlot(HousingNeighborhood* neighborhood, PlotTemplate const& tmpl);
        ~HousingPlot();

        HousingNeighborhood* GetNeighborhood() const { return _neighborhood; }

        PlotTemplate const& GetTemplate() const { return _template; }
        uint8 GetIndex() const { return _template.Index; }
        uint32 GetBiomeId() const { return _template.BiomeId; }
        uint32 GetPrice() const { return _template.Price; }
        uint8 GetSizeClass() const { return _template.SizeClass; }

        uint16 GetSlot() const;
        uint32 GetPlotPhase() const;
        uint32 GetInteriorPhase() const;

        bool IsOwned() const { return _housingId != 0; }
        uint32 GetHousingId() const { return _housingId; }
        uint32 GetOwnerAccount() const { return _ownerAccount; }
        ObjectGuid GetOwnerGuid() const { return _ownerGuid; }
        std::string const& GetOwnerName() const { return _ownerName; }

        HousingHouse* GetHouse() { return _house.get(); }
        HousingHouse const* GetHouse() const { return _house.get(); }

        /// Attach an owner (creating the house). Values come from the character_housing row.
        void SetOwner(uint32 housingId, uint32 accountId, ObjectGuid guid, std::string const& ownerName,
            uint8 tier, uint32 blueprint, std::string const& houseName, bool locked);

        /// Detach the owner. The caller is responsible for despawning the decor and
        /// deleting the rows first.
        void ClearOwner();

        PlotState GetStateFor(uint32 viewerAccountId) const;

    private:
        HousingNeighborhood* _neighborhood;
        PlotTemplate _template;
        uint32 _housingId;
        uint32 _ownerAccount;
        ObjectGuid _ownerGuid;
        std::string _ownerName;
        std::unique_ptr<HousingHouse> _house;
    };

} // namespace Housing

#endif // HousingPlot_h__
