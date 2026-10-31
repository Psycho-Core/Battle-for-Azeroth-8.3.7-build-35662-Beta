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

#ifndef HousingDecor_h__
#define HousingDecor_h__

#include "HousingDefines.h"
#include "ObjectGuid.h"

class GameObject;
class Map;

namespace Housing
{
    /// One placed decor piece, mirrors a row of character_housing_decor.
    struct TC_GAME_API DecorPlacement
    {
        uint64 Id = 0;              // placement id, server assigned
        uint32 DecorId = 0;         // -> housing_decor.id
        float X = 0.0f;
        float Y = 0.0f;
        float Z = 0.0f;
        float O = 0.0f;
        float Scale = 1.0f;
        uint8 Surface = SurfaceFloor;
        bool Indoor = false;
        uint32 Placed = 0;
        ObjectGuid SpawnGuid;       // live object, empty when not spawned
    };

    /// One unlocked decor item for an account, mirrors character_housing_collection.
    struct TC_GAME_API CollectionEntry
    {
        uint32 DecorId = 0;
        uint8 SourceType = SourceVendor;
        uint32 SourceId = 0;
        uint32 Acquired = 0;
    };

    /// Spawn a decor piece on a map in the given phase. Returns nullptr on failure; the
    /// placement is untouched when that happens so the caller can retry later.
    TC_GAME_API GameObject* SpawnDecorObject(DecorInfo const& info, DecorPlacement& placement, Map* map,
        uint32 phaseId, ObjectGuid ownerGuid);

    /// Despawn a placement's live object. Safe to call when nothing is spawned.
    TC_GAME_API void DespawnDecorObject(DecorPlacement& placement, Map* map);

} // namespace Housing

#endif // HousingDecor_h__
