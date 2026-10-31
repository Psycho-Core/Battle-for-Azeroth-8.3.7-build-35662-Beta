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
    if (!map || !info.GameObjectEntry)
        return nullptr;

    Position const pos(placement.X, placement.Y, placement.Z, placement.O);
    QuaternionData const rotation = QuaternionData::fromEulerAnglesZYX(placement.O, 0.0f, 0.0f);

    GameObject* go = GameObject::CreateGameObject(info.GameObjectEntry, map, pos, rotation, 255, GO_STATE_READY);
    if (!go)
    {
        TC_LOG_ERROR("housing", "Failed to create decor object %u (gameobject entry %u, placement " UI64FMTD ")",
            info.Id, info.GameObjectEntry, placement.Id);
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
