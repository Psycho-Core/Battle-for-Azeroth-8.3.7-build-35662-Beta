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
 * Player-placed objects.
 *
 * The facilities the Steward sells are the *building*: a bank, a forge, a portal. This is the
 * furniture: whatever the character wants standing in their own yard, up to a limit the realm
 * sets in `PsychoHouse.Objects.Max`. A piece is a game object the realm already has - nothing
 * is invented here - stored as one row in `psychohouse_object` and spawned into the house's
 * phase when the owner walks in.
 *
 * Four rules, and each one is the reason a column exists:
 *
 *   the limit   is read from the config per call, so changing it and reloading takes effect
 *               without a restart, and 0 turns the feature off without leaving the command
 *               answering happily.
 *   the radius  is measured from the house anchor in three dimensions. Without it a house is
 *               a way to put a mailbox anywhere on the island.
 *   the entry   is resolved against `gameobject_template` and must have a display id, because
 *               a template with no model spawns an invisible piece that still counts against
 *               the limit.
 *   the slot    is the identity. `(guid, slot)` is the key, the spawn map is keyed by it, and
 *               a piece keeps its number for as long as it stands, so a removal always names
 *               exactly one piece.
 */

#include "mod_psychohouse.h"

#include "DatabaseEnv.h"
#include "GameObject.h"
#include "Log.h"
#include "Map.h"
#include "MapManager.h"
#include "ObjectAccessor.h"
#include "ObjectMgr.h"
#include "PhasingHandler.h"
#include "Player.h"
#include "QuaternionData.h"
#include "StringFormat.h"

#include <cmath>
#include <sstream>

namespace PsychoHouse
{
    namespace
    {
        // The anchor is the house: everything the module places is an offset from it, and the
        // radius check uses the same point so "inside the yard" means one thing everywhere.
        float Distance3D(Position const& left, float x, float y, float z)
        {
            float const dx = left.GetPositionX() - x;
            float const dy = left.GetPositionY() - y;
            float const dz = left.GetPositionZ() - z;
            return std::sqrt(dx * dx + dy * dy + dz * dz);
        }
    }

    std::map<uint8, Placement> const& PsychoHouseMgr::GetObjects(Owner const* owner) const
    {
        static std::map<uint8, Placement> const none;
        return owner ? owner->Objects : none;
    }

    bool PsychoHouseMgr::LoadObjects(Owner* owner)
    {
        if (!owner)
            return false;

        owner->Objects.clear();
        owner->ObjectSpawns.clear();

        if (!_objectLimit)
            return false;

        if (QueryResult result = CharacterDatabase.PQuery(
            "SELECT `slot`, `entry`, `x`, `y`, `z`, `o`, `scale` FROM `psychohouse_object` "
            "WHERE `guid` = " UI64FMTD " ORDER BY `slot`", owner->Guid))
        {
            do
            {
                Field* fields = result->Fetch();

                Placement placement;
                placement.Slot  = fields[0].GetUInt8();
                placement.Entry = fields[1].GetUInt32();
                placement.X     = fields[2].GetFloat();
                placement.Y     = fields[3].GetFloat();
                placement.Z     = fields[4].GetFloat();
                placement.O     = fields[5].GetFloat();
                placement.Scale = fields[6].GetFloat();

                if (!placement.Scale)
                    placement.Scale = 1.0f;

                owner->Objects[placement.Slot] = placement;
            }
            while (result->NextRow());
        }

        return !owner->Objects.empty();
    }

    uint8 PsychoHouseMgr::FreeSlot(Owner const* owner) const
    {
        if (!owner)
            return 0;

        // The lowest free slot, so a house that has had pieces removed and re-added does not
        // climb past its own limit.
        for (uint8 slot = 0; slot < _objectLimit; ++slot)
        {
            if (owner->Objects.find(slot) == owner->Objects.end())
                return slot;
        }

        return _objectLimit;
    }

    void PsychoHouseMgr::SpawnObjects(Player* player, Owner* owner)
    {
        if (!player || !owner || !_objectLimit)
            return;

        for (auto const& standing : owner->Objects)
            SpawnObject(player, owner, standing.second);
    }

    void PsychoHouseMgr::SpawnObject(Player* player, Owner* owner, Placement const& placement)
    {
        if (!player || !owner || !placement.Entry)
            return;

        Map* map = player->GetMap();
        if (!map || map->GetId() != _mapId)
            return;

        Position const pos(placement.X, placement.Y, placement.Z, placement.O);

        GameObject* object = GameObject::CreateGameObject(placement.Entry, map, pos,
            QuaternionData::fromEulerAnglesZYX(pos.GetOrientation(), 0.0f, 0.0f), 255, GO_STATE_READY);
        if (!object)
        {
            TC_LOG_ERROR("psychohouse", "could not create object %u (slot %u) for character " UI64FMTD,
                placement.Entry, uint32(placement.Slot), owner->Guid);
            return;
        }

        // Phase first, then the map, exactly as the facilities are placed: a player-placed
        // object must never exist for one frame where somebody else can see it.
        PhasingHandler::AddPhase(object, owner->Phase, false);
        object->SetObjectScale(placement.Scale > 0.0f ? placement.Scale : 1.0f);
        object->SetSpawnedByDefault(true);
        object->SetOwnerGUID(ObjectGuid(ObjectGuid::Create<HighGuid::Player>(owner->Guid)));

        map->AddToMap(object);
        owner->ObjectSpawns[placement.Slot] = object->GetGUID();
    }

    void PsychoHouseMgr::DespawnObjects(Owner* owner)
    {
        if (!owner)
            return;

        Map* map = sMapMgr->FindMap(_mapId, 0);
        if (map)
        {
            for (auto const& pair : owner->ObjectSpawns)
            {
                if (GameObject* object = map->GetGameObject(pair.second))
                    object->Delete();
            }
        }

        owner->ObjectSpawns.clear();
    }

    HouseResult PsychoHouseMgr::PlaceObject(Player* player, uint32 entry, float x, float y, float z,
        float o, float scale)
    {
        if (!_enabled || !player)
            return Disabled;

        if (!_objectLimit)
            return Disabled;

        Owner* owner = GetOwner(player);
        if (!owner)
            return NoHouse;

        if (!owner->InHouse)
            return NotInHouse;

        if (owner->Objects.size() >= _objectLimit)
            return ObjectLimit;

        // The template decides everything about what the piece is. A missing template, or one
        // with no display id, would spawn nothing and still take a slot.
        GameObjectTemplate const* info = sObjectMgr->GetGameObjectTemplate(entry);
        if (!info || !info->displayId)
            return BadObject;

        if (Distance3D(_anchor, x, y, z) > _objectRadius)
            return OutOfRange;

        if (!std::isfinite(x) || !std::isfinite(y) || !std::isfinite(z) || !std::isfinite(o))
            return OutOfRange;

        if (scale <= 0.0f)
            scale = 1.0f;

        scale = std::min(_objectScaleMax, std::max(_objectScaleMin, scale));

        Placement placement;
        placement.Slot  = FreeSlot(owner);
        placement.Entry = entry;
        placement.X     = x;
        placement.Y     = y;
        placement.Z     = z;
        placement.O     = o;
        placement.Scale = scale;

        CharacterDatabase.DirectPExecute(
            "REPLACE INTO `psychohouse_object` (`guid`, `slot`, `entry`, `x`, `y`, `z`, `o`, `scale`) "
            "VALUES (" UI64FMTD ", %u, %u, %f, %f, %f, %f, %f)",
            owner->Guid, uint32(placement.Slot), placement.Entry, placement.X, placement.Y,
            placement.Z, placement.O, placement.Scale);

        owner->Objects[placement.Slot] = placement;

        if (owner->InHouse)
            SpawnObject(player, owner, placement);

        return Ok;
    }

    HouseResult PsychoHouseMgr::RemoveObject(Player* player, uint8 slot)
    {
        if (!_enabled || !player)
            return Disabled;

        Owner* owner = GetOwner(player);
        if (!owner)
            return NoHouse;

        auto const standing = owner->Objects.find(slot);
        if (standing == owner->Objects.end())
            return NoSuchObject;

        CharacterDatabase.DirectPExecute(
            "DELETE FROM `psychohouse_object` WHERE `guid` = " UI64FMTD " AND `slot` = %u",
            owner->Guid, uint32(slot));

        auto const spawned = owner->ObjectSpawns.find(slot);
        if (spawned != owner->ObjectSpawns.end())
        {
            if (Map* map = sMapMgr->FindMap(_mapId, 0))
            {
                if (GameObject* object = map->GetGameObject(spawned->second))
                    object->Delete();
            }

            owner->ObjectSpawns.erase(spawned);
        }

        owner->Objects.erase(standing);
        return Ok;
    }

    std::string PsychoHouseMgr::ObjectList(Player* player)
    {
        Owner* owner = player ? GetOwner(player) : nullptr;
        if (!owner)
            return "no house";

        std::ostringstream out;
        out << uint32(owner->Objects.size()) << "/" << uint32(_objectLimit) << " object(s)";

        if (owner->Objects.empty())
            return out.str();

        for (auto const& standing : owner->Objects)
        {
            Placement const& placement = standing.second;
            out << "\nslot " << uint32(placement.Slot) << ": entry " << placement.Entry
                << " at " << Trinity::StringFormat("{:.2f} {:.2f} {:.2f} o {:.2f} scale {:.2f}",
                    placement.X, placement.Y, placement.Z, placement.O, placement.Scale);
        }

        return out.str();
    }
}
