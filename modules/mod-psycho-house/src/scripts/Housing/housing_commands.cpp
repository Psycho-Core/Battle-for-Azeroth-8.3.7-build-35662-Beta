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

/* ScriptData
Name: housing_commandscript
%Complete: 90
Comment: Player and GM commands for the housing module
Category: commandscripts
EndScriptData */

#include "Chat.h"
#include "HousingMgr.h"
#include "ObjectAccessor.h"
#include "Player.h"
#include "RBAC.h"
#include "ScriptMgr.h"
#include "WorldSession.h"

#include <cstring>
#include <sstream>

namespace
{
    Player* GetSessionPlayer(ChatHandler* handler)
    {
        return handler->GetSession() ? handler->GetSession()->GetPlayer() : nullptr;
    }

    char const* ResultText(Housing::HousingResult result)
    {
        switch (result)
        {
            case Housing::Ok:                return "ok";
            case Housing::Disabled:          return "housing is disabled on this realm";
            case Housing::NotInNeighborhood: return "you are not in a neighbourhood";
            case Housing::InvalidPlot:       return "invalid plot";
            case Housing::PlotTaken:         return "that plot already has an owner";
            case Housing::NotOwner:          return "that is not yours";
            case Housing::NoHouse:           return "you do not own a house here";
            case Housing::NotEnoughMoney:    return "you cannot afford that";
            case Housing::BudgetExceeded:    return "decor budget exceeded";
            case Housing::InvalidDecor:      return "invalid decor item";
            case Housing::NotUnlocked:       return "you have not collected that decor";
            case Housing::OutOfBounds:       return "outside the plot";
            case Housing::WrongSurface:      return "not placeable like that";
            case Housing::BadScale:          return "scale out of range";
            case Housing::TooManyDecor:      return "too many placed items";
            case Housing::WrongFaction:      return "wrong faction for this neighbourhood";
            case Housing::TooManyHouses:     return "you already own a house";
            case Housing::NeighborhoodFull:  return "no free plots in this neighbourhood";
            case Housing::TierTooLow:        return "that tier is not an upgrade";
            case Housing::TierMax:           return "invalid tier";
            case Housing::NotFound:          return "not found";
            case Housing::GuestRestricted:   return "that house is locked to visitors";
            case Housing::RateLimited:       return "too many requests";
            case Housing::Cooldown:          return "on cooldown";
            case Housing::NotAllowedHere:    return "not allowed here";
            default:                         return "internal error";
        }
    }
}

class housing_commandscript : public CommandScript
{
public:
    housing_commandscript() : CommandScript("housing_commandscript") { }

    std::vector<ChatCommand> GetCommands() const override
    {
        static std::vector<ChatCommand> guestCommandTable =
        {
            { "add",  0, false, &HandleGuestAddCommand, "" },
            { "del",  0, false, &HandleGuestDelCommand, "" },
            { "",     0, false, &HandleGuestListCommand, "" },
        };

        static std::vector<ChatCommand> housingCommandTable =
        {
            { "list",     0, false, &HandleListCommand,       "List plots in your neighbourhood" },
            { "info",     0, false, &HandleInfoCommand,       "Plot details" },
            { "buy",      0, false, &HandleBuyCommand,        "Buy a plot" },
            { "sell",     0, false, &HandleSellCommand,       "Sell your plot" },
            { "enter",    0, false, &HandleEnterCommand,      "Teleport to a plot" },
            { "home",     0, false, &HandleHomeCommand,       "Teleport to your plot (homestone)" },
            { "leave",    0, false, &HandleLeaveCommand,      "Return to the town center" },
            { "interior", 0, false, &HandleInteriorCommand,   "Enter or leave your house interior" },
            { "tier",     0, false, &HandleTierCommand,       "Upgrade your house tier" },
            { "name",     0, false, &HandleNameCommand,       "Name your house" },
            { "lock",     0, false, &HandleLockCommand,       "Lock or unlock your plot" },
            { "decor",    0, false, &HandleDecorCommand,      "List the decor catalogue" },
            { "buydecor", 0, false, &HandleBuyDecorCommand,   "Buy a decor item" },
            { "placed",   0, false, &HandlePlacedCommand,     "List placed decor in your house" },
            { "place",    0, false, &HandlePlaceCommand,      "Place a decor item" },
            { "move",     0, false, &HandleMoveCommand,       "Move a placed decor item" },
            { "remove",   0, false, &HandleRemoveCommand,     "Remove a placed decor item" },
            { "dir",      0, false, &HandleDirectoryCommand,  "Neighbourhood directory" },
            { "guest",    0, false, &HandleGuestListCommand,  "Manage plot guests", guestCommandTable },
            { "audit",    rbac::RBAC_PERM_COMMAND_GM, false, &HandleAuditCommand,    "Audit housing state" },
            { "save",     rbac::RBAC_PERM_COMMAND_GM, false, &HandleSaveCommand,     "Flush housing state" },
            { "reload",   rbac::RBAC_PERM_COMMAND_GM, false, &HandleReloadCommand,   "Reload housing data" },
            { "create",   rbac::RBAC_PERM_COMMAND_GM, false, &HandleCreateCommand,   "Create a neighbourhood" },
            { "dissolve", rbac::RBAC_PERM_COMMAND_GM, false, &HandleDissolveCommand, "Dissolve a neighbourhood" },
            { "forcesell", rbac::RBAC_PERM_COMMAND_GM, false, &HandleForceSellCommand, "Force sell a plot" },
        };

        static std::vector<ChatCommand> commandTable =
        {
            { "housing", 0, false, &HandleHousingCommand, "Player housing", housingCommandTable },
        };

        return commandTable;
    }

    static bool HandleHousingCommand(ChatHandler* handler, char const* /*args*/)
    {
        handler->SendSysMessage("Player housing commands:");
        handler->SendSysMessage("  .housing list | info <plot> | buy <plot> | sell | enter <plot>");
        handler->SendSysMessage("  .housing home | leave | interior <enter|exit> | tier <n> | name <text> | lock <on|off>");
        handler->SendSysMessage("  .housing decor [category] [page] | buydecor <id> | placed");
        handler->SendSysMessage("  .housing place <decorId> <x> <y> <z> <o> <scale> <indoor>");
        handler->SendSysMessage("  .housing move <placementId> <x> <y> <z> <o> <scale> | remove <placementId>");
        handler->SendSysMessage("  .housing dir [page] | guest add <name> [flags] | guest del <name>");
        return true;
    }

    // ---- read ------------------------------------------------------------

    static bool HandleListCommand(ChatHandler* handler, char const* /*args*/)
    {
        Player* player = GetSessionPlayer(handler);
        if (!player)
            return false;

        Housing::HousingNeighborhood* neighborhood = sHousingMgr->FindNeighborhoodFor(player);
        if (!neighborhood)
        {
            handler->SendSysMessage("You are not in a neighbourhood.");
            return true;
        }

        uint32 const accountId = player->GetSession()->GetAccountId();
        handler->PSendSysMessage("Neighbourhood #{} '{}' - {}/{} plots owned",
            neighborhood->GetId(), neighborhood->GetName(), neighborhood->GetOwnedCount(), uint32(Housing::kPlotCount));

        for (auto const& plot : neighborhood->GetPlots())
        {
            char const* state = "available";
            switch (plot->GetStateFor(accountId))
            {
                case Housing::PlotOwned:  state = "owned"; break;
                case Housing::PlotMine:   state = "yours"; break;
                case Housing::PlotLocked: state = "locked"; break;
                default: break;
            }

            if (plot->IsOwned())
                handler->PSendSysMessage("  plot {} [{}] tier {} - {}", uint32(plot->GetIndex()), state,
                    uint32(plot->GetHouse() ? plot->GetHouse()->GetTier() : 0), plot->GetOwnerName());
            else
                handler->PSendSysMessage("  plot {} [{}] biome {} - {}g", uint32(plot->GetIndex()), state,
                    plot->GetBiomeId(), plot->GetPrice());
        }

        return true;
    }

    static bool HandleInfoCommand(ChatHandler* handler, char const* args)
    {
        Player* player = GetSessionPlayer(handler);
        if (!player || !args)
            return false;

        uint32 plotIndex = 0;
        if (std::sscanf(args, "%u", &plotIndex) != 1 || plotIndex >= Housing::kPlotCount)
        {
            handler->SendSysMessage("Usage: .housing info <plot 0-54>");
            return true;
        }

        Housing::HousingNeighborhood* neighborhood = sHousingMgr->FindNeighborhoodFor(player);
        if (!neighborhood)
        {
            handler->SendSysMessage("You are not in a neighbourhood.");
            return true;
        }

        Housing::HousingPlot* plot = neighborhood->GetPlot(uint8(plotIndex));
        if (!plot)
        {
            handler->SendSysMessage("No such plot.");
            return true;
        }

        Housing::BiomeInfo const* biome = sHousingMgr->GetBiome(plot->GetBiomeId());

        handler->PSendSysMessage("Plot {} - biome {} ({})", uint32(plotIndex), plot->GetBiomeId(),
            biome ? biome->Name : "unknown");
        handler->PSendSysMessage("  price {}g, size class {}, yard radius {:.1f}",
            plot->GetPrice(), uint32(plot->GetSizeClass()), plot->GetTemplate().YardRadius);
        handler->PSendSysMessage("  spawn authored: {}", plot->GetTemplate().HasSpawn ? "yes" : "no");

        if (Housing::HousingHouse* house = plot->GetHouse())
        {
            handler->PSendSysMessage("  owner {}, tier {}, locked {}, budget {}/{} indoor, {}/{} outdoor",
                plot->GetOwnerName(), uint32(house->GetTier()), house->IsLocked() ? "yes" : "no",
                house->GetUsedBudget(true), house->GetBudget(true),
                house->GetUsedBudget(false), house->GetBudget(false));
            handler->PSendSysMessage("  {} placed decor pieces", uint32(house->GetPlacements().size()));
        }
        else
        {
            handler->SendSysMessage("  unowned");
        }

        return true;
    }

    static bool HandleDirectoryCommand(ChatHandler* handler, char const* args)
    {
        Player* player = GetSessionPlayer(handler);
        if (!player)
            return false;

        Housing::HousingNeighborhood* neighborhood = sHousingMgr->FindNeighborhoodFor(player);
        if (!neighborhood)
        {
            handler->SendSysMessage("You are not in a neighbourhood.");
            return true;
        }

        uint32 page = 0;
        if (args)
            std::sscanf(args, "%u", &page);

        uint32 const start = page * Housing::kDirectoryPageSize;
        uint32 index = 0;
        uint32 shown = 0;

        for (auto const& plot : neighborhood->GetPlots())
        {
            if (!plot->IsOwned())
                continue;

            if (index++ < start)
                continue;

            if (shown++ >= Housing::kDirectoryPageSize)
                break;

            handler->PSendSysMessage("plot {} - {} (tier {})", uint32(plot->GetIndex()), plot->GetOwnerName(),
                uint32(plot->GetHouse() ? plot->GetHouse()->GetTier() : 0));
        }

        if (!shown)
            handler->SendSysMessage("Nothing on that page.");

        return true;
    }

    static bool HandleDecorCommand(ChatHandler* handler, char const* args)
    {
        Player* player = GetSessionPlayer(handler);
        if (!player)
            return false;

        std::string category;
        uint32 page = 0;

        if (args)
        {
            std::istringstream stream(args);
            std::string first;
            stream >> first;

            uint32 parsed = 0;
            if (std::sscanf(first.c_str(), "%u", &parsed) == 1)
            {
                page = parsed;
            }
            else
            {
                category = first;

                uint32 second = 0;
                if (stream >> second)
                    page = second;
            }
        }

        uint32 const accountId = player->GetSession()->GetAccountId();
        uint8 const factionBit = player->GetTeamId() == TEAM_ALLIANCE ? 0x01 : 0x02;
        uint32 const start = page * Housing::kCataloguePageSize;

        uint32 index = 0;
        uint32 shown = 0;

        for (auto const& entry : sHousingMgr->GetDecorCatalogue())
        {
            Housing::DecorInfo const& info = entry.second;

            if (info.Flags & Housing::DecorFlagHidden)
                continue;
            if (!(info.FactionMask & factionBit))
                continue;
            if (info.SourceType == Housing::SourceGm)
                continue;
            if (!category.empty() && info.Category != category)
                continue;

            if (index++ < start)
                continue;

            if (shown++ >= Housing::kCataloguePageSize)
                break;

            handler->PSendSysMessage("[{}] {} - {} budget, {}g, {} - {}", info.Id, info.Name,
                uint32(info.BudgetCost), info.Price, info.Category,
                sHousingMgr->HasUnlocked(accountId, info.Id) ? "collected" : "not collected");
        }

        if (!shown)
            handler->SendSysMessage("Nothing on that page. Usage: .housing decor [category] [page]");

        return true;
    }

    static bool HandlePlacedCommand(ChatHandler* handler, char const* /*args*/)
    {
        Player* player = GetSessionPlayer(handler);
        if (!player)
            return false;

        Housing::HousingPlot* plot = sHousingMgr->GetPlotForEdit(player);
        if (!plot || !plot->GetHouse())
        {
            handler->SendSysMessage("You do not own a house here.");
            return true;
        }

        for (Housing::DecorPlacement const& placement : plot->GetHouse()->GetPlacements())
        {
            handler->PSendSysMessage("placement {} decor {} at ({:.2f}, {:.2f}, {:.2f}) o {:.2f} scale {:.2f} {}",
                placement.Id, placement.DecorId, placement.X, placement.Y, placement.Z, placement.O,
                placement.Scale, placement.Indoor ? "indoor" : "outdoor");
        }

        return true;
    }

    static bool HandleGuestListCommand(ChatHandler* handler, char const* /*args*/)
    {
        handler->SendSysMessage("Guest management: .housing guest add <name> <flags>, .housing guest del <name>");
        return true;
    }

    // ---- player actions --------------------------------------------------

    static bool HandleBuyCommand(ChatHandler* handler, char const* args)
    {
        Player* player = GetSessionPlayer(handler);
        if (!player || !args)
            return false;

        uint32 plotIndex = 0;
        if (std::sscanf(args, "%u", &plotIndex) != 1 || plotIndex >= Housing::kPlotCount)
        {
            handler->SendSysMessage("Usage: .housing buy <plot 0-54>");
            return true;
        }

        Housing::HousingNeighborhood* neighborhood = sHousingMgr->FindNeighborhoodFor(player);
        Housing::HousingResult const result = sHousingMgr->BuyPlot(player, neighborhood, uint8(plotIndex));
        handler->PSendSysMessage("Buy plot {}: {}", plotIndex, ResultText(result));
        return true;
    }

    static bool HandleSellCommand(ChatHandler* handler, char const* /*args*/)
    {
        Player* player = GetSessionPlayer(handler);
        if (!player)
            return false;

        handler->PSendSysMessage("Sell plot: {}", ResultText(sHousingMgr->SellPlot(player)));
        return true;
    }

    static bool HandleEnterCommand(ChatHandler* handler, char const* args)
    {
        Player* player = GetSessionPlayer(handler);
        if (!player || !args)
            return false;

        uint32 plotIndex = 0;
        if (std::sscanf(args, "%u", &plotIndex) != 1 || plotIndex >= Housing::kPlotCount)
        {
            handler->SendSysMessage("Usage: .housing enter <plot 0-54>");
            return true;
        }

        handler->PSendSysMessage("Enter plot {}: {}", plotIndex,
            ResultText(sHousingMgr->TeleportToPlot(player, uint8(plotIndex))));
        return true;
    }

    static bool HandleHomeCommand(ChatHandler* handler, char const* /*args*/)
    {
        Player* player = GetSessionPlayer(handler);
        if (!player)
            return false;

        handler->PSendSysMessage("Homestone: {}", ResultText(sHousingMgr->TeleportHome(player)));
        return true;
    }

    static bool HandleLeaveCommand(ChatHandler* handler, char const* /*args*/)
    {
        Player* player = GetSessionPlayer(handler);
        if (!player)
            return false;

        handler->PSendSysMessage("Leave: {}", ResultText(sHousingMgr->TeleportToTownCenter(player)));
        return true;
    }

    static bool HandleInteriorCommand(ChatHandler* handler, char const* args)
    {
        Player* player = GetSessionPlayer(handler);
        if (!player)
            return false;

        std::string direction = args ? args : "enter";
        bool const enter = direction.compare(0, 4, "exit") != 0;

        handler->PSendSysMessage("Interior: {}", ResultText(sHousingMgr->EnterHouse(player, enter)));
        return true;
    }

    static bool HandleTierCommand(ChatHandler* handler, char const* args)
    {
        Player* player = GetSessionPlayer(handler);
        if (!player || !args)
            return false;

        uint32 tier = 0;
        if (std::sscanf(args, "%u", &tier) != 1 || tier < 1 || tier > Housing::kMaxTier)
        {
            handler->PSendSysMessage("Usage: .housing tier <1-{}>", uint32(Housing::kMaxTier));
            return true;
        }

        handler->PSendSysMessage("Upgrade to tier {}: {}", tier, ResultText(sHousingMgr->UpgradeTier(player, uint8(tier))));
        return true;
    }

    static bool HandleNameCommand(ChatHandler* handler, char const* args)
    {
        Player* player = GetSessionPlayer(handler);
        if (!player || !args)
            return false;

        handler->PSendSysMessage("Rename: {}", ResultText(sHousingMgr->RenameHouse(player, args)));
        return true;
    }

    static bool HandleLockCommand(ChatHandler* handler, char const* args)
    {
        Player* player = GetSessionPlayer(handler);
        if (!player)
            return false;

        bool const lock = args && (args[0] == '1' || args[0] == 'y' || args[0] == 'o');

        handler->PSendSysMessage("Lock: {}", ResultText(sHousingMgr->SetLocked(player, lock)));
        return true;
    }

    static bool HandleBuyDecorCommand(ChatHandler* handler, char const* args)
    {
        Player* player = GetSessionPlayer(handler);
        if (!player || !args)
            return false;

        uint32 decorId = 0;
        if (std::sscanf(args, "%u", &decorId) != 1)
        {
            handler->SendSysMessage("Usage: .housing buydecor <decor id>");
            return true;
        }

        handler->PSendSysMessage("Buy decor {}: {}", decorId, ResultText(sHousingMgr->BuyDecor(player, decorId)));
        return true;
    }

    static bool HandlePlaceCommand(ChatHandler* handler, char const* args)
    {
        Player* player = GetSessionPlayer(handler);
        if (!player || !args)
            return false;

        uint32 decorId = 0, indoor = 0;
        float x = 0.0f, y = 0.0f, z = 0.0f, o = 0.0f, scale = 1.0f;

        if (std::sscanf(args, "%u %f %f %f %f %f %u", &decorId, &x, &y, &z, &o, &scale, &indoor) < 7)
        {
            handler->SendSysMessage("Usage: .housing place <decorId> <x> <y> <z> <o> <scale> <indoor>");
            return true;
        }

        uint64 placementId = 0;
        Housing::HousingResult const result = sHousingMgr->PlaceDecor(player, decorId, x, y, z, o, scale,
            indoor != 0, placementId);

        if (result == Housing::Ok)
            handler->PSendSysMessage("Placed decor {} as placement {}.", decorId, placementId);
        else
            handler->PSendSysMessage("Place decor: {}", ResultText(result));

        return true;
    }

    static bool HandleMoveCommand(ChatHandler* handler, char const* args)
    {
        Player* player = GetSessionPlayer(handler);
        if (!player || !args)
            return false;

        uint32 placementId = 0;
        float x = 0.0f, y = 0.0f, z = 0.0f, o = 0.0f, scale = 1.0f;

        if (std::sscanf(args, "%u %f %f %f %f %f", &placementId, &x, &y, &z, &o, &scale) < 6)
        {
            handler->SendSysMessage("Usage: .housing move <placementId> <x> <y> <z> <o> <scale>");
            return true;
        }

        handler->PSendSysMessage("Move: {}", ResultText(sHousingMgr->MoveDecor(player, placementId, x, y, z, o, scale)));
        return true;
    }

    static bool HandleRemoveCommand(ChatHandler* handler, char const* args)
    {
        Player* player = GetSessionPlayer(handler);
        if (!player || !args)
            return false;

        uint32 placementId = 0;
        if (std::sscanf(args, "%u", &placementId) != 1)
        {
            handler->SendSysMessage("Usage: .housing remove <placementId>");
            return true;
        }

        handler->PSendSysMessage("Remove: {}", ResultText(sHousingMgr->RemoveDecor(player, placementId)));
        return true;
    }

    static bool HandleGuestAddCommand(ChatHandler* handler, char const* args)
    {
        Player* player = GetSessionPlayer(handler);
        if (!player || !args)
            return false;

        char name[64] = { 0 };
        uint32 flags = 1;

        if (std::sscanf(args, "%63s %u", name, &flags) < 1)
        {
            handler->SendSysMessage("Usage: .housing guest add <name> [flags]");
            return true;
        }

        Housing::HousingPlot* plot = sHousingMgr->GetPlotForEdit(player);
        if (!plot || !plot->IsOwned() || plot->GetOwnerAccount() != player->GetSession()->GetAccountId())
        {
            handler->SendSysMessage("You do not own a house here.");
            return true;
        }

        Player* guest = ObjectAccessor::FindPlayerByName(name);
        if (!guest)
        {
            handler->SendSysMessage("That player is not online.");
            return true;
        }

        sHousingMgr->AddGuest(plot->GetHousingId(), guest->GetSession()->GetAccountId(), uint8(flags & 0x07));
        handler->PSendSysMessage("{} added as a guest.", name);

        return true;
    }

    static bool HandleGuestDelCommand(ChatHandler* handler, char const* args)
    {
        Player* player = GetSessionPlayer(handler);
        if (!player || !args)
            return false;

        char name[64] = { 0 };
        if (std::sscanf(args, "%63s", name) != 1)
        {
            handler->SendSysMessage("Usage: .housing guest del <name>");
            return true;
        }

        Housing::HousingPlot* plot = sHousingMgr->GetPlotForEdit(player);
        if (!plot || !plot->IsOwned() || plot->GetOwnerAccount() != player->GetSession()->GetAccountId())
        {
            handler->SendSysMessage("You do not own a house here.");
            return true;
        }

        Player* guest = ObjectAccessor::FindPlayerByName(name);
        if (!guest)
        {
            handler->SendSysMessage("That player is not online.");
            return true;
        }

        sHousingMgr->RemoveGuest(plot->GetHousingId(), guest->GetSession()->GetAccountId());
        handler->PSendSysMessage("{} removed from guests.", name);

        return true;
    }

    // ---- GM --------------------------------------------------------------

    static bool HandleAuditCommand(ChatHandler* handler, char const* /*args*/)
    {
        handler->SendSysMessage(sHousingMgr->BuildAuditReport().c_str());
        return true;
    }

    static bool HandleSaveCommand(ChatHandler* handler, char const* /*args*/)
    {
        sHousingMgr->SaveAll();
        handler->SendSysMessage("Housing state flushed.");
        return true;
    }

    static bool HandleReloadCommand(ChatHandler* handler, char const* /*args*/)
    {
        sHousingMgr->Unload();
        sHousingMgr->LoadConfig();
        sHousingMgr->LoadHousingData();
        sHousingMgr->LoadNeighborhoods();
        handler->SendSysMessage("Housing data reloaded.");
        return true;
    }

    static bool HandleCreateCommand(ChatHandler* handler, char const* args)
    {
        Player* player = GetSessionPlayer(handler);
        if (!args)
        {
            handler->SendSysMessage("Usage: .housing create <public|guild|private> <name>");
            return true;
        }

        char type[16] = { 0 };
        char name[64] = { 0 };

        if (std::sscanf(args, "%15s %63[^\n]", type, name) < 2)
        {
            handler->SendSysMessage("Usage: .housing create <public|guild|private> <name>");
            return true;
        }

        uint8 neighborhoodType = Housing::NeighborhoodPublic;
        if (!strcmp(type, "guild"))
            neighborhoodType = Housing::NeighborhoodGuild;
        else if (!strcmp(type, "private"))
            neighborhoodType = Housing::NeighborhoodPrivate;

        uint8 const faction = (player && player->GetTeamId() != TEAM_ALLIANCE)
            ? Housing::FactionHorde
            : Housing::FactionAlliance;

        uint64 const guildId = (player && neighborhoodType == Housing::NeighborhoodGuild) ? player->GetGuildId() : 0;
        uint32 const ownerAccount = player ? player->GetSession()->GetAccountId() : 0;

        if (Housing::HousingNeighborhood* neighborhood = sHousingMgr->CreateNeighborhood(neighborhoodType,
            faction, name, guildId, ownerAccount))
            handler->PSendSysMessage("Created neighbourhood {} ('{}') slot {}.", neighborhood->GetId(),
                neighborhood->GetName(), uint32(neighborhood->GetSlot()));
        else
            handler->SendSysMessage("Could not create the neighbourhood (see server log).");

        return true;
    }

    static bool HandleDissolveCommand(ChatHandler* handler, char const* args)
    {
        uint32 id = 0;
        if (!args || std::sscanf(args, "%u", &id) != 1)
        {
            handler->SendSysMessage("Usage: .housing dissolve <neighbourhood id>");
            return true;
        }

        Housing::HousingNeighborhood* neighborhood = sHousingMgr->GetNeighborhood(id);
        if (!neighborhood)
        {
            handler->SendSysMessage("No such neighbourhood.");
            return true;
        }

        sHousingMgr->DissolveNeighborhood(neighborhood);
        handler->PSendSysMessage("Neighbourhood {} dissolved.", id);
        return true;
    }

    static bool HandleForceSellCommand(ChatHandler* handler, char const* args)
    {
        Player* player = GetSessionPlayer(handler);
        if (!player || !args)
            return false;

        uint32 plotIndex = 0;
        if (std::sscanf(args, "%u", &plotIndex) != 1 || plotIndex >= Housing::kPlotCount)
        {
            handler->SendSysMessage("Usage: .housing forcesell <plot 0-54>");
            return true;
        }

        Housing::HousingNeighborhood* neighborhood = sHousingMgr->FindNeighborhoodFor(player);
        if (!neighborhood)
        {
            handler->SendSysMessage("You are not in a neighbourhood.");
            return true;
        }

        Housing::HousingPlot* plot = neighborhood->GetPlot(uint8(plotIndex));
        if (!plot || !plot->IsOwned())
        {
            handler->SendSysMessage("That plot is not owned.");
            return true;
        }

        sHousingMgr->ForceSellPlot(plot);
        handler->PSendSysMessage("Plot {} force sold.", plotIndex);
        return true;
    }
};

void AddSC_housing_commands()
{
    new housing_commandscript();
}
