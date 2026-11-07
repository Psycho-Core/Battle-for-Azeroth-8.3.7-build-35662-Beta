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
        float Pitch = 0.0f;         // tilt on the forward axis, 0 = upright
        float Roll = 0.0f;          // tilt on the side axis, 0 = upright
        float Scale = 1.0f;
        uint8 Surface = SurfaceFloor;
        bool Indoor = false;
        uint32 Placed = 0;
        uint32 PetEntry = 0;        // companion assigned to this bed, 0 = the bed's own creature
        bool PetRoaming = false;    // whether the assigned companion wanders
        uint8 DyeGroup = 0;         // 0 = undyed
        uint8 DyeColor = 0;
        uint32 VariantEntry = 0;    // the dyed game object entry, 0 = the catalogue entry
        uint32 NightEntry = 0;      // the lit game object entry while the Armillary is dark, 0 = none
        ObjectGuid SpawnGuid;       // live object, empty when not spawned
        ObjectGuid PetGuid;         // creature a pet bed owns, empty when not spawned
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
    /// The creature a pet bed owns. Returns nullptr when the piece has no creature or the
    /// summon fails, which leaves the bed as ordinary furniture.
    TC_GAME_API Creature* SpawnDecorCreature(DecorPlacement& placement, PetBedInfo const& pet, Map* map,
        uint32 phaseId);

    TC_GAME_API void DespawnDecorCreature(DecorPlacement& placement, Map* map);

    TC_GAME_API GameObject* SpawnDecorObject(DecorInfo const& info, DecorPlacement& placement, Map* map,
        uint32 phaseId, ObjectGuid ownerGuid);

    /// Despawn a placement's live object. Safe to call when nothing is spawned.
    TC_GAME_API void DespawnDecorObject(DecorPlacement& placement, Map* map);

} // namespace Housing

#endif // HousingDecor_h__
