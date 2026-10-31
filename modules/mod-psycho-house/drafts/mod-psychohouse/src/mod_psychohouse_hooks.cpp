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
 * Lifecycle and commands: the world loads the catalogue, the player script moves a
 * character in and out of their phase, and the command tree gives the same actions a
 * keyboard path for anyone who would rather not walk to an NPC.
 */

#include "mod_psychohouse.h"

#include "Chat.h"
#include "Player.h"
#include "RBAC.h"
#include "ScriptMgr.h"
#include "WorldSession.h"

#include <string>
#include <vector>

class PsychoHouseWorldScript : public WorldScript
{
public:
    PsychoHouseWorldScript() : WorldScript("psychohouse_world") { }

    void OnConfigLoad(bool /*reload*/) override
    {
        sPsychoHouse->LoadConfig();
    }

    void OnStartup() override
    {
        sPsychoHouse->LoadConfig();
        sPsychoHouse->LoadFacilities();
    }

    void OnShutdown() override
    {
        sPsychoHouse->Unload();
    }
};

class PsychoHousePlayerScript : public PlayerScript
{
public:
    PsychoHousePlayerScript() : PlayerScript("psychohouse_player") { }

    void OnLogin(Player* player, bool /*firstLogin*/) override
    {
        sPsychoHouse->OnPlayerLogin(player);
    }

    void OnLogout(Player* player) override
    {
        sPsychoHouse->OnPlayerLogout(player);
    }

    void OnUpdateZone(Player* player, uint32 /*newZone*/, uint32 /*oldZone*/, uint32 newArea) override
    {
        if (!player)
            return;

        if (newArea == sPsychoHouse->AreaId())
            sPsychoHouse->OnPlayerEnterArea(player);
        else
            sPsychoHouse->OnPlayerLeaveArea(player);
    }

    void OnMapChanged(Player* player) override
    {
        if (!player)
            return;

        if (player->GetAreaId() == sPsychoHouse->AreaId())
            sPsychoHouse->OnPlayerEnterArea(player);
        else
            sPsychoHouse->OnPlayerLeaveArea(player);
    }
};

class PsychoHouseCommandScript : public CommandScript
{
public:
    PsychoHouseCommandScript() : CommandScript("psychohouse_commandscript") { }

    std::vector<ChatCommand> GetCommands() const override
    {
        static std::vector<ChatCommand> commandTable =
        {
            // 0 is every account: these four are the player's own house, not a GM tool.
            { "psychohouse",  0, false, &HandleInfoCommand, "Island house: status" },
            { "psychobuy",    0, false, &HandleBuyCommand,  "Island house: buy" },
            { "psychosell",   0, false, &HandleSellCommand, "Island house: sell" },
            { "psychohome",   0, false, &HandleTeleCommand, "Island house: travel" },
            { "psychodiag",   rbac::RBAC_PERM_COMMAND_GM, false, &HandleDiagCommand,   "Island house: catalogue state" },
            { "psychoreload", rbac::RBAC_PERM_COMMAND_GM, false, &HandleReloadCommand, "Island house: reload" },
        };

        return commandTable;
    }

    static bool HandleInfoCommand(ChatHandler* handler, char const* /*args*/)
    {
        Player* player = handler->GetSession() ? handler->GetSession()->GetPlayer() : nullptr;
        if (!player)
            return false;

        PsychoHouse::Owner* owner = sPsychoHouse->GetOwner(player);
        if (!owner)
        {
            handler->SendSysMessage("No island house on this character yet. Buy one from the Keeper.");
            return true;
        }

        handler->PSendSysMessage("House phase %u, %u of %u facilities bought.",
            uint32(owner->Phase), uint32(owner->Bought.size()),
            uint32(sPsychoHouse->GetFacilities().size()));
        return true;
    }

    static bool HandleBuyCommand(ChatHandler* handler, char const* /*args*/)
    {
        Player* player = handler->GetSession() ? handler->GetSession()->GetPlayer() : nullptr;
        if (!player)
            return false;

        PsychoHouse::HouseResult const result = sPsychoHouse->BuyHouse(player);
        handler->SendSysMessage(PsychoHouse::ResultText(result));
        return true;
    }

    static bool HandleSellCommand(ChatHandler* handler, char const* /*args*/)
    {
        Player* player = handler->GetSession() ? handler->GetSession()->GetPlayer() : nullptr;
        if (!player)
            return false;

        PsychoHouse::HouseResult const result = sPsychoHouse->SellHouse(player);
        handler->SendSysMessage(PsychoHouse::ResultText(result));
        return true;
    }

    static bool HandleTeleCommand(ChatHandler* handler, char const* /*args*/)
    {
        Player* player = handler->GetSession() ? handler->GetSession()->GetPlayer() : nullptr;
        if (!player)
            return false;

        if (!sPsychoHouse->TeleportHome(player))
        {
            handler->SendSysMessage(PsychoHouse::ResultText(PsychoHouse::NoHouse));
            return true;
        }

        handler->SendSysMessage("Travelling to the island house.");
        return true;
    }

    static bool HandleDiagCommand(ChatHandler* handler, char const* /*args*/)
    {
        handler->SendSysMessage(sPsychoHouse->Diagnostics().c_str());
        return true;
    }

    static bool HandleReloadCommand(ChatHandler* handler, char const* /*args*/)
    {
        sPsychoHouse->LoadConfig();
        sPsychoHouse->LoadFacilities();
        handler->SendSysMessage(sPsychoHouse->Diagnostics().c_str());
        return true;
    }
};

void AddSC_psychohouse_hooks()
{
    new PsychoHouseWorldScript();
    new PsychoHousePlayerScript();
    new PsychoHouseCommandScript();
}
