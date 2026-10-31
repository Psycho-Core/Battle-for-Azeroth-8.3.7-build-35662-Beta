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
 * The two NPCs the module owns, and the only interface it has: gossip.
 *
 * The Keeper sells the house and takes the character home.
 * The Steward sells the facilities, one at a time, from the catalogue table.
 *
 * Both use the money popup the core already provides. The core checks the price before it
 * dispatches the option but never takes it, so the take is here, in the select handler,
 * once per purchase and only after every other check has passed.
 */

#include "mod_psychohouse.h"

#include "Chat.h"
#include "Creature.h"
#include "GossipDef.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "ScriptedGossip.h"
#include "WorldSession.h"

#include <string>

namespace
{
    enum KeeperAction : uint32
    {
        KeeperBuy     = 1,
        KeeperTeleport = 2,
        KeeperSell    = 3,
        KeeperClose   = 4
    };

    uint32 const StewardBuyBase = 100;

    std::string Coins(uint32 copper)
    {
        uint32 gold = copper / 10000;
        uint32 silver = (copper % 10000) / 100;
        uint32 rest = copper % 100;

        std::string text = std::to_string(gold);
        text += "g";

        if (silver)
        {
            text += " ";
            text += std::to_string(silver);
            text += "s";
        }

        if (rest)
        {
            text += " ";
            text += std::to_string(rest);
            text += "c";
        }

        return text;
    }

    void Tell(Player* player, std::string const& text)
    {
        ChatHandler(player->GetSession()).SendSysMessage(text.c_str());
    }
}

class PsychoHouseKeeper : public CreatureScript
{
public:
    PsychoHouseKeeper() : CreatureScript("psychohouse_keeper") { }

    bool OnGossipHello(Player* player, Creature* creature) override
    {
        if (!sPsychoHouse->Enabled())
        {
            Tell(player, "Housing is switched off on this realm.");
            CloseGossipMenuFor(player);
            return true;
        }

        ClearGossipMenuFor(player);

        PsychoHouse::Owner* owner = sPsychoHouse->GetOwner(player);

        if (!owner)
        {
            std::string text = "Buy this island house for ";
            text += Coins(sPsychoHouse->HousePrice());
            text += "\nOne per character. The Steward furnishes it once the key is yours.";

            AddGossipItemFor(player, GOSSIP_ICON_MONEY_BAG, text, GOSSIP_SENDER_MAIN, KeeperBuy,
                "Buy the house?", sPsychoHouse->HousePrice(), false);
        }
        else
        {
            AddGossipItemFor(player, GOSSIP_ICON_TAXI, "Take me to my house", GOSSIP_SENDER_MAIN, KeeperTeleport);
            AddGossipItemFor(player, GOSSIP_ICON_MONEY_BAG, "Sell this house back", GOSSIP_SENDER_MAIN, KeeperSell,
                "Sell the house? The refund is a share of the price, and every facility goes with it.", 0, false);
        }

        AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Never mind", GOSSIP_SENDER_MAIN, KeeperClose);
        SendGossipMenuFor(player, DEFAULT_GOSSIP_MESSAGE, creature->GetGUID());
        return true;
    }

    bool OnGossipSelect(Player* player, Creature* creature, uint32 /*sender*/, uint32 action) override
    {
        ClearGossipMenuFor(player);

        switch (action)
        {
            case KeeperBuy:
            {
                PsychoHouse::HouseResult const result = sPsychoHouse->BuyHouse(player);
                Tell(player, std::string("House: ") + PsychoHouse::ResultText(result));
                if (result == PsychoHouse::Ok)
                    Tell(player, "The house is behind this island's gate. Talk to the Keeper again to travel.");

                CloseGossipMenuFor(player);
                return true;
            }
            case KeeperTeleport:
            {
                if (!sPsychoHouse->TeleportHome(player))
                    Tell(player, std::string("House: ") + PsychoHouse::ResultText(PsychoHouse::NoHouse));

                CloseGossipMenuFor(player);
                return true;
            }
            case KeeperSell:
            {
                PsychoHouse::HouseResult const result = sPsychoHouse->SellHouse(player);
                Tell(player, std::string("Sale: ") + PsychoHouse::ResultText(result));
                CloseGossipMenuFor(player);
                return true;
            }
            case KeeperClose:
            default:
                CloseGossipMenuFor(player);
                return true;
        }
    }
};

class PsychoHouseSteward : public CreatureScript
{
public:
    PsychoHouseSteward() : CreatureScript("psychohouse_steward") { }

    bool OnGossipHello(Player* player, Creature* creature) override
    {
        if (!sPsychoHouse->Enabled())
        {
            Tell(player, "Housing is switched off on this realm.");
            CloseGossipMenuFor(player);
            return true;
        }

        PsychoHouse::Owner* owner = sPsychoHouse->GetOwner(player);
        if (!owner || !owner->InHouse)
        {
            Tell(player, "The Steward only serves inside the house the character owns.");
            CloseGossipMenuFor(player);
            return true;
        }

        ClearGossipMenuFor(player);

        uint32 offered = 0;
        for (PsychoHouse::Facility const& facility : sPsychoHouse->GetFacilities())
        {
            if (!facility.Entry)
                continue;

            if (sPsychoHouse->HasFacility(owner, facility.Id))
                continue;

            std::string text = facility.Name;
            text += " - ";
            text += Coins(facility.Price);

            if (facility.Faction != PsychoHouse::FactionBoth)
            {
                text += " (";
                text += facility.Faction == PsychoHouse::FactionAlliance ? "Alliance" : "Horde";
                text += ")";
            }

            AddGossipItemFor(player, GOSSIP_ICON_MONEY_BAG, text, GOSSIP_SENDER_MAIN,
                StewardBuyBase + facility.Id, "Buy this for the house?", facility.Price, false);
            ++offered;
        }

        if (!offered)
            AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Everything is already furnished", GOSSIP_SENDER_MAIN, 1);

        AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Never mind", GOSSIP_SENDER_MAIN, 1);
        SendGossipMenuFor(player, DEFAULT_GOSSIP_MESSAGE, creature->GetGUID());
        return true;
    }

    bool OnGossipSelect(Player* player, Creature* creature, uint32 /*sender*/, uint32 action) override
    {
        ClearGossipMenuFor(player);

        if (action >= StewardBuyBase)
        {
            uint16 const facilityId = uint16(action - StewardBuyBase);
            PsychoHouse::HouseResult const result = sPsychoHouse->BuyFacility(player, facilityId);

            if (result == PsychoHouse::Ok)
            {
                PsychoHouse::Facility const* facility = sPsychoHouse->FindFacility(facilityId);
                std::string text = facility ? facility->Name : std::string("Facility");
                text += " is in place.";
                Tell(player, text);
            }
            else
            {
                Tell(player, std::string("Purchase: ") + PsychoHouse::ResultText(result));
            }

            CloseGossipMenuFor(player);
            return true;
        }

        CloseGossipMenuFor(player);
        return true;
    }
};

void AddSC_psychohouse_gossip()
{
    new PsychoHouseKeeper();
    new PsychoHouseSteward();
}
