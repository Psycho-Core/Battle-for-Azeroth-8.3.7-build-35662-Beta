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

#ifndef HousingHouse_h__
#define HousingHouse_h__

#include "HousingDecor.h"
#include "HousingDefines.h"
#include "ObjectGuid.h"
#include <string>
#include <vector>

class GameObject;
class Map;

namespace Housing
{
    /// A house and its furniture. One per owned plot: the house exists because the plot
    /// has an owner, and dies with it.
    ///
    /// The house owns the placed decor rows and the live game objects, and it is the only
    /// place that knows about budgets. It does not know who owns the plot or who may edit
    /// it - that is HousingMgr / HousingPlot territory.
    class TC_GAME_API HousingHouse
    {
    public:
        HousingHouse(uint32 id, uint8 plotIndex);
        ~HousingHouse();

        uint32 GetId() const { return _id; }
        uint8 GetPlotIndex() const { return _plotIndex; }

        uint8 GetTier() const { return _tier; }
        void SetTier(uint8 tier);

        uint32 GetBlueprint() const { return _blueprint; }
        void SetBlueprint(uint32 blueprint);

        /// The exterior shell applied to the plot, and how many extra rooms have been built.
        /// Both are bought at the General Contractor and both cost decor budget through
        /// BudgetRoom(), which is why the house has to know them.
        uint8 GetShellId() const { return _shellId; }
        void SetShellId(uint8 shellId) { _shellId = shellId; MarkDirty(); }
        uint8 GetRoomsOwned() const { return _roomsOwned; }
        void SetRoomsOwned(uint8 rooms) { _roomsOwned = rooms; MarkDirty(); }

        /// The Hearthlight Armillary's preset, 0-4. Stored on the house so it survives a relog
        /// and is the same for everyone who walks in.
        uint8 GetArmillary() const { return _armillary; }
        void SetArmillary(uint8 preset)
        {
            _armillary = preset <= ArmillaryMidnight ? preset : uint8(ArmillaryDefault);
            MarkDirty();
        }

        /// Convenience for the spawn path: is this house showing its lit pieces?
        bool IsNightMode() const { return IsArmillaryDark(_armillary); }

        std::string const& GetName() const { return _name; }
        void SetName(std::string const& name);

        bool IsLocked() const { return _locked; }
        void SetLocked(bool locked);

        /// The two permission masks: which audiences may stand on the yard and which may come
        /// inside. Independent of `locked`, which is a blanket denial on top of them.
        uint8 GetAccessMask(HousingSpace space) const { return space == SpaceHouse ? _houseMask : _plotMask; }
        void SetAccessMask(HousingSpace space, uint8 mask)
        {
            if (space == SpaceHouse)
                _houseMask = mask;
            else
                _plotMask = mask;

            MarkDirty();
        }

        uint32 GetBudget(bool indoor) const
        {
            return indoor ? BudgetIndoor(_tier) : BudgetOutdoor(_tier);
        }

        uint32 GetUsedBudget(bool indoor) const;
        bool IsOverBudget() const;

        // ---- decor -----------------------------------------------------------

        /// Load the placement rows. Returns false when the query could not be executed.
        bool LoadDecor();

        std::vector<DecorPlacement> const& GetPlacements() const { return _placements; }
        DecorPlacement const* GetPlacement(uint64 id) const;
        DecorPlacement* GetPlacementMutable(uint64 id);

        /// Validate a position/scale against the plot bounds, catalog rules and budget.
        /// Pure: does not change anything, so the caller can reuse it for MOVE.
        HousingResult Validate(DecorInfo const& info, float x, float y, float z, float scale,
            bool indoor, Position const& plotSpawn, float yardRadius, uint64 ignorePlacementId = 0) const;

        // `pitch` and `roll` are the two tilt axes advanced mode adds; both default to 0, so
        // every caller that only knows about yaw still compiles and behaves as it did.
        HousingResult AddPlacement(uint64 placementId, DecorInfo const& info, float x, float y, float z,
            float o, float scale, bool indoor, Position const& plotSpawn, float yardRadius,
            float pitch = 0.0f, float roll = 0.0f);
        HousingResult MovePlacement(uint64 id, float x, float y, float z, float o, float scale,
            DecorInfo const& info, Position const& plotSpawn, float yardRadius,
            float pitch = 0.0f, float roll = 0.0f);
        HousingResult RemovePlacement(uint64 id);

        /// Put a collected companion on a bed, or take one off with entry 0. Persists and
        /// returns the placement so the caller can respawn the creature.
        HousingResult SetPet(uint64 id, uint32 entry, bool roaming);

        // ---- live objects ----------------------------------------------------

        /// Spawn every placement on the given map/phase. Idempotent while _spawned is true.
        void EnsureSpawned(Map* map, uint32 phaseId, ObjectGuid ownerGuid);
        /// Spawn a single placement (used right after a successful place/move).
        void SpawnPlacement(DecorPlacement& placement, Map* map, uint32 phaseId, ObjectGuid ownerGuid);
        /// Remove every live object.
        void DespawnAll();

        bool IsSpawned() const { return _spawned; }
        Map* GetSpawnMap() const { return _spawnMap; }

        // ---- persistence -----------------------------------------------------

        void MarkDirty() { _dirty = true; }
        bool IsDirty() const { return _dirty; }
        /// Write tier/name/locked/budget back to character_housing when dirty.
        void Flush();

        /// Wipe every decor row for this house (used when the plot is sold).
        void DeleteAllDecor();

    private:
        void RecalculateBudget();

        uint32 _id;
        uint8 _plotIndex;
        uint8 _tier;
        uint32 _blueprint;
        std::string _name;
        bool _locked;
        uint8 _armillary;
        uint8 _plotMask;
        uint8 _houseMask;
        uint8 _shellId;
        uint8 _roomsOwned;
        bool _decorLoaded;
        bool _spawned;
        Map* _spawnMap;
        bool _dirty;
        uint32 _usedIndoor;
        uint32 _usedOutdoor;
        std::vector<DecorPlacement> _placements;
    };

} // namespace Housing

#endif // HousingHouse_h__
