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

#ifndef HousingNeighborhood_h__
#define HousingNeighborhood_h__

#include "HousingDefines.h"
#include "HousingPlot.h"
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace Housing
{
    /// A village: one phase slot, 55 plots, an administration type and a member list.
    class TC_GAME_API HousingNeighborhood
    {
    public:
        HousingNeighborhood(uint32 id, uint16 slot, uint32 mapId);
        ~HousingNeighborhood();

        uint32 GetId() const { return _id; }
        uint16 GetSlot() const { return _slot; }
        uint32 GetMapId() const { return _mapId; }

        uint8 GetType() const { return _type; }
        uint8 GetFaction() const { return _faction; }
        std::string const& GetName() const { return _name; }
        uint64 GetGuildId() const { return _guildId; }
        uint32 GetOwnerAccount() const { return _ownerAccount; }
        uint32 GetCreated() const { return _created; }

        void SetInfo(uint8 type, uint8 faction, std::string const& name, uint64 guildId, uint32 ownerAccount, uint32 created);

        /// Create the 55 plot objects from the templates of this neighbourhood's map.
        void InitPlots(std::vector<PlotTemplate> const& templates);

        HousingPlot* GetPlot(uint8 index);
        HousingPlot const* GetPlot(uint8 index) const;
        std::vector<std::unique_ptr<HousingPlot>> const& GetPlots() const { return _plots; }

        uint32 GetOwnedCount() const;
        HousingPlot* FindFreePlot() const;
        HousingPlot* FindPlotOfAccount(uint32 accountId) const;

        // ---- membership (guild / private neighbourhoods only) ----------------

        bool IsMember(uint32 accountId) const;
        void AddMember(uint32 accountId, uint8 rank);
        void RemoveMember(uint32 accountId);
        uint8 GetRank(uint32 accountId) const;
        std::map<uint32, uint8> const& GetMembers() const { return _members; }
        void LoadMembers();

    private:
        uint32 _id;
        uint16 _slot;
        uint32 _mapId;
        uint8 _type;
        uint8 _faction;
        std::string _name;
        uint64 _guildId;
        uint32 _ownerAccount;
        uint32 _created;
        std::vector<std::unique_ptr<HousingPlot>> _plots;
        std::map<uint32, uint8> _members;
    };

} // namespace Housing

#endif // HousingNeighborhood_h__
