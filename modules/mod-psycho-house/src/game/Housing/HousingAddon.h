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

#ifndef HousingAddon_h__
#define HousingAddon_h__

#include "HousingDefines.h"
#include <string>
#include <vector>

class Player;

namespace Housing
{
    /// Wire format for the BFAH addon channel. Pure serialisation, no game logic.
    /// Never mutates world state, so it is safe to call from anywhere on the world thread.
    class TC_GAME_API HousingAddon
    {
    public:
        /// Split a framed payload on one separator, keeping empty fields.
        static std::vector<std::string> Split(std::string const& payload, char separator = '|');

        static bool ParseUInt(std::string const& field, uint32& out);
        static bool ParseFloat(std::string const& field, float& out);
        static bool ParseBool(std::string const& field, bool& out);

        /// Send a payload to a player, chunking it on ';' boundaries when it does not
        /// fit in one addon message. messageId is bumped by the caller (one per player).
        static void Send(Player* player, uint8& messageId, std::string const& payload);

        /// Send exactly one addon message, no framing. Only for payloads known to fit.
        static void SendRaw(Player* player, std::string const& payload);

        /// Strip framing characters and control bytes from a player-supplied string and
        /// truncate on a UTF-8 boundary.
        static std::string SanitiseText(std::string const& text, size_t maxLength);
    };

} // namespace Housing

#endif // HousingAddon_h__
