/*
 * Copyright (C) 2022 BfaCore Reforged
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "HousingBlueprint.h"

#include <cmath>
#include <cstdint>

namespace Housing
{
    namespace
    {
        char const* kAlphabet =
            "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
        char const* kPrefix = "PHP1:";
        size_t const kPrefixLen = 5;
        uint8 const kVersion = 1;

        std::string ToBase62(std::vector<uint8_t> const& bytes)
        {
            std::string out;
            size_t i = 0;

            // Big-integer division of the byte string by 62, one digit at a time. Slow on
            // paper, instant on a 400-piece house, and it needs no library.
            std::vector<uint8_t> work = bytes;
            while (!work.empty())
            {
                uint32 remainder = 0;
                std::vector<uint8_t> quotient;
                quotient.reserve(work.size());

                for (size_t index = 0; index < work.size(); ++index)
                {
                    uint32 const value = (remainder << 8) | work[index];
                    uint8 const digit = uint8(value / 62);
                    remainder = value % 62;
                    if (!quotient.empty() || digit)
                        quotient.push_back(digit);
                }

                out.push_back(kAlphabet[remainder]);
                work.swap(quotient);

                if (++i > 4096)   // a code that needs more digits than this is garbage
                    break;
            }

            if (out.empty())
                out.push_back('0');

            return std::string(out.rbegin(), out.rend());
        }

        bool FromBase62(std::string const& text, std::vector<uint8_t>& out)
        {
            std::vector<uint8_t> work;   // little endian accumulator of base-62 digits

            for (char c : text)
            {
                char const* found = std::char_traits<char>::find(kAlphabet, 62, c);
                if (!found)
                    return false;

                uint32 carry = uint32(found - kAlphabet);
                for (size_t index = 0; index < work.size(); ++index)
                {
                    uint32 const value = uint32(work[index]) * 62 + carry;
                    work[index] = uint8(value & 0xFF);
                    carry = value >> 8;
                }

                while (carry)
                {
                    work.push_back(uint8(carry & 0xFF));
                    carry >>= 8;
                }
            }

            // Trailing zero bytes are not significant and are dropped on the way out.
            while (!work.empty() && work.back() == 0)
                work.pop_back();

            out.assign(work.rbegin(), work.rend());
            return !out.empty();
        }

        void PutVarint(std::vector<uint8_t>& out, uint32 value)
        {
            while (value >= 0x80)
            {
                out.push_back(uint8((value & 0x7F) | 0x80));
                value >>= 7;
            }
            out.push_back(uint8(value));
        }

        bool GetVarint(std::vector<uint8_t> const& in, size_t& pos, uint32& value)
        {
            value = 0;
            uint32 shift = 0;

            while (pos < in.size())
            {
                uint8 const byte = in[pos++];
                value |= uint32(byte & 0x7F) << shift;
                if (!(byte & 0x80))
                    return true;

                shift += 7;
                if (shift > 28)
                    return false;
            }

            return false;
        }

        void PutInt16(std::vector<uint8_t>& out, int32 value)
        {
            int16 clamped = int16(value < -32768 ? -32768 : (value > 32767 ? 32767 : value));
            out.push_back(uint8(uint16(clamped) & 0xFF));
            out.push_back(uint8((uint16(clamped) >> 8) & 0xFF));
        }

        int16 GetInt16(std::vector<uint8_t> const& in, size_t& pos)
        {
            uint16 const low = in[pos];
            uint16 const high = in[pos + 1];
            pos += 2;
            return int16(low | (high << 8));
        }

        uint8 Checksum(std::vector<uint8_t> const& bytes, size_t from, size_t to)
        {
            uint32 sum = 0x1F;
            for (size_t index = from; index < to; ++index)
                sum = (sum * 31u + bytes[index]) & 0xFF;
            return uint8(sum);
        }
    } // namespace

    bool BlueprintCodec::LooksLikeCode(std::string const& code)
    {
        return code.size() > kPrefixLen && code.compare(0, kPrefixLen, kPrefix) == 0
            && code.size() < 20000;
    }

    std::string BlueprintCodec::Encode(std::vector<DecorPlacement> const& placements, uint8 kind, uint8 faction)
    {
        if (placements.empty() || placements.size() > 0xFFFF)
            return std::string();

        std::vector<uint8_t> bytes;
        bytes.reserve(4 + placements.size() * 11 + 1);

        bytes.push_back(kVersion);
        bytes.push_back(kind);
        bytes.push_back(faction);
        bytes.push_back(uint8((placements.size() >> 8) & 0xFF));
        bytes.push_back(uint8(placements.size() & 0xFF));

        for (DecorPlacement const& placement : placements)
        {
            PutVarint(bytes, placement.DecorId);
            PutInt16(bytes, int32(std::lround(placement.X * 100.0f)));
            PutInt16(bytes, int32(std::lround(placement.Y * 100.0f)));
            PutInt16(bytes, int32(std::lround(placement.Z * 100.0f)));

            float orientation = std::fmod(placement.O, 6.2831853f);
            if (orientation < 0.0f)
                orientation += 6.2831853f;

            bytes.push_back(uint8(std::lround(orientation * 40.625f) & 0xFF));   // 1.40625 deg
            bytes.push_back(uint8(std::lround(placement.Scale * 50.0f) > 255 ? 255
                : std::lround(placement.Scale * 50.0f)));
            bytes.push_back(uint8((placement.Indoor ? 0x01 : 0x00) | ((placement.Surface & 0x03) << 1)));
        }

        bytes.push_back(Checksum(bytes, 0, bytes.size()));

        return std::string(kPrefix) + ToBase62(bytes);
    }

    bool BlueprintCodec::Decode(std::string const& code, std::vector<DecorPlacement>& out,
        uint8& kind, uint8& faction)
    {
        if (!LooksLikeCode(code))
            return false;

        std::vector<uint8_t> bytes;
        if (!FromBase62(code.substr(kPrefixLen), bytes))
            return false;

        if (bytes.size() < 6)
            return false;

        uint8 const expected = Checksum(bytes, 0, bytes.size() - 1);
        if (bytes.back() != expected)
            return false;

        size_t pos = 0;
        if (bytes[pos++] != kVersion)
            return false;

        kind = bytes[pos++];
        faction = bytes[pos++];
        uint16 const count = uint16((uint16(bytes[pos]) << 8) | bytes[pos + 1]);
        pos += 2;

        if (!count || count > kMaxPlacedDecor)
            return false;

        std::vector<DecorPlacement> decoded;
        decoded.reserve(count);

        for (uint16 index = 0; index < count; ++index)
        {
            DecorPlacement placement;

            if (!GetVarint(bytes, pos, placement.DecorId))
                return false;

            if (pos + 6 > bytes.size())
                return false;

            placement.X = float(GetInt16(bytes, pos)) / 100.0f;
            placement.Y = float(GetInt16(bytes, pos)) / 100.0f;
            placement.Z = float(GetInt16(bytes, pos)) / 100.0f;
            placement.O = float(bytes[pos++]) / 40.625f;

            float scale = float(bytes[pos++]) / 50.0f;
            if (scale < 0.1f)
                scale = 0.1f;
            placement.Scale = scale;

            uint8 const flags = bytes[pos++];
            placement.Indoor = (flags & 0x01) != 0;
            placement.Surface = uint8((flags >> 1) & 0x03);

            decoded.push_back(placement);
        }

        out.swap(decoded);
        return true;
    }
} // namespace Housing
