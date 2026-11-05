/*
 * Copyright (C) 2022 BfaCore Reforged
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#ifndef HousingBlueprint_h__
#define HousingBlueprint_h__

#include "HousingDecor.h"
#include "HousingDefines.h"

#include <string>
#include <vector>

namespace Housing
{
    /// Build codes: a whole house, one room, or a yard, packed into a string a player can
    /// paste into chat. Retail generates these in the client; there is no client-side
    /// serialiser here, so the server does it and the addon only carries the text.
    ///
    /// Format: PHP1:<base62 payload>. The payload is
    ///
    ///   byte  version, byte kind, byte faction, uint16 count,
    ///   count x { varint decorId, int16 x(cm), int16 y(cm), int16 z(cm),
    ///             byte orientation(1.40625 deg), byte scale(x50), byte flags }
    ///
    /// which is 11 bytes a piece against the ~40 a CSV of the same data takes. The codec is
    /// deliberately boring: no compression, no binary blobs in chat, and a checksum so a
    /// truncated paste is rejected instead of half-imported.
    class BlueprintCodec
    {
    public:
        /// Pack. Returns an empty string when there is nothing to pack.
        static std::string Encode(std::vector<DecorPlacement> const& placements, uint8 kind, uint8 faction);

        /// Unpack. Fills `out` only on success, so a bad code cannot half-apply.
        static bool Decode(std::string const& code, std::vector<DecorPlacement>& out,
            uint8& kind, uint8& faction);

        /// Cheap rejection before anything is decoded - the addon sends whatever it likes.
        static bool LooksLikeCode(std::string const& code);
    };
} // namespace Housing

#endif // HousingBlueprint_h__
