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

#ifndef HousingMgr_h__
#define HousingMgr_h__

#include "HousingAddon.h"
#include "HousingDecor.h"
#include "HousingDefines.h"
#include "HousingNeighborhood.h"

#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

class Map;
class Player;

namespace Housing
{
    /// Per-player housing state. Created on login, dropped on logout.
    struct PlayerState
    {
        uint8 ChunkMessageId = 0;
        uint32 RateWindowStart = 0;
        uint32 RateCount = 0;
        uint32 TickAccum = 0;
        uint32 NeighborhoodId = 0;   // neighbourhood the player is currently in
        uint8 InsidePlot = 0xFF;     // 0xFF = not standing in a plot
        bool InsideInterior = false;
        uint32 LastHomeUse = 0;      // ms timestamp of the last homestone use
        bool HasOwnedPlot = false;
        uint32 OwnedHousingId = 0;
    };

    /// Singleton that owns every piece of housing state on this worldserver.
    ///
    /// Threading: world thread only. Nothing here is safe to call from a map-update
    /// thread, and nothing here is mutex protected on purpose - the core already runs
    /// player and script updates on the world thread.
    class TC_GAME_API HousingMgr
    {
    public:
        static HousingMgr* instance();

        // ---- lifecycle -------------------------------------------------------

        void LoadConfig();
        void LoadHousingData();
        void LoadNeighborhoods();
        void Unload();
        void Update(uint32 diff);

        bool IsEnabled() const { return _enabled; }
        uint32 GetModuleVersion() const { return 1; }

        // ---- world data ------------------------------------------------------

        BiomeInfo const* GetBiome(uint32 id) const;
        PlotTemplate const* GetPlotTemplate(uint32 mapId, uint8 index) const;
        DecorInfo const* GetDecor(uint32 id) const;
        VendorInfo const* GetVendor(uint32 id) const;
        AmenityInfo const* GetAmenity(uint32 id) const;

        std::map<uint32, DecorInfo> const& GetDecorCatalogue() const { return _decor; }

        std::vector<PlotTemplate> const& GetPlotTemplates(uint32 mapId) const;
        std::vector<AmenityInfo> const& GetAmenities() const { return _amenityList; }
        std::vector<VendorInfo> const& GetVendors() const { return _vendorList; }
        std::vector<VendorStockEntry> const& GetVendorStock(uint32 vendorId) const;

        uint32 GetHousingMapId() const { return _housingMapId; }
        uint32 GetInteriorMapId() const { return _interiorMapId; }
        bool IsHousingMap(uint32 mapId) const;

        uint64 GeneratePlacementId() { return _nextPlacementId++; }
        uint32 GenerateHousingId() { return _nextHousingId++; }
        uint32 GenerateNeighborhoodId() { return _nextNeighborhoodId++; }

        // ---- neighbourhoods --------------------------------------------------

        HousingNeighborhood* GetNeighborhood(uint32 id);
        HousingNeighborhood* GetNeighborhoodBySlot(uint16 slot);
        std::map<uint32, std::unique_ptr<HousingNeighborhood>> const& GetNeighborhoods() const { return _neighborhoods; }
        HousingNeighborhood* FindNeighborhoodFor(Player* player);
        HousingNeighborhood* CreateNeighborhood(uint8 type, uint8 faction, std::string const& name,
            uint64 guildId, uint32 ownerAccount);
        void DissolveNeighborhood(HousingNeighborhood* neighborhood);

        // ---- player hooks ----------------------------------------------------

        void OnPlayerLogin(Player* player);
        void OnPlayerLogout(Player* player);
        void OnPlayerUpdate(Player* player, uint32 diff);
        void OnPlayerZoneChange(Player* player);

        // ---- protocol --------------------------------------------------------

        void HandleAddonMessage(Player* player, std::string const& message);
        void SendResult(Player* player, char const* op, HousingResult result, uint64 extra = 0);
        void SendLine(Player* player, std::string const& payload);

        // ---- player operations -----------------------------------------------

        HousingResult BuyPlot(Player* player, HousingNeighborhood* neighborhood, uint8 plotIndex);
        HousingResult SellPlot(Player* player);
        HousingResult UpgradeTier(Player* player, uint8 tier);
        HousingResult RenameHouse(Player* player, std::string const& name);
        HousingResult SetLocked(Player* player, bool locked);

        HousingResult TeleportToPlot(Player* player, uint8 plotIndex);
        HousingResult TeleportHome(Player* player);
        HousingResult TeleportToNeighborhood(Player* player, HousingNeighborhood* neighborhood);
        HousingResult TeleportToTownCenter(Player* player);
        HousingResult EnterHouse(Player* player, bool enter);

        // ---- decor -----------------------------------------------------------

        HousingResult PlaceDecor(Player* player, uint32 decorId, float x, float y, float z, float o,
            float scale, bool indoor, uint64& placementId);
        HousingResult MoveDecor(Player* player, uint64 placementId, float x, float y, float z, float o, float scale);
        HousingResult RemoveDecor(Player* player, uint64 placementId);
        HousingResult BuyDecor(Player* player, uint32 decorId);

        bool HasUnlocked(uint32 accountId, uint32 decorId);
        void LoadCollection(uint32 accountId);

        // ---- access ----------------------------------------------------------

        HousingAccess CheckAccess(Player* player, HousingPlot* plot);
        HousingPlot* GetOwnedPlot(Player* player);
        HousingPlot* GetPlotForEdit(Player* player);
        PlayerState* GetPlayerState(Player* player);

        // ---- maintenance -----------------------------------------------------

        void SaveAll();
        void ForceSellPlot(HousingPlot* plot);
        std::string BuildAuditReport();

    private:
        HousingMgr();
        ~HousingMgr() = default;

        // config
        bool _enabled;
        uint32 _housingMapId;
        uint32 _interiorMapId;
        Position _interiorAnchor;
        Position _townCenter;
        uint32 _saveInterval;
        uint32 _rateLimit;
        uint32 _homeCooldown;
        uint32 _refundPercent;
        bool _plotScopedPhasing;
        bool _nativeMap;

        // world data
        std::map<uint32, BiomeInfo> _biomes;
        std::map<uint64, PlotTemplate> _plotTemplates;                 // key: (mapId << 8) | index
        std::map<uint32, std::vector<PlotTemplate>> _plotTemplatesByMap;
        std::map<uint32, DecorInfo> _decor;
        std::map<uint32, VendorInfo> _vendors;
        std::map<uint32, std::vector<VendorStockEntry>> _vendorStock;
        std::map<uint32, AmenityInfo> _amenities;
        std::vector<AmenityInfo> _amenityList;
        std::vector<VendorInfo> _vendorList;
        std::map<uint32, std::vector<std::string>> _namePool;           // faction -> names

        // runtime state
        std::map<uint32, std::unique_ptr<HousingNeighborhood>> _neighborhoods;
        std::vector<bool> _usedSlots;
        std::map<uint64, PlayerState> _playerStates;                    // key: player guid counter
        std::map<uint32, std::set<uint32>> _collection;                 // accountId -> decor ids
        std::set<uint32> _collectionLoaded;                             // accounts already read
        std::map<uint32, std::map<uint32, uint8>> _guestCache;           // housingId -> accountId -> flags
        std::map<uint32, uint32> _houseViewers;                         // housingId -> players holding its phase

        uint64 _nextPlacementId;
        uint32 _nextHousingId;
        uint32 _nextNeighborhoodId;
        uint32 _saveTimer;
        uint32 _tick;

        // helpers
        static uint64 PlotKey(uint32 mapId, uint8 index);
        void LoadBiomes();
        void LoadPlotTemplates();
        void LoadDecor();
        void LoadVendors();
        void LoadAmenities();
        void LoadNamePool();

        uint16 AllocateSlot();
        void ReleaseSlot(uint16 slot);

        void ResolvePlot(Player* player, PlayerState& state);
        void ClearPlotPhases(Player* player, PlayerState& state);
        void ApplyPlotPhase(Player* player, HousingPlot* plot);
        void ReleaseViewer(HousingPlot* plot);

        void HandleHello(Player* player, PlayerState& state);
        void HandleNeighborhoodInfo(Player* player, PlayerState& state);
        void HandlePlotList(Player* player, PlayerState& state);
        void HandlePlotInfo(Player* player, PlayerState& state, uint8 plotIndex);
        void HandleDirectory(Player* player, PlayerState& state, uint32 page);
        void HandleDecorList(Player* player, PlayerState& state, std::string const& category, uint32 page);
        void HandlePlacedList(Player* player, PlayerState& state);
        void HandleGuestList(Player* player, PlayerState& state);
        void HandleAmenityList(Player* player, PlayerState& state);

        bool IsGuest(uint32 housingId, uint32 accountId);

        std::map<uint32, uint8> const& LoadGuests(uint32 housingId);
        uint8 GetGuestFlags(uint32 housingId, uint32 accountId);
        void AddGuest(uint32 housingId, uint32 accountId, uint8 flags);
        void RemoveGuest(uint32 housingId, uint32 accountId);

        void SendChunk(Player* player, std::string const& payload);
    };

} // namespace Housing

#define sHousingMgr Housing::HousingMgr::instance()

#endif // HousingMgr_h__
