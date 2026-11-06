/*
 * Copyright (C) 2022 BfaCore Reforged
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

/*
 * The layer that turns the module from "a plot with furniture in it" into a neighbourhood
 * that behaves like the one players know:
 *
 *   - progression: the twelve-level ladder, XP, exterior shells and the rooms inside them
 *   - storage: owning copies of a piece instead of owning the idea of it
 *   - anchor mode: a plot layout derived around the town centre when the map has no
 *     authored spawn points, so the module runs before the custom map exists
 *   - neighbourhood phase: everything static is spawned in the neighbourhood's own phase
 *   - endeavors: milestone reporting and claim plumbing (the engine is HousingEndeavor)
 *   - build codes, dyes, portals, visiting, reservations and the regret window
 *
 * Kept out of HousingMgr.cpp so that file stays about ownership, phases and access.
 */

#include "HousingMgr.h"

#include "Chat.h"
#include "Config.h"
#include "DatabaseEnv.h"
#include "GameObject.h"
#include "HousingBlueprint.h"
#include "HousingEndeavor.h"
#include "Log.h"
#include "Creature.h"
#include "Map.h"
#include "MapManager.h"
#include "Mails/Mail.h"
#include "ObjectMgr.h"
#include "PhasingHandler.h"
#include "Player.h"
#include "QuaternionData.h"
#include "Timer.h"
#include "Util.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <ctime>

namespace Housing
{
    namespace
    {
        uint64 VariantKey(uint32 decorId, uint8 group, uint8 color)
        {
            return (uint64(decorId) << 16) | (uint64(group) << 8) | uint64(color);
        }

        /// Gold to copper. HousingMgr.cpp has its own copy in its own anonymous namespace;
        /// the two are deliberately identical and deliberately local, because a shared
        /// header for one multiplication is worse than two lines.
        uint64 GoldToCopper(uint32 gold)
        {
            return uint64(gold) * 10000ULL;
        }

        uint64 PlotKey(uint32 mapId, uint8 index)
        {
            return (uint64(mapId) << 8) | uint64(index);
        }
    } // namespace

    // -------------------------------------------------------------------------
    // Loading
    // -------------------------------------------------------------------------

    void HousingMgr::LoadProgression()
    {
        LoadLevels();
        LoadShells();
        LoadRooms();
        LoadPortals();
        LoadVariants();
        LoadDecorCreatures();

        BuildAnchorLayout();
    }

    void HousingMgr::LoadLevels()
    {
        _levels.clear();

        if (QueryResult result = WorldDatabase.Query(
            "SELECT `level`, `xp`, `budget_indoor`, `budget_room`, `budget_outdoor`, `rooms` "
            "FROM `housing_house_level` ORDER BY `level`"))
        {
            do
            {
                Field* fields = result->Fetch();
                HouseLevelInfo info;
                info.Level = fields[0].GetUInt8();
                info.Xp = fields[1].GetUInt32();
                info.BudgetIndoor = fields[2].GetUInt32();
                info.BudgetRoom = fields[3].GetUInt32();
                info.BudgetOutdoor = fields[4].GetUInt32();
                info.Rooms = fields[5].GetUInt8();
                _levels[info.Level] = info;
            }
            while (result->NextRow());
        }

        if (_levels.empty())
        {
            // The module is still usable with no table: the compiled ladder is retail's.
            for (uint8 level = 1; level <= kMaxLevel; ++level)
            {
                HouseLevelInfo info;
                info.Level = level;
                info.Xp = LevelXp(level);
                info.BudgetIndoor = BudgetIndoor(level);
                info.BudgetRoom = BudgetRoom(level);
                info.BudgetOutdoor = BudgetOutdoor(level);
                info.Rooms = RoomCount(level);
                _levels[level] = info;
            }

            TC_LOG_WARN("housing", "housing_house_level is empty - using the compiled ladder");
        }
    }

    void HousingMgr::LoadShells()
    {
        _shells.clear();

        if (QueryResult result = WorldDatabase.Query(
            "SELECT `id`, `name`, `gameobject_entry`, `size_class`, `faction_mask`, "
            "`level_required`, `flags` FROM `housing_shell`"))
        {
            do
            {
                Field* fields = result->Fetch();
                ShellInfo info;
                info.Id = fields[0].GetUInt32();
                info.Name = fields[1].GetString();
                info.GameObjectEntry = fields[2].GetUInt32();
                info.SizeClass = fields[3].GetUInt8();
                info.FactionMask = fields[4].GetUInt8();
                info.LevelRequired = fields[5].GetUInt8();
                info.Flags = fields[6].GetUInt32();
                _shells[info.Id] = info;
            }
            while (result->NextRow());
        }
    }

    void HousingMgr::LoadRooms()
    {
        _rooms.clear();

        if (QueryResult result = WorldDatabase.Query(
            "SELECT `id`, `shell_id`, `name`, `x`, `y`, `z`, `radius`, `budget_cost`, "
            "`level_required` FROM `housing_room`"))
        {
            do
            {
                Field* fields = result->Fetch();
                RoomInfo info;
                info.Id = fields[0].GetUInt32();
                info.ShellId = fields[1].GetUInt32();
                info.Name = fields[2].GetString();
                info.X = fields[3].GetFloat();
                info.Y = fields[4].GetFloat();
                info.Z = fields[5].GetFloat();
                info.Radius = fields[6].GetFloat();
                info.BudgetCost = fields[7].GetUInt8();
                info.LevelRequired = fields[8].GetUInt8();
                _rooms[info.Id] = info;
            }
            while (result->NextRow());
        }
    }

    void HousingMgr::LoadPortals()
    {
        _portals.clear();
        _portalList.clear();

        if (QueryResult result = WorldDatabase.Query(
            "SELECT `id`, `amenity_id`, `name`, `map`, `x`, `y`, `z`, `o`, `faction_mask`, "
            "`level_required` FROM `housing_portal`"))
        {
            do
            {
                Field* fields = result->Fetch();
                HousingPortal portal;
                portal.Id = fields[0].GetUInt32();
                portal.AmenityId = fields[1].GetUInt32();
                portal.Name = fields[2].GetString();
                portal.Map = fields[3].GetUInt32();
                portal.X = fields[4].GetFloat();
                portal.Y = fields[5].GetFloat();
                portal.Z = fields[6].GetFloat();
                portal.O = fields[7].GetFloat();
                portal.FactionMask = fields[8].GetUInt8();
                portal.LevelRequired = fields[9].GetUInt8();
                _portals[portal.Id] = portal;
                _portalList.push_back(portal);
            }
            while (result->NextRow());
        }
    }

    void HousingMgr::LoadVariants()
    {
        _variants.clear();

        if (QueryResult result = WorldDatabase.Query(
            "SELECT `decor_id`, `dye_group`, `dye_color`, `dye_name`, `gameobject_entry` "
            "FROM `housing_decor_variant`"))
        {
            do
            {
                Field* fields = result->Fetch();
                DecorVariant variant;
                variant.DecorId = fields[0].GetUInt32();
                variant.DyeGroup = fields[1].GetUInt8();
                variant.DyeColor = fields[2].GetUInt8();
                variant.DyeName = fields[3].GetString();
                variant.GameObjectEntry = fields[4].GetUInt32();
                _variants[VariantKey(variant.DecorId, variant.DyeGroup, variant.DyeColor)] = variant;
            }
            while (result->NextRow());
        }
    }

    void HousingMgr::LoadDecorCreatures()
    {
        _decorCreatures.clear();

        if (QueryResult result = WorldDatabase.Query(
            "SELECT `decor_id`, `kind`, `creature_entry`, `roaming`, `radius` "
            "FROM `housing_decor_creature`"))
        {
            do
            {
                Field* fields = result->Fetch();
                PetBedInfo info;
                info.DecorId = fields[0].GetUInt32();
                info.Kind = fields[1].GetUInt8();
                info.CreatureEntry = fields[2].GetUInt32();
                info.Roaming = fields[3].GetUInt32() != 0;
                info.Radius = fields[4].GetFloat();
                _decorCreatures[info.DecorId] = info;
            }
            while (result->NextRow());
        }
    }

    // -------------------------------------------------------------------------
    // Lookups
    // -------------------------------------------------------------------------

    HouseLevelInfo const* HousingMgr::GetHouseLevel(uint8 level) const
    {
        auto const itr = _levels.find(level);
        return itr != _levels.end() ? &itr->second : nullptr;
    }

    ShellInfo const* HousingMgr::GetShell(uint32 id) const
    {
        auto const itr = _shells.find(id);
        return itr != _shells.end() ? &itr->second : nullptr;
    }

    RoomInfo const* HousingMgr::GetRoom(uint32 id) const
    {
        auto const itr = _rooms.find(id);
        return itr != _rooms.end() ? &itr->second : nullptr;
    }

    std::vector<RoomInfo> HousingMgr::GetRoomsForShell(uint32 shellId) const
    {
        std::vector<RoomInfo> out;

        for (auto const& pair : _rooms)
            if (!pair.second.ShellId || pair.second.ShellId == shellId)
                out.push_back(pair.second);

        std::sort(out.begin(), out.end(), [](RoomInfo const& left, RoomInfo const& right)
        {
            return left.LevelRequired != right.LevelRequired ? left.LevelRequired < right.LevelRequired
                                                             : left.Id < right.Id;
        });

        return out;
    }

    DecorVariant const* HousingMgr::GetVariant(uint32 decorId, uint8 group, uint8 color) const
    {
        auto const itr = _variants.find(VariantKey(decorId, group, color));
        return itr != _variants.end() ? &itr->second : nullptr;
    }

    bool HousingMgr::HasVariants(uint32 decorId) const
    {
        for (auto const& pair : _variants)
            if (pair.second.DecorId == decorId)
                return true;

        return false;
    }

    PetBedInfo const* HousingMgr::GetDecorCreature(uint32 decorId) const
    {
        auto const itr = _decorCreatures.find(decorId);
        return itr != _decorCreatures.end() ? &itr->second : nullptr;
    }

    HousingPortal const* HousingMgr::GetPortal(uint32 id) const
    {
        auto const itr = _portals.find(id);
        return itr != _portals.end() ? &itr->second : nullptr;
    }

    // -------------------------------------------------------------------------
    // Anchor mode
    // -------------------------------------------------------------------------

    Position HousingMgr::AnchorForPlot(uint8 plotIndex) const
    {
        // Concentric rings. Plot 0 starts due north; each ring is further out and holds two
        // more plots than the one inside it. The maths is deliberately dull: a layout that
        // can be recomputed from an index needs no table and no map editor.
        uint8 remaining = plotIndex;
        uint8 ring = 0;
        uint8 inRing = 0;
        uint8 perRing = kAnchorPerRing;

        while (remaining >= perRing && ring < 24)
        {
            remaining = uint8(remaining - perRing);
            ++ring;
            inRing += perRing;
            perRing = uint8(perRing + kAnchorRingGrowth);
        }

        float const radius = kAnchorRingStart + float(ring) * kAnchorRingSpacing;
        float const angle = 6.2831853f * float(remaining) / float(perRing) + float(ring) * 0.35f;

        return Position(_townCenter.m_positionX + std::cos(angle) * radius,
            _townCenter.m_positionY + std::sin(angle) * radius,
            _townCenter.m_positionZ,
            angle + 3.1415927f);
    }

    Position HousingMgr::AnchorForAmenity(uint32 index) const
    {
        // Amenities sit on the inner ring, evenly spaced, so the town centre is a ring of
        // objects around the teleport anchor rather than twenty-two things at 0,0,0.
        uint32 const count = std::max<uint32>(1, uint32(_amenityList.size()));
        float const angle = 6.2831853f * float(index % count) / float(count);

        return Position(_townCenter.m_positionX + std::cos(angle) * kAnchorAmenityRadius,
            _townCenter.m_positionY + std::sin(angle) * kAnchorAmenityRadius,
            _townCenter.m_positionZ, angle + 3.1415927f);
    }

    void HousingMgr::BuildAnchorLayout()
    {
        // Every template without a spawn gets one, in either mode. A native map with a plot
        // row that has not been authored yet would otherwise ship an unbuyable plot, and the
        // difference between "the map is finished" and "nobody filled this row in" is not
        // something the row itself can show, so the log line says which one it was.
        uint32 filled = 0;
        for (auto& entry : _plotTemplates)
        {
            PlotTemplate& tmpl = entry.second;
            if (tmpl.HasSpawn)
                continue;

            tmpl.Spawn = AnchorForPlot(tmpl.Index);
            tmpl.HasSpawn = true;
            ++filled;
        }

        if (_nativeMap && filled)
            TC_LOG_ERROR("housing", "{} plot template(s) have no authored spawn and were placed on "
                "the anchor layout instead - author them in housing_plot before going live", filled);

        if (_nativeMap)
            return;

        uint32 amenityIndex = 0;
        for (AmenityInfo& amenity : _amenityList)
        {
            if (amenity.Pos.m_positionX == 0.0f && amenity.Pos.m_positionY == 0.0f)
                amenity.Pos = AnchorForAmenity(amenityIndex);

            ++amenityIndex;
        }

        for (auto& pair : _amenities)
            if (pair.second.Pos.m_positionX == 0.0f && pair.second.Pos.m_positionY == 0.0f)
                pair.second.Pos = AnchorForAmenity(pair.second.Id);

        if (filled || !_amenityList.empty())
            TC_LOG_INFO("housing", "anchor mode: {} plot spawn(s) and {} amenity position(s) derived "
                "around the town centre", filled, uint32(_amenityList.size()));
    }

    // -------------------------------------------------------------------------
    // Level and XP
    // -------------------------------------------------------------------------

    uint8 HousingMgr::GetLevelForHousing(uint32 housingId) const
    {
        if (!housingId)
            return 1;

        if (QueryResult result = CharacterDatabase.Query(
            "SELECT `tier` FROM `character_housing` WHERE `id` = {}", housingId))
            return std::max<uint8>(1, result->Fetch()[0].GetUInt8());

        return 1;
    }

    uint32 HousingMgr::GetXpForHousing(uint32 housingId) const
    {
        if (!housingId)
            return 0;

        if (QueryResult result = CharacterDatabase.Query(
            "SELECT `xp` FROM `character_housing` WHERE `id` = {}", housingId))
            return result->Fetch()[0].GetUInt32();

        return 0;
    }

    void HousingMgr::GrantHouseXp(Player* player, uint32 xp)
    {
        if (!player || !xp)
            return;

        HousingPlot* plot = GetOwnedPlot(player);
        if (!plot || !plot->IsOwned())
            return;

        HousingHouse* house = plot->GetHouse();
        if (!house)
            return;

        uint8 const oldLevel = house->GetTier();
        uint32 const total = GetXpForHousing(plot->GetHousingId()) + xp;
        uint8 const newLevel = LevelForXp(total);

        // The XP goes to the database first: a level that only exists in memory is lost on
        // the next restart, and the player has already been told about it.
        CharacterDatabase.DirectPExecute(
            "UPDATE `character_housing` SET `xp` = {}, `tier` = {} WHERE `id` = {}",
            total, uint32(std::max<uint8>(newLevel, oldLevel)), plot->GetHousingId());

        if (newLevel > oldLevel)
        {
            house->SetTier(std::max<uint8>(newLevel, oldLevel));
            house->MarkDirty();

            if (player->GetSession())
                ChatHandler(player->GetSession()).PSendSysMessage(
                    "Your house reached level %u.", uint32(newLevel));
        }
    }

    HousingResult HousingMgr::LevelUpWithGold(Player* player)
    {
        if (!_enabled)
            return Disabled;

        HousingPlot* plot = GetOwnedPlot(player);
        if (!plot || !plot->IsOwned())
            return NoHouse;

        HousingHouse* house = plot->GetHouse();
        if (!house)
            return NoHouse;

        uint8 const level = house->GetTier();
        if (level >= kMaxLevel)
            return TierMax;

        uint64 const cost = GoldToCopper(LevelSkipCost(level));
        if (!player->HasEnoughMoney(int64(cost)))
            return NotEnoughMoney;

        player->ModifyMoney(-int64(cost));

        uint8 const next = uint8(level + 1);
        CharacterDatabase.DirectPExecute(
            "UPDATE `character_housing` SET `tier` = {}, `xp` = {} WHERE `id` = {}",
            uint32(next), LevelXp(next), plot->GetHousingId());

        house->SetTier(next);
        house->MarkDirty();

        return Ok;
    }

    // -------------------------------------------------------------------------
    // Result names
    // -------------------------------------------------------------------------

    char const* HousingMgr::ResultName(HousingResult result)
    {
        switch (result)
        {
            case Ok:                return "ok";
            case Disabled:          return "housing is disabled";
            case NotInNeighborhood: return "not in a neighbourhood";
            case InvalidPlot:       return "invalid plot";
            case PlotTaken:         return "plot taken";
            case NotOwner:          return "not yours";
            case NoHouse:           return "no house";
            case NotEnoughMoney:    return "not enough money";
            case BudgetExceeded:    return "decor budget exceeded";
            case InvalidDecor:      return "unknown decor";
            case NotUnlocked:       return "not collected";
            case OutOfBounds:       return "outside the plot";
            case WrongSurface:      return "wrong surface";
            case BadScale:          return "scale out of range";
            case TooManyDecor:      return "too many pieces";
            case WrongFaction:      return "wrong faction";
            case TooManyHouses:     return "already own a house";
            case NeighborhoodFull:  return "neighbourhood full";
            case TierTooLow:        return "not an upgrade";
            case TierMax:           return "already the highest level";
            case NotFound:          return "not found";
            case GuestRestricted:   return "not allowed in";
            case NotInGuild:        return "not in a guild";
            case GuildRankTooLow:   return "guild rank too low";
            case WrongNeighborhood: return "wrong neighbourhood";
            case NotReady:          return "not ready";
            case OnCooldown:        return "on cooldown";
            case LevelTooLow:       return "house level too low";
            case StorageEmpty:      return "no copy in storage";
            case EndeavorIncomplete:return "endeavor not complete";
            case BadCode:           return "bad code";
            case ArchiveExpired:    return "the archive has expired";
            case NotGuild:          return "that is not a guild neighbourhood";
            case AlreadyKnown:      return "already done";
            default:                return "unknown";
        }
    }

    // -------------------------------------------------------------------------
    // Membership
    // -------------------------------------------------------------------------

    HousingResult HousingMgr::FoundNeighborhood(Player* player)
    {
        if (!_enabled)
            return Disabled;

        if (!player || !player->GetSession())
            return InternalError;

        uint32 const accountId = player->GetSession()->GetAccountId();

        // One founded neighbourhood per account. A player who wants a second one has to leave
        // the first: otherwise a single account can allocate 255 slots and lock everyone else
        // out of the phase space.
        for (auto const& entry : _neighborhoods)
            if (entry.second->GetOwnerAccount() == accountId
                && entry.second->GetType() != NeighborhoodPublic)
                return AlreadyKnown;

        uint8 const faction = uint8(player->GetTeamId() == TEAM_HORDE ? FactionHorde : FactionAlliance);

        uint64 const guildId = player->GetGuildId();
        uint8 const type = guildId ? NeighborhoodGuild : NeighborhoodPrivate;

        if (type == NeighborhoodGuild)
        {
            Guild* guild = sGuildMgr->GetGuildById(guildId);
            if (!guild || guild->GetLeaderGUID() != player->GetGUID())
                return GuildRankTooLow;
        }

        std::string const name = GenerateNeighborhoodName(faction);

        HousingNeighborhood* neighborhood = CreateNeighborhood(type, faction, name, guildId, accountId);
        if (!neighborhood)
            return NeighborhoodFull;

        TC_LOG_INFO("housing", "Account %u founded %s neighbourhood %u ('%s')",
            accountId, type == NeighborhoodGuild ? "a guild" : "a private",
            neighborhood->GetId(), name.c_str());

        return Ok;
    }

    HousingResult HousingMgr::LeaveNeighborhood(Player* player)
    {
        if (!_enabled || !player || !player->GetSession())
            return Disabled;

        uint32 const accountId = player->GetSession()->GetAccountId();
        HousingNeighborhood* neighborhood = FindNeighborhoodFor(player);

        if (!neighborhood)
            return NotInNeighborhood;

        // The founder cannot walk out of a neighbourhood that still has residents or plots:
        // the street would be orphaned. They dissolve it instead, which sells the plots first.
        if (neighborhood->GetOwnerAccount() == accountId
            && (neighborhood->GetOwnedCount() || neighborhood->GetMembers().size() > 1))
            return NotAllowedHere;

        if (neighborhood->GetType() == NeighborhoodGuild)
            return NotGuild;   // a guild neighbourhood is left by leaving the guild

        neighborhood->RemoveMember(accountId);

        PlayerState* state = GetPlayerState(player);
        if (state)
            state->NeighborhoodId = 0;

        return Ok;
    }

    namespace
    {
        /// The rank a member needs to do something to somebody else. Founder only manages
        /// officers; an officer manages residents.
        bool CanManage(uint8 actorRank, uint8 targetRank, bool changingRank)
        {
            if (actorRank < 2)
                return false;

            if (actorRank == 3)
                return changingRank ? targetRank < 3 : targetRank <= 2;

            return !changingRank && targetRank <= 1;
        }
    }

    HousingResult HousingMgr::InviteToNeighborhood(Player* player, std::string const& name)
    {
        if (!_enabled || !player || !player->GetSession())
            return Disabled;

        HousingNeighborhood* neighborhood = FindNeighborhoodFor(player);
        if (!neighborhood)
            return NotInNeighborhood;

        uint32 const accountId = player->GetSession()->GetAccountId();
        if (!CanManage(neighborhood->GetRank(accountId), 1, false))
            return NotOwner;

        if (_populationCap && GetNeighborhoodPopulation(neighborhood) >= _populationCap)
            return NeighborhoodFull;

        Player* target = ObjectAccessor::FindPlayerByName(name);
        if (!target || !target->GetSession())
            return NotFound;

        if (target->GetTeamId() != player->GetTeamId())
            return WrongFaction;

        if (neighborhood->IsMember(target->GetSession()->GetAccountId()))
            return AlreadyKnown;

        neighborhood->AddMember(target->GetSession()->GetAccountId(), 1);

        ChatHandler(target->GetSession()).PSendSysMessage(
            "You have been invited to the neighbourhood '{}'.", neighborhood->GetName());

        return Ok;
    }

    HousingResult HousingMgr::SetMemberRank(Player* player, std::string const& name, uint8 rank)
    {
        if (!_enabled || !player || !player->GetSession())
            return Disabled;

        if (rank < 1 || rank > 2)
            return InvalidPlot;   // rank 3 is the founder and is not assignable

        HousingNeighborhood* neighborhood = FindNeighborhoodFor(player);
        if (!neighborhood)
            return NotInNeighborhood;

        uint32 const accountId = player->GetSession()->GetAccountId();
        if (neighborhood->GetRank(accountId) != 3)
            return NotOwner;

        Player* target = ObjectAccessor::FindPlayerByName(name);
        if (!target || !target->GetSession())
            return NotFound;

        uint32 const targetAccount = target->GetSession()->GetAccountId();
        if (!neighborhood->IsMember(targetAccount))
            return NotFound;

        if (!CanManage(3, neighborhood->GetRank(targetAccount), true))
            return NotAllowedHere;

        neighborhood->AddMember(targetAccount, rank);
        return Ok;
    }

    HousingResult HousingMgr::RemoveFromNeighborhood(Player* player, std::string const& name)
    {
        if (!_enabled || !player || !player->GetSession())
            return Disabled;

        HousingNeighborhood* neighborhood = FindNeighborhoodFor(player);
        if (!neighborhood)
            return NotInNeighborhood;

        uint32 const accountId = player->GetSession()->GetAccountId();
        Player* target = ObjectAccessor::FindPlayerByName(name);
        if (!target || !target->GetSession())
            return NotFound;

        uint32 const targetAccount = target->GetSession()->GetAccountId();

        if (targetAccount == accountId)
            return NotAllowedHere;   // use .housing leave

        if (!CanManage(neighborhood->GetRank(accountId), neighborhood->GetRank(targetAccount), false))
            return NotOwner;

        // A resident's house goes back on the market when they are removed: the plot is the
        // membership, and an ex-resident keeping a plot in the street is not a street.
        if (HousingPlot* plot = neighborhood->FindPlotOfAccount(targetAccount))
            ForceSellPlot(plot);

        neighborhood->RemoveMember(targetAccount);
        return Ok;
    }

    // -------------------------------------------------------------------------
    // Move house
    // -------------------------------------------------------------------------

    HousingResult HousingMgr::MoveHouse(Player* player, uint8 plotIndex)
    {
        if (!_enabled || !player || !player->GetSession())
            return Disabled;

        HousingPlot* from = GetOwnedPlot(player);
        if (!from || !from->IsOwned())
            return NoHouse;

        HousingNeighborhood* neighborhood = from->GetNeighborhood();
        if (!neighborhood)
            return NotInNeighborhood;

        if (plotIndex == from->GetIndex())
            return Ok;

        HousingPlot* to = neighborhood->GetPlot(plotIndex);
        if (!to)
            return InvalidPlot;

        if (to->IsOwned())
            return PlotTaken;

        if (!to->GetTemplate().HasSpawn)
            return NotReady;   // a plot with no door cannot be moved into

        // The furniture moves with the house because it never moved in the first place: the
        // placement rows are keyed on the housing id, not on the plot index. What changes is
        // which plot points at that housing id.
        uint32 const housingId = from->GetHousingId();
        uint32 const accountId = from->GetOwnerAccount();

        HousingHouse* house = from->GetHouse();
        uint8 const tier = house ? house->GetTier() : 1;
        uint32 const blueprint = house ? house->GetBlueprint() : 0;
        std::string const name = house ? house->GetName() : std::string();
        bool const locked = house ? house->IsLocked() : false;
        uint8 const shellId = house ? house->GetShellId() : 0;
        uint8 const rooms = house ? house->GetRoomsOwned() : 0;
        bool const night = house ? house->IsNightMode() : false;

        if (house)
            house->DespawnAll();

        CharacterDatabase.DirectPExecute(
            "UPDATE `character_housing` SET `plotIndex` = {}, `last_edited` = {} WHERE `id` = {}",
            uint32(plotIndex), uint32(std::time(nullptr)), housingId);

        from->ClearOwner();

        to->SetOwner(housingId, accountId, player->GetGUID(), player->GetName(), tier, blueprint,
            name, locked);

        if (HousingHouse* moved = to->GetHouse())
        {
            moved->SetShellId(shellId);
            moved->SetRoomsOwned(rooms);
            moved->SetNightMode(night);
        }

        _houseViewers.erase(housingId);

        PlayerState* state = GetPlayerState(player);
        if (state && state->InsidePlot == from->GetIndex())
            state->InsidePlot = plotIndex;

        ApplyPlotPhase(player, to);

        if (Map* map = player->GetMap())
            if (to->GetHouse())
                to->GetHouse()->EnsureSpawned(map, to->GetPlotPhase(), player->GetGUID());

        return Ok;
    }

    // -------------------------------------------------------------------------
    // Gryphon
    // -------------------------------------------------------------------------

    HousingResult HousingMgr::CallGryphon(Player* player)
    {
        if (!_enabled || !player || !player->GetSession())
            return Disabled;

        HousingPlot* plot = GetOwnedPlot(player);
        if (!plot || !plot->IsOwned())
            return NoHouse;

        uint32 const accountId = player->GetSession()->GetAccountId();
        uint32 const now = getMSTime();

        auto const used = _gryphonUsed.find(accountId);
        if (used != _gryphonUsed.end() && (now - used->second) < _gryphonCooldown * 1000)
            return OnCooldown;

        uint64 const cost = GoldToCopper(_gryphonCost);
        if (cost && !player->HasEnoughMoney(int64(cost)))
            return NotEnoughMoney;

        if (cost)
            player->ModifyMoney(-int64(cost));

        HousingResult const result = TeleportToPlot(player, plot->GetIndex());
        if (result == Ok)
            _gryphonUsed[accountId] = now;

        return result;
    }

    // -------------------------------------------------------------------------
    // Guild wings
    // -------------------------------------------------------------------------

    HousingNeighborhood* HousingMgr::GetOrCreateGuildWing(uint64 guildId, uint8 faction)
    {
        if (!_enabled || !guildId || !_guildWings)
            return nullptr;

        // The first guild neighbourhood is created by the founder; every later one is a wing
        // with its own slot, so a guild of two hundred gets four streets rather than a queue.
        HousingNeighborhood* best = nullptr;
        uint32 bestId = 0;

        for (auto const& entry : _neighborhoods)
        {
            HousingNeighborhood* candidate = entry.second.get();
            if (candidate->GetType() != NeighborhoodGuild || candidate->GetGuildId() != guildId)
                continue;

            if (!candidate->FindFreePlot())
                continue;

            if (!best || candidate->GetId() < bestId)
            {
                best = candidate;
                bestId = candidate->GetId();
            }
        }

        if (best)
            return best;

        std::string const name = Trinity::StringFormat("{} Wing", GenerateNeighborhoodName(faction));
        return CreateNeighborhood(NeighborhoodGuild, faction, name, guildId, 0);
    }

    // -------------------------------------------------------------------------
    // Rooms
    // -------------------------------------------------------------------------

    std::vector<RoomInfo> HousingMgr::GetRoomsForPlot(HousingPlot* plot) const
    {
        uint8 const shellId = (plot && plot->GetHouse()) ? plot->GetHouse()->GetShellId() : 0;
        return GetRoomsForShell(shellId);
    }

    HousingResult HousingMgr::CanBuyRoom(Player* player, uint32 roomId) const
    {
        RoomInfo const* room = GetRoom(roomId);
        if (!room)
            return NotFound;

        if (!player)
            return InternalError;

        HousingPlot* plot = const_cast<HousingMgr*>(this)->GetOwnedPlot(player);
        if (!plot || !plot->IsOwned())
            return NoHouse;

        HousingHouse* house = plot->GetHouse();
        if (!house)
            return NoHouse;

        if (house->GetTier() < room->LevelRequired)
            return LevelTooLow;

        if (room->ShellId && house->GetShellId() && room->ShellId != house->GetShellId())
            return InvalidPlot;

        return Ok;
    }

    HousingResult HousingMgr::BuyRoom(Player* player, uint32 roomId)
    {
        if (!_enabled)
            return Disabled;

        HousingResult const allowed = CanBuyRoom(player, roomId);
        if (allowed != Ok)
            return allowed;

        RoomInfo const* room = GetRoom(roomId);

        HousingPlot* plot = GetOwnedPlot(player);
        HousingHouse* house = plot ? plot->GetHouse() : nullptr;
        if (!house)
            return NoHouse;

        HouseLevelInfo const* level = GetHouseLevel(house->GetTier());
        if (!level)
            return NotFound;

        // A room costs budget the same way a chair does, and the budget is what the level
        // hands out - so a level-3 house cannot buy an 18th room by owning fewer chairs.
        uint32 const spent = house->GetRoomsOwned() * room->BudgetCost;
        if (spent + room->BudgetCost > level->BudgetRoom)
            return BudgetExceeded;

        house->SetRoomsOwned(uint8(house->GetRoomsOwned() + 1));

        CharacterDatabase.DirectPExecute(
            "UPDATE `character_housing` SET `rooms_owned` = {} WHERE `id` = {}",
            uint32(house->GetRoomsOwned()), plot->GetHousingId());

        return Ok;
    }

    // -------------------------------------------------------------------------
    // Arcanostorage Sphere
    // -------------------------------------------------------------------------

    HousingResult HousingMgr::OpenArcanostorage(Player* player)
    {
        if (!_enabled || !player || !player->GetSession())
            return Disabled;

        HousingPlot* plot = GetOwnedPlot(player);
        if (!plot || !plot->GetHouse())
            return NoHouse;

        if (plot->GetHouse()->GetTier() < _arcaneLevel)
            return LevelTooLow;

        uint32 const accountId = player->GetSession()->GetAccountId();
        uint32 const now = uint32(std::time(nullptr));

        auto const used = _arcaneUsed.find(accountId);
        if (used != _arcaneUsed.end() && (now - used->second) < _arcaneCooldownDays * 86400)
            return OnCooldown;

        // The roll is over the pieces this house can already have: an account that has not
        // collected a piece cannot be handed it, because there is nothing to hand over.
        std::vector<uint32> pool;
        for (auto const& entry : _decor)
        {
            DecorInfo const& info = entry.second;
            if (info.Flags & DecorFlagHidden)
                continue;
            if (info.FactionMask & (player->GetTeamId() == TEAM_ALLIANCE ? 0x01 : 0x02))
                pool.push_back(info.Id);
        }

        if (pool.empty())
            return NotFound;

        uint32 const chosen = pool[urand(0, uint32(pool.size() - 1))];

        AddStorage(accountId, chosen, 1);

        if (!HasUnlocked(accountId, chosen))
        {
            CharacterDatabase.DirectPExecute(
                "REPLACE INTO `character_housing_collection` (`accountId`, `decorId`, `source_type`, "
                "`source_id`, `acquired`) VALUES ({}, {}, 2, 0, UNIX_TIMESTAMP())",
                accountId, chosen);
        }

        _arcaneUsed[accountId] = now;

        if (DecorInfo const* info = GetDecor(chosen))
            if (player->GetSession())
                ChatHandler(player->GetSession()).PSendSysMessage(
                    "The Sphere gives up a copy of %s.", info->Name.c_str());

        return Ok;
    }

    // -------------------------------------------------------------------------
    // Hearthlight Armillary
    // -------------------------------------------------------------------------

    HousingResult HousingMgr::ToggleArmillary(Player* player)
    {
        if (!_enabled || !player || !player->GetSession())
            return Disabled;

        HousingPlot* plot = GetOwnedPlot(player);
        if (!plot || !plot->GetHouse())
            return NoHouse;

        HousingHouse* house = plot->GetHouse();

        if (house->GetTier() < _armillaryLevel)
            return LevelTooLow;

        if (!CanEdit(player, plot))
            return NotOwner;

        bool const night = !house->IsNightMode();
        house->SetNightMode(night);

        CharacterDatabase.DirectPExecute(
            "UPDATE `character_housing` SET `night_mode` = {} WHERE `id` = {}",
            uint32(night ? 1 : 0), plot->GetHousingId());

        // The ambience is a phase, not a shader. Every lighting piece in the catalogue that
        // has a lit variant spawns as that variant while the Armillary is lit, and the world
        // state below tells the client's UI which way round it is.
        if (HousingHouse* owned = plot->GetHouse())
        {
            owned->DespawnAll();

            if (Map* map = player->GetMap())
                owned->EnsureSpawned(map, plot->GetInteriorPhase(), player->GetGUID());
        }

        player->SendUpdateWorldState(kArmillaryWorldState, night ? 1 : 0);

        return Ok;
    }

    // -------------------------------------------------------------------------
    // Endeavor favour and scaling
    // -------------------------------------------------------------------------

    uint32 HousingMgr::GetFavor(uint32 neighborhoodId) const
    {
        if (!neighborhoodId)
            return 0;

        HousingNeighborhood* neighborhood = const_cast<HousingMgr*>(this)->GetNeighborhood(neighborhoodId);
        if (!neighborhood)
            return 0;

        EndeavorInfo const* info = sHousingEndeavor->GetInfo(sHousingEndeavor->GetActiveId(neighborhoodId));
        if (!info || info->Tasks.empty())
            return 0;

        uint32 const done = sHousingEndeavor->GetCompletedTasks(neighborhoodId);
        return std::min<uint32>(1000, (done * 1000) / uint32(info->Tasks.size()));
    }

    void HousingMgr::SendFavor(Player* player, uint32 neighborhoodId)
    {
        if (!player)
            return;

        player->SendUpdateWorldState(kFavorWorldState, GetFavor(neighborhoodId));
        player->SendUpdateWorldState(kMilestoneWorldState,
            uint32(sHousingEndeavor->GetMilestone(neighborhoodId)));
    }

    void HousingMgr::GrantSourceDecor(Player* player, uint8 sourceType, uint32 sourceId)
    {
        if (!_enabled || !player || !sourceId)
            return;

        uint32 const accountId = player->GetSession()->GetAccountId();
        LoadCollection(accountId);

        uint8 const factionBit = player->GetTeamId() == TEAM_ALLIANCE ? 0x01 : 0x02;
        uint32 granted = 0;

        for (auto const& entry : _decor)
        {
            DecorInfo const& info = entry.second;

            if (info.SourceType != sourceType || info.SourceId != sourceId)
                continue;

            if ((info.Flags & DecorFlagHidden) || !(info.FactionMask & factionBit))
                continue;

            if (HasUnlocked(accountId, info.Id))
                continue;

            CharacterDatabase.DirectPExecute(
                "REPLACE INTO `character_housing_collection` (`accountId`, `decorId`, `source_type`, "
                "`source_id`, `acquired`) VALUES ({}, {}, {}, {}, UNIX_TIMESTAMP())",
                accountId, info.Id, uint32(info.SourceType), info.SourceId);

            _collection[accountId].insert(info.Id);
            AddStorage(accountId, info.Id, 1);
            ++granted;
        }

        if (!granted)
            return;

        TC_LOG_INFO("housing", "account {} earned {} piece(s) of decor from source type {} id {}",
            accountId, granted, uint32(sourceType), sourceId);

        GrantHouseXp(player, kXpPerNewDecor * granted);
        SendChunk(player, Trinity::StringFormat("S:RESULT|DECOR|0|{}", granted));
    }

    uint32 HousingMgr::ScaledRequirement(uint32 baseRequired, uint32 neighborhoodId) const
    {
        HousingNeighborhood* neighborhood =
            const_cast<HousingMgr*>(this)->GetNeighborhood(neighborhoodId);

        if (!neighborhood || !baseRequired)
            return baseRequired;

        // A street of three and a street of thirty should not need the same number of kills.
        // The factor is clamped so a nearly empty neighbourhood is still playable and a huge
        // one is not punished with an impossible number.
        uint32 const residents = std::max<uint32>(1, GetNeighborhoodPopulation(neighborhood));
        float const factor = std::min(3.0f, std::max(0.5f, float(residents) / 10.0f));

        return std::max<uint32>(1, uint32(float(baseRequired) * factor));
    }

    void HousingMgr::SpawnEndeavorTheming(HousingNeighborhood* neighborhood, Map* map,
        uint32 endeavorId, uint8 milestone)
    {
        if (!neighborhood || !map || !endeavorId || !milestone)
            return;

        // Theming is data: the Endeavor's own name picks the pieces, and the pieces are the
        // town-centre props that already exist. Four milestones, each adding one prop, so a
        // street that has finished everything looks finished.
        uint8 const count = std::min<uint8>(milestone, 4);

        for (uint8 step = 0; step < count; ++step)
        {
            uint32 const amenityId = 1 + uint32(step % std::max<size_t>(1, _amenityList.size()));
            AmenityInfo const* amenity = GetAmenity(amenityId);
            if (!amenity)
                continue;

            if (!amenity->GameObjectEntry)
                continue;

            Position const pos = AnchorForAmenity(uint32(_amenityList.size()) + step);
            QuaternionData const rotation = QuaternionData::fromEulerAnglesZYX(pos.GetOrientation(), 0.0f, 0.0f);

            if (GameObject* object = GameObject::CreateGameObject(amenity->GameObjectEntry, map, pos,
                    rotation, 255, GO_STATE_READY))
            {
                PhasingHandler::AddPhase(object, GetNeighborhoodPhaseOf(neighborhood->GetId()), false);
                map->AddToMap(object);
            }
        }
    }

    HousingResult HousingMgr::ApplyShell(Player* player, uint32 shellId)
    {
        if (!_enabled)
            return Disabled;

        ShellInfo const* shell = GetShell(shellId);
        if (!shell)
            return InvalidPlot;

        HousingPlot* plot = GetOwnedPlot(player);
        if (!plot || !plot->IsOwned())
            return NoHouse;

        if (plot->GetOwnerAccount() != player->GetSession()->GetAccountId())
            return NotOwner;

        uint8 const factionBit = player->GetTeamId() == TEAM_ALLIANCE ? 0x01 : 0x02;
        if (!(shell->FactionMask & factionBit))
            return WrongFaction;

        HousingHouse* house = plot->GetHouse();
        if (house && house->GetTier() < shell->LevelRequired)
            return LevelTooLow;

        CharacterDatabase.DirectPExecute(
            "UPDATE `character_housing` SET `shell_id` = {} WHERE `id` = {}",
            shellId, plot->GetHousingId());

        if (HousingNeighborhood* neighborhood = plot->GetNeighborhood())
        {
            if (HousingHouse* owned = plot->GetHouse())
            {
                owned->SetShellId(uint8(shellId));
                owned->MarkDirty();
            }

            if (Map* map = sMapMgr->FindMap(neighborhood->GetMapId(), 0))
                SpawnPlotShell(plot, neighborhood, map);
        }

        return Ok;
    }

    // -------------------------------------------------------------------------
    // Storage
    // -------------------------------------------------------------------------

    std::map<uint32, uint32> const& HousingMgr::GetStorage(uint32 accountId)
    {
        LoadStorage(accountId);
        return _storage[accountId];
    }

    void HousingMgr::LoadStorage(uint32 accountId)
    {
        if (!accountId || _storageLoaded.count(accountId))
            return;

        _storageLoaded.insert(accountId);
        auto& storage = _storage[accountId];

        if (QueryResult result = CharacterDatabase.Query(
            "SELECT `decorId`, `count` FROM `character_housing_storage` WHERE `accountId` = {}",
            accountId))
        {
            do
            {
                Field* fields = result->Fetch();
                uint32 const count = fields[1].GetUInt32();
                if (count)
                    storage[fields[0].GetUInt32()] = count;
            }
            while (result->NextRow());
        }
    }

    uint32 HousingMgr::GetStorageCount(uint32 accountId, uint32 decorId)
    {
        LoadStorage(accountId);

        auto const accountItr = _storage.find(accountId);
        if (accountItr == _storage.end())
            return 0;

        auto const itr = accountItr->second.find(decorId);
        return itr != accountItr->second.end() ? itr->second : 0;
    }

    void HousingMgr::AddStorage(uint32 accountId, uint32 decorId, uint32 count)
    {
        if (!accountId || !decorId || !count)
            return;

        LoadStorage(accountId);

        uint32& owned = _storage[accountId][decorId];
        owned += count;

        CharacterDatabase.DirectPExecute(
            "REPLACE INTO `character_housing_storage` (`accountId`, `decorId`, `count`, `acquired`) "
            "VALUES ({}, {}, {}, UNIX_TIMESTAMP())", accountId, decorId, owned);
    }

    bool HousingMgr::ConsumeStorage(uint32 accountId, uint32 decorId)
    {
        LoadStorage(accountId);

        auto const accountItr = _storage.find(accountId);
        if (accountItr == _storage.end())
            return false;

        auto itr = accountItr->second.find(decorId);
        if (itr == accountItr->second.end() || !itr->second)
            return false;

        if (!--itr->second)
            accountItr->second.erase(itr);

        auto const remaining = accountItr->second.find(decorId);
        uint32 const count = remaining != accountItr->second.end() ? remaining->second : 0;

        CharacterDatabase.DirectPExecute(
            "REPLACE INTO `character_housing_storage` (`accountId`, `decorId`, `count`, `acquired`) "
            "VALUES ({}, {}, {}, UNIX_TIMESTAMP())", accountId, decorId, count);

        return true;
    }

    // -------------------------------------------------------------------------
    // Dyes
    // -------------------------------------------------------------------------

    HousingResult HousingMgr::DyeDecor(Player* player, uint64 placementId, uint8 group, uint8 color)
    {
        if (!_enabled)
            return Disabled;

        HousingPlot* plot = GetPlotForEdit(player);
        if (!plot || !plot->IsOwned())
            return NoHouse;

        if (!CanEdit(player, plot))
            return NotOwner;

        HousingHouse* house = plot->GetHouse();
        if (!house)
            return NoHouse;

        DecorPlacement* placement = house->GetPlacementMutable(placementId);
        if (!placement)
            return NotFound;

        DecorVariant const* variant = GetVariant(placement->DecorId, group, color);
        if (!variant || !variant->GameObjectEntry)
            return NotFound;   // this piece has no dyed appearance; the catalogue says so

        // The dye is written to the row and takes effect the next time the placement is
        // spawned - re-entering the house, or a relog. Recolouring the object that is already
        // standing there would mean removing and re-creating it mid-frame for no gain, and a
        // player who dyes a chair is looking at it from inside the house anyway.
        CharacterDatabase.DirectPExecute(
            "UPDATE `character_housing_decor` SET `dye_group` = {}, `dye_color` = {}, "
            "`variant_entry` = {} WHERE `id` = " UI64FMTD,
            uint32(group), uint32(color), variant->GameObjectEntry, placementId);

        return Ok;
    }

    // -------------------------------------------------------------------------
    // Travel, finder, visit, reservation
    // -------------------------------------------------------------------------

    std::vector<HousingNeighborhood*> HousingMgr::GetPublicNeighborhoods(uint8 faction)
    {
        std::vector<HousingNeighborhood*> out;

        for (auto const& entry : _neighborhoods)
        {
            HousingNeighborhood* neighborhood = entry.second.get();
            if (neighborhood->GetType() != NeighborhoodPublic)
                continue;

            if (faction <= FactionHorde && neighborhood->GetFaction() != faction)
                continue;

            out.push_back(neighborhood);
        }

        std::sort(out.begin(), out.end(), [](HousingNeighborhood* left, HousingNeighborhood* right)
        {
            return left->GetId() < right->GetId();
        });

        return out;
    }

    HousingResult HousingMgr::VisitNeighborhood(Player* player, uint32 neighborhoodId, uint8 plotIndex)
    {
        if (!_enabled)
            return Disabled;

        HousingNeighborhood* neighborhood = GetNeighborhood(neighborhoodId);
        if (!neighborhood)
            return NotInNeighborhood;

        HousingPlot* plot = neighborhood->GetPlot(plotIndex);
        if (!plot || !plot->IsOwned())
            return InvalidPlot;

        if (!plot->GetTemplate().HasSpawn)
            return NotAllowedHere;

        if (CheckAccess(player, plot) < AccessNeighbor)
            return GuestRestricted;

        PlayerState* state = GetPlayerState(player);
        if (!state)
            return InternalError;

        ClearPlotPhases(player, *state);

        state->NeighborhoodId = neighborhood->GetId();
        state->InsidePlot = plotIndex;

        ApplyNeighborhoodPhase(player, neighborhood);
        ApplyPlotPhase(player, plot);

        if (!player->TeleportTo(neighborhood->GetMapId(), PlotSpawn(plot, player)))
        {
            state->InsidePlot = 0xFF;
            return InternalError;
        }

        if (Map* map = player->GetMap())
            SpawnNeighborhoodContent(neighborhood, map);

        if (HousingHouse* house = plot->GetHouse())
            house->EnsureSpawned(player->GetMap(), plot->GetPlotPhase(), plot->GetOwnerGuid());

        return Ok;
    }

    HousingResult HousingMgr::UsePortal(Player* player, uint32 portalId)
    {
        if (!_enabled)
            return Disabled;

        HousingPortal const* portal = GetPortal(portalId);
        if (!portal || !portal->Map)
            return NotFound;

        uint8 const factionBit = player->GetTeamId() == TEAM_ALLIANCE ? 0x01 : 0x02;
        if (!(portal->FactionMask & factionBit))
            return WrongFaction;

        uint8 const level = GetLevelForHousing(GetOwnedPlot(player) ? GetOwnedPlot(player)->GetHousingId() : 0);
        if (level < portal->LevelRequired)
            return LevelTooLow;

        if (!player->TeleportTo(portal->Map, portal->X, portal->Y, portal->Z, portal->O))
            return InternalError;

        return Ok;
    }

    HousingResult HousingMgr::ReservePlot(Player* player, uint8 plotIndex)
    {
        if (!_enabled)
            return Disabled;

        if (plotIndex >= kPlotCount)
            return InvalidPlot;

        uint32 const accountId = player->GetSession()->GetAccountId();
        uint32 const now = getMSTime();

        auto itr = _reservations.find(plotIndex);
        if (itr != _reservations.end())
        {
            if (itr->second.second > now && itr->second.first != accountId)
                return PlotTaken;

            if (itr->second.first == accountId)
            {
                itr->second.second = now + kReservationSeconds * 1000;
                return Ok;
            }
        }

        _reservations[plotIndex] = std::make_pair(accountId, now + kReservationSeconds * 1000);
        return Ok;
    }

    // -------------------------------------------------------------------------
    // Archive - the regret window
    // -------------------------------------------------------------------------

    bool HousingMgr::HasArchive(uint32 accountId)
    {
        if (!accountId)
            return false;

        QueryResult result = CharacterDatabase.Query(
            "SELECT 1 FROM `character_housing_archive` WHERE `accountId` = {} "
            "AND `archived` > (UNIX_TIMESTAMP() - {})", accountId, kArchiveDays * 86400);

        return bool(result);
    }

    HousingResult HousingMgr::RestoreArchivedHouse(Player* player)
    {
        if (!_enabled)
            return Disabled;

        uint32 const accountId = player->GetSession()->GetAccountId();

        QueryResult result = CharacterDatabase.Query(
            "SELECT `id`, `plotIndex`, `neighborhoodId`, `name`, `tier`, `xp`, `shell_id`, `decor` "
            "FROM `character_housing_archive` WHERE `accountId` = {} "
            "AND `archived` > (UNIX_TIMESTAMP() - {}) ORDER BY `archived` DESC LIMIT 1",
            accountId, kArchiveDays * 86400);

        if (!result)
            return NotFound;

        Field* fields = result->Fetch();
        uint32 const archiveId = fields[0].GetUInt32();

        HousingNeighborhood* neighborhood = GetNeighborhood(fields[2].GetUInt32());
        if (!neighborhood)
            return ArchiveExpired;

        uint8 const plotIndex = fields[1].GetUInt8();
        HousingPlot* plot = neighborhood->GetPlot(plotIndex);
        if (!plot || plot->IsOwned())
            return PlotTaken;

        if (HasArchive(accountId) == false)
            return ArchiveExpired;

        // Take the plot back at the archived level. The decor comes back through the code in
        // the `decor` column, the same format a blueprint uses, so there is one importer.
        uint32 const housingId = GenerateHousingId();
        std::string name = fields[3].GetString();
        CharacterDatabase.EscapeString(name);

        CharacterDatabase.DirectPExecute(
            "INSERT INTO `character_housing` (`id`, `neighborhoodId`, `plotIndex`, `ownerAccount`, "
            "`ownerGuid`, `ownerName`, `tier`, `xp`, `shell_id`, `name`, `locked`, `budget_indoor`, "
            "`budget_outdoor`, `budget_room`, `purchase_time`, `last_edited`) "
            "VALUES ({}, {}, {}, {}, {}, '{}', {}, {}, {}, '{}', 0, 0, 0, 0, UNIX_TIMESTAMP(), UNIX_TIMESTAMP())",
            housingId, neighborhood->GetId(), uint32(plotIndex), accountId,
            player->GetGUID().GetCounter(), name, uint32(fields[4].GetUInt8()), fields[5].GetUInt32(),
            fields[6].GetUInt32(), name);

        plot->SetOwner(housingId, accountId, player->GetGUID(), fields[3].GetString(),
            fields[4].GetUInt8(), 0, fields[3].GetString(), false);

        CharacterDatabase.DirectPExecute(
            "DELETE FROM `character_housing_archive` WHERE `id` = {}", archiveId);

        if (!fields[7].IsNull() && !fields[7].GetString().empty())
            ImportBlueprint(player, fields[7].GetString());

        return Ok;
    }

    // -------------------------------------------------------------------------
    // Blueprints
    // -------------------------------------------------------------------------

    HousingResult HousingMgr::ReadBlueprintCode(Player* player, std::string const& code,
        std::vector<DecorPlacement>& out, uint8& kind)
    {
        uint8 faction = 0;
        if (!BlueprintCodec::Decode(code, out, kind, faction))
            return BadCode;

        uint8 const factionBit = player->GetTeamId() == TEAM_ALLIANCE ? 0x01 : 0x02;
        if (kind != BlueprintRoom && !(faction & factionBit) && faction)
            return WrongFaction;

        return Ok;
    }

    HousingResult HousingMgr::ExportBlueprint(Player* player, uint8 kind, std::string const& name,
        std::string& code)
    {
        if (!_enabled)
            return Disabled;

        HousingPlot* plot = GetPlotForEdit(player);
        if (!plot || !plot->IsOwned())
            return NoHouse;

        HousingHouse* house = plot->GetHouse();
        if (!house)
            return NoHouse;

        std::vector<DecorPlacement> picked;
        for (DecorPlacement const& placement : house->GetPlacements())
        {
            if (kind == BlueprintExterior && placement.Indoor)
                continue;

            picked.push_back(placement);
        }

        uint8 const faction = player->GetTeamId() == TEAM_ALLIANCE ? FactionAlliance : FactionHorde;
        code = BlueprintCodec::Encode(picked, kind, faction);

        if (code.empty())
            return NotFound;

        std::string safeName = name;
        CharacterDatabase.EscapeString(safeName);

        CharacterDatabase.DirectPExecute(
            "INSERT INTO `character_housing_blueprint` (`accountId`, `name`, `kind`, `faction`, "
            "`code`, `created`) VALUES ({}, '{}', {}, {}, '{}', UNIX_TIMESTAMP())",
            player->GetSession()->GetAccountId(), safeName, uint32(kind), uint32(faction), code);

        return Ok;
    }

    HousingResult HousingMgr::ImportBlueprint(Player* player, std::string const& code)
    {
        if (!_enabled)
            return Disabled;

        HousingPlot* plot = GetPlotForEdit(player);
        if (!plot || !plot->IsOwned())
            return NoHouse;

        if (!CanEdit(player, plot))
            return NotOwner;

        std::vector<DecorPlacement> placements;
        uint8 kind = BlueprintBuild;

        HousingResult const read = ReadBlueprintCode(player, code, placements, kind);
        if (read != Ok)
            return read;

        HousingHouse* house = plot->GetHouse();
        if (!house)
            return NoHouse;

        uint32 const accountId = player->GetSession()->GetAccountId();
        uint32 placed = 0;

        for (DecorPlacement const& source : placements)
        {
            DecorInfo const* info = GetDecor(source.DecorId);
            if (!info || !GetStorageCount(accountId, source.DecorId))
                continue;   // not unlocked, or no copy left: skipped, and the page says so

            uint64 placementId = GeneratePlacementId();
            if (house->AddPlacement(placementId, *info, source.X, source.Y, source.Z, source.O,
                source.Scale, source.Indoor, plot->GetTemplate().Spawn,
                plot->GetTemplate().YardRadius > 0.0f ? plot->GetTemplate().YardRadius : kDefaultYardRadius) != Ok)
                continue;

            ConsumeStorage(accountId, source.DecorId);
            ++placed;
        }

        return placed ? Ok : StorageEmpty;
    }

    // -------------------------------------------------------------------------
    // Neighbourhood content and rules
    // -------------------------------------------------------------------------

    uint32 HousingMgr::GetNeighborhoodPhaseOf(uint32 neighborhoodId) const
    {
        for (auto const& entry : _neighborhoods)
            if (entry.first == neighborhoodId)
                return NeighborhoodPhase(entry.second->GetSlot());

        return 0;
    }

    void HousingMgr::ApplyNeighborhoodPhase(Player* player, HousingNeighborhood* neighborhood)
    {
        if (!player || !neighborhood || !_plotScopedPhasing)
            return;

        PhasingHandler::AddPhase(player, NeighborhoodPhase(neighborhood->GetSlot()), true);
    }

    uint32 HousingMgr::GetPlayerNeighborhoodId(Player* player)
    {
        if (!player)
            return 0;

        PlayerState* state = GetPlayerState(player);
        if (state && state->NeighborhoodId)
            return state->NeighborhoodId;

        if (HousingPlot* plot = GetOwnedPlot(player))
            if (HousingNeighborhood* neighborhood = plot->GetNeighborhood())
                return neighborhood->GetId();

        if (HousingNeighborhood* neighborhood = FindNeighborhoodFor(player))
            return neighborhood->GetId();

        return 0;
    }

    void HousingMgr::SpawnNeighborhoodContent(HousingNeighborhood* neighborhood, Map* map)
    {
        if (!neighborhood || !map)
            return;

        if (_contentSpawned.count(neighborhood->GetId()))
            return;

        _contentSpawned.insert(neighborhood->GetId());

        uint32 const phase = NeighborhoodPhase(neighborhood->GetSlot());
        uint32 const mapId = neighborhood->GetMapId();

        // Amenities: a pin is data, an object is furniture. Only the ones the realm bound
        // are spawned; an amenity with entry 0 stays a pin on the addon map.
        uint32 index = 0;
        for (AmenityInfo const& amenity : _amenityList)
        {
            Position pos = amenity.Pos;
            if (pos.m_positionX == 0.0f && pos.m_positionY == 0.0f)
                pos = AnchorForAmenity(index);

            if (amenity.GameObjectEntry)
            {
                if (GameObject* object = GameObject::CreateGameObject(amenity.GameObjectEntry, map, pos,
                    QuaternionData::fromEulerAnglesZYX(pos.GetOrientation(), 0.0f, 0.0f), 255, GO_STATE_READY))
                {
                    PhasingHandler::AddPhase(object, phase, false);
                    map->AddToMap(object);
                }
            }

            if (amenity.CreatureEntry)
            {
                if (Creature* creature = map->SummonCreature(amenity.CreatureEntry, pos, nullptr, 0,
                    nullptr, 0, 0, false))
                {
                    PhasingHandler::AddPhase(creature, phase, true);
                    creature->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IMMUNE_TO_PC);
                }
            }

            ++index;
        }

        for (auto const& entry : neighborhood->GetPlots())
        {
            SpawnCornerstone(entry.get(), neighborhood, map);

            if (!entry->IsOwned())
                continue;

            if (entry->GetTemplate().MapId != mapId)
                continue;

            SpawnPlotShell(entry.get(), neighborhood, map);
        }
    }

    /*
     * The stone at the edge of an empty plot. Retail's plot is bought by clicking one, and
     * the stone is the only thing on an empty plot to click, so without it the only door into
     * housing is the addon or a command.
     *
     * The entry is configuration rather than a constant: the object has to exist in the
     * realm's own gameobject table, and no 8.3.7 entry is the retail cornerstone. At 0 the
     * step is skipped and plots are bought from the addon or the Steward, which is how the
     * draft behaves on a fresh install.
     */
    void HousingMgr::SpawnCornerstone(HousingPlot* plot, HousingNeighborhood* neighborhood, Map* map)
    {
        if (!plot || !neighborhood || !map || !_cornerstoneEntry)
            return;

        if (plot->IsOwned())
            return;

        Position const pos = plot->GetTemplate().HasSpawn ? plot->GetTemplate().Spawn
                                                         : AnchorForPlot(plot->GetIndex());

        if (GameObject* object = GameObject::CreateGameObject(_cornerstoneEntry, map, pos,
            QuaternionData::fromEulerAnglesZYX(pos.GetOrientation(), 0.0f, 0.0f), 255, GO_STATE_READY))
        {
            PhasingHandler::AddPhase(object, NeighborhoodPhase(neighborhood->GetSlot()), false);
            map->AddToMap(object);
        }
    }

    void HousingMgr::SpawnPlotShell(HousingPlot* plot, HousingNeighborhood* neighborhood, Map* map)
    {
        if (!plot || !neighborhood || !map || !plot->IsOwned())
            return;

        // The shell is the house itself. Without one the plot is a patch of grass with a
        // door trigger; with one it is a building. Which object is `character_housing.shell_id`
        // -> housing_shell.gameobject_entry, and 0 means the plot has no shell yet.
        if (QueryResult result = CharacterDatabase.Query(
            "SELECT `shell_id` FROM `character_housing` WHERE `id` = {}", plot->GetHousingId()))
        {
            uint32 const shellId = result->Fetch()[0].GetUInt32();
            if (ShellInfo const* shell = GetShell(shellId))
            {
                if (shell->GameObjectEntry && plot->GetTemplate().HasSpawn)
                {
                    Position const pos = plot->GetTemplate().Spawn;

                    if (GameObject* object = GameObject::CreateGameObject(shell->GameObjectEntry, map, pos,
                        QuaternionData::fromEulerAnglesZYX(pos.GetOrientation(), 0.0f, 0.0f), 255, GO_STATE_READY))
                    {
                        PhasingHandler::AddPhase(object, plot->GetPlotPhase(), false);
                        map->AddToMap(object);
                        return;
                    }
                }
            }
        }

        (void)neighborhood;
    }

    void HousingMgr::EnforceBiomeRules(Player* player)
    {
        if (!player || !_enabled)
            return;

        PlayerState* state = GetPlayerState(player);
        if (!state || !state->NeighborhoodId)
            return;

        if (!IsHousingMap(player->GetMapId()) && player->GetMapId() != _interiorMapId)
            return;

        uint32 const now = getMSTime();
        if (state->LastRuleCheck && (now - state->LastRuleCheck) < 2000)
            return;

        state->LastRuleCheck = now;

        HousingNeighborhood* neighborhood = GetNeighborhood(state->NeighborhoodId);
        if (!neighborhood)
            return;

        // The flags come from the biome the player is standing on. Before this the function
        // named both bits itself, which made every row of housing_biome.flags decoration.
        uint32 biomeFlags = BiomeFlagSanctuary;

        if (state->InsidePlot != 0xFF)
            if (HousingPlot* plot = neighborhood->GetPlot(state->InsidePlot))
            {
                if (BiomeInfo const* biome = GetBiome(plot->GetTemplate().BiomeId))
                    biomeFlags = biome->Flags;

                // An island or a bridge-only plot has one way in and no flight path out.
                // Both bits are read here so a plot cannot be used as a flying shortcut.
                if (plot->GetTemplate().Flags & (PlotFlagIsland | PlotFlagBridgeOnly))
                    biomeFlags |= BiomeFlagNoFly;
            }
        else
        {
            // Between plots the town centre keeps the sanctuary, which every neighbourhood
            // has by definition.
            biomeFlags |= BiomeFlagSanctuary;
        }

        // Sanctuary: no PvP on housing ground. Retail's neighbourhood is a sanctuary and a
        // player who arrives flagged should stop being a target, not stay one.
        if (biomeFlags & BiomeFlagSanctuary)
        {
            if (player->IsPvP())
                player->SetPvP(false);
        }

        if (biomeFlags & BiomeFlagNoFly)
            if (player->CanFly())
                player->SetCanFly(false);
    }

    bool HousingMgr::CanEdit(Player* player, HousingPlot* plot)
    {
        if (!player || !plot)
            return false;

        if (player->IsGameMaster())
            return true;

        HousingAccess const access = CheckAccess(player, plot);
        if (access >= AccessOwner)
            return true;

        if (access != AccessGuest)
            return false;

        return (GetGuestFlags(plot->GetHousingId(),
            player->GetSession()->GetAccountId()) & GuestFlagEdit) != 0;
    }

    Position HousingMgr::PlotSpawn(HousingPlot* plot, Player* player)
    {
        if (!plot || !player)
            return plot ? plot->GetTemplate().Spawn : Position();

        PlotTemplate const& authored = plot->GetTemplate();
        if (!authored.TerrainZ)
            return authored.Spawn;

        Map* map = player->GetMap();
        if (!map)
            return authored.Spawn;

        // Resolve once and write it back, so the expensive height probe happens on the first
        // visit to the plot and never again.
        auto itr = _plotTemplates.find(PlotKey(authored.MapId, authored.Index));
        if (itr == _plotTemplates.end())
            return authored.Spawn;

        PlotTemplate& tmpl = itr->second;

        float const ground = map->GetHeight(player->GetPhaseShift(), tmpl.Spawn.m_positionX,
            tmpl.Spawn.m_positionY, tmpl.Spawn.m_positionZ, true, 100.0f);

        if (ground <= INVALID_HEIGHT)
            return tmpl.Spawn;

        tmpl.Spawn.m_positionZ = ground + 0.5f;
        tmpl.Spawn.m_orientation = authored.Spawn.m_orientation;
        tmpl.TerrainZ = false;
        return tmpl.Spawn;
    }

    // -------------------------------------------------------------------------
    // Lifecycle: occupancy, population cap, eviction
    // -------------------------------------------------------------------------

    bool HousingMgr::IsHouseOccupied(uint32 housingId) const
    {
        if (!housingId)
            return false;

        for (auto const& entry : _playerStates)
            if (entry.second.InsideInterior && entry.second.InsideHousingId == housingId)
                return true;

        return false;
    }

    uint32 HousingMgr::GetNeighborhoodPopulation(HousingNeighborhood* neighborhood) const
    {
        if (!neighborhood)
            return 0;

        uint32 residents = 0;

        for (auto const& plot : neighborhood->GetPlots())
            if (plot->IsOwned())
                ++residents;

        return residents;
    }

    HousingResult HousingMgr::JoinNeighborhood(Player* player, uint32 neighborhoodId)
    {
        if (!_enabled)
            return Disabled;

        if (!player || !player->GetSession())
            return InternalError;

        HousingNeighborhood* neighborhood = GetNeighborhood(neighborhoodId);
        if (!neighborhood)
            return NotFound;

        uint32 const accountId = player->GetSession()->GetAccountId();

        if (neighborhood->IsMember(accountId))
            return AlreadyKnown;

        // A guild neighbourhood is joined by the guild, not by asking: the guild script adds
        // members on invite and the rank carries the permissions. Only the two open types can
        // be joined by name.
        if (neighborhood->GetType() != NeighborhoodPublic && neighborhood->GetType() != NeighborhoodPrivate)
            return NotAllowedHere;

        // Every neighbourhood is one faction's. A character cannot join the other side's
        // street even when the account is allowed to visit it.
        uint8 const faction = uint8(player->GetTeamId() == TEAM_HORDE ? FactionHorde : FactionAlliance);
        if (neighborhood->GetFaction() != faction)
            return NotAllowedHere;

        if (_populationCap && GetNeighborhoodPopulation(neighborhood) >= _populationCap)
            return NeighborhoodFull;

        neighborhood->AddMember(accountId, 1);

        TC_LOG_INFO("housing", "Account %u joined neighbourhood %u", accountId, neighborhoodId);
        return Ok;
    }

    void HousingMgr::RunLifecycle()
    {
        if (!_enabled)
            return;

        RunDissolutionSweep();

        if (!_evictDays)
            return;

        // Idle days are measured from last_edited, which every edit and every level change
        // touches. A house in the archive is not on the table any more, so it is not selected.
        uint32 const now = uint32(std::time(nullptr));
        uint32 const evictSeconds = _evictDays * 86400;
        uint32 const graceSeconds = std::min(_evictDays, _graceDays) * 86400;
        uint32 const warnSeconds = evictSeconds - graceSeconds;

        QueryResult result = CharacterDatabase.Query(
            "SELECT `id`, `neighborhoodId`, `plotIndex`, `ownerGuid`, `ownerAccount`, `last_edited`, "
            "`evict_warned` FROM `character_housing` "
            "WHERE `last_edited` < (UNIX_TIMESTAMP() - {})", warnSeconds);

        if (!result)
            return;

        std::vector<std::pair<uint32, uint32>> doomed;   // neighbourhoodId, plotIndex
        uint32 warned = 0;

        do
        {
            Field* fields = result->Fetch();
            uint32 const housingId = fields[0].GetUInt32();
            uint32 const neighborhoodId = fields[1].GetUInt32();
            uint8 const plotIndex = fields[2].GetUInt8();
            uint64 const ownerGuid = fields[3].GetUInt64();
            uint32 const ownerAccount = fields[4].GetUInt32();
            uint32 const lastEdited = fields[5].GetUInt32();
            bool const wasWarned = fields[6].GetBool();

            if ((now - lastEdited) >= evictSeconds)
            {
                doomed.emplace_back(neighborhoodId, uint32(plotIndex));
                continue;
            }

            if (wasWarned)
                continue;

            // The warning is a letter, not a chat line: the owner may be offline for the whole
            // grace window, and mail is the only channel that survives that.
            CharacterDatabase.DirectPExecute(
                "UPDATE `character_housing` SET `evict_warned` = 1 WHERE `id` = {}", housingId);

            if (ownerGuid)
            {
                std::string const subject = "Your house is at risk";
                std::string const body = Trinity::StringFormat(
                    "Your house has not been changed in {} day(s). If it is still untouched in {} "
                    "day(s) it goes back on the market and everything in it moves to your archive, "
                    "where you can restore it for a fee. Visit the town centre to keep it.",
                    uint32((now - lastEdited) / 86400), graceSeconds / 86400);

                MailDraft draft(subject, body);
                CharacterDatabaseTransaction trans = CharacterDatabase.BeginTransaction();
                draft.SendMailTo(trans, MailReceiver(ownerGuid),
                    MailSender(MAIL_NORMAL, 0, MAIL_STATIONERY_GM));
                CharacterDatabase.CommitTransaction(trans);

                (void)ownerAccount;
            }

            ++warned;
        }
        while (result->NextRow());

        uint32 sold = 0;

        for (auto const& entry : doomed)
        {
            HousingNeighborhood* neighborhood = GetNeighborhood(entry.first);
            if (!neighborhood)
                continue;

            HousingPlot* plot = neighborhood->GetPlot(uint8(entry.second));
            if (!plot || !plot->IsOwned())
                continue;

            TC_LOG_INFO("housing", "Evicting housed plot %u/%u for inactivity",
                entry.first, entry.second);

            ForceSellPlot(plot);
            ++sold;
        }

        if (warned || sold)
            TC_LOG_INFO("housing", "Lifecycle: %u house(s) warned, %u house(s) returned to the market",
                warned, sold);
    }

    HousingPlot* HousingMgr::FindPlotAt(uint32 mapId, float x, float y, float radius)
    {
        float best = radius;
        HousingPlot* found = nullptr;

        for (auto const& entry : _neighborhoods)
        {
            HousingNeighborhood* neighborhood = entry.second.get();
            if (neighborhood->GetMapId() != mapId)
                continue;

            for (auto const& plotOwner : neighborhood->GetPlots())
            {
                HousingPlot* plot = plotOwner.get();
                if (plot->GetIndex() >= kPlotCount)
                    continue;

                Position const pos = plot->GetTemplate().HasSpawn ? plot->GetTemplate().Spawn
                                                                 : AnchorForPlot(plot->GetIndex());

                float const distance = std::sqrt((pos.m_positionX - x) * (pos.m_positionX - x)
                    + (pos.m_positionY - y) * (pos.m_positionY - y));

                if (distance <= best)
                {
                    best = distance;
                    found = plot;
                }
            }

            if (found)
                return found;
        }

        return found;
    }

    void HousingMgr::RunDissolutionSweep()
    {
        if (!_guildWings && !_minResidents)
            return;

        uint32 const now = uint32(std::time(nullptr));
        uint32 const grace = _dissolveGraceDays * 86400;

        std::vector<uint32> dissolve;
        uint32 dissolved = 0;

        for (auto const& entry : _neighborhoods)
        {
            HousingNeighborhood* neighborhood = entry.second.get();

            // Public neighbourhoods are never dissolved: the auto-scaler made them and the
            // auto-scaler owns them.
            if (neighborhood->GetType() == NeighborhoodPublic)
            {
                _emptySince.erase(neighborhood->GetId());
                continue;
            }

            bool const empty = GetNeighborhoodPopulation(neighborhood) == 0
                && neighborhood->GetMembers().size() <= 1;   // the founder alone is not a street

            if (!empty)
            {
                _emptySince.erase(neighborhood->GetId());
                continue;
            }

            auto const since = _emptySince.find(neighborhood->GetId());
            if (since == _emptySince.end())
            {
                _emptySince[neighborhood->GetId()] = now;
                continue;
            }

            if ((now - since->second) < grace)
                continue;

            dissolve.push_back(neighborhood->GetId());
        }

        for (uint32 id : dissolve)
        {
            HousingNeighborhood* neighborhood = GetNeighborhood(id);
            if (!neighborhood)
                continue;

            for (auto const& plot : neighborhood->GetPlots())
                if (plot->IsOwned())
                    ForceSellPlot(plot.get());

            TC_LOG_INFO("housing", "Dissolving neighbourhood %u: nobody has lived there for %u day(s)",
                id, _dissolveGraceDays);

            DissolveNeighborhood(neighborhood);
            _emptySince.erase(id);
            ++dissolved;
        }

        (void)dissolved;
    }

    // -------------------------------------------------------------------------
    // Extended opcodes
    // -------------------------------------------------------------------------

    bool HousingMgr::HandleExtendedOp(Player* player, PlayerState& state, std::string const& op,
        std::vector<std::string> const& fields)
    {
        uint32 const accountId = player->GetSession()->GetAccountId();

        // ---- finder and visiting ---------------------------------------------

        if (op == "JOIN")
        {
            uint32 const id = fields.size() > 1 ? uint32(strtoul(fields[1].c_str(), nullptr, 10)) : 0;
            HousingResult const result = JoinNeighborhood(player, id);
            SendResult(player, "JOIN", result, result == Ok ? id : 0);
            return true;
        }

        if (op == "FINDER")
        {
            uint8 const faction = player->GetTeamId() == TEAM_ALLIANCE ? FactionAlliance : FactionHorde;
            std::string payload;

            for (HousingNeighborhood* neighborhood : GetPublicNeighborhoods(faction))
                payload += Trinity::StringFormat("S:FINDER|{}|{}|{}|{};", neighborhood->GetId(),
                    neighborhood->GetName(), neighborhood->GetOwnedCount(), uint32(kPlotCount));

            payload += "S:FINDEREND;";
            SendChunk(player, payload);
            return true;
        }

        if (op == "VISIT")
        {
            uint32 neighborhoodId = 0;
            uint32 plotIndex = 0;

            if (fields.size() < 3 || !HousingAddon::ParseUInt(fields[1], neighborhoodId)
                || !HousingAddon::ParseUInt(fields[2], plotIndex) || plotIndex >= kPlotCount)
            {
                SendResult(player, "VISIT", InvalidPlot);
                return true;
            }

            SendResult(player, "VISIT", VisitNeighborhood(player, neighborhoodId, uint8(plotIndex)));
            return true;
        }

        if (op == "RESERVE")
        {
            uint32 plotIndex = 0;
            if (fields.size() < 2 || !HousingAddon::ParseUInt(fields[1], plotIndex))
            {
                SendResult(player, "RESERVE", InvalidPlot);
                return true;
            }

            SendResult(player, "RESERVE", ReservePlot(player, uint8(plotIndex)));
            return true;
        }

        if (op == "PORTALS")
        {
            std::string payload;
            uint8 const factionBit = player->GetTeamId() == TEAM_ALLIANCE ? 0x01 : 0x02;

            for (HousingPortal const& portal : _portalList)
            {
                if (!portal.Map || !(portal.FactionMask & factionBit))
                    continue;

                payload += Trinity::StringFormat("S:PORTAL|{}|{}|{};", portal.Id, portal.Name,
                    uint32(portal.LevelRequired));
            }

            payload += "S:PORTALEND;";
            SendChunk(player, payload);
            return true;
        }

        if (op == "GOPORTAL")
        {
            uint32 portalId = 0;
            if (fields.size() < 2 || !HousingAddon::ParseUInt(fields[1], portalId))
            {
                SendResult(player, "GOPORTAL", NotFound);
                return true;
            }

            SendResult(player, "GOPORTAL", UsePortal(player, portalId));
            return true;
        }

        // ---- progression -----------------------------------------------------

        if (op == "LEVELUP")
        {
            SendResult(player, "LEVELUP", LevelUpWithGold(player));
            return true;
        }

        if (op == "SHELL")
        {
            uint32 shellId = 0;
            if (fields.size() < 2 || !HousingAddon::ParseUInt(fields[1], shellId))
            {
                SendResult(player, "SHELL", InvalidPlot);
                return true;
            }

            SendResult(player, "SHELL", ApplyShell(player, shellId));
            return true;
        }

        if (op == "SHELLS")
        {
            std::string payload;
            uint8 const factionBit = player->GetTeamId() == TEAM_ALLIANCE ? 0x01 : 0x02;

            for (auto const& pair : _shells)
            {
                ShellInfo const& shell = pair.second;
                if (!(shell.FactionMask & factionBit))
                    continue;

                payload += Trinity::StringFormat("S:SHELL|{}|{}|{}|{};", shell.Id, shell.Name,
                    uint32(shell.SizeClass), uint32(shell.LevelRequired));
            }

            payload += "S:SHELLEND;";
            SendChunk(player, payload);
            return true;
        }

        // ---- storage ---------------------------------------------------------

        if (op == "STORAGE")
        {
            std::string payload;

            for (auto const& entry : GetStorage(accountId))
                payload += Trinity::StringFormat("S:STORAGE|{}|{};", entry.first, entry.second);

            payload += "S:STORAGEEND;";
            SendChunk(player, payload);
            return true;
        }

        if (op == "BUYCOPY")
        {
            uint32 decorId = 0;
            uint32 count = 1;

            if (fields.size() < 2 || !HousingAddon::ParseUInt(fields[1], decorId))
            {
                SendResult(player, "BUYCOPY", InvalidDecor);
                return true;
            }

            if (fields.size() >= 3)
                HousingAddon::ParseUInt(fields[2], count);

            count = std::min<uint32>(std::max<uint32>(1, count), 50);

            DecorInfo const* info = GetDecor(decorId);
            if (!info)
            {
                SendResult(player, "BUYCOPY", InvalidDecor);
                return true;
            }

            uint64 const cost = GoldToCopper(info->Price) * uint64(count);
            if (info->Currency == kCurrencyGold && !player->HasEnoughMoney(int64(cost)))
            {
                SendResult(player, "BUYCOPY", NotEnoughMoney);
                return true;
            }

            if (info->Currency == kCurrencyGold)
                player->ModifyMoney(-int64(cost));

            if (!HasUnlocked(accountId, decorId))
            {
                CharacterDatabase.DirectPExecute(
                    "REPLACE INTO `character_housing_collection` (`accountId`, `decorId`, `source_type`, "
                    "`source_id`, `acquired`) VALUES ({}, {}, {}, {}, UNIX_TIMESTAMP())",
                    accountId, decorId, uint32(info->SourceType), info->SourceId);
                _collection[accountId].insert(decorId);
            }

            AddStorage(accountId, decorId, count);
            SendChunk(player, Trinity::StringFormat("S:STORAGE|{}|{};", decorId,
                GetStorageCount(accountId, decorId)));
            SendResult(player, "BUYCOPY", Ok);
            return true;
        }

        // ---- dyes ------------------------------------------------------------

        if (op == "DYE")
        {
            uint32 placementId = 0;
            uint32 group = 0;
            uint32 color = 0;

            if (fields.size() < 4 || !HousingAddon::ParseUInt(fields[1], placementId)
                || !HousingAddon::ParseUInt(fields[2], group) || !HousingAddon::ParseUInt(fields[3], color))
            {
                SendResult(player, "DYE", InvalidDecor);
                return true;
            }

            SendResult(player, "DYE", DyeDecor(player, placementId, uint8(group), uint8(color)));
            return true;
        }

        // ---- endeavors -------------------------------------------------------

        if (op == "ENDEAVOR")
        {
            uint32 const neighborhoodId = GetPlayerNeighborhoodId(player);
            EndeavorInfo const* info = sHousingEndeavor->GetInfo(sHousingEndeavor->GetActiveId(neighborhoodId));
            uint8 const milestone = sHousingEndeavor->GetMilestone(neighborhoodId);

            std::string payload = Trinity::StringFormat("S:ENDEAVOR|{}|{}|{}|{};", neighborhoodId,
                sHousingEndeavor->GetActiveId(neighborhoodId), uint32(milestone),
                sHousingEndeavor->GetCompletedTasks(neighborhoodId));

            if (info)
            {
                for (EndeavorTask const& task : info->Tasks)
                    payload += Trinity::StringFormat("S:ENDTASK|{}|{}|{}|{};", task.Id,
                        sHousingEndeavor->GetTaskProgress(neighborhoodId, task.Id), task.Required, task.Text);
            }

            payload += "S:ENDTASKEND;";
            SendChunk(player, payload);
            return true;
        }

        if (op == "DELIVER")
        {
            SendResult(player, "DELIVER", sHousingEndeavor->Deliver(player) ? Ok : EndeavorIncomplete);
            return true;
        }

        if (op == "CLAIM")
        {
            uint32 milestone = 0;
            if (fields.size() < 2 || !HousingAddon::ParseUInt(fields[1], milestone))
            {
                SendResult(player, "CLAIM", EndeavorIncomplete);
                return true;
            }

            SendResult(player, "CLAIM", sHousingEndeavor->Claim(player, uint8(milestone))
                ? Ok : EndeavorIncomplete);
            return true;
        }

        // ---- build codes -----------------------------------------------------

        if (op == "BPEXP")
        {
            uint32 kind = BlueprintBuild;
            std::string name = "build";

            if (fields.size() >= 2)
                HousingAddon::ParseUInt(fields[1], kind);

            if (fields.size() >= 3 && !fields[2].empty())
                name = fields[2];

            std::string code;
            HousingResult const result = ExportBlueprint(player, uint8(std::min<uint32>(kind, BlueprintExterior)),
                name, code);

            if (result == Ok)
                SendChunk(player, "S:BPCODE|" + code + ";");

            SendResult(player, "BPEXP", result);
            return true;
        }

        if (op == "BPIMP")
        {
            if (fields.size() < 2)
            {
                SendResult(player, "BPIMP", BadCode);
                return true;
            }

            SendResult(player, "BPIMP", ImportBlueprint(player, fields[1]));
            return true;
        }

        // ---- regret window ---------------------------------------------------

        if (op == "RESTORE")
        {
            SendResult(player, "RESTORE", RestoreArchivedHouse(player));
            return true;
        }

        // ---- build mode ------------------------------------------------------

        // ---- placement without typing coordinates -----------------------------
        //
        // Retail's decorator drops the piece in front of the player and shows a gizmo. Typing
        // six numbers is not a user interface, so PLACEAT takes the player's own position and
        // facing, walks the distance forward, snaps to the ground and places there. The
        // coordinates never leave the server, which also means no client can claim a spot
        // the player is not standing near.

        if (op == "PLACEAT")
        {
            uint32 decorId = 0;
            float distance = 2.0f;
            uint32 indoor = 0;

            if (fields.size() < 2 || !HousingAddon::ParseUInt(fields[1], decorId))
            {
                SendResult(player, "PLACEAT", InvalidDecor);
                return true;
            }

            if (fields.size() >= 3)
            {
                uint32 metres = 0;
                if (HousingAddon::ParseUInt(fields[2], metres))
                    distance = std::min(30.0f, std::max(0.0f, float(metres)));
            }

            if (fields.size() >= 4)
                HousingAddon::ParseUInt(fields[3], indoor);

            bool const isIndoor = indoor != 0 || player->GetMapId() == _interiorMapId;

            float x = player->GetPositionX() + std::cos(player->GetOrientation()) * distance;
            float y = player->GetPositionY() + std::sin(player->GetOrientation()) * distance;
            float z = player->GetPositionZ();

            if (Map* map = player->GetMap())
            {
                float const ground = map->GetHeight(player->GetPhaseShift(), x, y, z, true, 50.0f);
                if (ground > INVALID_HEIGHT)
                    z = ground;
            }

            // Facing is kept, so a chair placed this way faces the way the player is looking
            // at it rather than the way the player is facing. Half a turn is the difference.
            float const o = Position::NormalizeOrientation(player->GetOrientation() + 3.1415927f);

            uint64 placementId = 0;
            HousingResult const result = PlaceDecor(player, decorId, x, y, z, o, 1.0f, isIndoor, placementId);

            SendResult(player, "PLACEAT", result, result == Ok ? placementId : 0);
            return true;
        }

        if (op == "BUILDMODE")
        {
            uint32 on = 0;
            if (fields.size() < 2 || !HousingAddon::ParseUInt(fields[1], on))
            {
                SendResult(player, "BUILDMODE", NotAllowedHere);
                return true;
            }

            state.InBuildMode = on != 0;

            if (fields.size() >= 3)
            {
                uint32 decorId = 0;
                HousingAddon::ParseUInt(fields[2], decorId);
                state.PendingDecorId = decorId;
            }

            SendResult(player, "BUILDMODE", Ok);
            return true;
        }

        // ---- neighbourhood administration -------------------------------------

        if (op == "FOUND")
        {
            SendResult(player, "FOUND", FoundNeighborhood(player));
            return true;
        }

        if (op == "LEAVENB")
        {
            SendResult(player, "LEAVENB", LeaveNeighborhood(player));
            return true;
        }

        if (op == "MOVEHOUSE")
        {
            uint32 plotIndex = 0xFF;
            if (fields.size() < 2 || !HousingAddon::ParseUInt(fields[1], plotIndex))
            {
                SendResult(player, "MOVEHOUSE", InvalidPlot);
                return true;
            }

            SendResult(player, "MOVEHOUSE", MoveHouse(player, uint8(std::min<uint32>(plotIndex, 0xFF))));
            return true;
        }

        if (op == "GRYPHON")
        {
            SendResult(player, "GRYPHON", CallGryphon(player));
            return true;
        }

        if (op == "ROOM")
        {
            uint32 roomId = 0;
            if (fields.size() < 2 || !HousingAddon::ParseUInt(fields[1], roomId))
            {
                SendResult(player, "ROOM", NotFound);
                return true;
            }

            SendResult(player, "ROOM", BuyRoom(player, roomId), roomId);
            return true;
        }

        if (op == "ROOMS")
        {
            HousingPlot* plot = GetOwnedPlot(player);
            if (plot && plot->GetHouse())
            {
                for (RoomInfo const& room : GetRoomsForPlot(plot))
                    SendChunk(player, Trinity::StringFormat("S:ROOM|{}|{}|{}|{}|{}",
                        room.Id, room.Name, uint32(room.BudgetCost), uint32(room.LevelRequired),
                        uint32(CanBuyRoom(player, room.Id))));

                SendChunk(player, Trinity::StringFormat("S:ROOMS|{}",
                    uint32(plot->GetHouse()->GetRoomsOwned())));
            }
            else
                SendResult(player, "ROOMS", NoHouse);

            return true;
        }

        if (op == "ARCANE")
        {
            SendResult(player, "ARCANE", OpenArcanostorage(player));
            return true;
        }

        if (op == "ARMILLARY")
        {
            SendResult(player, "ARMILLARY", ToggleArmillary(player));
            return true;
        }

        if (op == "FAVOR")
        {
            uint32 const neighborhoodId = GetPlayerNeighborhoodId(player);
            if (!neighborhoodId)
            {
                SendResult(player, "FAVOR", NotInNeighborhood);
                return true;
            }

            SendFavor(player, neighborhoodId);
            SendResult(player, "FAVOR", Ok, GetFavor(neighborhoodId));
            return true;
        }

        (void)accountId;
        return false;
    }
} // namespace Housing
