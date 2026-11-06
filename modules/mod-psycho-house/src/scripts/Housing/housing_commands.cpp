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
#include "HousingEndeavor.h"
#include "HousingMgr.h"
#include "ObjectAccessor.h"
#include "Player.h"
#include "RBAC.h"
#include "ScriptMgr.h"
#include "StringFormat.h"
#include "WorldSession.h"

#include <algorithm>
#include <cstring>
#include <sstream>

namespace
{
    /// Parse a bare unsigned id out of a command argument. Deliberately strict: a typo
    /// must not resolve to zero and act on plot 0.
    uint32 ParseId(std::string const& text)
    {
        std::istringstream stream(text);
        uint32 value = 0;
        stream >> value;
        return stream.fail() ? 0 : value;
    }

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
            case Housing::LevelTooLow:       return "your house is not a high enough level";
            case Housing::StorageEmpty:      return "you do not own a copy of that";
            case Housing::EndeavorIncomplete: return "the endeavour has not reached that step";
            case Housing::BadCode:           return "that build code is not valid";
            case Housing::ArchiveExpired:    return "the regret window has closed";
            case Housing::NotGuild:          return "that neighbourhood is not a guild one";
            case Housing::AlreadyKnown:      return "already known";
            default:                         return "internal error";
        }
    }
}

namespace
{
    // ---- the commands that arrived with the completeness pass -----------------

    static bool HandleFinderCommand(ChatHandler* handler, char const* /*args*/)
    {
        Player* player = GetSessionPlayer(handler);
        if (!player)
            return false;

        uint8 const faction = player->GetTeamId() == TEAM_ALLIANCE ? Housing::FactionAlliance
                                                                   : Housing::FactionHorde;

        std::vector<Housing::HousingNeighborhood*> neighborhoods =
            sHousingMgr->GetPublicNeighborhoods(faction);

        if (neighborhoods.empty())
        {
            handler->PSendSysMessage("No public neighbourhoods yet.");
            return true;
        }

        for (Housing::HousingNeighborhood* neighborhood : neighborhoods)
            handler->PSendSysMessage("{} ({}) - {}/{} plots taken", neighborhood->GetName(),
                neighborhood->GetId(), neighborhood->GetOwnedCount(), uint32(Housing::kPlotCount));

        handler->PSendSysMessage("Visit one with .housing visit <neighbourhood> <plot>.");
        return true;
    }

    static bool HandleVisitCommand(ChatHandler* handler, char const* args)
    {
        Player* player = GetSessionPlayer(handler);
        if (!player || !args)
            return false;

        uint32 neighborhoodId = 0;
        uint32 plotIndex = 0;

        std::istringstream stream(args);
        if (!(stream >> neighborhoodId >> plotIndex))
        {
            handler->PSendSysMessage("Usage: .housing visit <neighbourhood> <plot>");
            return true;
        }

        handler->PSendSysMessage("Visit: {}", ResultText(
            sHousingMgr->VisitNeighborhood(player, neighborhoodId, uint8(plotIndex))));
        return true;
    }

    static bool HandleReserveCommand(ChatHandler* handler, char const* args)
    {
        Player* player = GetSessionPlayer(handler);
        if (!player || !args)
            return false;

        uint32 plotIndex = 0;
        if (!(plotIndex = ParseId(args)))
        {
            handler->PSendSysMessage("Usage: .housing reserve <plot>");
            return true;
        }

        handler->PSendSysMessage("Reserve: {}", ResultText(sHousingMgr->ReservePlot(player, uint8(plotIndex))));
        return true;
    }

    static bool HandleStorageCommand(ChatHandler* handler, char const* /*args*/)
    {
        Player* player = GetSessionPlayer(handler);
        if (!player)
            return false;

        auto const& storage = sHousingMgr->GetStorage(player->GetSession()->GetAccountId());
        if (storage.empty())
        {
            handler->PSendSysMessage("Storage is empty.");
            return true;
        }

        uint32 total = 0;
        for (auto const& entry : storage)
        {
            handler->PSendSysMessage("  decor {} x{}", entry.first, entry.second);
            total += entry.second;
        }

        handler->PSendSysMessage("{} copies in storage.", total);
        return true;
    }

    static bool HandleLevelUpCommand(ChatHandler* handler, char const* /*args*/)
    {
        Player* player = GetSessionPlayer(handler);
        if (!player)
            return false;

        handler->PSendSysMessage("Level up: {}", ResultText(sHousingMgr->LevelUpWithGold(player)));
        return true;
    }

    static bool HandleShellCommand(ChatHandler* handler, char const* args)
    {
        Player* player = GetSessionPlayer(handler);
        if (!player)
            return false;

        if (!args)
        {
            for (auto const& pair : sHousingMgr->GetShells())
                handler->PSendSysMessage("  shell {}: {} (level {})", pair.first, pair.second.Name,
                    uint32(pair.second.LevelRequired));

            handler->PSendSysMessage("Usage: .housing shell <id>");
            return true;
        }

        uint32 shellId = 0;
        if (!(shellId = ParseId(args)))
            return false;

        handler->PSendSysMessage("Shell: {}", ResultText(sHousingMgr->ApplyShell(player, shellId)));
        return true;
    }

    static bool HandleDyeCommand(ChatHandler* handler, char const* args)
    {
        Player* player = GetSessionPlayer(handler);
        if (!player || !args)
            return false;

        uint64 placementId = 0;
        uint32 color = 0;

        // Group is optional and defaults to 0, which means "the group this piece keeps that
        // colour in". Retail's dye slot belongs to the furniture, so a player normally means
        // the piece's own group; naming one is only for a piece with more than one.
        std::istringstream stream(args);
        std::string first;
        std::string second;
        std::string third;

        if (!(stream >> first >> second))
        {
            handler->PSendSysMessage("Usage: .housing dye <placement> <colour> [group]");
            return true;
        }

        stream >> third;

        placementId = uint64(strtoull(first.c_str(), nullptr, 10));
        color = uint32(atoi(second.c_str()));

        uint32 const group = third.empty() ? 0 : uint32(atoi(third.c_str()));

        handler->PSendSysMessage("Dye: {}", ResultText(
            sHousingMgr->DyeDecor(player, placementId, uint8(group), uint8(color))));
        return true;
    }

    static bool HandleBlueprintCommand(ChatHandler* handler, char const* args)
    {
        Player* player = GetSessionPlayer(handler);
        if (!player)
            return false;

        uint32 kind = Housing::BlueprintBuild;
        std::string name = "build";

        if (args)
        {
            std::istringstream stream(args);
            stream >> kind;
            stream >> name;
        }

        std::string code;
        Housing::HousingResult const result = sHousingMgr->ExportBlueprint(player,
            uint8(std::min<uint32>(kind, Housing::BlueprintExterior)), name, code);

        if (result != Housing::Ok)
        {
            handler->PSendSysMessage("Export: {}", ResultText(result));
            return true;
        }

        handler->PSendSysMessage("Build code ({} pieces):", uint32(code.size()));
        handler->PSendSysMessage("{}", code);
        return true;
    }

    static bool HandleBlueprintImportCommand(ChatHandler* handler, char const* args)
    {
        Player* player = GetSessionPlayer(handler);
        if (!player || !args)
            return false;

        handler->PSendSysMessage("Import: {}", ResultText(sHousingMgr->ImportBlueprint(player, args)));
        return true;
    }

    static bool HandleRestoreCommand(ChatHandler* handler, char const* /*args*/)
    {
        Player* player = GetSessionPlayer(handler);
        if (!player)
            return false;

        handler->PSendSysMessage("Restore: {}", ResultText(sHousingMgr->RestoreArchivedHouse(player)));
        return true;
    }

    static bool HandlePortalCommand(ChatHandler* handler, char const* args)
    {
        Player* player = GetSessionPlayer(handler);
        if (!player)
            return false;

        if (!args)
        {
            for (Housing::HousingPortal const& portal : sHousingMgr->GetPortals())
                handler->PSendSysMessage("  portal {}: {} -> map {}", portal.Id, portal.Name, portal.Map);

            handler->PSendSysMessage("Usage: .housing portal <id>");
            return true;
        }

        uint32 portalId = 0;
        if (!(portalId = ParseId(args)))
            return false;

        handler->PSendSysMessage("Portal: {}", ResultText(sHousingMgr->UsePortal(player, portalId)));
        return true;
    }

    static bool HandleEndeavorCommand(ChatHandler* handler, char const* /*args*/)
    {
        Player* player = GetSessionPlayer(handler);
        if (!player)
            return false;

        uint32 const neighborhoodId = sHousingMgr->GetPlayerNeighborhoodId(player);
        Housing::EndeavorInfo const* info = sHousingEndeavor->GetInfo(
            sHousingEndeavor->GetActiveId(neighborhoodId));

        if (!info)
        {
            handler->PSendSysMessage("No endeavour is running here.");
            return true;
        }

        handler->PSendSysMessage("{} - milestone {}/4, {} tasks done", info->Name,
            uint32(sHousingEndeavor->GetMilestone(neighborhoodId)),
            sHousingEndeavor->GetCompletedTasks(neighborhoodId));

        for (Housing::EndeavorTask const& task : info->Tasks)
            handler->PSendSysMessage("  {}: {}/{}", task.Text,
                sHousingEndeavor->GetTaskProgress(neighborhoodId, task.Id), task.Required);

        return true;
    }

    static bool HandleDeliverCommand(ChatHandler* handler, char const* /*args*/)
    {
        Player* player = GetSessionPlayer(handler);
        if (!player)
            return false;

        handler->PSendSysMessage("Delivery: {}", sHousingEndeavor->Deliver(player)
            ? "counted" : ResultText(Housing::EndeavorIncomplete));
        return true;
    }

    static bool HandleClaimCommand(ChatHandler* handler, char const* args)
    {
        Player* player = GetSessionPlayer(handler);
        if (!player || !args)
            return false;

        uint32 milestone = 0;
        if (!(milestone = ParseId(args)))
        {
            handler->PSendSysMessage("Usage: .housing claim <1-4>");
            return true;
        }

        handler->PSendSysMessage("Claim: {}", sHousingEndeavor->Claim(player, uint8(milestone))
            ? "claimed" : ResultText(Housing::EndeavorIncomplete));
        return true;
    }

    /*
     * Choosing the Endeavor. A public neighbourhood is given one by the rotation and cannot
     * argue; a private one is the founder's household and takes up whatever it likes, which
     * is the only way a group of friends can pick a theme they can actually finish.
     */
    static bool HandleChooseEndeavorCommand(ChatHandler* handler, char const* args)
    {
        Player* player = GetSessionPlayer(handler);
        if (!player)
            return false;

        uint32 const id = args ? ParseId(args) : 0;

        HousingNeighborhood* neighborhood = sHousingMgr->FindNeighborhoodFor(player);
        if (!neighborhood)
        {
            handler->SendSysMessage("You are not in a neighbourhood.");
            return true;
        }

        if (neighborhood->GetType() != Housing::NeighborhoodPrivate)
        {
            handler->SendSysMessage("Only a private neighbourhood chooses its own Endeavor.");
            return true;
        }

        if (neighborhood->GetRank(player->GetSession()->GetAccountId()) < 3)
        {
            handler->SendSysMessage("Only the founder of the neighbourhood may choose.");
            return true;
        }

        if (!id)
        {
            handler->SendSysMessage("Endeavors this neighbourhood can take up:");

            uint32 const current = sHousingEndeavor->GetActiveId(neighborhood->GetId());
            for (Housing::EndeavorInfo const& info : sHousingEndeavor->GetInfos())
                handler->PSendSysMessage("  {} - {}{}", info.Id, info.Name,
                    info.Id == current ? " (running now)" : "");

            handler->SendSysMessage("Usage: .housing choose <id>");
            return true;
        }

        if (!sHousingEndeavor->GetInfo(id))
        {
            handler->PSendSysMessage("There is no Endeavor {}.", id);
            return true;
        }

        if (sHousingEndeavor->HasActive(neighborhood->GetId()))
        {
            handler->SendSysMessage("One is already running here; it has to finish first.");
            return true;
        }

        sHousingEndeavor->Start(neighborhood->GetId(), id);
        handler->PSendSysMessage("The neighbourhood takes up Endeavor {}.", id);
        return true;
    }
}

namespace
{
    // ---- neighbourhood membership, which had a manager and no way in ------------
    //
    // InviteToNeighborhood, SetMemberRank and RemoveFromNeighborhood were written and
    // reachable from nothing: the Steward's gossip told the player to type a command that
    // did not exist. This is that command.

    static bool HandleMilestonesCommand(ChatHandler* handler, char const* /*args*/)
    {
        Player* player = GetSessionPlayer(handler);
        if (!player)
            return false;

        std::string const report = sHousingMgr->BuildMilestoneReport(player);
        std::string line;

        handler->SendSysMessage("Collection milestones:");

        for (char const ch : report)
        {
            if (ch != '\n')
            {
                line += ch;
                continue;
            }

            if (!line.empty())
                handler->PSendSysMessage("{}", line);
            line.clear();
        }

        return true;
    }

    static bool HandleDyeListCommand(ChatHandler* handler, char const* /*args*/)
    {
        auto const& palette = sHousingMgr->GetDyes();

        if (palette.empty())
        {
            handler->SendSysMessage("No dye palette: `housing_dye` is empty on this realm.");
            return true;
        }

        handler->SendSysMessage("Colours the workbench can mix:");

        for (auto const& dye : palette)
            handler->PSendSysMessage("  {} - {} - .housing dye <placement> {}",
                uint32(dye.first), dye.second, uint32(dye.first));

        return true;
    }

    static bool HandleNeighborListCommand(ChatHandler* handler, char const* /*args*/)
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

        handler->PSendSysMessage("'{}' - {} resident(s), {} owned plot(s):",
            neighborhood->GetName(), uint32(neighborhood->GetMembers().size()),
            neighborhood->GetOwnedCount());

        for (auto const& member : neighborhood->GetMembers())
        {
            std::string const name = sHousingMgr->GetAccountCharacterName(member.first);
            handler->PSendSysMessage("  {} - rank {}", name.empty() ? "(no character)" : name,
                uint32(member.second));
        }

        handler->SendSysMessage("invite <name> | rank <name> <1-2> | del <name>");
        return true;
    }

    static bool HandleNeighborInviteCommand(ChatHandler* handler, char const* args)
    {
        Player* player = GetSessionPlayer(handler);
        if (!player || !args || !*args)
        {
            handler->SendSysMessage("Usage: .housing neighbor invite <name>");
            return true;
        }

        handler->PSendSysMessage("Invite: {}",
            ResultText(sHousingMgr->InviteToNeighborhood(player, args)));
        return true;
    }

    static bool HandleNeighborRankCommand(ChatHandler* handler, char const* args)
    {
        Player* player = GetSessionPlayer(handler);
        if (!player || !args)
        {
            handler->SendSysMessage("Usage: .housing neighbor rank <name> <1-2>");
            return true;
        }

        std::string const text = args;
        std::string::size_type const split = text.find_last_of(' ');

        if (split == std::string::npos)
        {
            handler->SendSysMessage("Usage: .housing neighbor rank <name> <1-2>");
            return true;
        }

        std::string const name = text.substr(0, split);
        uint8 const rank = uint8(atoi(text.substr(split + 1).c_str()));

        handler->PSendSysMessage("Rank: {}",
            ResultText(sHousingMgr->SetMemberRank(player, name, rank)));
        return true;
    }

    static bool HandleNeighborRemoveCommand(ChatHandler* handler, char const* args)
    {
        Player* player = GetSessionPlayer(handler);
        if (!player || !args || !*args)
        {
            handler->SendSysMessage("Usage: .housing neighbor del <name>");
            return true;
        }

        handler->PSendSysMessage("Removed: {}",
            ResultText(sHousingMgr->RemoveFromNeighborhood(player, args)));
        return true;
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

        static std::vector<ChatCommand> neighborCommandTable =
        {
            { "invite", 0, false, &HandleNeighborInviteCommand, "Invite somebody to the neighbourhood" },
            { "rank",   0, false, &HandleNeighborRankCommand,   "Promote or demote a resident" },
            { "del",    0, false, &HandleNeighborRemoveCommand, "Remove a resident" },
            { "",       0, false, &HandleNeighborListCommand,   "Who lives here" },
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
            { "tilt",     0, false, &HandleTiltCommand,       "Tilt a placed piece on its two axes" },
            { "pet",      0, false, &HandlePetCommand,        "Put your summoned companion on a pet bed" },
            { "dir",      0, false, &HandleDirectoryCommand,  "Neighbourhood directory" },
            { "finder",   0, false, &HandleFinderCommand,     "List public neighbourhoods" },
            { "visit",    0, false, &HandleVisitCommand,      "Visit a plot in another neighbourhood" },
            { "reserve",  0, false, &HandleReserveCommand,    "Hold a free plot for 60 seconds" },
            { "storage",  0, false, &HandleStorageCommand,    "Owned decor copies" },
            { "levelup",  0, false, &HandleLevelUpCommand,    "Pay the contractor to skip a level" },
            { "shell",    0, false, &HandleShellCommand,      "Choose or list exterior shells" },
            { "dye",      0, false, &HandleDyeCommand,        "Dye a placed piece" },
            { "bp",       0, false, &HandleBlueprintCommand,  "Export a build code" },
            { "bpimport", 0, false, &HandleBlueprintImportCommand, "Import a build code" },
            { "restore",  0, false, &HandleRestoreCommand,    "Recover a sold house" },
            { "portal",   0, false, &HandlePortalCommand,     "Use a portal in your house" },
            { "endeavor", 0, false, &HandleEndeavorCommand,   "Endeavour progress here" },
            { "deliver",  0, false, &HandleDeliverCommand,    "Turn in an Endeavour delivery" },
            { "claim",    0, false, &HandleClaimCommand,      "Claim a milestone reward" },
            { "choose",   0, false, &HandleChooseEndeavorCommand, "Choose the Endeavor in a private neighbourhood" },
            { "guest",    0, false, &HandleGuestListCommand,  "Manage plot guests", guestCommandTable },
            { "neighbor", 0, false, &HandleNeighborListCommand, "Manage neighbourhood membership", neighborCommandTable },
            { "dyes",     0, false, &HandleDyeListCommand,     "Colours the dye station can mix" },
            { "milestones", 0, false, &HandleMilestonesCommand, "What this house has earned" },
            { "audit",    rbac::RBAC_PERM_COMMAND_GM, false, &HandleAuditCommand,    "Audit housing state" },
            { "save",     rbac::RBAC_PERM_COMMAND_GM, false, &HandleSaveCommand,     "Flush housing state" },
            { "reload",   rbac::RBAC_PERM_COMMAND_GM, false, &HandleReloadCommand,   "Reload housing data" },
            { "config",   rbac::RBAC_PERM_COMMAND_GM, false, &HandleConfigCommand,   "Every housing setting and the value it took" },
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

    /*
     * The help text used to be six hand-written lines, which meant the six a player was told
     * about were the six somebody remembered, and the other thirty-seven were not mentioned
     * anywhere in the client. It is now read out of the command table itself: whatever the
     * server dispatches is what this prints, so the two cannot disagree.
     */
    static bool HandleHousingCommand(ChatHandler* handler, char const* /*args*/)
    {
        for (ChatCommand const& outer : ChatHandler::getCommandTable())
        {
            if (std::string(outer.Name) != "housing")
                continue;

            handler->PSendSysMessage("Housing commands ({}):", uint32(outer.ChildCommands.size()));

            for (ChatCommand const& row : outer.ChildCommands)
            {
                std::string line = Trinity::StringFormat("  .housing {}", std::string(row.Name));

                if (!row.ChildCommands.empty())
                {
                    line += " <";

                    for (std::size_t i = 0; i < row.ChildCommands.size(); ++i)
                    {
                        if (i)
                            line += "|";

                        line += row.ChildCommands[i].Name;
                    }

                    line += ">";
                }

                line += " - ";
                line += row.Help;

                if (row.Permission)
                    line += " [GM]";

                handler->SendSysMessage(line.c_str());
            }

            return true;
        }

        handler->SendSysMessage("The housing command table is not registered.");
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

            // Only pieces that are actually tilted get a second line; a house of 300 upright
            // chairs does not need 300 lines saying "tilt 0".
            if (placement.Pitch != 0.0f || placement.Roll != 0.0f)
                handler->PSendSysMessage("    tilt {} deg pitch, {} deg roll",
                    int32(placement.Pitch * 57.2958f), int32(placement.Roll * 57.2958f));
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
        float pitch = 0.0f, roll = 0.0f;

        int const parsed = std::sscanf(args, "%u %f %f %f %f %f %u %f %f",
            &decorId, &x, &y, &z, &o, &scale, &indoor, &pitch, &roll);

        if (parsed < 7)
        {
            handler->SendSysMessage("Usage: .housing place <decorId> <x> <y> <z> <o> <scale> <indoor> [pitch] [roll]");
            return true;
        }

        if (parsed < 9)
            roll = 0.0f;

        uint64 placementId = 0;
        Housing::HousingResult const result = sHousingMgr->PlaceDecor(player, decorId, x, y, z, o, scale,
            indoor != 0, placementId, pitch, roll);

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
        float pitch = 0.0f, roll = 0.0f;

        int const parsed = std::sscanf(args, "%u %f %f %f %f %f %f %f",
            &placementId, &x, &y, &z, &o, &scale, &pitch, &roll);

        if (parsed < 6)
        {
            handler->SendSysMessage("Usage: .housing move <placementId> <x> <y> <z> <o> <scale> [pitch] [roll]");
            return true;
        }

        if (parsed < 8)
            roll = 0.0f;

        handler->PSendSysMessage("Move: {}", ResultText(sHousingMgr->MoveDecor(player, placementId, x, y, z,
            o, scale, pitch, roll)));
        return true;
    }

    /*
     * Tilting a piece by hand otherwise means retyping six numbers read off `.housing placed`
     * to change two of them, and getting one wrong drops the chair through the floor. This
     * moves the tilt and nothing else.
     */
    static bool HandleTiltCommand(ChatHandler* handler, char const* args)
    {
        Player* player = GetSessionPlayer(handler);
        if (!player || !args)
            return false;

        uint32 placementId = 0;
        float pitch = 0.0f, roll = 0.0f;

        int const parsed = std::sscanf(args, "%u %f %f", &placementId, &pitch, &roll);
        if (parsed < 2)
        {
            handler->SendSysMessage("Usage: .housing tilt <placementId> <pitch deg> [roll deg]");
            return true;
        }

        Housing::HousingPlot* plot = sHousingMgr->GetPlotForEdit(player);
        if (!plot || !plot->GetHouse())
        {
            handler->SendSysMessage("You do not own a house here.");
            return true;
        }

        Housing::DecorPlacement const* placement = plot->GetHouse()->GetPlacement(placementId);
        if (!placement)
        {
            handler->SendSysMessage("No such placement.");
            return true;
        }

        handler->PSendSysMessage("Tilt: {}", ResultText(sHousingMgr->MoveDecor(player, placementId,
            placement->X, placement->Y, placement->Z, placement->O, placement->Scale,
            pitch / 57.2958f, roll / 57.2958f)));
        return true;
    }

    static bool HandlePetCommand(ChatHandler* handler, char const* args)
    {
        Player* player = GetSessionPlayer(handler);
        if (!player || !args)
            return false;

        uint32 placementId = 0;
        char mode[16] = { 0 };

        if (std::sscanf(args, "%u %15s", &placementId, mode) != 2)
        {
            handler->SendSysMessage("Usage: .housing pet <placementId> <stationary|roaming|clear>");
            handler->SendSysMessage("  the companion has to be out: summon it first, then assign it");
            return true;
        }

        std::string const choice = mode;
        uint8 value = 0;

        if (choice == "stationary" || choice == "sleep" || choice == "1")
            value = 1;
        else if (choice == "roaming" || choice == "roam" || choice == "2")
            value = 2;
        else if (choice == "clear" || choice == "off" || choice == "0")
            value = 0;
        else
        {
            handler->SendSysMessage("Usage: .housing pet <placementId> <stationary|roaming|clear>");
            return true;
        }

        handler->PSendSysMessage("Pet: {}", ResultText(sHousingMgr->AssignPet(player, placementId, value)));
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

    static bool HandleConfigCommand(ChatHandler* handler, char const* args)
    {
        std::string const filter = args ? args : "";
        auto const& snapshot = sHousingMgr->GetConfigSnapshot();

        if (snapshot.empty())
        {
            handler->SendSysMessage("No housing settings have been read - check Housing.Enable and the log.");
            return true;
        }

        handler->PSendSysMessage("{} housing settings, as LoadConfig kept them:", uint32(snapshot.size()));

        for (auto const& entry : snapshot)
        {
            if (!filter.empty() && entry.first.find(filter) == std::string::npos)
                continue;

            handler->PSendSysMessage("  {} = {}", entry.first, entry.second);
        }

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
