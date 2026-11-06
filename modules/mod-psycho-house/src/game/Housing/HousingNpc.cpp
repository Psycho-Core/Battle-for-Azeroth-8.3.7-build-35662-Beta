/*
 * Copyright (C) 2022 BfaCore Reforged
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

/*
 * Player housing - the NPCs a player can talk to.
 *
 * The addon is the interface for everything a player does on a housing map, and a command
 * exists for every one of those actions, but neither of them is an entry point: a player who
 * has never installed an addon and does not read patch notes has to be able to walk up to an
 * NPC and be told what housing is. That is what this file is.
 *
 * Three roles, all selected by `creature_template.ScriptName` rather than by entry id, so the
 * module never has to own a creature:
 *
 *   npc_housing_steward      found a neighbourhood, travel home, visit somewhere, the gryphon
 *   npc_housing_contractor   upgrade the house, change the exterior shell, buy a room
 *   npc_housing_coffer       Endeavor progress, the Coffer, the weekly Arcanostorage Sphere
 *
 * The gossip text is written out here rather than in `npc_text`: a module that needs a world
 * database row before its NPCs say anything is a module that ships silent on half the realms
 * that install it. `AddGossipItemFor` with a literal string needs nothing in the database.
 */

#include "HousingMgr.h"

#include "Chat.h"
#include "Creature.h"
#include "HousingEndeavor.h"
#include "ObjectAccessor.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "ScriptedGossip.h"
#include "StringFormat.h"

#include "WorldSession.h"

#include <map>
#include <string>
#include <vector>

namespace Housing
{
    namespace
    {
        // Gossip action ids are per-NPC and only have to be unique within the menu.
        enum StewardAction : uint32
        {
            StewardHome       = GOSSIP_ACTION_INFO_DEF + 1,
            StewardFound      = GOSSIP_ACTION_INFO_DEF + 2,
            StewardFinder     = GOSSIP_ACTION_INFO_DEF + 3,
            StewardFindPlot   = GOSSIP_ACTION_INFO_DEF + 4,   // "show me a free plot"
            StewardGryphon    = GOSSIP_ACTION_INFO_DEF + 5,
            StewardInvite     = GOSSIP_ACTION_INFO_DEF + 6,
            StewardLeave      = GOSSIP_ACTION_INFO_DEF + 7,
            StewardRank       = GOSSIP_ACTION_INFO_DEF + 8,
            StewardEndeavor   = GOSSIP_ACTION_INFO_DEF + 9   // private neighbourhoods only
        };

        enum ContractorAction : uint32
        {
            ContractorUpgrade  = GOSSIP_ACTION_INFO_DEF + 1,
            ContractorShell    = GOSSIP_ACTION_INFO_DEF + 2,
            ContractorRoom     = GOSSIP_ACTION_INFO_DEF + 3,
            ContractorExpand   = GOSSIP_ACTION_INFO_DEF + 4
        };

        enum CofferAction : uint32
        {
            CofferStatus      = GOSSIP_ACTION_INFO_DEF + 1,
            CofferDeliver     = GOSSIP_ACTION_INFO_DEF + 2,
            CofferClaim       = GOSSIP_ACTION_INFO_DEF + 3,
            CofferArcane      = GOSSIP_ACTION_INFO_DEF + 4,
            CofferArmillary   = GOSSIP_ACTION_INFO_DEF + 5
        };

        void Say(Player* player, HousingResult result, char const* ok)
        {
            if (!player || !player->GetSession())
                return;

            ChatHandler handler(player->GetSession());

            if (result == Ok)
                handler.SendSysMessage(ok);
            else
                handler.PSendSysMessage("Housing: {} (code {})", HousingMgr::ResultName(result),
                    uint32(result));
        }
    }

    // -------------------------------------------------------------------------
    // Steward
    // -------------------------------------------------------------------------

    class npc_housing_steward : public CreatureScript
    {
    public:
        npc_housing_steward() : CreatureScript("npc_housing_steward") { }

        bool OnGossipHello(Player* player, Creature* creature) override
        {
            if (!sHousingMgr->IsEnabled())
                return false;

            HousingNeighborhood* neighborhood = sHousingMgr->FindNeighborhoodFor(player);
            HousingPlot* owned = sHousingMgr->GetOwnedPlot(player);

            AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Take me to my neighbourhood.", GOSSIP_SENDER_MAIN, StewardHome);

            if (!owned)
                AddGossipItemFor(player, GOSSIP_ICON_CHAT, "I want to found a neighbourhood.", GOSSIP_SENDER_MAIN, StewardFound);
            else
            {
                AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Show me the plots.", GOSSIP_SENDER_MAIN, StewardFindPlot);

                if (!neighborhood || neighborhood->GetType() != NeighborhoodPublic)
                {
                    AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Invite someone into the neighbourhood.", GOSSIP_SENDER_MAIN, StewardInvite);
                    AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Promote or demote a neighbour.", GOSSIP_SENDER_MAIN, StewardRank);
                }

                if (neighborhood && neighborhood->GetType() != NeighborhoodGuild && !owned->IsOwned()
                    && neighborhood->IsMember(player->GetSession()->GetAccountId()))
                    AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Leave the neighbourhood.", GOSSIP_SENDER_MAIN, StewardLeave);

                // A public neighbourhood takes whatever Endeavor the rotation gives it. A
                // private one is the founder's to choose, which is the whole difference
                // between a street and a household.
                if (neighborhood && neighborhood->GetType() == NeighborhoodPrivate
                    && neighborhood->GetRank(player->GetSession()->GetAccountId()) >= 3)   // founder
                    AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Choose what we work on.", GOSSIP_SENDER_MAIN, StewardEndeavor);
            }

            AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Which houses can I visit?", GOSSIP_SENDER_MAIN, StewardFinder);
            AddGossipItemFor(player, GOSSIP_ICON_TAXI, "Fly me to my plot.", GOSSIP_SENDER_MAIN, StewardGryphon);

            player->PlayerTalkClass->SendGossipMenu(1, creature->GetGUID());
            return true;
        }

        bool OnGossipSelect(Player* player, Creature* /*creature*/, uint32 /*sender*/, uint32 action) override
        {
            if (!player)
                return false;

            player->PlayerTalkClass->ClearMenus();

            switch (action)
            {
                case StewardHome:
                    Say(player, sHousingMgr->TeleportToTownCenter(player), "Here we are.");
                    break;
                case StewardFound:
                    Say(player, sHousingMgr->FoundNeighborhood(player), "Founded. Plots are on sale.");
                    break;
                case StewardEndeavor:
                {
                    HousingNeighborhood* neighborhood = sHousingMgr->FindNeighborhoodFor(player);
                    if (!neighborhood)
                        break;

                    uint32 const current = sHousingEndeavor->GetActiveId(neighborhood->GetId());
                    ChatHandler menu(player->GetSession());
                    menu.SendSysMessage("Endeavors the neighbourhood can take up:");

                    for (EndeavorInfo const& info : sHousingEndeavor->GetInfos())
                        menu.PSendSysMessage("  {} - {}{}", info.Id, info.Name,
                            info.Id == current ? " (running now)" : "");

                    menu.SendSysMessage("Pick one with .psychohouse endeavor <id>.");
                    break;
                }
                case StewardFindPlot:
                {
                    HousingNeighborhood* neighborhood = sHousingMgr->FindNeighborhoodFor(player);
                    ChatHandler handler(player->GetSession());

                    if (!neighborhood)
                    {
                        handler.SendSysMessage("You are not in a neighbourhood.");
                        break;
                    }

                    uint32 free = 0;
                    for (auto const& plot : neighborhood->GetPlots())
                        if (!plot->IsOwned())
                            ++free;

                    handler.PSendSysMessage("'{}' has {} free plot(s) of {}.", neighborhood->GetName(),
                        free, uint32(kPlotCount), uint32(kPlotCount));
                    handler.PSendSysMessage("Plots are bought at the gate of the plot itself, or with "
                        ".housing buy <plot>.");
                    break;
                }
                case StewardFinder:
                    for (HousingNeighborhood* open : sHousingMgr->GetPublicNeighborhoods(
                             uint8(player->GetTeamId() == TEAM_HORDE ? FactionHorde : FactionAlliance)))
                        handler.PSendSysMessage("#{} '{}' - {}/{} occupied. /housing visit {} <plot>",
                            open->GetId(), open->GetName(), open->GetOwnedCount(), uint32(kPlotCount),
                            open->GetId());

                    sHousingMgr->SendResult(player, "FINDER", Ok);
                    break;
                case StewardGryphon:
                    Say(player, sHousingMgr->CallGryphon(player), "Mind the drop.");
                    break;
                case StewardInvite:
                {
                    ChatHandler handler(player->GetSession());
                    handler.SendSysMessage("Use .housing neighbor invite <name> - the chat command, "
                        "because gossip cannot take a name.");
                    break;
                }
                case StewardRank:
                    ChatHandler(player->GetSession()).SendSysMessage(
                        "Use .housing neighbor rank <name> <1-3>.");
                    break;
                case StewardLeave:
                    Say(player, sHousingMgr->LeaveNeighborhood(player), "You have left the neighbourhood.");
                    break;
                default:
                    break;
            }

            player->PlayerTalkClass->SendCloseGossip();
            return true;
        }
    };

    // -------------------------------------------------------------------------
    // General Contractor
    // -------------------------------------------------------------------------

    class npc_housing_contractor : public CreatureScript
    {
    public:
        npc_housing_contractor() : CreatureScript("npc_housing_contractor") { }

        bool OnGossipHello(Player* player, Creature* creature) override
        {
            if (!sHousingMgr->IsEnabled())
                return false;

            HousingPlot* plot = sHousingMgr->GetOwnedPlot(player);
            if (!plot)
            {
                AddGossipItemFor(player, GOSSIP_ICON_CHAT, "I do not own a house yet.", GOSSIP_SENDER_MAIN,
                    ContractorExpand);
            }
            else
            {
                uint8 const level = plot->GetHouse() ? plot->GetHouse()->GetTier() : 1;
                AddGossipItemFor(player, GOSSIP_ICON_MONEY_BAG, "Upgrade my house.", GOSSIP_SENDER_MAIN, ContractorUpgrade);
                AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Change the outside of my house.", GOSSIP_SENDER_MAIN, ContractorShell);
                AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Add a room.", GOSSIP_SENDER_MAIN, ContractorRoom);
                AddGossipItemFor(player, GOSSIP_ICON_CHAT, "How big is my house?", GOSSIP_SENDER_MAIN, ContractorExpand);

                (void)level;
            }

            player->PlayerTalkClass->SendGossipMenu(1, creature->GetGUID());
            return true;
        }

        bool OnGossipSelect(Player* player, Creature* /*creature*/, uint32 /*sender*/, uint32 action) override
        {
            if (!player)
                return false;

            player->PlayerTalkClass->ClearMenus();
            ChatHandler handler(player->GetSession());

            switch (action)
            {
                case ContractorUpgrade:
                    Say(player, sHousingMgr->LevelUpWithGold(player), "Signed, sealed, one level higher.");
                    break;
                case ContractorShell:
                {
                    HousingPlot* plot = sHousingMgr->GetOwnedPlot(player);
                    if (!plot)
                        break;

                    handler.SendSysMessage("Exteriors you can build:");
                    for (auto const& entry : sHousingMgr->GetShells())
                    {
                        ShellInfo const& shell = entry.second;
                        if (shell.LevelRequired > (plot->GetHouse() ? plot->GetHouse()->GetTier() : 1))
                            continue;

                        handler.PSendSysMessage("  {} - {} - build with .housing shell {}", shell.Id,
                            shell.Name, shell.Id);
                    }
                    break;
                }
                case ContractorRoom:
                {
                    HousingPlot* plot = sHousingMgr->GetOwnedPlot(player);
                    if (!plot)
                        break;

                    handler.SendSysMessage("Rooms you can add:");

                    for (RoomInfo const& room : sHousingMgr->GetRoomsForPlot(plot))
                    {
                        HousingResult const deed = sHousingMgr->CanBuyRoom(player, room.Id);
                        handler.PSendSysMessage("  {} - {} - {} - .housing room {}", room.Id, room.Name,
                            HousingMgr::ResultName(deed), room.Id);
                    }
                    break;
                }
                case ContractorExpand:
                {
                    HousingPlot* plot = sHousingMgr->GetOwnedPlot(player);
                    if (!plot || !plot->GetHouse())
                    {
                        handler.SendSysMessage("Buy a plot first - the Steward sells them.");
                        break;
                    }

                    HousingHouse* house = plot->GetHouse();
                    HouseLevelInfo const* info = sHousingMgr->GetHouseLevel(house->GetTier());
                    handler.PSendSysMessage("House level {}: {} indoor budget, {} room budget, {} outdoor.",
                        uint32(house->GetTier()), info ? info->BudgetIndoor : 0,
                        info ? info->BudgetRoom : 0, info ? info->BudgetOutdoor : 0);
                    handler.PSendSysMessage("Applied exterior: {}, rooms bought: {}.",
                        house->GetShellId(), house->GetRoomsOwned());
                    break;
                }
                default:
                    break;
            }

            player->PlayerTalkClass->SendCloseGossip();
            return true;
        }
    };

    // -------------------------------------------------------------------------
    // Coffer
    // -------------------------------------------------------------------------

    class npc_housing_coffer : public CreatureScript
    {
    public:
        npc_housing_coffer() : CreatureScript("npc_housing_coffer") { }

        bool OnGossipHello(Player* player, Creature* creature) override
        {
            if (!sHousingMgr->IsEnabled())
                return false;

            AddGossipItemFor(player, GOSSIP_ICON_CHAT, "How is the Endeavor going?", GOSSIP_SENDER_MAIN, CofferStatus);
            AddGossipItemFor(player, GOSSIP_ICON_CHAT, "I have brought what the Endeavor asked for.", GOSSIP_SENDER_MAIN, CofferDeliver);
            AddGossipItemFor(player, GOSSIP_ICON_MONEY_BAG, "I have earned a milestone.", GOSSIP_SENDER_MAIN, CofferClaim);
            AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Is the Arcanostorage Sphere ready?", GOSSIP_SENDER_MAIN, CofferArcane);
            AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Light the Armillary.", GOSSIP_SENDER_MAIN, CofferArmillary);

            player->PlayerTalkClass->SendGossipMenu(1, creature->GetGUID());
            return true;
        }

        bool OnGossipSelect(Player* player, Creature* /*creature*/, uint32 /*sender*/, uint32 action) override
        {
            if (!player)
                return false;

            player->PlayerTalkClass->ClearMenus();
            ChatHandler handler(player->GetSession());

            switch (action)
            {
                case CofferStatus:
                {
                    uint32 const neighborhoodId = sHousingMgr->GetPlayerNeighborhoodId(player);
                    if (!neighborhoodId)
                    {
                        handler.SendSysMessage("You are not in a neighbourhood.");
                        break;
                    }

                    EndeavorInfo const* info = sHousingEndeavor->GetInfo(sHousingEndeavor->GetActiveId(neighborhoodId));
                    handler.PSendSysMessage("Endeavor: {}", info ? info->Name : "none running");
                    handler.PSendSysMessage("Milestone {}/4, {} task(s) complete.",
                        uint32(sHousingEndeavor->GetMilestone(neighborhoodId)),
                        sHousingEndeavor->GetCompletedTasks(neighborhoodId));
                    break;
                }
                case CofferDeliver:
                    Say(player, sHousingEndeavor->Deliver(player) ? Ok : NotReady, "Accepted.");
                    break;
                case CofferClaim:
                {
                    uint8 const milestone = sHousingEndeavor->GetMilestoneFor(player);
                    ChatHandler(player->GetSession()).PSendSysMessage(
                        "You can claim milestone {}. Use .housing claim <milestone>.", uint32(milestone));
                    break;
                }
                case CofferArcane:
                    Say(player, sHousingMgr->OpenArcanostorage(player), "The Sphere turns.");
                    break;
                case CofferArmillary:
                    Say(player, sHousingMgr->ToggleArmillary(player), "The lights change.");
                    break;
                default:
                    break;
            }

            player->PlayerTalkClass->SendCloseGossip();
            return true;
        }
    };

    // -------------------------------------------------------------------------
    // Vendor
    // -------------------------------------------------------------------------

    /*
     * The decor shops. A shop is bound by name, not by entry, because `housing_vendor` is
     * authored against the retail NPCs and a realm that has renamed one has still got the
     * same shop on the same spot.
     *
     * Ten lines a page, which is what a gossip menu holds without scrolling off the screen.
     * The page's decor ids are kept per player so that an action id only has to mean "line 3
     * of what you are looking at".
     */
    class npc_housing_vendor : public CreatureScript
    {
    public:
        npc_housing_vendor() : CreatureScript("npc_housing_vendor") { }

        bool OnGossipHello(Player* player, Creature* creature) override
        {
            if (!sHousingMgr->IsEnabled())
                return false;

            VendorInfo const* vendor = ResolveVendor(creature);
            if (!vendor)
            {
                // A creature tagged as a housing vendor that no `housing_vendor` row names is
                // a mistake in the world database. Say so rather than opening an empty menu.
                AddGossipItemFor(player, GOSSIP_ICON_CHAT, "You have nothing to sell yet.", GOSSIP_SENDER_MAIN, VendorNothing);
                player->PlayerTalkClass->SendGossipMenu(1, creature->GetGUID());
                return true;
            }

            ShowPage(player, creature, *vendor, 0);
            return true;
        }

        bool OnGossipSelect(Player* player, Creature* creature, uint32 /*sender*/, uint32 action) override
        {
            if (!player || !creature)
                return false;

            VendorInfo const* vendor = ResolveVendor(creature);

            player->PlayerTalkClass->ClearMenus();

            if (!vendor)
                return false;

            if (action == VendorBack)
            {
                ShowPage(player, creature, *vendor, 0);
                return true;
            }

            if (action == VendorNext)
            {
                ShowPage(player, creature, *vendor, _pageIndex[player->GetGUID()] + 1);
                return true;
            }

            if (action == VendorNothing)
            {
                player->PlayerTalkClass->SendCloseGossip();
                return true;
            }

            uint32 const slot = action >= VendorBuy ? action - VendorBuy : 0xFFFFFFFF;
            if (slot < kPageSize)
            {
                std::vector<uint32>& page = _pages[player->GetGUID()];
                if (slot >= page.size())
                    return false;

                HousingResult const result = sHousingMgr->BuyDecor(player, page[slot]);
                ChatHandler handler(player->GetSession());
                handler.PSendSysMessage("{}", HousingMgr::ResultName(result));

                if (result == Ok)
                    ShowPage(player, creature, *vendor, uint32(_pageIndex[player->GetGUID()]));
                else
                    player->PlayerTalkClass->SendCloseGossip();

                return true;
            }

            return false;
        }

    private:
        static constexpr uint32 kPageSize = 10;
        static constexpr uint32 VendorBuy = GOSSIP_ACTION_INFO_DEF + 1;
        static constexpr uint32 VendorNext = GOSSIP_ACTION_INFO_DEF + kPageSize + 1;
        static constexpr uint32 VendorBack = GOSSIP_ACTION_INFO_DEF + kPageSize + 2;
        static constexpr uint32 VendorNothing = GOSSIP_ACTION_INFO_DEF + kPageSize + 3;

        std::map<ObjectGuid, std::vector<uint32>> _pages;
        std::map<ObjectGuid, uint32> _pageIndex;

        static VendorInfo const* ResolveVendor(Creature* creature)
        {
            if (!creature)
                return nullptr;

            uint32 const entry = creature->GetEntry();
            std::string const name = creature->GetName();

            for (VendorInfo const& vendor : sHousingMgr->GetVendors())
            {
                if (vendor.CreatureEntry && vendor.CreatureEntry == entry)
                    return &vendor;

                if (vendor.Name == name)
                    return &vendor;
            }

            return nullptr;
        }

        void ShowPage(Player* player, Creature* creature, VendorInfo const& vendor, uint32 page)
        {
            uint8 const factionBit = player->GetTeamId() == TEAM_ALLIANCE ? 0x01 : 0x02;
            bool const smuggler = vendor.Role == RoleSmuggler;

            std::vector<uint32> shown;

            for (VendorStockEntry const& entry : sHousingMgr->GetVendorStock(vendor.Id))
            {
                DecorInfo const* info = sHousingMgr->GetDecor(entry.DecorId);
                if (!info)
                    continue;

                // A smuggler's whole point is the stock the buyer's own faction cannot get,
                // so its faction mask is ignored here and the mark-up is quoted instead.
                if (!smuggler && !(info->FactionMask & factionBit))
                    continue;

                if (!sHousingMgr->IsStockAvailable(player, *info))
                    continue;

                shown.push_back(info->Id);
            }

            if (shown.empty())
            {
                AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Nothing on the shelf today.", GOSSIP_SENDER_MAIN, VendorNothing);
                player->PlayerTalkClass->SendGossipMenu(1, creature->GetGUID());
                return;
            }

            uint32 const pages = (uint32(shown.size()) + kPageSize - 1) / kPageSize;
            page = std::min(page, pages - 1);

            uint32 const start = page * kPageSize;
            uint32 const end = std::min<uint32>(start + kPageSize, uint32(shown.size()));

            std::vector<uint32>& slotIds = _pages[player->GetGUID()];
            slotIds.assign(shown.begin() + start, shown.begin() + end);
            _pageIndex[player->GetGUID()] = page;

            for (uint32 index = start; index < end; ++index)
            {
                DecorInfo const* info = sHousingMgr->GetDecor(shown[index]);
                if (!info)
                    continue;

                std::string const owned = sHousingMgr->HasUnlocked(player->GetSession()->GetAccountId(), info->Id)
                    ? " (known)" : "";

                std::string const line = Trinity::StringFormat("{} - {}{}",
                    info->Name, sHousingMgr->GetPurchasePrice(player, *info), owned);

                AddGossipItemFor(player, GOSSIP_ICON_MONEY_BAG, line, GOSSIP_SENDER_MAIN,
                    VendorBuy + (index - start));
            }

            if (page + 1 < pages)
                AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Show me the rest.", GOSSIP_SENDER_MAIN, VendorNext);

            if (page > 0)
                AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Back to the first page.", GOSSIP_SENDER_MAIN, VendorBack);

            player->PlayerTalkClass->SendGossipMenu(1, creature->GetGUID());
        }
    };
}

void AddSC_housing_npcs()
{
    new Housing::npc_housing_steward();
    new Housing::npc_housing_contractor();
    new Housing::npc_housing_coffer();
    new Housing::npc_housing_vendor();
}
