/*
 * Copyright (C) 2022 BfaCore Reforged
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

/*
 * Guild neighbourhoods, driven by the guild rather than by a command.
 *
 * Creating one is still an explicit act (`.housing create guild`), because a guild should
 * not be handed a village the moment it is founded. What was missing is the other half:
 * when a member leaves or is kicked their plot is released, and when the guild disbands the
 * neighbourhood is dissolved. Without these two hooks a guild village slowly fills with
 * houses belonging to people who are no longer in the guild, and a dead guild keeps a phase
 * slot forever.
 *
 * Member removal arrives as a character GUID, not an account id, so the lookup starts from
 * the module's own table: `character_housing.ownerGuid` is the character that bought the
 * plot. A plot bought by an alt of a member is released when that alt leaves, which is the
 * correct reading - the plot belongs to the account that bought it, and that account's
 * member character is gone.
 */

#include "DatabaseEnv.h"
#include "HousingMgr.h"
#include "Guild.h"
#include "Log.h"
#include "Player.h"
#include "ScriptMgr.h"

using namespace Housing;

class HousingGuildScript : public GuildScript
{
public:
    HousingGuildScript() : GuildScript("HousingGuildScript") { }

    void OnRemoveMember(Guild* guild, ObjectGuid guid, bool isDisbanding, bool /*isKicked*/) override
    {
        if (!guild || !guid || isDisbanding)
            return;   // a disband is handled by OnDisband, one neighbourhood at a time

        HousingNeighborhood* neighborhood = FindGuildNeighborhood(guild->GetId());
        if (!neighborhood)
            return;

        HousingPlot* plot = FindPlotOfCharacter(neighborhood, guid.GetCounter());
        if (!plot)
            return;

        TC_LOG_INFO("housing", "guild {} lost member " UI64FMTD ": releasing plot {}",
            guild->GetId(), uint64(guid.GetCounter()), uint32(plot->GetIndex()));

        sHousingMgr->ForceSellPlot(plot);
    }

    void OnDisband(Guild* guild) override
    {
        if (!guild)
            return;

        HousingNeighborhood* neighborhood = FindGuildNeighborhood(guild->GetId());
        if (!neighborhood)
            return;

        TC_LOG_INFO("housing", "guild {} disbanded: dissolving neighbourhood {}",
            guild->GetId(), neighborhood->GetId());

        sHousingMgr->DissolveNeighborhood(neighborhood);
    }

private:
    static HousingNeighborhood* FindGuildNeighborhood(uint64 guildId)
    {
        for (auto const& entry : sHousingMgr->GetNeighborhoods())
            if (entry.second->GetType() == NeighborhoodGuild && entry.second->GetGuildId() == guildId)
                return entry.second.get();

        return nullptr;
    }

    static HousingPlot* FindPlotOfCharacter(HousingNeighborhood* neighborhood, uint64 characterGuid)
    {
        if (!neighborhood || !characterGuid)
            return nullptr;

        for (auto const& plot : neighborhood->GetPlots())
            if (plot->IsOwned() && plot->GetOwnerGuid().GetCounter() == characterGuid)
                return plot.get();

        // The plot may have been bought by another character on the same account, which is
        // still that account leaving the guild. Resolve the account from the module table.
        if (QueryResult result = CharacterDatabase.Query(
            "SELECT `ownerAccount` FROM `character_housing` WHERE `ownerGuid` = {}", characterGuid))
        {
            uint32 const accountId = result->Fetch()[0].GetUInt32();
            for (auto const& plot : neighborhood->GetPlots())
                if (plot->IsOwned() && plot->GetOwnerAccount() == accountId)
                    return plot.get();
        }

        return nullptr;
    }
};

void AddSC_housing_guild_script()
{
    new HousingGuildScript();
}
