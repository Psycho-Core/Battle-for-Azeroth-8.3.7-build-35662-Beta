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

/*
 * House state, the phase band, and every spawn the module makes.
 *
 * Two rules run through the whole file:
 *
 *   - the phase is allocated once and then stored. It is never derived from the character
 *     id, so two characters cannot collide through arithmetic;
 *   - nothing is written to `creature` or `gameobject`. Every NPC and object in a house is
 *     transient, created into the owner's phase and despawned when the owner leaves, so a
 *     house cannot leave an orphan behind and there is no grid bookkeeping to get wrong.
 */

#include "mod_psychohouse.h"

#include "Chat.h"
#include "Config.h"
#include "DatabaseEnv.h"
#include "GameObject.h"
#include "Log.h"
#include "Map.h"
#include "MapManager.h"
#include "ObjectMgr.h"
#include "ObjectAccessor.h"
#include "PhasingHandler.h"
#include "Player.h"
#include "QuaternionData.h"
#include "RestMgr.h"
#include "TemporarySummon.h"
#include "WorldSession.h"

#include <algorithm>
#include <string>

namespace PsychoHouse
{

char const* ResultText(HouseResult result)
{
    switch (result)
    {
        case Ok:              return "done";
        case Disabled:        return "housing is switched off on this realm";
        case NoHouse:         return "no house on this character yet";
        case HasHouse:        return "this character already owns a house";
        case NoMoney:         return "not enough gold";
        case NoSpace:         return "no phase left: the realm has reached the house limit";
        case NotInHouse:      return "stand in the house first";
        case UnknownFacility: return "that facility is not available";
        case AlreadyBought:   return "already bought";
        case WrongFaction:    return "not for this character's faction";
        case NotEnoughLevel:  return "level too low";
        case Storage:         return "the character database refused the write";
        case Failed:          return "failed";
    }

    return "failed";
}

PsychoHouseMgr* PsychoHouseMgr::instance()
{
    static PsychoHouseMgr instance;
    return &instance;
}

void PsychoHouseMgr::LoadConfig()
{
    _enabled       = sConfigMgr->GetBoolDefault("PsychoHouse.Enable", true);
    _mapId         = uint32(std::max(0, sConfigMgr->GetIntDefault("PsychoHouse.Map", 1)));
    _zoneId        = uint32(std::max(0, sConfigMgr->GetIntDefault("PsychoHouse.Zone", 876)));
    _areaId        = uint32(std::max(0, sConfigMgr->GetIntDefault("PsychoHouse.Area", 876)));
    _housePrice    = uint32(std::max(0, sConfigMgr->GetIntDefault("PsychoHouse.Price.House", 10000000)));
    _refundPercent = uint8(std::min(100, std::max(0, sConfigMgr->GetIntDefault("PsychoHouse.Refund.Percent", 50))));

    float const x = sConfigMgr->GetFloatDefault("PsychoHouse.Anchor.X", 16222.97f);
    float const y = sConfigMgr->GetFloatDefault("PsychoHouse.Anchor.Y", 16267.80f);
    float const z = sConfigMgr->GetFloatDefault("PsychoHouse.Anchor.Z", 13.14f);
    float const o = sConfigMgr->GetFloatDefault("PsychoHouse.Anchor.O", 1.46f);
    _anchor.Relocate(x, y, z, o);

    _keeperX  = sConfigMgr->GetFloatDefault("PsychoHouse.Keeper.Offset.X", -1.5f);
    _keeperY  = sConfigMgr->GetFloatDefault("PsychoHouse.Keeper.Offset.Y", -2.5f);
    _keeperZ  = sConfigMgr->GetFloatDefault("PsychoHouse.Keeper.Offset.Z", 0.0f);
    _keeperO  = sConfigMgr->GetFloatDefault("PsychoHouse.Keeper.Offset.O", 0.0f);
    _stewardX = sConfigMgr->GetFloatDefault("PsychoHouse.Steward.Offset.X", 1.5f);
    _stewardY = sConfigMgr->GetFloatDefault("PsychoHouse.Steward.Offset.Y", -2.5f);
    _stewardZ = sConfigMgr->GetFloatDefault("PsychoHouse.Steward.Offset.Z", 0.0f);
    _stewardO = sConfigMgr->GetFloatDefault("PsychoHouse.Steward.Offset.O", 0.0f);
}

void PsychoHouseMgr::LoadFacilities()
{
    _facilities.clear();
    _facilityById.clear();

    if (!_enabled)
        return;

    QueryResult result = WorldDatabase.Query(
        "SELECT `id`, `name`, `kind`, `entry`, `is_object`, `faction`, `price`, "
        "`pos_x`, `pos_y`, `pos_z`, `pos_o`, `required_level`, `sort` "
        "FROM `psychohouse_facility` ORDER BY `sort`, `id`");

    if (result)
    {
        do
        {
            Field* fields = result->Fetch();

            Facility facility;
            facility.Id            = fields[0].GetUInt16();
            facility.Name          = fields[1].GetString();
            facility.Kind          = fields[2].GetUInt8();
            facility.Entry         = fields[3].GetUInt32();
            facility.IsObject      = fields[4].GetBool();
            facility.Faction       = fields[5].GetUInt8();
            facility.Price         = fields[6].GetUInt32();
            facility.X             = fields[7].GetFloat();
            facility.Y             = fields[8].GetFloat();
            facility.Z             = fields[9].GetFloat();
            facility.O             = fields[10].GetFloat();
            facility.RequiredLevel = fields[11].GetUInt8();
            facility.Sort          = fields[12].GetUInt8();

            _facilities.push_back(facility);
        } while (result->NextRow());
    }

    uint32 bound = 0;
    for (Facility const& facility : _facilities)
    {
        _facilityById[facility.Id] = facility;
        if (facility.Entry)
            ++bound;
    }

    TC_LOG_INFO("psychohouse", "loaded %u facilities, %u of them bound to realm content",
        uint32(_facilities.size()), bound);

    if (bound < uint32(_facilities.size()))
        TC_LOG_INFO("psychohouse", "run sql/world/03_bind_facilities.sql to bind the rest");

    // A missing template is the one mistake that turns into an empty island rather than a
    // log line, so it is worth saying out loud at startup.
    if (!sObjectMgr->GetCreatureTemplate(kKeeperEntry))
        TC_LOG_ERROR("psychohouse", "creature template %u is missing: run sql/world/02_creatures.sql", kKeeperEntry);

    if (!sObjectMgr->GetCreatureTemplate(kStewardEntry))
        TC_LOG_ERROR("psychohouse", "creature template %u is missing: run sql/world/02_creatures.sql", kStewardEntry);
}

void PsychoHouseMgr::Unload()
{
    _facilities.clear();
    _facilityById.clear();
    _owners.clear();
}

Facility const* PsychoHouseMgr::FindFacility(uint16 id) const
{
    auto it = _facilityById.find(id);
    return it == _facilityById.end() ? nullptr : &it->second;
}

bool PsychoHouseMgr::HasFacility(Owner const* owner, uint16 id) const
{
    if (!owner)
        return false;

    return std::find(owner->Bought.begin(), owner->Bought.end(), id) != owner->Bought.end();
}

bool PsychoHouseMgr::FacilityFitsTeam(Facility const& facility, Owner const* owner) const
{
    if (!owner)
        return false;

    return facility.Faction == FactionBoth || facility.Faction == owner->Team;
}

Owner* PsychoHouseMgr::FindOwner(uint64 guid)
{
    auto it = _owners.find(guid);
    return it == _owners.end() ? nullptr : &it->second;
}

bool PsychoHouseMgr::LoadPurchases(Owner* owner)
{
    owner->Bought.clear();

    if (QueryResult result = CharacterDatabase.PQuery(
        "SELECT `facility` FROM `psychohouse_purchase` WHERE `owner` = " UI64FMTD, owner->Guid))
    {
        do
        {
            owner->Bought.push_back(result->Fetch()[0].GetUInt16());
        } while (result->NextRow());
    }

    return !owner->Bought.empty();
}

Owner* PsychoHouseMgr::GetOwner(Player* player)
{
    if (!_enabled || !player)
        return nullptr;

    uint64 const guid = player->GetGUID().GetCounter();

    if (Owner* cached = FindOwner(guid))
        return cached;

    if (QueryResult result = CharacterDatabase.PQuery(
        "SELECT `account`, `team`, `phase`, `name` FROM `psychohouse_owner` "
        "WHERE `guid` = " UI64FMTD, guid))
    {
        Field* fields = result->Fetch();

        Owner owner;
        owner.Guid    = guid;
        owner.Account = fields[0].GetUInt32();
        owner.Team    = fields[1].GetUInt8();
        owner.Phase   = fields[2].GetUInt16();
        owner.Name    = fields[3].GetString();

        Owner& stored = _owners[guid];
        stored = owner;
        LoadPurchases(&stored);
        return &stored;
    }

    return nullptr;
}

uint16 PsychoHouseMgr::AllocatePhase(uint64 guid, uint32 account, uint8 team, std::string const& name)
{
    std::string escaped = name;
    CharacterDatabase.EscapeString(escaped);

    // The phase is allocated rather than derived, so two characters cannot collide by
    // arithmetic. UNIQUE KEY `phase` is the arbiter: a lost race makes the INSERT fail and
    // the read-back below miss, and the next attempt takes the new maximum.
    //
    // A plain INSERT, not INSERT IGNORE: with IGNORE the duplicate would be swallowed, the
    // read-back would return the phase somebody else already owns, and two characters would
    // share a house.
    for (uint8 attempt = 0; attempt < 5; ++attempt)
    {
        uint32 next = kPhaseBase;

        if (QueryResult result = CharacterDatabase.Query("SELECT MAX(`phase`) FROM `psychohouse_owner`"))
        {
            uint32 const highest = result->Fetch()[0].GetUInt32();
            if (highest >= kPhaseLast)
            {
                TC_LOG_ERROR("psychohouse", "phase range exhausted at %u", highest);
                return 0;
            }

            if (highest >= kPhaseBase)
                next = highest + 1;
        }

        // DirectPExecute, not PExecute: PExecute queues the write on a worker thread, and
        // the read-back two lines down would then run before the row exists. The loop would
        // retry, the queued INSERT would land afterwards, and a character could end up with
        // a house row it was never charged for.
        CharacterDatabase.DirectPExecute(
            "INSERT INTO `psychohouse_owner` (`guid`, `account`, `team`, `phase`, `name`, `created`) "
            "VALUES (" UI64FMTD ", %u, %u, %u, '%s', UNIX_TIMESTAMP())",
            guid, account, uint32(team), next, escaped.c_str());

        if (QueryResult result = CharacterDatabase.PQuery(
            "SELECT `phase` FROM `psychohouse_owner` WHERE `guid` = " UI64FMTD, guid))
        {
            return result->Fetch()[0].GetUInt16();
        }
    }

    return 0;
}

HouseResult PsychoHouseMgr::BuyHouse(Player* player)
{
    if (!_enabled)
        return Disabled;

    if (!player)
        return Failed;

    uint64 const guid = player->GetGUID().GetCounter();
    if (FindOwner(guid))
        return HasHouse;

    if (!player->HasEnoughMoney(int64(_housePrice)))
        return NoMoney;

    uint8 const team = player->GetTeamId() == TEAM_ALLIANCE ? FactionAlliance : FactionHorde;
    uint16 const phase = AllocatePhase(guid, player->GetSession()->GetAccountId(), team, player->GetName());
    if (!phase)
        return NoSpace;

    player->ModifyMoney(-int64(_housePrice));

    Owner* owner = GetOwner(player);
    if (!owner)
    {
        CharacterDatabase.DirectPExecute("DELETE FROM `psychohouse_owner` WHERE `guid` = " UI64FMTD, guid);
        player->ModifyMoney(int64(_housePrice));
        return Storage;
    }

    TC_LOG_INFO("psychohouse", "character " UI64FMTD " bought a house in phase %u", guid, uint32(phase));
    return Ok;
}

HouseResult PsychoHouseMgr::SellHouse(Player* player)
{
    if (!_enabled)
        return Disabled;

    if (!player)
        return Failed;

    uint64 const guid = player->GetGUID().GetCounter();
    Owner* owner = GetOwner(player);
    if (!owner)
        return NoHouse;

    DespawnHouse(owner);
    ClearPhase(player, owner);
    player->GetRestMgr().RemoveRestFlag(REST_FLAG_IN_TAVERN);

    CharacterDatabase.DirectPExecute("DELETE FROM `psychohouse_owner` WHERE `guid` = " UI64FMTD, guid);
    CharacterDatabase.DirectPExecute("DELETE FROM `psychohouse_purchase` WHERE `owner` = " UI64FMTD, guid);

    uint64 const refund = uint64(_housePrice) * _refundPercent / 100;
    if (refund)
        player->ModifyMoney(int64(refund));

    _owners.erase(guid);
    TC_LOG_INFO("psychohouse", "character " UI64FMTD " sold a house, refunded " UI64FMTD " copper", guid, refund);
    return Ok;
}

HouseResult PsychoHouseMgr::BuyFacility(Player* player, uint16 facilityId)
{
    if (!_enabled)
        return Disabled;

    Facility const* facility = FindFacility(facilityId);
    if (!facility)
        return UnknownFacility;

    Owner* owner = GetOwner(player);
    if (!owner)
        return NoHouse;

    if (!owner->InHouse)
        return NotInHouse;

    if (HasFacility(owner, facilityId))
        return AlreadyBought;

    if (!FacilityFitsTeam(*facility, owner))
        return WrongFaction;

    if (!facility->Entry)
        return UnknownFacility;

    if (facility->RequiredLevel && player->getLevel() < facility->RequiredLevel)
        return NotEnoughLevel;

    if (facility->Price && !player->HasEnoughMoney(int64(facility->Price)))
        return NoMoney;

    if (facility->Price)
        player->ModifyMoney(-int64(facility->Price));

    owner->Bought.push_back(facilityId);
    CharacterDatabase.DirectPExecute(
        "INSERT INTO `psychohouse_purchase` (`owner`, `facility`, `bought`) "
        "VALUES (" UI64FMTD ", %u, UNIX_TIMESTAMP())", owner->Guid, uint32(facilityId));

    SpawnFacility(player, owner, *facility);
    return Ok;
}

bool PsychoHouseMgr::TeleportHome(Player* player)
{
    if (!_enabled || !player)
        return false;

    if (!FindOwner(player->GetGUID().GetCounter()))
        return false;

    if (player->GetMapId() == _mapId && player->GetAreaId() == _areaId)
        return true;

    return player->TeleportTo(_mapId, _anchor.GetPositionX(), _anchor.GetPositionY(),
        _anchor.GetPositionZ(), _anchor.GetOrientation());
}

void PsychoHouseMgr::OnPlayerLogin(Player* player)
{
    if (!_enabled || !player)
        return;

    if (player->GetAreaId() != _areaId)
        return;

    OnPlayerEnterArea(player);
}

void PsychoHouseMgr::OnPlayerLogout(Player* player)
{
    if (!player)
        return;

    Owner* owner = FindOwner(player->GetGUID().GetCounter());
    if (!owner || !owner->InHouse)
        return;

    // The summons die with the map, so the flag has to die with the session - otherwise the
    // next login would think the house is already furnished and spawn nothing.
    DespawnHouse(owner);
    owner->InHouse = false;
}

void PsychoHouseMgr::OnPlayerEnterArea(Player* player)
{
    if (!_enabled || !player)
        return;

    // A game master standing on the island sees it unphased: no house of their own is
    // forced on them and nothing of anybody else's is hidden from them.
    if (player->IsGameMaster())
        return;

    Owner* owner = GetOwner(player);
    if (!owner)
    {
        ChatHandler(player->GetSession()).SendSysMessage(
            "No house stands here for this character. The Keeper sells one in the capitals.");
        return;
    }

    if (owner->InHouse)
        return;

    ApplyPhase(player, owner);
    SpawnHouse(player, owner);
    owner->InHouse = true;

    // A house is a rest area: the flag is what the client reads to show the resting state.
    player->GetRestMgr().SetRestFlag(REST_FLAG_IN_TAVERN);
}

void PsychoHouseMgr::OnPlayerLeaveArea(Player* player)
{
    if (!_enabled || !player)
        return;

    Owner* owner = FindOwner(player->GetGUID().GetCounter());
    if (!owner || !owner->InHouse)
        return;

    DespawnHouse(owner);
    ClearPhase(player, owner);
    owner->InHouse = false;
    player->GetRestMgr().RemoveRestFlag(REST_FLAG_IN_TAVERN);
}

void PsychoHouseMgr::RepairSpawns()
{
    if (!_enabled)
        return;

    for (auto& pair : _owners)
    {
        Owner& owner = pair.second;
        if (!owner.InHouse)
            continue;

        Player* player = ObjectAccessor::FindPlayerByLowGUID(owner.Guid);
        if (!player || !player->IsInWorld())
            continue;

        Map* map = player->GetMap();
        if (!map || map->GetId() != _mapId)
            continue;

        if (owner.Keeper.IsEmpty() || !map->GetCreature(owner.Keeper))
            SpawnKeeper(player, &owner);

        if (owner.Steward.IsEmpty() || !map->GetCreature(owner.Steward))
            SpawnSteward(player, &owner);

        for (uint16 id : owner.Bought)
        {
            Facility const* facility = FindFacility(id);
            if (!facility)
                continue;

            auto spawned = owner.Spawned.find(id);
            if (spawned != owner.Spawned.end() && !spawned->second.IsEmpty() && map->GetCreature(spawned->second))
                continue;

            if (facility->IsObject)
            {
                if (spawned != owner.Spawned.end() && !spawned->second.IsEmpty() &&
                    map->GetGameObject(spawned->second))
                    continue;
            }

            SpawnFacility(player, &owner, *facility);
            TC_LOG_INFO("psychohouse", "restored facility %u in the house of " UI64FMTD,
                uint32(id), owner.Guid);
        }
    }
}

void PsychoHouseMgr::ApplyPhase(Player* player, Owner const* owner)
{
    if (!player || !owner)
        return;

    PhasingHandler::AddPhase(player, owner->Phase, true);
}

void PsychoHouseMgr::ClearPhase(Player* player, Owner const* owner)
{
    if (!player || !owner)
        return;

    PhasingHandler::RemovePhase(player, owner->Phase, true);
}

void PsychoHouseMgr::SpawnHouse(Player* player, Owner* owner)
{
    if (!player || !owner)
        return;

    Map* map = player->GetMap();
    if (!map || map->GetId() != _mapId)
        return;

    SpawnKeeper(player, owner);
    SpawnSteward(player, owner);

    for (uint16 id : owner->Bought)
    {
        if (Facility const* facility = FindFacility(id))
            SpawnFacility(player, owner, *facility);
    }
}

void PsychoHouseMgr::SpawnKeeper(Player* player, Owner* owner)
{
    if (!player || !owner)
        return;

    Map* map = player->GetMap();
    if (!map || map->GetId() != _mapId)
        return;

    Position const keeper(_anchor.GetPositionX() + _keeperX, _anchor.GetPositionY() + _keeperY,
        _anchor.GetPositionZ() + _keeperZ, _keeperO);
    if (Creature* creature = map->SummonCreature(kKeeperEntry, keeper, nullptr, 0, nullptr, 0, 0, false))
    {
        // true, not false: Map::SummonCreature has already put the creature on the map, so
        // the phase change has to send the visibility update or the NPC stays invisible
        // until something else happens to refresh it.
        PhasingHandler::AddPhase(creature, owner->Phase, true);
        creature->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IMMUNE_TO_PC);
        owner->Keeper = creature->GetGUID();
    }
    else
    {
        TC_LOG_ERROR("psychohouse", "could not summon the keeper for character " UI64FMTD, owner->Guid);
    }
}

void PsychoHouseMgr::SpawnSteward(Player* player, Owner* owner)
{
    if (!player || !owner)
        return;

    Map* map = player->GetMap();
    if (!map || map->GetId() != _mapId)
        return;

    Position const steward(_anchor.GetPositionX() + _stewardX, _anchor.GetPositionY() + _stewardY,
        _anchor.GetPositionZ() + _stewardZ, _stewardO);
    if (Creature* creature = map->SummonCreature(kStewardEntry, steward, nullptr, 0, nullptr, 0, 0, false))
    {
        PhasingHandler::AddPhase(creature, owner->Phase, true);
        creature->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IMMUNE_TO_PC);
        owner->Steward = creature->GetGUID();
    }
    else
    {
        TC_LOG_ERROR("psychohouse", "could not summon the steward for character " UI64FMTD, owner->Guid);
    }
}

void PsychoHouseMgr::SpawnFacility(Player* player, Owner* owner, Facility const& facility)
{
    if (!player || !owner || !facility.Entry)
        return;

    if (!FacilityFitsTeam(facility, owner))
        return;

    Map* map = player->GetMap();
    if (!map || map->GetId() != _mapId)
        return;

    Position const pos(_anchor.GetPositionX() + facility.X, _anchor.GetPositionY() + facility.Y,
        _anchor.GetPositionZ() + facility.Z, facility.O);

    if (facility.IsObject)
    {
        GameObject* object = GameObject::CreateGameObject(facility.Entry, map, pos,
            QuaternionData::fromEulerAnglesZYX(pos.GetOrientation(), 0.0f, 0.0f), 255, GO_STATE_READY);
        if (!object)
        {
            TC_LOG_ERROR("psychohouse", "could not create object %u for character " UI64FMTD,
                facility.Entry, owner->Guid);
            return;
        }

        // Phase first, then the map: a facility must never exist for one frame where
        // somebody else can see it.
        PhasingHandler::AddPhase(object, owner->Phase, false);
        map->AddToMap(object);
        owner->Spawned[facility.Id] = object->GetGUID();
        return;
    }

    if (Creature* creature = map->SummonCreature(facility.Entry, pos, nullptr, 0, nullptr, 0, 0, false))
    {
        PhasingHandler::AddPhase(creature, owner->Phase, true);
        // A summoned creature lives with TEMPSUMMON_DEAD_DESPAWN, so one death would remove
        // a vendor for good. Immune to players is also how a city vendor behaves.
        creature->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IMMUNE_TO_PC);
        owner->Spawned[facility.Id] = creature->GetGUID();
    }
    else
    {
        TC_LOG_ERROR("psychohouse", "could not summon facility %u (entry %u) for character " UI64FMTD,
            uint32(facility.Id), facility.Entry, owner->Guid);
    }
}

void PsychoHouseMgr::DespawnHouse(Owner* owner)
{
    if (!owner)
        return;

    Map* map = sMapMgr->FindMap(_mapId, 0);
    if (map)
    {
        for (auto const& pair : owner->Spawned)
        {
            if (Creature* creature = map->GetCreature(pair.second))
            {
                creature->DespawnOrUnsummon();
                continue;
            }

            if (GameObject* object = map->GetGameObject(pair.second))
                object->Delete();
        }

        if (Creature* keeper = map->GetCreature(owner->Keeper))
            keeper->DespawnOrUnsummon();

        if (Creature* steward = map->GetCreature(owner->Steward))
            steward->DespawnOrUnsummon();
    }

    owner->Spawned.clear();
    owner->Keeper.Clear();
    owner->Steward.Clear();
}

std::string PsychoHouseMgr::Diagnostics()
{
    uint32 bound = 0;
    std::string unbound;

    for (Facility const& facility : _facilities)
    {
        if (facility.Entry)
        {
            ++bound;
            continue;
        }

        if (!unbound.empty())
            unbound += ", ";

        unbound += facility.Name;
    }

    std::string text;
    text += "facilities ";
    text += std::to_string(uint32(_facilities.size()));
    text += ", bound ";
    text += std::to_string(bound);
    text += ", loaded houses ";
    text += std::to_string(uint32(_owners.size()));
    text += ", phase range ";
    text += std::to_string(uint32(kPhaseBase));
    text += "-";
    text += std::to_string(uint32(kPhaseLast));

    if (!unbound.empty())
    {
        text += ", unbound: ";
        text += unbound;
    }

    return text;
}

} // namespace PsychoHouse
