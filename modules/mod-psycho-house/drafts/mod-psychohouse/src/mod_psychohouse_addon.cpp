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
 * The PHSH addon channel.
 *
 * The module's first interface is gossip, because a house is bought and furnished by walking
 * up to two NPCs. This is the second: a small message channel so the shipped addon can show
 * the same state in a window and place objects by clicking instead of typing coordinates.
 *
 * Rules, in the order they are applied to every message:
 *
 *   the prefix   is checked by the core before the message reaches here, so anything that
 *                arrives is for this module.
 *   the opcode   must be one of the nine below. An unknown op is answered with a result line
 *                rather than being dropped, because silence is the hardest thing to debug
 *                from a client.
 *   the fields   are parsed strictly: an integer field is digits only with a length cap, a
 *                float must parse completely and be finite, and a missing field is a bad
 *                request rather than a zero.
 *   ownership    is re-checked inside the mutation, not only where the window was drawn, so
 *                a message cannot act on a house the player does not own.
 *
 * The protocol is written down in the draft's ADDON-PROTOCOL.md, and the tables there and the
 * switch here are compared by tools/housing/check_psychohouse.py.
 */

#include "mod_psychohouse.h"

#include "ChatPackets.h"
#include "Chat.h"
#include "Log.h"
#include "Player.h"
#include "SharedDefines.h"
#include "StringFormat.h"
#include "Timer.h"
#include "WorldSession.h"

#include <cctype>
#include <cmath>
#include <cstdlib>
#include <string>
#include <vector>

namespace PsychoHouse
{
    namespace
    {
        // The second prefix this tree routes, and the only place it is written down.
        char const* const kPrefix = "PHSH";

        // Bumped only when a field changes meaning. The addon refuses to draw a reply from a
        // protocol it does not know.
        constexpr uint32 kProtocolVersion = 1;

        // A field long enough to hold a 10-digit id or a coordinate with six decimals.
        constexpr size_t kMaxFieldLength = 24;

        std::vector<std::string> Split(std::string const& payload)
        {
            std::vector<std::string> fields;
            std::string current;

            for (char const character : payload)
            {
                if (character == '|')
                {
                    fields.push_back(current);
                    current.clear();
                    continue;
                }

                current.push_back(character);
            }

            fields.push_back(current);
            return fields;
        }

        bool ParseUInt(std::string const& field, uint32& out)
        {
            if (field.empty() || field.size() > kMaxFieldLength)
                return false;

            for (char const character : field)
            {
                if (!std::isdigit(static_cast<unsigned char>(character)))
                    return false;
            }

            out = uint32(std::strtoul(field.c_str(), nullptr, 10));
            return true;
        }

        bool ParseFloat(std::string const& field, float& out)
        {
            if (field.empty() || field.size() > kMaxFieldLength)
                return false;

            char* end = nullptr;
            double const value = std::strtod(field.c_str(), &end);

            if (!end || *end != '\0' || !std::isfinite(value))
                return false;

            out = float(value);
            return true;
        }

        void SendRaw(Player* player, std::string const& payload)
        {
            if (!player || !player->GetSession())
                return;

            WorldPackets::Chat::Chat packet;
            packet.Initialize(CHAT_MSG_WHISPER, LANG_ADDON, player, player, payload, 0, "",
                player->GetSession()->GetSessionDbLocaleIndex(), kPrefix);

            player->SendDirectMessage(packet.Write());
        }

        void SendResult(Player* player, char const* op, HouseResult result, uint64 extra = 0)
        {
            std::string payload = Trinity::StringFormat("S:RESULT|{}|{}", op, uint32(result));

            if (extra)
                payload += Trinity::StringFormat("|{}", extra);

            SendRaw(player, payload);
        }
    }

    void PsychoHouseMgr::HandleAddonMessage(Player* player, std::string const& message)
    {
        if (!_enabled || !player || !player->GetSession())
            return;

        std::vector<std::string> const fields = Split(message);

        if (fields.empty())
            return;

        std::string const& op = fields[0];

        if (op == "C:HELLO")
        {
            Owner* owner = GetOwner(player);

            SendRaw(player, Trinity::StringFormat("S:HELLO|{}|{}|{}", kProtocolVersion,
                uint32(_objectLimit), owner ? 1 : 0));
            return;
        }

        if (op == "C:PING")
        {
            SendRaw(player, Trinity::StringFormat("S:PONG|{}", getMSTime()));
            return;
        }

        if (op == "C:STATUS")
        {
            Owner* owner = GetOwner(player);

            SendRaw(player, Trinity::StringFormat("S:STATUS|{}|{}|{}|{}|{}|{}|{}",
                owner ? 1 : 0,
                owner ? uint32(owner->Phase) : 0,
                owner ? uint32(owner->Bought.size()) : 0,
                uint32(GetFacilities().size()),
                owner ? uint32(owner->Objects.size()) : 0,
                uint32(_objectLimit),
                uint32(_housePrice)));
            return;
        }

        if (op == "C:FACILITIES")
        {
            Owner* owner = GetOwner(player);

            for (Facility const& facility : GetFacilities())
            {
                bool const owned = owner && HasFacility(owner, facility.Id);

                SendRaw(player, Trinity::StringFormat("S:FAC|{}|{}|{}|{}|{}|{}|{}", facility.Id,
                    uint32(facility.Kind), facility.Name, facility.Price, owned ? 1 : 0,
                    facility.Entry, facility.IsObject ? 1 : 0));
            }

            SendRaw(player, Trinity::StringFormat("S:FACEND|{}", uint32(GetFacilities().size())));
            return;
        }

        if (op == "C:BUY")
        {
            SendResult(player, "BUY", BuyHouse(player));
            return;
        }

        if (op == "C:SELL")
        {
            SendResult(player, "SELL", SellHouse(player));
            return;
        }

        if (op == "C:HOME")
        {
            bool const travelled = TeleportHome(player);
            SendResult(player, "HOME", travelled ? Ok : NoHouse);
            return;
        }

        if (op == "C:BUYFAC")
        {
            uint32 id = 0;
            if (fields.size() < 2 || !ParseUInt(fields[1], id) || id > 0xFFFF)
            {
                SendResult(player, "BUYFAC", Failed);
                return;
            }

            SendResult(player, "BUYFAC", BuyFacility(player, uint16(id)), id);
            return;
        }

        if (op == "C:OBJECTS")
        {
            Owner* owner = GetOwner(player);

            if (owner)
            {
                for (auto const& standing : owner->Objects)
                {
                    Placement const& placement = standing.second;
                    SendRaw(player, Trinity::StringFormat("S:OBJ|{}|{}|{:.2f}|{:.2f}|{:.2f}|{:.4f}|{:.2f}",
                        uint32(placement.Slot), placement.Entry, placement.X, placement.Y,
                        placement.Z, placement.O, placement.Scale));
                }
            }

            SendRaw(player, Trinity::StringFormat("S:OBJEND|{}|{}",
                owner ? uint32(owner->Objects.size()) : 0, uint32(_objectLimit)));
            return;
        }

        if (op == "C:PLACE")
        {
            uint32 entry = 0;
            float x = 0.0f, y = 0.0f, z = 0.0f, o = 0.0f, scale = 1.0f;

            if (fields.size() < 7 || !ParseUInt(fields[1], entry) || !ParseFloat(fields[2], x) ||
                !ParseFloat(fields[3], y) || !ParseFloat(fields[4], z) || !ParseFloat(fields[5], o) ||
                !ParseFloat(fields[6], scale))
            {
                SendResult(player, "PLACE", Failed);
                return;
            }

            HouseResult const result = PlaceObject(player, entry, x, y, z, o, scale);
            SendResult(player, "PLACE", result, entry);
            return;
        }

        // The client cannot read world coordinates, so "put it where I am standing" is the one
        // placement the addon can offer without a number typed into it. The server reads the
        // position and applies exactly the same limit, template and radius checks.
        if (op == "C:PLACEHERE")
        {
            uint32 entry = 0;
            float o = 0.0f, scale = 1.0f;

            if (fields.size() < 2 || !ParseUInt(fields[1], entry))
            {
                SendResult(player, "PLACEHERE", Failed);
                return;
            }

            if (fields.size() > 2 && !ParseFloat(fields[2], o))
            {
                SendResult(player, "PLACEHERE", Failed);
                return;
            }

            if (fields.size() > 3 && !ParseFloat(fields[3], scale))
            {
                SendResult(player, "PLACEHERE", Failed);
                return;
            }

            HouseResult const result = PlaceObject(player, entry, player->GetPositionX(),
                player->GetPositionY(), player->GetPositionZ(), o, scale);

            SendResult(player, "PLACEHERE", result, entry);
            return;
        }

        if (op == "C:REMOVE")
        {
            uint32 slot = 0;
            if (fields.size() < 2 || !ParseUInt(fields[1], slot) || slot > 0xFF)
            {
                SendResult(player, "REMOVE", Failed);
                return;
            }

            HouseResult const result = RemoveObject(player, uint8(slot));
            SendResult(player, "REMOVE", result, slot);
            return;
        }

        // An op this module does not speak. Answered rather than dropped: a client that sends
        // into silence is the worst kind of bug to chase.
        SendResult(player, "UNKNOWN", Failed);
        TC_LOG_DEBUG("psychohouse", "unknown addon op from " UI64FMTD ": %s", player->GetGUID().GetCounter(),
            op.c_str());
    }
}

// The one symbol the core calls. It is deliberately outside the namespace and takes primitives
// only, so the single line the install adds to ChatHandler.cpp does not have to know anything
// about this module beyond a forward declaration.
void PsychoHouseHandleAddonMessage(Player* player, std::string const& message)
{
    sPsychoHouse->HandleAddonMessage(player, message);
}
