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
#include <string>

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
            // The list says what a shell costs and whether it has a building behind it. A
            // shell with no object is not broken - it is one somebody has not chosen a
            // building for yet, which is every shell on a fresh install (doc 06 sec.1.5).
            static char const* const sizes[] = { "small", "medium", "large" };

            for (auto const& pair : sHousingMgr->GetShells())
            {
                Housing::ShellInfo const& shell = pair.second;
                uint8 const size = shell.SizeClass < 3 ? shell.SizeClass : 0;

                handler->PSendSysMessage("  shell {}: {} ({}, level {}, {}, {})", pair.first,
                    shell.Name, sizes[size], uint32(shell.LevelRequired),
                    shell.FactionMask == 1 ? "Alliance" : (shell.FactionMask == 2 ? "Horde" : "both"),
                    shell.GameObjectEntry ? std::to_string(shell.GameObjectEntry).c_str() : "no object yet");
            }

            handler->PSendSysMessage("Usage: .housing shell <id>");
            return true;
        }

        uint32 shellId = 0;
        if (!(shellId = ParseId(args)))
            return false;

        handler->PSendSysMessage("Shell: {}", ResultText(sHousingMgr->ApplyShell(player, shellId)));
        return true;
    }

    /*
     * Which building a shell is, is a fact about the realm, not about this module: there is no
     * game object in 8.3.7 called "the Human cottage". So the twelve shells ship unbound and
     * this is where the object gets chosen. The entry is validated against
     * `gameobject_template` before the row is written - a typo must not leave a shell pointing
     * at a display that does not exist.
     */
    static bool HandleBindShellCommand(ChatHandler* handler, char const* args)
    {
        if (!args)
        {
            handler->PSendSysMessage("Usage: .housing bindshell <shellId> <gameobject entry>");
            return true;
        }

        std::istringstream stream(args);
        std::string shellText;
        std::string entryText;

        if (!(stream >> shellText >> entryText))
        {
            handler->PSendSysMessage("Usage: .housing bindshell <shellId> <gameobject entry>");
            return true;
        }

        uint32 const shellId = ParseId(shellText);
        uint32 const entry = ParseId(entryText);

        if (!shellId || !entry)
        {
            handler->PSendSysMessage("Both arguments are numbers: shell id and game object entry.");
            return true;
        }

        Housing::HousingResult const result = sHousingMgr->BindShell(shellId, entry);

        if (result != Housing::Ok)
        {
            handler->PSendSysMessage("Shell {} not bound: {}.", shellId, ResultText(result));
            return true;
        }

        handler->PSendSysMessage("Shell {} now spawns object {}. Plots already standing keep the "
            "old building until their shell is spawned again - .housing shell <id> on the plot, "
            "or a restart.", shellId, entry);

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

    // ---- the features that only the addon could reach ---------------------------
    //
    // Every `C:` op the client sends had a command except these: moving a house, joining a
    // public street, the amenity list, the riddle hunts, the two-house selector, the duplicate
    // purchase, the drop-in-front placement and the build-mode toggle. A player without the
    // addon has to be able to do all of it, so these are the missing verbs - each one calling
    // the same manager method the protocol handler calls, not a second copy of the logic.

    static bool HandleHomesCommand(ChatHandler* handler, char const* args)
    {
        Player* player = GetSessionPlayer(handler);
        if (!player)
            return false;

        if (args && *args)
        {
            uint32 housingId = 0;
            if (!(housingId = ParseId(args)))
            {
                handler->SendSysMessage("Usage: .housing homes [housingId]");
                return true;
            }

            handler->PSendSysMessage("Select home: {}",
                ResultText(sHousingMgr->SelectHouse(player, housingId)));
            return true;
        }

        uint32 const accountId = player->GetSession()->GetAccountId();
        std::vector<Housing::HousingPlot*> const houses = sHousingMgr->GetOwnedPlots(accountId);

        if (houses.empty())
        {
            handler->SendSysMessage("No houses on this account yet.");
            return true;
        }

        // The same four facts C:HOMES answers with, plus the neighbourhood, because a chat
        // line has room for it and a player with two houses needs to tell them apart.
        Housing::HousingPlot* selected = sHousingMgr->GetHomePlot(accountId);

        handler->SendSysMessage("Houses on this account - * is the one the homestone uses:");

        for (Housing::HousingPlot* plot : houses)
        {
            if (!plot || !plot->GetHouse())
                continue;

            Housing::HousingNeighborhood* neighborhood = plot->GetNeighborhood();

            handler->PSendSysMessage("{} {} - {} - {} - plot {}, level {}", plot == selected ? "*" : " ",
                plot->GetHousingId(),
                plot->GetHouse()->GetName().empty() ? std::string("(unnamed)")
                                                    : plot->GetHouse()->GetName(),
                neighborhood ? neighborhood->GetName() : std::string("(no street)"),
                uint32(plot->GetIndex()), uint32(plot->GetHouse()->GetTier()));
        }

        handler->SendSysMessage(".housing selecthome <housingId> picks one; .housing home goes there.");
        return true;
    }

    static bool HandleSelectHomeCommand(ChatHandler* handler, char const* args)
    {
        Player* player = GetSessionPlayer(handler);
        if (!player || !args)
        {
            handler->SendSysMessage("Usage: .housing selecthome <housingId>");
            return true;
        }

        uint32 housingId = 0;
        if (!(housingId = ParseId(args)))
        {
            handler->SendSysMessage("Usage: .housing selecthome <housingId>");
            return true;
        }

        handler->PSendSysMessage("Selected home: {}",
            ResultText(sHousingMgr->SelectHouse(player, housingId)));
        return true;
    }

    static bool HandleJoinCommand(ChatHandler* handler, char const* args)
    {
        Player* player = GetSessionPlayer(handler);
        if (!player || !args)
        {
            handler->SendSysMessage("Usage: .housing join <neighbourhood> - see .housing finder");
            return true;
        }

        uint32 neighborhoodId = 0;
        if (!(neighborhoodId = ParseId(args)))
        {
            handler->SendSysMessage("Usage: .housing join <neighbourhood> - see .housing finder");
            return true;
        }

        handler->PSendSysMessage("Join: {}",
            ResultText(sHousingMgr->JoinNeighborhood(player, neighborhoodId)));
        return true;
    }

    static bool HandleLeaveNeighborhoodCommand(ChatHandler* handler, char const* /*args*/)
    {
        Player* player = GetSessionPlayer(handler);
        if (!player)
            return false;

        handler->PSendSysMessage("Left the neighbourhood: {}",
            ResultText(sHousingMgr->LeaveNeighborhood(player)));
        return true;
    }

    static bool HandleFoundCommand(ChatHandler* handler, char const* /*args*/)
    {
        Player* player = GetSessionPlayer(handler);
        if (!player)
            return false;

        handler->PSendSysMessage("Founded: {}",
            ResultText(sHousingMgr->FoundNeighborhood(player)));
        return true;
    }

    static bool HandleMoveHouseCommand(ChatHandler* handler, char const* args)
    {
        Player* player = GetSessionPlayer(handler);
        if (!player || !args)
        {
            handler->SendSysMessage("Usage: .housing movehouse <plot> - .housing list shows which are free");
            return true;
        }

        uint32 plotIndex = 0;
        if (!(plotIndex = ParseId(args)))
        {
            handler->SendSysMessage("Usage: .housing movehouse <plot> - .housing list shows which are free");
            return true;
        }

        handler->PSendSysMessage("Move: {}",
            ResultText(sHousingMgr->MoveHouse(player, uint8(plotIndex))));
        return true;
    }

    static bool HandleAmenitiesCommand(ChatHandler* handler, char const* /*args*/)
    {
        Player* player = GetSessionPlayer(handler);
        if (!player)
            return false;

        auto const& amenities = sHousingMgr->GetAmenities();

        if (amenities.empty())
        {
            handler->SendSysMessage("No amenities: `housing_amenity` is empty on this realm.");
            return true;
        }

        // The same five fields the addon's Amenities tab prints from S:VAREA - type, name, x,
        // y, biome - so a player reading the chat line gets what the window would have shown.
        // The names are AmenityType, in its own order, including the Coffer.
        //
        // One field the window does not print, because a player only needs the pin: what the
        // row spawned. An amenity whose gameobject_entry and creature_entry are both 0 is a
        // marker on a map and nothing in the world, and it looks exactly the same as a bound
        // one until somebody stands where the object should be. `bind_housing_content.sql`
        // reports the same thing in SQL; this is the same answer from in game.
        static char const* const types[] = {
            "bulletin board", "portal", "contractor", "flight point", "gathering",
            "scenery", "vendor", "endeavor", "coffer" };

        handler->SendSysMessage("Amenities in this module's town centre:");

        for (Housing::AmenityInfo const& amenity : amenities)
        {
            std::string const spawned = amenity.GameObjectEntry
                ? "object " + std::to_string(amenity.GameObjectEntry)
                : (amenity.CreatureEntry ? "npc " + std::to_string(amenity.CreatureEntry)
                                         : std::string("pin only - nothing spawns"));

            handler->PSendSysMessage("  {} - {} at {:.1f} {:.1f} (biome {}) - {}",
                amenity.Type < 9 ? types[amenity.Type] : "unknown type", amenity.Name,
                amenity.Pos.m_positionX, amenity.Pos.m_positionY, uint32(amenity.BiomeId), spawned);
        }

        return true;
    }

    static bool HandleHuntCommand(ChatHandler* handler, char const* args)
    {
        Player* player = GetSessionPlayer(handler);
        if (!player)
            return false;

        if (args && *args)
        {
            uint32 huntId = 0;
            if (!(huntId = ParseId(args)))
            {
                handler->SendSysMessage("Usage: .housing hunt [id]");
                return true;
            }

            handler->PSendSysMessage("Hunt {}: {}", huntId,
                ResultText(sHousingMgr->AcceptHunt(player, huntId)));
            return true;
        }

        auto const& hunts = sHousingMgr->GetHunts();

        if (hunts.empty())
        {
            handler->SendSysMessage("No riddle hunts: `housing_hunt` is empty on this realm.");
            return true;
        }

        handler->SendSysMessage("Riddle hunts - 0 not taken, 1 looking, 2 found:");

        for (Housing::HuntInfo const& hunt : hunts)
        {
            Housing::DecorInfo const* reward = hunt.RewardDecor
                ? sHousingMgr->GetDecor(hunt.RewardDecor) : nullptr;

            handler->PSendSysMessage("  {} [{}] {} - {} - {}{}", hunt.Id,
                uint32(sHousingMgr->GetHuntState(player, hunt.Id)), hunt.Name, hunt.Riddle,
                reward ? reward->Name : "no reward on this realm",
                hunt.CacheEntry ? "" : " (no cache yet)");
        }

        handler->SendSysMessage(".housing hunt <id> takes one.");
        return true;
    }

    static bool HandleBuyCopyCommand(ChatHandler* handler, char const* args)
    {
        Player* player = GetSessionPlayer(handler);
        if (!player || !args)
        {
            handler->SendSysMessage("Usage: .housing buycopy <decor> [count]");
            return true;
        }

        std::istringstream stream(args);
        std::string decorText;
        std::string countText;

        if (!(stream >> decorText))
        {
            handler->SendSysMessage("Usage: .housing buycopy <decor> [count]");
            return true;
        }

        uint32 const decorId = ParseId(decorText);
        uint32 const count = (stream >> countText) ? ParseId(countText) : 1;

        if (!decorId)
        {
            handler->SendSysMessage("Usage: .housing buycopy <decor> [count]");
            return true;
        }

        handler->PSendSysMessage("Copy: {}",
            ResultText(sHousingMgr->BuyDecorCopy(player, decorId, count)));
        return true;
    }

    static bool HandlePlaceAtCommand(ChatHandler* handler, char const* args)
    {
        Player* player = GetSessionPlayer(handler);
        if (!player || !args)
        {
            handler->SendSysMessage("Usage: .housing placeat <decor> [metres] [indoor]");
            return true;
        }

        std::istringstream stream(args);
        std::string decorText;
        std::string metresText;
        std::string indoorText;

        if (!(stream >> decorText))
        {
            handler->SendSysMessage("Usage: .housing placeat <decor> [metres] [indoor]");
            return true;
        }

        uint32 const decorId = ParseId(decorText);
        float const metres = (stream >> metresText) ? float(ParseId(metresText)) : 2.0f;
        uint32 const indoor = (stream >> indoorText) ? ParseId(indoorText) : 0;

        uint64 placementId = 0;
        Housing::HousingResult const result =
            sHousingMgr->PlaceDecorInFront(player, decorId, metres, indoor != 0, placementId);

        if (result == Housing::Ok)
            handler->PSendSysMessage("Placed as {}.", placementId);
        else
            handler->PSendSysMessage("Not placed: {}.", ResultText(result));

        return true;
    }

    static bool HandleBuildModeCommand(ChatHandler* handler, char const* args)
    {
        Player* player = GetSessionPlayer(handler);
        if (!player || !args)
        {
            handler->SendSysMessage("Usage: .housing buildmode <0|1> [decor]");
            return true;
        }

        std::istringstream stream(args);
        std::string onText;
        std::string decorText;

        if (!(stream >> onText))
        {
            handler->SendSysMessage("Usage: .housing buildmode <0|1> [decor]");
            return true;
        }

        uint32 const on = ParseId(onText);
        uint32 const decorId = (stream >> decorText) ? ParseId(decorText) : 0;

        handler->PSendSysMessage("Build mode {}.",
            ResultText(sHousingMgr->SetBuildMode(player, on != 0, decorId)));
        return true;
    }

    static bool HandleGryphonCommand(ChatHandler* handler, char const* /*args*/)
    {
        Player* player = GetSessionPlayer(handler);
        if (!player)
            return false;

        handler->PSendSysMessage("Gryphon: {}", ResultText(sHousingMgr->CallGryphon(player)));
        return true;
    }

    static bool HandleArcanostorageCommand(ChatHandler* handler, char const* /*args*/)
    {
        Player* player = GetSessionPlayer(handler);
        if (!player)
            return false;

        handler->PSendSysMessage("Arcanostorage: {}",
            ResultText(sHousingMgr->OpenArcanostorage(player)));
        return true;
    }

    static bool HandleFavorCommand(ChatHandler* handler, char const* /*args*/)
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

        handler->PSendSysMessage("Neighbourhood Favor in '{}': {}",
            neighborhood->GetName(), sHousingMgr->GetFavor(neighborhood->GetId()));

        return true;
    }

    static bool HandleRoomsCommand(ChatHandler* handler, char const* args)
    {
        Player* player = GetSessionPlayer(handler);
        if (!player)
            return false;

        if (args && *args)
        {
            uint32 roomId = 0;
            if (!(roomId = ParseId(args)))
            {
                handler->SendSysMessage("Usage: .housing rooms [id] - id buys");
                return true;
            }

            handler->PSendSysMessage("Room {}: {}", roomId,
                ResultText(sHousingMgr->BuyRoom(player, roomId)));
            return true;
        }

        Housing::HousingPlot* plot = sHousingMgr->GetOwnedPlot(player);
        if (!plot || !plot->GetHouse())
        {
            handler->SendSysMessage("You do not have a house.");
            return true;
        }

        // Listed with the level gate, the slot cost and the footprint, because all three decide
        // whether the row can be placed today.
        for (Housing::RoomInfo const& room : sHousingMgr->GetRoomsForPlot(plot))
            handler->PSendSysMessage("  {} {} - {} slot(s), level {}, {}x{} cells", room.Id,
                room.Name, uint32(room.BudgetCost), uint32(room.LevelRequired),
                uint32(room.CellsX), uint32(room.CellsY));

        // What is standing on the plan, and where. `rooms <id>` is the one-click form: it buys
        // the shape and puts it on the first cell that fits, which is what the addon's button
        // does too.
        sHousingMgr->LoadPlacedRooms(plot->GetHousingId());

        auto const& plan = sHousingMgr->GetPlacedRooms(plot->GetHousingId());

        if (plan.empty())
            handler->SendSysMessage("  the plan is empty: .housing rooms <id> places the first room on 0,0");
        else
            for (Housing::PlacedRoom const& placed : plan)
                if (Housing::RoomInfo const* room = sHousingMgr->GetRoom(placed.RoomId))
                    handler->PSendSysMessage("  on the plan: {} {} at {},{}, floor {}", placed.RoomId,
                        room->Name, int32(placed.CellX), int32(placed.CellY), uint32(placed.Floor));

        handler->PSendSysMessage("  plan cost {}/{} slot(s). .housing rooms <id> places one, "
            ".housing roomat <id> <x> <y> [floor] places it exactly, .housing roomremove takes it out",
            sHousingMgr->PlacedRoomCost(plot->GetHousingId()),
            sHousingMgr->GetHouseLevel(plot->GetHouse()->GetTier())
                ? sHousingMgr->GetHouseLevel(plot->GetHouse()->GetTier())->BudgetRoom : 0);
        return true;
    }

    static bool HandleRoomAtCommand(ChatHandler* handler, char const* args)
    {
        Player* player = GetSessionPlayer(handler);
        if (!player)
            return false;

        if (!args || !*args)
        {
            handler->SendSysMessage("Usage: .housing roomat <roomId> <cellX> <cellY> [floor]");
            return true;
        }

        // The cell is signed: the anchor is 0,0 and a plan grows in every direction from it.
        std::vector<std::string> parts = Housing::HousingAddon::Split(args, ' ');

        if (parts.size() < 3)
        {
            handler->SendSysMessage("Usage: .housing roomat <roomId> <cellX> <cellY> [floor]");
            return true;
        }

        uint32 roomId = 0;
        int32 cellX = 0;
        int32 cellY = 0;
        uint32 floor = 0;

        if (!Housing::HousingAddon::ParseUInt(parts[0], roomId)
            || !Housing::HousingAddon::ParseInt(parts[1], cellX)
            || !Housing::HousingAddon::ParseInt(parts[2], cellY)
            || (parts.size() > 3 && !Housing::HousingAddon::ParseUInt(parts[3], floor)))
        {
            handler->SendSysMessage("Usage: .housing roomat <roomId> <cellX> <cellY> [floor]");
            return true;
        }

        handler->PSendSysMessage("Room {} at {},{}, floor {}: {}", roomId, cellX, cellY, floor,
            ResultText(sHousingMgr->PlaceRoom(player, roomId, cellX, cellY, uint8(std::min<uint32>(floor, 255)))));
        return true;
    }

    static bool HandleRoomRemoveCommand(ChatHandler* handler, char const* args)
    {
        Player* player = GetSessionPlayer(handler);
        if (!player)
            return false;

        if (!args || !*args)
        {
            handler->SendSysMessage("Usage: .housing roomremove <roomId> <cellX> <cellY> [floor]");
            return true;
        }

        std::vector<std::string> parts = Housing::HousingAddon::Split(args, ' ');

        uint32 roomId = 0;
        int32 cellX = 0;
        int32 cellY = 0;
        uint32 floor = 0;

        if (parts.size() < 3
            || !Housing::HousingAddon::ParseUInt(parts[0], roomId)
            || !Housing::HousingAddon::ParseInt(parts[1], cellX)
            || !Housing::HousingAddon::ParseInt(parts[2], cellY)
            || (parts.size() > 3 && !Housing::HousingAddon::ParseUInt(parts[3], floor)))
        {
            handler->SendSysMessage("Usage: .housing roomremove <roomId> <cellX> <cellY> [floor]");
            return true;
        }

        handler->PSendSysMessage("Room {} at {},{}, floor {}: {}", roomId, cellX, cellY, floor,
            ResultText(sHousingMgr->RemovePlacedRoom(player, roomId, cellX, cellY, uint8(std::min<uint32>(floor, 255)))));
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
            { "access",   0, false, &HandleAccessCommand,     "Who may stand on the yard and who may come inside" },
            { "armillary", 0, false, &HandleArmillaryCommand, "The five Hearthlight Armillary presets" },
            { "dir",      0, false, &HandleDirectoryCommand,  "Neighbourhood directory" },
            { "finder",   0, false, &HandleFinderCommand,     "List public neighbourhoods" },
            { "visit",    0, false, &HandleVisitCommand,      "Visit a plot in another neighbourhood" },
            { "reserve",  0, false, &HandleReserveCommand,    "Hold a free plot for 60 seconds" },
            { "storage",  0, false, &HandleStorageCommand,    "Owned decor copies" },
            { "levelup",  0, false, &HandleLevelUpCommand,    "Pay the contractor to skip a level" },
            { "shell",    0, false, &HandleShellCommand,      "Choose or list exterior shells" },
            { "bindshell", rbac::RBAC_PERM_COMMAND_GM, false, &HandleBindShellCommand, "Point a shell at a game object" },
            { "homes",    0, false, &HandleHomesCommand,      "The houses on this account, and which one the homestone uses" },
            { "selecthome", 0, false, &HandleSelectHomeCommand, "Choose the house the homestone goes to" },
            { "join",     0, false, &HandleJoinCommand,       "Join a public or charter neighbourhood" },
            { "leavenb",  0, false, &HandleLeaveNeighborhoodCommand, "Leave the neighbourhood you live in" },
            { "found",    0, false, &HandleFoundCommand,      "Found your own neighbourhood" },
            { "movehouse", 0, false, &HandleMoveHouseCommand, "Move your house to another free plot" },
            { "amenities", 0, false, &HandleAmenitiesCommand, "The amenities in the town centre" },
            { "hunt",     0, false, &HandleHuntCommand,       "List the riddle hunts, or take one by id" },
            { "rooms",    0, false, &HandleRoomsCommand,      "List the room shapes and the floorplan, or place one by id" },
            { "roomat",   0, false, &HandleRoomAtCommand,     "Place a room shape on an exact floorplan cell" },
            { "roomremove", 0, false, &HandleRoomRemoveCommand, "Take a placed room shape off the floorplan" },
            { "buycopy",  0, false, &HandleBuyCopyCommand,    "Buy another copy of a piece you own" },
            { "placeat",  0, false, &HandlePlaceAtCommand,    "Place a piece in front of you" },
            { "buildmode", 0, false, &HandleBuildModeCommand, "The placement gizmo on or off" },
            { "gryphon",  0, false, &HandleGryphonCommand,    "Call the travel gryphon" },
            { "arcanostorage", 0, false, &HandleArcanostorageCommand, "Open the weekly Arcanostorage Sphere" },
            { "favor",    0, false, &HandleFavorCommand,      "Neighbourhood Favor here" },
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

    /*
     * `.housing access <plot|house> <mask|names>`. The mask is the six audience bits, and names
     * are accepted because nobody should have to convert 0x22 in their head to allow
     * neighbours and guests. Printing the current masks is the default.
     */
    static bool HandleArmillaryCommand(ChatHandler* handler, char const* args)
    {
        Player* player = GetSessionPlayer(handler);
        if (!player)
            return false;

        static char const* const names[] = {
            "default", "natural light", "unnatural dark", "evening calm", "midnight"
        };

        if (!args || !*args)
        {
            handler->SendSysMessage("Hearthlight Armillary presets:");
            for (uint32 preset = 0; preset < 5; ++preset)
                handler->PSendSysMessage("  {} - {}", preset, names[preset]);

            handler->SendSysMessage("Usage: .housing armillary <0-4>, or no argument to cycle");
            handler->PSendSysMessage("Ambience: {}", ResultText(sHousingMgr->ToggleArmillary(player)));
            return true;
        }

        uint32 preset = 0;

        if (std::sscanf(args, "%u", &preset) != 1 || preset > 4)
        {
            handler->SendSysMessage("Usage: .housing armillary <0-4>");
            return true;
        }

        handler->PSendSysMessage("Now {}: {}", names[preset],
            ResultText(sHousingMgr->SetArmillary(player, uint8(preset))));
        return true;
    }

    static bool HandleAccessCommand(ChatHandler* handler, char const* args)
    {
        Player* player = GetSessionPlayer(handler);
        if (!player)
            return false;

        Housing::HousingPlot* plot = sHousingMgr->GetOwnedPlot(player);
        if (!plot || !plot->GetHouse())
        {
            handler->SendSysMessage("You do not own a house.");
            return true;
        }

        std::istringstream stream(args ? args : "");
        std::string spaceText;
        stream >> spaceText;

        auto describe = [](uint8 mask)
        {
            std::string out;

            struct { uint8 bit; char const* name; } const names[] = {
                { Housing::AudienceAnyone, "anyone" },
                { Housing::AudienceNeighbors, "neighbours" },
                { Housing::AudienceGuild, "guild" },
                { Housing::AudienceParty, "party" },
                { Housing::AudienceFriends, "friends" },
                { Housing::AudienceGuests, "guests" }
            };

            for (auto const& entry : names)
            {
                if (!(mask & entry.bit))
                    continue;

                if (!out.empty())
                    out += ", ";

                out += entry.name;
            }

            return out.empty() ? std::string("nobody") : out;
        };

        if (spaceText.empty())
        {
            handler->PSendSysMessage("Yard:    {} ({})",
                describe(plot->GetHouse()->GetAccessMask(Housing::SpacePlot)),
                uint32(plot->GetHouse()->GetAccessMask(Housing::SpacePlot)));
            handler->PSendSysMessage("Inside:  {} ({})",
                describe(plot->GetHouse()->GetAccessMask(Housing::SpaceHouse)),
                uint32(plot->GetHouse()->GetAccessMask(Housing::SpaceHouse)));
            handler->SendSysMessage("Usage: .housing access <yard|house> <mask|names>");
            return true;
        }

        uint8 space = 0;

        if (spaceText == "yard" || spaceText == "plot" || spaceText == "0")
            space = Housing::SpacePlot;
        else if (spaceText == "house" || spaceText == "inside" || spaceText == "1")
            space = Housing::SpaceHouse;
        else
        {
            handler->SendSysMessage("Usage: .housing access <yard|house> <mask|names>");
            return true;
        }

        std::string rest;
        std::getline(stream, rest);

        uint32 mask = 0;
        std::istringstream numbers(rest);
        std::string token;
        bool named = false;

        while (numbers >> token)
        {
            uint32 parsed = 0;

            if (std::sscanf(token.c_str(), "%u", &parsed) == 1)
            {
                mask |= parsed;
                continue;
            }

            named = true;

            if (token == "anyone" || token == "all")
                mask |= Housing::AudienceAnyone;
            else if (token == "neighbours" || token == "neighbors")
                mask |= Housing::AudienceNeighbors;
            else if (token == "guild")
                mask |= Housing::AudienceGuild;
            else if (token == "party")
                mask |= Housing::AudienceParty;
            else if (token == "friends")
                mask |= Housing::AudienceFriends;
            else if (token == "guests")
                mask |= Housing::AudienceGuests;
            else if (token == "none" || token == "nobody")
                mask = 0;
            else
            {
                handler->PSendSysMessage("Unknown audience '{}'. Use anyone, neighbours, guild, "
                    "party, friends, guests, none, or a mask number.", token);
                return true;
            }
        }

        (void)named;

        handler->PSendSysMessage("Access: {}", ResultText(sHousingMgr->SetAccess(player, space, uint8(mask))));
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
