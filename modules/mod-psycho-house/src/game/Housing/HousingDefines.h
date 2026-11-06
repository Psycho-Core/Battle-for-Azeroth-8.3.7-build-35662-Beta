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

    // House level, 1..12. Retail's ladder, not a five-rung substitute.
    constexpr uint8 kMaxLevel = 12;
    constexpr uint8 kMaxTier = kMaxLevel;            // the old name, still on the wire
    constexpr uint8 kPlotCount = 55;                 // plot indices 0..54
    constexpr uint16 kNeighborhoodSlotMax = 255;     // phase slot space
    constexpr uint8 kPhasesPerSlot = 64;             // 55 plots + 9 spare
    constexpr uint32 kPlotPhaseBase = 10000;         // 10000 .. 26374
    constexpr uint32 kInteriorPhaseBase = 30000;     // 30000 .. 46374
    // One phase per neighbourhood slot. Everything static in a neighbourhood - the town
    // centre, the amenities, the vendors, the shells - is spawned in it, so two
    // neighbourhoods that share a map and a set of coordinates cannot see each other.
    constexpr uint32 kNeighborhoodPhaseBase = 60000; // 60000 .. 60255
    constexpr uint32 kMaxPlacedDecor = 400;          // hard stop, above any budget cap
    constexpr float kDefaultYardRadius = 40.0f;

    // Advanced mode tilts a piece on two axes. The yaw is unbounded because it wraps, but a
    // tilt past a quarter turn puts furniture through the floor or upside down, which is a
    // way to make a house look broken rather than decorated, so it is clamped to +-90 deg.
    constexpr float kMaxTilt = 1.5707964f;
    constexpr uint32 kDirectoryPageSize = 20;
    constexpr uint32 kCataloguePageSize = 20;
    constexpr int32 kSaveIntervalSeconds = 30;

    // World states the addon reads. 8.3.7 lets a server push any id; these three are the
    // module's own, high enough not to collide with a Blizzard world state in the 8.3.7 range.
    constexpr uint32 kFavorWorldState = 61200;      // Endeavor favour, 0-1000
    constexpr uint32 kMilestoneWorldState = 61201;  // Endeavor milestone, 0-4
    constexpr uint32 kArmillaryWorldState = 61202;  // 1 while the Hearthlight Armillary is lit

    // Anchor mode: plots are laid out on concentric rings around the town centre when a
    // plot template has no authored spawn, so the module is playable on a borrowed map
    // before the custom one exists.
    constexpr float kAnchorRingStart = 90.0f;
    constexpr float kAnchorRingSpacing = 70.0f;
    constexpr uint8 kAnchorPerRing = 10;
    constexpr uint8 kAnchorRingGrowth = 2;
    constexpr float kAnchorAmenityRadius = 26.0f;

    constexpr uint32 kArchiveDays = 7;
    constexpr uint32 kReservationSeconds = 60;

    // Cost channels. `housing_decor.currency` is 0 for gold, a currency id for a real
    // currency, or an item id for stock that is paid for in a tradable item. 33226
    // (Tricky Treat) is the live example.
    constexpr uint32 kCurrencyGold = 0;
    constexpr uint32 kCurrencyCommunityCoupons = 3363;

    inline bool IsItemCost(uint32 cost)
    {
        return cost != kCurrencyGold && cost != kCurrencyCommunityCoupons;
    }

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
        InternalError        = 25,
        LevelTooLow          = 26,
        StorageEmpty         = 27,
        EndeavorIncomplete   = 28,
        BadCode              = 29,
        ArchiveExpired       = 30,
        NotGuild             = 31,
        AlreadyKnown         = 32,
        NoBattlePet          = 33,   // nothing summoned, or the species is not in the journal
        NotReady             = 34,   // level, cooldown or prerequisite not met
        OnCooldown           = 35,
        NotInGuild           = 36,
        GuildRankTooLow      = 37,
        WrongNeighborhood    = 38
    };

    // Flag bits
    enum PlotFlags : uint32
    {
        PlotFlagIsland         = 0x001,  // Outer Banks private island, non contiguous
        PlotFlagBridgeOnly     = 0x002,  // Stoneveil Ridge, reachable only by its bridge
        PlotFlagNoOutdoorDecor = 0x004,
        PlotFlagArmillary      = 0x008   // the plot has a Hearthlight Armillary
    };

    enum DecorFlags : uint32
    {
        DecorFlagInteractive = 0x001,    // clicking it does something (Pepe)
        DecorFlagNoFloat     = 0x002,    // must sit on a surface, no free placement
        DecorFlagHidden      = 0x004,    // row exists but is not listed
        DecorFlagUnmapped    = 0x008,    // catalogue entry with no 8.3.7 game object yet
        DecorFlagPetBed      = 0x010,    // owns a pet creature (housing_decor_creature kind 0)
        DecorFlagJukebox     = 0x020,    // reserved: needs a verifiable sound id
        DecorFlagMountStand  = 0x040,    // shows the owner's mount on a creature
        DecorFlagMannequin   = 0x080,    // shows the owner's transmog on a creature
        DecorFlagDyable      = 0x100,    // has at least one row in housing_decor_variant
        DecorFlagSmuggled    = 0x200     // only a smuggler sells it to the other faction
    };

    enum AmenityFlags : uint32
    {
        AmenityFlagMapPin    = 0x001,
        AmenityFlagSanctuary = 0x002,
        AmenityFlagPortal    = 0x004
    };

    enum BiomeFlags : uint32
    {
        BiomeFlagSanctuary = 0x001,
        BiomeFlagNoFly     = 0x002
    };

    /// Guest permissions. All three are enforced; the schema has carried the bits since the
    /// first draft and only the first one was ever read.
    enum GuestFlags : uint8
    {
        GuestFlagEnter  = 0x01,
        GuestFlagEdit   = 0x02,
        GuestFlagManage = 0x04
    };

    enum EndeavorTaskType : uint8
    {
        TaskKillCreature   = 0,
        TaskKillBoss       = 1,
        TaskCompleteQuest  = 2,
        TaskLootItem       = 3,
        TaskCraftItem      = 4,
        TaskPetBattle      = 5,
        TaskDeliver        = 6
    };

    enum BlueprintKind : uint8
    {
        BlueprintBuild    = 0,
        BlueprintRoom     = 1,
        BlueprintExterior = 2
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

    /// Phase the static content of a neighbourhood is spawned in - town centre, amenity
    /// objects, vendors, plot shells.
    inline uint32 NeighborhoodPhase(uint16 slot)
    {
        return kNeighborhoodPhaseBase + uint32(slot);
    }

    // -------------------------------------------------------------------------
    // The level ladder. Retail publishes the XP thresholds and the endpoints of the three
    // budgets (910 to 5975 indoor, 19 to 134 room, 200 to 350 exterior); the eleven steps
    // between the endpoints are the straight line. housing_house_level carries the same
    // numbers and is what the server loads, so a realm can retune without a rebuild.
    // -------------------------------------------------------------------------

    inline uint32 LevelXp(uint8 level)
    {
        static uint32 const xp[kMaxLevel + 1] =
            { 0, 0, 10, 1200, 2400, 3700, 5700, 7900, 10300, 12900, 15750, 18850, 22200 };
        return level <= kMaxLevel ? xp[level] : xp[kMaxLevel];
    }

    inline uint32 BudgetIndoor(uint8 level)
    {
        static uint32 const budget[kMaxLevel + 1] = { 0, 910, 1370, 1831, 2291, 2752,
            3212, 3672, 4133, 4593, 5054, 5514, 5975 };
        return level <= kMaxLevel ? budget[level] : budget[kMaxLevel];
    }

    inline uint32 BudgetOutdoor(uint8 level)
    {
        static uint32 const budget[kMaxLevel + 1] = { 0, 200, 214, 227, 241, 255, 268,
            282, 295, 309, 323, 336, 350 };
        return level <= kMaxLevel ? budget[level] : budget[kMaxLevel];
    }

    inline uint32 BudgetRoom(uint8 level)
    {
        static uint32 const budget[kMaxLevel + 1] = { 0, 19, 29, 40, 50, 61, 71, 82,
            92, 103, 113, 124, 134 };
        return level <= kMaxLevel ? budget[level] : budget[kMaxLevel];
    }

    /// Rooms unlocked at this level. The guide gives two points - twelve rooms, then
    /// eighteen - and nothing in between, so the ramp is mine.
    inline uint8 RoomCount(uint8 level)
    {
        static uint8 const rooms[kMaxLevel + 1] =
            { 0, 2, 3, 4, 5, 6, 7, 8, 9, 10, 12, 15, 18 };
        return level <= kMaxLevel ? rooms[level] : rooms[kMaxLevel];
    }

    constexpr uint32 kXpPerNewDecor = 10;
    constexpr uint32 kXpPerEndeavorTask = 50;
    constexpr uint32 kXpPerEndeavor = 250;

    inline uint8 LevelForXp(uint32 xp)
    {
        uint8 level = 1;
        for (uint8 i = 2; i <= kMaxLevel; ++i)
            if (xp >= LevelXp(i))
                level = i;
        return level;
    }

    /// Gold to skip one level. Retail has no such thing; it exists so a realm that does not
    /// want to wait for XP can still hand out a bigger house, and it is off by default.
    inline uint32 LevelSkipCost(uint8 level)
    {
        return 1000u * uint32(level) * uint32(level);
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
        bool TerrainZ = false;  // spawn_z was authored NULL: take the ground on first entry
    };

    struct TC_GAME_API DecorInfo
    {
        uint32 Id = 0;
        std::string Name;
        uint32 GameObjectEntry = 0;
        uint32 ItemEntry = 0;   // retail decor item id, 0 for native pieces
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
        uint8 DyeGroup = 0;     // 0 = not dyable
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
        uint32 GameObjectEntry = 0;
        uint32 CreatureEntry = 0;
    };

    struct TC_GAME_API VendorStockEntry
    {
        uint32 DecorId = 0;
        uint32 Price = 0;               // 0 = use the decor row's own price
        uint8 Milestone = 0;            // 0 = always on sale, 1..4 = Endeavor milestone
    };

    struct TC_GAME_API HouseLevelInfo
    {
        uint8 Level = 1;
        uint32 Xp = 0;
        uint32 BudgetIndoor = 0;
        uint32 BudgetRoom = 0;
        uint32 BudgetOutdoor = 0;
        uint8 Rooms = 0;
    };

    struct TC_GAME_API ShellInfo
    {
        uint32 Id = 0;
        std::string Name;
        uint32 GameObjectEntry = 0;
        uint8 SizeClass = SizeStandard;
        uint8 FactionMask = 3;
        uint8 LevelRequired = 1;
        uint32 Flags = 0;
    };

    /// A room is an area of the interior, not separate geometry: the room a player stands
    /// in is the room they may decorate, and the room budget is what pays for it.
    struct TC_GAME_API RoomInfo
    {
        uint32 Id = 0;
        uint32 ShellId = 0;     // 0 = every shell
        std::string Name;
        float X = 0.0f;
        float Y = 0.0f;
        float Z = 0.0f;
        float Radius = 12.0f;
        uint8 BudgetCost = 5;
        uint8 LevelRequired = 1;
    };

    /// A dyed appearance. A game object cannot be recoloured at runtime, so a dye is a
    /// different entry with the same footprint, and only pieces with rows can be dyed.
    struct TC_GAME_API DecorVariant
    {
        uint32 DecorId = 0;
        uint8 DyeGroup = 0;
        uint8 DyeColor = 0;
        uint32 GameObjectEntry = 0;
        std::string DyeName;
    };

    /// The creature a piece of decor owns: a pet bed, a mount stand, a mannequin.
    struct TC_GAME_API PetBedInfo
    {
        uint32 DecorId = 0;
        uint8 Kind = 0;
        uint32 CreatureEntry = 0;
        bool Roaming = false;
        float Radius = 8.0f;
    };

    struct TC_GAME_API HousingPortal
    {
        uint32 Id = 0;
        uint32 AmenityId = 0;
        std::string Name;
        uint32 Map = 0;
        float X = 0.0f;
        float Y = 0.0f;
        float Z = 0.0f;
        float O = 0.0f;
        uint8 FactionMask = 3;
        uint8 LevelRequired = 1;
    };

    struct TC_GAME_API EndeavorTask
    {
        uint32 Id = 0;
        uint32 EndeavorId = 0;
        uint8 Type = TaskDeliver;
        uint32 Target = 0;
        uint32 Required = 1;
        std::string Text;
    };

    struct TC_GAME_API EndeavorReward
    {
        uint32 EndeavorId = 0;
        uint8 Milestone = 1;
        uint32 ItemId = 0;
        uint32 Count = 1;
        uint32 CurrencyId = 0;
        uint32 CurrencyAmount = 0;
        uint32 Xp = 0;
    };

    struct TC_GAME_API EndeavorInfo
    {
        uint32 Id = 0;
        std::string Name;
        std::string Description;
        uint32 DurationDays = 30;
        uint32 BaseRequired = 1;
        std::vector<EndeavorTask> Tasks;
        std::vector<EndeavorReward> Rewards;
    };

} // namespace Housing

#endif // HousingDefines_h__
