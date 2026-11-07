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

#include "HousingDecor.h"

#include "Creature.h"
#include "MotionMaster.h"
#include "GameObject.h"
#include "Log.h"
#include "Map.h"
#include "ObjectAccessor.h"
#include "PhasingHandler.h"
#include "QuaternionData.h"

namespace Housing
{

GameObject* SpawnDecorObject(DecorInfo const& info, DecorPlacement& placement, Map* map,
    uint32 phaseId, ObjectGuid ownerGuid)
{
    if (!map)
        return nullptr;

    // A dyed piece spawns as the variant game object, not the catalogue one. Without this the
    // dye row was written and read back and never seen.
    // Dye wins over the lit variant: a dyed piece is a colour the player chose, and a night
    // entry for that exact colour does not exist. A dyed piece therefore stays its dyed self
    // when the Armillary goes dark, which is a limitation rather than a bug - the alternative
    // would be silently losing the dye.
    uint32 const entry = placement.VariantEntry ? placement.VariantEntry
        : (placement.NightEntry ? placement.NightEntry : info.GameObjectEntry);
    if (!entry)
        return nullptr;

    Position const pos(placement.X, placement.Y, placement.Z, placement.O);

    // Yaw, then the two tilt axes. With pitch and roll at zero this is the quaternion the
    // spawner has always built, so an untilted piece is placed exactly where it was.
    QuaternionData const rotation = QuaternionData::fromEulerAnglesZYX(placement.O,
        placement.Pitch, placement.Roll);

    GameObject* go = GameObject::CreateGameObject(entry, map, pos, rotation, 255, GO_STATE_READY);
    if (!go)
    {
        TC_LOG_ERROR("housing", "Failed to create decor object %u (gameobject entry %u, placement " UI64FMTD ")",
            info.Id, entry, placement.Id);
        return nullptr;
    }

    go->SetObjectScale(placement.Scale > 0.0f ? placement.Scale : 1.0f);
    go->SetSpawnedByDefault(true);

    // Phase first, then the map: the object must never exist for a frame in the wrong phase.
    PhasingHandler::AddPhase(go, phaseId, false);
    if (!ownerGuid.IsEmpty())
        go->SetOwnerGUID(ownerGuid);

    map->AddToMap(go);

    placement.SpawnGuid = go->GetGUID();
    return go;
}

Creature* SpawnDecorCreature(DecorPlacement& placement, PetBedInfo const& pet, Map* map, uint32 phaseId)
{
    // A pet bed is furniture with something asleep on it. The creature is a temp summon in the
    // same phase as the bed, so it appears to the people who can see the bed and nobody else,
    // and it is removed with the bed rather than orphaned when the last visitor walks out.
    // A bed with a companion assigned to it spawns that companion instead of the bed's own
    // creature: the assignment is the point, and the row-level creature is what the bed shows
    // when nobody has chosen. The mode travels with the assignment, so one bed can be a
    // sleeping cat and the next a wandering one.
    uint32 const entry = placement.PetEntry ? placement.PetEntry : pet.CreatureEntry;
    bool const roaming = placement.PetEntry ? placement.PetRoaming : pet.Roaming;

    if (!map || !entry)
        return nullptr;

    Position const pos(placement.X, placement.Y, placement.Z, placement.O);

    TempSummon* summon = map->SummonCreature(entry, pos, nullptr, 0, nullptr);
    if (!summon)
    {
        TC_LOG_ERROR("housing", "Pet bed decor %u could not summon creature %u",
            placement.DecorId, entry);
        return nullptr;
    }

    PhasingHandler::AddPhase(summon, phaseId, false);

    // A pet is furniture with an opinion. It must not be attackable, and a roaming one has to
    // be given somewhere to roam: SetHomePosition alone is a coordinate, not a movement.
    summon->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IMMUNE_TO_PC | UNIT_FLAG_IMMUNE_TO_NPC);

    if (roaming && pet.Radius > 0.0f)
    {
        summon->SetHomePosition(pos);
        summon->GetMotionMaster()->MoveRandom(pet.Radius);
    }
    else
        summon->SetWalk(false);

    placement.PetGuid = summon->GetGUID();
    return summon;
}

void DespawnDecorCreature(DecorPlacement& placement, Map* map)
{
    if (placement.PetGuid.IsEmpty())
        return;

    if (Creature* creature = map ? map->GetCreature(placement.PetGuid) : nullptr)
        creature->DespawnOrUnsummon();

    placement.PetGuid.Clear();
}

void DespawnDecorObject(DecorPlacement& placement, Map* map)
{
    if (placement.SpawnGuid.IsEmpty())
        return;

    GameObject* go = nullptr;
    if (map)
        go = map->GetGameObject(placement.SpawnGuid);

    if (!go && map)
        go = ObjectAccessor::GetGameObject(*map, placement.SpawnGuid);

    if (go)
        go->Delete();

    placement.SpawnGuid.Clear();
}

} // namespace Housing
