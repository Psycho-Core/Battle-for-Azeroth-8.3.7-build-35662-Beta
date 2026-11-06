/*
 * Copyright (C) 2022 BfaCore Reforged
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

/*
 * Player housing - the objects in the world that answer when clicked.
 *
 * Three of them, all selected by `gameobject_template.ScriptName` so that the module never
 * has to own a gameobject entry:
 *
 *   go_housing_cornerstone   the stone at the edge of a plot: buy it, move in, inspect it
 *   go_housing_coffer        the Endeavor coffer: progress, delivery, the milestone payout
 *   go_housing_bulletin      the notice board: who lives here and which doors are open
 *   go_housing_portal        a portal: out of the neighbourhood, or back into it from a city
 *
 * Every one of them works out which plot it belongs to from its own position, because that
 * is the only thing a gameobject can tell a script. `HousingMgr::FindPlotAt` does the search
 * and returns nothing rather than the wrong house when the stone has been placed off its
 * anchor.
 */

#include "HousingMgr.h"

#include "Chat.h"
#include "GameObject.h"
#include "GossipDef.h"
#include "HousingEndeavor.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "ScriptedGossip.h"
#include "StringFormat.h"

#include <cmath>

namespace Housing
{
    namespace
    {
        enum StoneAction : uint32
        {
            StoneBuy      = GOSSIP_ACTION_INFO_DEF + 1,
            StoneInspect  = GOSSIP_ACTION_INFO_DEF + 2,
            StoneMoveIn   = GOSSIP_ACTION_INFO_DEF + 3,
            StoneSell     = GOSSIP_ACTION_INFO_DEF + 4
        };

        enum CofferGoAction : uint32
        {
            CofferGoStatus  = GOSSIP_ACTION_INFO_DEF + 1,
            CofferGoDeliver = GOSSIP_ACTION_INFO_DEF + 2,
            CofferGoClaim   = GOSSIP_ACTION_INFO_DEF + 3
        };

        enum BulletinAction : uint32
        {
            BulletinResidents = GOSSIP_ACTION_INFO_DEF + 1,
            BulletinOpenDoors = GOSSIP_ACTION_INFO_DEF + 2
        };

        /// The portal row standing at this object's position, if the object is one. Portals
        /// are amenities, so the row carries the amenity, the amenity carries the position.
        HousingPortal const* PortalUnder(GameObject* go)
        {
            if (!go)
                return nullptr;

            float best = 40.0f;
            HousingPortal const* found = nullptr;

            for (HousingPortal const& portal : sHousingMgr->GetPortals())
            {
                if (!portal.Map)
                    continue;

                AmenityInfo const* amenity = sHousingMgr->GetAmenity(portal.AmenityId);
                if (!amenity)
                    continue;

                float const distance = std::sqrt(
                    (amenity->Pos.m_positionX - go->GetPositionX()) * (amenity->Pos.m_positionX - go->GetPositionX())
                    + (amenity->Pos.m_positionY - go->GetPositionY()) * (amenity->Pos.m_positionY - go->GetPositionY()));

                if (distance <= best)
                {
                    best = distance;
                    found = &portal;
                }
            }

            return found;
        }

        HousingPlot* PlotUnder(GameObject* go)
        {
            if (!go)
                return nullptr;

            return sHousingMgr->FindPlotAt(go->GetMapId(), go->GetPositionX(), go->GetPositionY());
        }

        void Say(HousingResult result, Player* player, char const* ok)
        {
            if (!player || !player->GetSession())
                return;

            ChatHandler handler(player->GetSession());
            handler.PSendSysMessage("{}", result == Ok ? ok : HousingMgr::ResultName(result));
        }

        // ---- cornerstone ------------------------------------------------------

        class go_housing_cornerstone : public GameObjectScript
        {
        public:
            go_housing_cornerstone() : GameObjectScript("go_housing_cornerstone") { }

            bool OnGossipHello(Player* player, GameObject* go) override
            {
                if (!sHousingMgr->IsEnabled())
                    return false;

                HousingPlot* plot = PlotUnder(go);
                if (!plot)
                {
                    AddGossipItemFor(player, GOSSIP_ICON_CHAT, "This stone marks nothing.", GOSSIP_SENDER_MAIN, StoneInspect);
                    player->PlayerTalkClass->SendGossipMenu(1, go->GetGUID());
                    return true;
                }

                if (plot->IsOwned())
                {
                    AddGossipItemFor(player, GOSSIP_ICON_CHAT,
                        Trinity::StringFormat("Whose house is this? ({})", plot->GetOwnerName()),
                        GOSSIP_SENDER_MAIN, StoneInspect);
                }
                else
                {
                    AddGossipItemFor(player, GOSSIP_ICON_MONEY_BAG,
                        Trinity::StringFormat("I will take this plot ({}g).", plot->GetPrice()),
                        GOSSIP_SENDER_MAIN, StoneBuy);

                    AddGossipItemFor(player, GOSSIP_ICON_CHAT, "What is this place?", GOSSIP_SENDER_MAIN, StoneInspect);
                }

                if (HousingPlot* mine = sHousingMgr->GetOwnedPlot(player))
                {
                    if (mine != plot)
                        AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Move my house here.", GOSSIP_SENDER_MAIN, StoneMoveIn);
                    else
                        AddGossipItemFor(player, GOSSIP_ICON_CHAT, "I am selling up.", GOSSIP_SENDER_MAIN, StoneSell);
                }

                player->PlayerTalkClass->SendGossipMenu(1, go->GetGUID());
                return true;
            }

            bool OnGossipSelect(Player* player, GameObject* go, uint32 /*sender*/, uint32 action) override
            {
                if (!player)
                    return false;

                player->PlayerTalkClass->ClearMenus();

                HousingPlot* plot = PlotUnder(go);
                if (!plot)
                    return false;

                switch (action)
                {
                    case StoneBuy:
                        Say(sHousingMgr->BuyPlot(player, plot->GetNeighborhood(), plot->GetIndex()),
                            player, "The plot is yours. The Steward will tell you what comes next.");
                        break;
                    case StoneInspect:
                    {
                        ChatHandler handler(player->GetSession());
                        handler.PSendSysMessage("Plot {} in neighbourhood {}. {}",
                            uint32(plot->GetIndex()), plot->GetNeighborhood()->GetId(),
                            plot->IsOwned() ? plot->GetOwnerName() : "Nobody lives here.");
                        break;
                    }
                    case StoneMoveIn:
                        Say(sHousingMgr->MoveHouse(player, plot->GetIndex()), player,
                            "Moved. Nothing was packed away.");
                        break;
                    case StoneSell:
                        Say(sHousingMgr->SellPlot(player), player,
                            "Sold. The house is held for a while in case of regret.");
                        break;
                    default:
                        break;
                }

                player->PlayerTalkClass->SendCloseGossip();
                return true;
            }
        };

        // ---- coffer -----------------------------------------------------------

        class go_housing_coffer : public GameObjectScript
        {
        public:
            go_housing_coffer() : GameObjectScript("go_housing_coffer") { }

            bool OnGossipHello(Player* player, GameObject* go) override
            {
                if (!sHousingMgr->IsEnabled())
                    return false;

                AddGossipItemFor(player, GOSSIP_ICON_CHAT, "How goes the Endeavor?", GOSSIP_SENDER_MAIN, CofferGoStatus);
                AddGossipItemFor(player, GOSSIP_ICON_CHAT, "I brought what was asked for.", GOSSIP_SENDER_MAIN, CofferGoDeliver);
                AddGossipItemFor(player, GOSSIP_ICON_MONEY_BAG, "I have earned a milestone.", GOSSIP_SENDER_MAIN, CofferGoClaim);

                player->PlayerTalkClass->SendGossipMenu(1, go->GetGUID());
                return true;
            }

            bool OnGossipSelect(Player* player, GameObject* /*go*/, uint32 /*sender*/, uint32 action) override
            {
                if (!player)
                    return false;

                player->PlayerTalkClass->ClearMenus();
                ChatHandler handler(player->GetSession());

                switch (action)
                {
                    case CofferGoStatus:
                    {
                        uint32 const neighborhoodId = sHousingMgr->GetPlayerNeighborhoodId(player);
                        if (!neighborhoodId)
                        {
                            handler.SendSysMessage("This coffer belongs to a neighbourhood that is not yours.");
                            break;
                        }

                        handler.PSendSysMessage("Milestone {}/4, {} task(s) done, favour {}.",
                            uint32(sHousingEndeavor->GetMilestone(neighborhoodId)),
                            sHousingEndeavor->GetCompletedTasks(neighborhoodId),
                            sHousingMgr->GetFavor(neighborhoodId));
                        break;
                    }
                    case CofferGoDeliver:
                        Say(sHousingEndeavor->Deliver(player) ? Ok : NotReady, player, "Accepted.");
                        break;
                    case CofferGoClaim:
                    {
                        // The coffer pays the next unclaimed milestone, in order: claiming the
                        // fourth before the first would skip three payouts.
                        uint8 const earned = sHousingEndeavor->GetMilestoneFor(player);
                        uint8 claimed = 0;

                        for (uint8 milestone = 1; milestone <= earned; ++milestone)
                        {
                            if (!sHousingEndeavor->Claim(player, milestone))
                                break;

                            ++claimed;
                        }

                        if (claimed)
                            handler.PSendSysMessage("The coffer pays out for {} milestone(s).", uint32(claimed));
                        else
                            handler.SendSysMessage("Nothing in the coffer that is yours to take.");
                        break;
                    }
                    default:
                        break;
                }

                player->PlayerTalkClass->SendCloseGossip();
                return true;
            }
        };

        // ---- notice board -----------------------------------------------------

        class go_housing_bulletin : public GameObjectScript
        {
        public:
            go_housing_bulletin() : GameObjectScript("go_housing_bulletin") { }

            bool OnGossipHello(Player* player, GameObject* go) override
            {
                if (!sHousingMgr->IsEnabled())
                    return false;

                AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Who lives here?", GOSSIP_SENDER_MAIN, BulletinResidents);
                AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Whose door is open?", GOSSIP_SENDER_MAIN, BulletinOpenDoors);

                player->PlayerTalkClass->SendGossipMenu(1, go->GetGUID());
                return true;
            }

            bool OnGossipSelect(Player* player, GameObject* go, uint32 /*sender*/, uint32 action) override
            {
                if (!player)
                    return false;

                player->PlayerTalkClass->ClearMenus();

                HousingPlot* here = PlotUnder(go);
                HousingNeighborhood* neighborhood = here ? here->GetNeighborhood()
                                                         : sHousingMgr->FindNeighborhoodFor(player);
                if (!neighborhood)
                    return false;

                ChatHandler handler(player->GetSession());

                switch (action)
                {
                    case BulletinResidents:
                    {
                        uint32 count = 0;

                        for (auto const& plot : neighborhood->GetPlots())
                        {
                            if (!plot->IsOwned())
                                continue;

                            handler.PSendSysMessage("Plot {} - {}", uint32(plot->GetIndex()), plot->GetOwnerName());
                            ++count;
                        }

                        if (!count)
                            handler.SendSysMessage("Nobody has built here yet.");

                        break;
                    }
                    case BulletinOpenDoors:
                    {
                        uint32 open = 0;

                        for (auto const& plot : neighborhood->GetPlots())
                        {
                            if (!plot->IsOwned() || (plot->GetHouse() && plot->GetHouse()->IsLocked()))
                                continue;

                            handler.PSendSysMessage("Plot {} is open to visitors.", uint32(plot->GetIndex()));
                            ++open;
                        }

                        if (!open)
                            handler.SendSysMessage("Every door in the neighbourhood is shut.");

                        break;
                    }
                    default:
                        break;
                }

                player->PlayerTalkClass->SendCloseGossip();
                return true;
            }
        };
    }

        // ---- portal -----------------------------------------------------------

        /*
         * One object, two directions. Standing in a neighbourhood, a portal leaves it for
         * wherever the row says; standing anywhere else, the same object is the door in, and
         * it puts the player at the town centre of their own neighbourhood.
         */
        class go_housing_portal : public GameObjectScript
        {
        public:
            go_housing_portal() : GameObjectScript("go_housing_portal") { }

            bool OnGossipHello(Player* player, GameObject* go) override
            {
                if (!sHousingMgr->IsEnabled() || !player)
                    return false;

                if (HousingPortal const* portal = PortalUnder(go))
                {
                    AddGossipItemFor(player, GOSSIP_ICON_CHAT,
                        Trinity::StringFormat("Step through to {}.", portal->Name),
                        GOSSIP_SENDER_MAIN, PortalOut);

                    player->PlayerTalkClass->SendGossipMenu(1, go->GetGUID());
                    return true;
                }

                AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Take me to my neighbourhood.",
                    GOSSIP_SENDER_MAIN, PortalIn);

                player->PlayerTalkClass->SendGossipMenu(1, go->GetGUID());
                return true;
            }

            bool OnGossipSelect(Player* player, GameObject* go, uint32 /*sender*/, uint32 action) override
            {
                if (!player)
                    return false;

                player->PlayerTalkClass->ClearMenus();
                ChatHandler handler(player->GetSession());

                switch (action)
                {
                    case PortalOut:
                        if (HousingPortal const* portal = PortalUnder(go))
                            handler.PSendSysMessage("{}",
                                HousingMgr::ResultName(sHousingMgr->UsePortal(player, portal->Id)));
                        else
                            handler.SendSysMessage("This portal leads nowhere yet.");
                        break;
                    case PortalIn:
                        handler.PSendSysMessage("{}",
                            HousingMgr::ResultName(sHousingMgr->TeleportToTownCenter(player)));
                        break;
                    default:
                        break;
                }

                player->PlayerTalkClass->SendCloseGossip();
                return true;
            }

        private:
            static constexpr uint32 PortalOut = GOSSIP_ACTION_INFO_DEF + 1;
            static constexpr uint32 PortalIn = GOSSIP_ACTION_INFO_DEF + 2;
        };
}

void AddSC_housing_go_scripts()
{
    new Housing::go_housing_cornerstone();
    new Housing::go_housing_coffer();
    new Housing::go_housing_bulletin();
    new Housing::go_housing_portal();
}
