/*
 * Copyright (C) 2022 BfaCore Reforged
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 3 of the License, or (at your
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

#ifndef MOD_PSYCHO_HOUSE_H
#define MOD_PSYCHO_HOUSE_H

#include "Define.h"
#include "ObjectGuid.h"
#include "Position.h"

#include <map>
#include <string>
#include <unordered_map>
#include <vector>

class Creature;
class GameObject;
class Player;

namespace PsychoHouse
{
    // Phase band reserved for this module: 50000-64999, 15 000 houses. Inside the uint16
    // phase space this build uses and clear of the neighbourhood module's 10000-46374.
    constexpr uint32 kPhaseBase = 50000;
    constexpr uint32 kPhaseLast = 64999;

    // The only creature entries this module invents. Everything else it spawns is an entry
    // the realm already has, resolved by sql/world/03_bind_facilities.sql.
    constexpr uint32 kKeeperEntry  = 900200;
    constexpr uint32 kStewardEntry = 900201;

    enum FacilityKind : uint8
    {
        KindBank       = 1,
        KindAuctioneer = 2,
        KindMailbox    = 3,
        KindInnkeeper  = 4,
        KindStable     = 5,
        KindSpirit     = 6,
        KindForge      = 7,
        KindAnvil      = 8,
        KindBarber     = 9,
        KindGuildVault = 10,
        KindPortal     = 11,
        KindRepair     = 12
    };

    enum FacilityFaction : uint8
    {
        FactionAlliance = 0,
        FactionHorde    = 1,
        FactionBoth     = 2
    };

    /// Appended only: the gossip text and the command output both switch on these.
    enum HouseResult : uint8
    {
        Ok = 0,
        Disabled,
        NoHouse,
        HasHouse,
        NoMoney,
        NoSpace,
        NotInHouse,
        UnknownFacility,
        AlreadyBought,
        WrongFaction,
        NotEnoughLevel,
        Storage,
        Failed
    };

    char const* ResultText(HouseResult result);

    struct Facility
    {
        uint16      Id = 0;
        std::string Name;
        uint8       Kind = 0;
        uint32      Entry = 0;
        bool        IsObject = false;
        uint8       Faction = FactionBoth;
        uint32      Price = 0;
        float       X = 0.0f;
        float       Y = 0.0f;
        float       Z = 0.0f;
        float       O = 0.0f;
        uint8       RequiredLevel = 0;
        uint8       Sort = 0;
    };

    /// One character's house, cached while the character is in the world.
    struct Owner
    {
        uint64      Guid = 0;
        uint32      Account = 0;
        uint8       Team = 0;
        uint16      Phase = 0;
        std::string Name;
        bool        InHouse = false;
        ObjectGuid  Keeper;
        ObjectGuid  Steward;
        std::vector<uint16>                    Bought;
        std::unordered_map<uint16, ObjectGuid> Spawned;
    };

    class PsychoHouseMgr
    {
    public:
        static PsychoHouseMgr* instance();

        void LoadConfig();
        void LoadFacilities();
        void Unload();

        bool Enabled() const { return _enabled; }
        uint32 HousePrice() const { return _housePrice; }
        uint32 MapId() const { return _mapId; }
        uint32 ZoneId() const { return _zoneId; }
        uint32 AreaId() const { return _areaId; }

        std::vector<Facility> const& GetFacilities() const { return _facilities; }
        Facility const* FindFacility(uint16 id) const;
        bool HasFacility(Owner const* owner, uint16 id) const;

        Owner* GetOwner(Player* player);
        Owner* FindOwner(uint64 guid);

        void OnPlayerLogin(Player* player);
        void OnPlayerLogout(Player* player);
        void OnPlayerEnterArea(Player* player);
        void OnPlayerLeaveArea(Player* player);

        HouseResult BuyHouse(Player* player);
        HouseResult SellHouse(Player* player);
        HouseResult BuyFacility(Player* player, uint16 facilityId);
        bool TeleportHome(Player* player);

        std::string Diagnostics();

    private:
        void ApplyPhase(Player* player, Owner const* owner);
        void ClearPhase(Player* player, Owner const* owner);
        void SpawnHouse(Player* player, Owner* owner);
        void DespawnHouse(Owner* owner);
        void SpawnFacility(Player* player, Owner* owner, Facility const& facility);
        bool LoadPurchases(Owner* owner);
        bool FacilityFitsTeam(Facility const& facility, Owner const* owner) const;
        uint16 AllocatePhase(uint64 guid, uint32 account, uint8 team, std::string const& name);

        bool                        _enabled = false;
        uint32                      _mapId = 1;
        uint32                      _zoneId = 876;
        uint32                      _areaId = 876;
        Position                    _anchor;
        float                       _keeperX = -1.5f;
        float                       _keeperY = -2.5f;
        float                       _keeperZ = 0.0f;
        float                       _keeperO = 0.0f;
        float                       _stewardX = 1.5f;
        float                       _stewardY = -2.5f;
        float                       _stewardZ = 0.0f;
        float                       _stewardO = 0.0f;
        uint32                      _housePrice = 10000000;
        uint8                       _refundPercent = 50;

        std::vector<Facility>              _facilities;
        std::map<uint16, Facility>         _facilityById;
        std::map<uint64, Owner>            _owners;
    };
}

#define sPsychoHouse PsychoHouse::PsychoHouseMgr::instance()

#endif // MOD_PSYCHO_HOUSE_H
