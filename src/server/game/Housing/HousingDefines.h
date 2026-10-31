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

#ifndef HousingDefines_h__
#define HousingDefines_h__

/*
 * Player housing - Founder's Point (alliance neighbourhood).
 *
 * Shared enums, limits, phase maths and value types for the module. Nothing in here
 * touches the world; it is safe to include from anywhere in the game or scripts libs.
 *
 * See docs/player-housing/02-module-architecture.md for the reasoning behind the
 * phase layout and docs/player-housing/04-addon-protocol.md for the wire values.
 */

#include "Define.h"
#include "Position.h"
#include <string>
#include <vector>

namespace Housing
{
    // -------------------------------------------------------------------------
    // Limits
    // -------------------------------------------------------------------------

    constexpr uint8 kMaxTier = 5;
    constexpr uint8 kPlotCount = 55;                 // plot indices 0..54
    constexpr uint16 kNeighborhoodSlotMax = 255;     // phase slot space
    constexpr uint8 kPhasesPerSlot = 64;             // 55 plots + 9 spare
    constexpr uint32 kPlotPhaseBase = 10000;         // 10000 .. 26374
    constexpr uint32 kInteriorPhaseBase = 30000;     // 30000 .. 46374
    constexpr uint32 kMaxPlacedDecor = 400;          // hard stop, above any budget cap
    constexpr float kDefaultYardRadius = 40.0f;
    constexpr uint32 kDirectoryPageSize = 20;
    constexpr uint32 kCataloguePageSize = 20;
    constexpr int32 kSaveIntervalSeconds = 30;

    // Addon protocol (docs/player-housing/04-addon-protocol.md)
    constexpr char const* kAddonPrefix = "BFAH";
    constexpr uint32 kAddonProtocolVersion = 3;
    constexpr size_t kAddonMaxChunk = 200;           // payload bytes per chunk
    constexpr uint32 kRateLimitMessages = 20;        // per window
    constexpr uint32 kRateLimitWindowMs = 10000;
    constexpr uint32 kChunkReassemblyTimeoutMs = 10000;

    // -------------------------------------------------------------------------
    // Enums (wire values - never renumber, the addon mirrors them)
    // -------------------------------------------------------------------------

    enum HousingFaction : uint8
    {
        FactionAlliance = 0,
        FactionHorde    = 1
    };

    enum NeighborhoodType : uint8
    {
        NeighborhoodPublic  = 0,
        NeighborhoodGuild   = 1,
        NeighborhoodPrivate = 2
    };

    enum PlotState : uint8
    {
        PlotAvailable = 0,
        PlotOwned     = 1,
        PlotLocked    = 2,
        PlotMine      = 3
    };

    enum SizeClass : uint8
    {
        SizeSmall    = 0,
        SizeStandard = 1,
        SizeLarge    = 2
    };

    enum Surface : uint8
    {
        SurfaceFloor   = 0,
        SurfaceWall    = 1,
        SurfaceCeiling = 2,
        SurfaceAny     = 3
    };

    enum Placement : uint8
    {
        PlacementOutdoor = 0,
        PlacementIndoor  = 1,
        PlacementBoth    = 2
    };

    enum DecorSource : uint8
    {
        SourceVendor        = 0,
        SourceQuest         = 1,
        SourceAchievement   = 2,
        SourceDrop          = 3,
        SourceTreasureHunt  = 4,
        SourceGm            = 5
    };

    enum VendorRole : uint8
    {
        RoleDecorVendor = 0,
        RoleContractor  = 1,
        RoleSteward     = 2,
        RoleSmuggler    = 3,
        RolePetDecor    = 4,
        RoleHoliday     = 5
    };

    enum AmenityType : uint8
    {
        AmenityBulletinBoard = 0,
        AmenityPortal        = 1,
        AmenityContractor    = 2,
        AmenityFlightPoint   = 3,
        AmenityGathering     = 4,
        AmenityScenery       = 5,
        AmenityVendor        = 6,
        AmenityEndeavor      = 7
    };

    enum HousingAccess : uint8
    {
        AccessNone     = 0,
        AccessStranger = 1,
        AccessNeighbor = 2,
        AccessGuest    = 3,
        AccessOwner    = 4,
        AccessGm       = 5
    };

    /// Result codes are part of the addon protocol. Append only.
    enum HousingResult : uint8
    {
        Ok                   = 0,
        Disabled             = 1,
        NotInNeighborhood    = 2,
        InvalidPlot          = 3,
        PlotTaken            = 4,
        NotOwner             = 5,
        NoHouse              = 6,
        NotEnoughMoney       = 7,
        BudgetExceeded       = 8,
        InvalidDecor         = 9,
        NotUnlocked          = 10,
        OutOfBounds          = 11,
        WrongSurface         = 12,
        BadScale             = 13,
        TooManyDecor         = 14,
        WrongFaction         = 15,
        TooManyHouses        = 16,
        NeighborhoodFull     = 17,
        TierTooLow           = 18,
        TierMax              = 19,
        NotFound             = 20,
        GuestRestricted      = 21,
        RateLimited          = 22,
        Cooldown             = 23,
        NotAllowedHere       = 24,
        InternalError        = 25
    };

    // Flag bits
    enum PlotFlags : uint32
    {
        PlotFlagIsland         = 0x001,  // Outer Banks private island, non contiguous
        PlotFlagBridgeOnly     = 0x002,  // Stoneveil Ridge, reachable only by its bridge
        PlotFlagNoOutdoorDecor = 0x004
    };

    enum DecorFlags : uint32
    {
        DecorFlagInteractive = 0x001,    // clicking it does something (Pepe)
        DecorFlagNoFloat     = 0x002,    // must sit on a surface, no free placement
        DecorFlagHidden      = 0x004     // row exists but is not listed
    };

    enum AmenityFlags : uint32
    {
        AmenityFlagMapPin = 0x001
    };

    // -------------------------------------------------------------------------
    // Phase maths
    // -------------------------------------------------------------------------

    /// Phase a plot's yard decor is spawned in.
    inline uint32 PlotPhase(uint16 slot, uint8 plotIndex)
    {
        return kPlotPhaseBase + uint32(slot) * kPhasesPerSlot + uint32(plotIndex);
    }

    /// Phase a house interior's decor is spawned in.
    inline uint32 InteriorPhase(uint16 slot, uint8 plotIndex)
    {
        return kInteriorPhaseBase + uint32(slot) * kPhasesPerSlot + uint32(plotIndex);
    }

    // -------------------------------------------------------------------------
    // House tier tables (my numbers, see docs/player-housing/02-module-architecture.md)
    // -------------------------------------------------------------------------

    inline uint32 BudgetIndoor(uint8 tier)
    {
        static uint32 const budget[kMaxTier + 1] = { 0, 60, 90, 120, 160, 200 };
        return tier <= kMaxTier ? budget[tier] : budget[kMaxTier];
    }

    inline uint32 BudgetOutdoor(uint8 tier)
    {
        static uint32 const budget[kMaxTier + 1] = { 0, 25, 45, 65, 85, 110 };
        return tier <= kMaxTier ? budget[tier] : budget[kMaxTier];
    }

    /// Gold cost to reach the given tier (0 when it is the starting tier).
    inline uint32 TierUnlockCost(uint8 tier)
    {
        static uint32 const cost[kMaxTier + 1] = { 0, 0, 5000, 15000, 40000, 100000 };
        return tier <= kMaxTier ? cost[tier] : 0;
    }

    // -------------------------------------------------------------------------
    // Value types loaded from the world database
    // -------------------------------------------------------------------------

    struct TC_GAME_API BiomeInfo
    {
        uint32 Id = 0;
        std::string Name;
        std::string Description;
        uint8 Faction = FactionAlliance;
        uint32 ZoneId = 0;      // retail zone id, reference only
        uint32 UiMapId = 0;     // retail UI map id, reference only
        uint32 MapId = 0;       // the map the module actually uses
        Position FlightPoint;
        float CenterX = 0.0f;
        float CenterY = 0.0f;
        uint32 Flags = 0;
    };

    struct TC_GAME_API PlotTemplate
    {
        uint32 MapId = 0;
        uint8 Index = 0;
        uint32 BiomeId = 0;
        uint8 SizeClass = SizeSmall;
        uint32 Price = 0;       // gold
        Position Spawn;
        float YardRadius = 0.0f;
        uint32 Flags = 0;
        std::string Comment;
        bool HasSpawn = false;  // spawn authored on the custom map yet?
    };

    struct TC_GAME_API DecorInfo
    {
        uint32 Id = 0;
        std::string Name;
        uint32 GameObjectEntry = 0;
        uint8 BudgetCost = 1;
        uint8 Surface = SurfaceFloor;
        uint8 Placement = PlacementOutdoor;
        std::string Category;
        uint8 SourceType = SourceVendor;
        uint32 SourceId = 0;
        uint32 Price = 0;       // gold
        uint32 Currency = 0;    // 0 = gold, otherwise currency id (3363 = community coupons)
        uint8 FactionMask = 1;  // bit0 alliance, bit1 horde
        float ScaleMin = 0.5f;
        float ScaleMax = 3.0f;
        uint32 Flags = 0;
    };

    struct TC_GAME_API VendorInfo
    {
        uint32 Id = 0;
        std::string Name;
        uint32 BiomeId = 0;     // 0 = town center
        uint8 Role = RoleDecorVendor;
        uint32 CreatureEntry = 0;   // appearance source, 0 = not spawned
        Position Pos;
        std::string RetailName;
        std::string Comment;
        bool HasSpawn = false;
    };

    struct TC_GAME_API AmenityInfo
    {
        uint32 Id = 0;
        uint32 BiomeId = 0;
        std::string Name;
        uint8 Type = AmenityScenery;
        Position Pos;
        float Radius = 0.0f;
        uint32 Flags = AmenityFlagMapPin;
    };

    struct TC_GAME_API VendorStockEntry
    {
        uint32 DecorId = 0;
        uint32 Price = 0;       // 0 = use the decor row's own price
    };

} // namespace Housing

#endif // HousingDefines_h__
