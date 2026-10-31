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

#include "HousingAddon.h"
#include "ChatPackets.h"
#include "Log.h"
#include "Player.h"
#include "StringFormat.h"
#include "WorldSession.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>

namespace Housing
{

std::vector<std::string> HousingAddon::Split(std::string const& payload, char separator)
{
    std::vector<std::string> fields;
    size_t start = 0;

    for (;;)
    {
        size_t const pos = payload.find(separator, start);
        if (pos == std::string::npos)
        {
            fields.push_back(payload.substr(start));
            break;
        }

        fields.push_back(payload.substr(start, pos - start));
        start = pos + 1;
    }

    return fields;
}

bool HousingAddon::ParseUInt(std::string const& field, uint32& out)
{
    if (field.empty() || field.size() > 10)
        return false;

    for (char c : field)
        if (c < '0' || c > '9')
            return false;

    out = uint32(std::strtoul(field.c_str(), nullptr, 10));
    return true;
}

bool HousingAddon::ParseFloat(std::string const& field, float& out)
{
    if (field.empty() || field.size() > 16)
        return false;

    char* end = nullptr;
    double const value = std::strtod(field.c_str(), &end);
    if (!end || *end != '\0' || !std::isfinite(value))
        return false;

    out = float(value);
    return true;
}

bool HousingAddon::ParseBool(std::string const& field, bool& out)
{
    uint32 value = 0;
    if (!ParseUInt(field, value))
        return false;

    out = value != 0;
    return true;
}

std::string HousingAddon::SanitiseText(std::string const& text, size_t maxLength)
{
    std::string clean;
    clean.reserve(std::min(text.size(), maxLength));

    for (char c : text)
    {
        unsigned char const byte = static_cast<unsigned char>(c);
        if (byte < 0x20 || byte == 0x7F)   // control characters
            continue;
        if (c == '|' || c == ';')          // framing characters
            continue;
        clean.push_back(c);
    }

    if (clean.size() > maxLength)
    {
        clean.resize(maxLength);
        // Do not leave a truncated multi-byte sequence behind: back off to the last
        // byte that is not a UTF-8 continuation byte.
        while (!clean.empty() && (static_cast<unsigned char>(clean.back()) & 0xC0) == 0x80)
            clean.pop_back();
    }

    return clean;
}

void HousingAddon::SendRaw(Player* player, std::string const& payload)
{
    if (!player || !player->GetSession())
        return;

    WorldPackets::Chat::Chat packet;
    packet.Initialize(CHAT_MSG_WHISPER, LANG_ADDON, player, player, payload, 0, "",
        player->GetSession()->GetSessionDbLocaleIndex(), kAddonPrefix);

    player->SendDirectMessage(packet.Write());
}

void HousingAddon::Send(Player* player, uint8& messageId, std::string const& payload)
{
    if (!player)
        return;

    if (payload.size() <= kAddonMaxChunk)
    {
        SendRaw(player, payload);
        return;
    }

    // Break the payload into whole records first, then pack records into fragments of at
    // most kAddonMaxChunk bytes so that reassembly on the client is plain concatenation.
    std::vector<std::string> fragments;
    std::string current;

    size_t pos = 0;
    while (pos < payload.size())
    {
        size_t const semi = payload.find(';', pos);
        std::string record;
        if (semi == std::string::npos)
        {
            record = payload.substr(pos);
            pos = payload.size();
        }
        else
        {
            record = payload.substr(pos, semi - pos + 1);
            pos = semi + 1;
        }

        // A single oversized record cannot be split on a ';' boundary; hard-split it. The
        // only payloads that can hit this are malformed and the client drops them anyway.
        while (record.size() > kAddonMaxChunk)
        {
            if (!current.empty())
            {
                fragments.push_back(current);
                current.clear();
            }

            fragments.push_back(record.substr(0, kAddonMaxChunk));
            record = record.substr(kAddonMaxChunk);
        }

        if (current.size() + record.size() > kAddonMaxChunk)
        {
            fragments.push_back(current);
            current.clear();
        }

        current += record;
    }

    if (!current.empty())
        fragments.push_back(current);

    if (fragments.empty())
        return;

    uint8 const frameId = messageId++;
    uint32 const total = uint32(fragments.size());

    for (uint32 i = 0; i < total; ++i)
        SendRaw(player, Trinity::StringFormat("S:CHUNK|{}|{}|{}|{}", uint32(frameId), i + 1, total, fragments[i]));
}

} // namespace Housing
