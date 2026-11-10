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

#ifndef HousingMgr_h__
#define HousingMgr_h__

#include "HousingAddon.h"
#include "HousingDecor.h"
#include "HousingDefines.h"
#include "HousingNeighborhood.h"

#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

class Creature;
class Map;
class Player;

namespace Housing
{
    /// Per-player housing state. Created on login, dropped on logout.
    struct PlayerState
    {
        uint8 ChunkMessageId = 0;
        uint32 RateWindowStart = 0;
        uint32 RateCount = 0;
        uint32 TickAccum = 0;
        uint32 NeighborhoodId = 0;   // neighbourhood the player is currently in
        uint8 InsidePlot = 0xFF;     // 0xFF = not standing in a plot
        bool InsideInterior = false;
        uint32 InsideHousingId = 0;  // house whose interior the player is standing in
        uint32 LastHomeUse = 0;      // ms timestamp of the last homestone use
        bool HasOwnedPlot = false;
        uint32 OwnedHousingId = 0;
        uint32 LastRuleCheck = 0;    // ms, throttles sanctuary and no-fly
        bool InBuildMode = false;    // placement gizmo: the owner is nudging a piece
        uint32 PendingDecorId = 0;   // the piece the gizmo is placing
        std::set<uint32> VisitedPlots;  // neighbourhood * 256 + plot, for the Grand Tour
    };

    /// Singleton that owns every piece of housing state on this worldserver.
    ///
    /// Threading: world thread only. Nothing here is safe to call from a map-update
    /// thread, and nothing here is mutex protected on purpose - the core already runs
    /// player and script updates on the world thread.
    class TC_GAME_API HousingMgr
    {
    public:
        static HousingMgr* instance();

        /// Human-readable name for a result code. The addon mirrors the numbers, but an NPC
        /// and a chat command have to say something a player can read.
        static char const* ResultName(HousingResult result);

        // ---- lifecycle -------------------------------------------------------

        void LoadConfig();
        void LoadHousingData();
        void LoadNeighborhoods();
        void Unload();
        void Update(uint32 diff);

        bool IsEnabled() const { return _enabled; }
        uint32 GetModuleVersion() const { return 1; }

        // ---- world data ------------------------------------------------------

        BiomeInfo const* GetBiome(uint32 id) const;        DecorInfo const* GetDecor(uint32 id) const;
        VendorInfo const* GetVendor(uint32 id) const;
        AmenityInfo const* GetAmenity(uint32 id) const;

        /// Every amenity row, in load order. Doc 14 lists what `.housing amenities` prints and
        /// the addon's Amenities tab prints the same five fields from the same list.
        std::vector<AmenityInfo> const& GetAmenities() const { return _amenityList; }

        // ---- riddle hunts ----------------------------------------------------

        std::vector<HuntInfo> const& GetHunts() const { return _huntList; }
        HuntInfo const* GetHunt(uint32 id) const;

        /// Which hunt a cache game object belongs to, by entry and nearest position. Null when
        /// the object is not one of the module's caches.
        HuntInfo const* HuntForCache(GameObject* go) const;

        /// Take a hunt: adds the quest if the player may have it.
        HousingResult AcceptHunt(Player* player, uint32 huntId);

        /// The cache is opened. Completes the quest, records the reward in the account's
        /// collection and reports the piece by name.
        HousingResult CompleteHunt(Player* player, uint32 huntId);

        /// 0 not taken, 1 taken and incomplete, 2 finished. What the window shows.
        uint8 GetHuntState(Player* player, uint32 huntId) const;

        /// Places the cache for every hunt that has an entry, in the neighbourhood's phase.
        void SpawnHuntCaches(HousingNeighborhood* neighborhood, Map* map);

        std::map<uint32, DecorInfo> const& GetDecorCatalogue() const { return _decor; }

        std::vector<PlotTemplate> const& GetPlotTemplates(uint32 mapId) const;        std::vector<VendorInfo> const& GetVendors() const { return _vendorList; }
        std::vector<VendorStockEntry> const& GetVendorStock(uint32 vendorId) const;        bool IsHousingMap(uint32 mapId) const;

        uint64 GeneratePlacementId() { return _nextPlacementId++; }
        uint32 GenerateHousingId() { return _nextHousingId++; }
        uint32 GenerateNeighborhoodId() { return _nextNeighborhoodId++; }

        // ---- neighbourhoods --------------------------------------------------

        HousingNeighborhood* GetNeighborhood(uint32 id);        std::map<uint32, std::unique_ptr<HousingNeighborhood>> const& GetNeighborhoods() const { return _neighborhoods; }
        HousingNeighborhood* FindNeighborhoodFor(Player* player);
        HousingNeighborhood* CreateNeighborhood(uint8 type, uint8 faction, std::string const& name,
            uint64 guildId, uint32 ownerAccount);
        void DissolveNeighborhood(HousingNeighborhood* neighborhood);

        // ---- player hooks ----------------------------------------------------

        void OnPlayerLogin(Player* player);
        void OnPlayerLogout(Player* player);
        void OnPlayerUpdate(Player* player, uint32 diff);
        void OnPlayerZoneChange(Player* player);

        // ---- protocol --------------------------------------------------------

        void HandleAddonMessage(Player* player, std::string const& message);
        void SendResult(Player* player, char const* op, HousingResult result, uint64 extra = 0);
        // ---- player operations -----------------------------------------------

        HousingResult BuyPlot(Player* player, HousingNeighborhood* neighborhood, uint8 plotIndex);
        HousingResult SellPlot(Player* player);
        HousingResult UpgradeTier(Player* player, uint8 tier);
        HousingResult RenameHouse(Player* player, std::string const& name);
        HousingResult SetLocked(Player* player, bool locked);

        HousingResult TeleportToPlot(Player* player, uint8 plotIndex);
        HousingResult TeleportHome(Player* player);
        HousingResult TeleportToNeighborhood(Player* player, HousingNeighborhood* neighborhood);
        HousingResult TeleportToTownCenter(Player* player);
        HousingResult EnterHouse(Player* player, bool enter);

        // ---- decor -----------------------------------------------------------

        /// The last two arguments are advanced mode's tilt axes. Both default to 0, so a
        /// caller that only knows about yaw - a command, a blueprint import, the addon before
        /// it grew tilt - places exactly what it placed before.
        HousingResult PlaceDecor(Player* player, uint32 decorId, float x, float y, float z, float o,
            float scale, bool indoor, uint64& placementId, float pitch = 0.0f, float roll = 0.0f);
        HousingResult MoveDecor(Player* player, uint64 placementId, float x, float y, float z, float o,
            float scale, float pitch = 0.0f, float roll = 0.0f);
        HousingResult RemoveDecor(Player* player, uint64 placementId);
        HousingResult BuyDecor(Player* player, uint32 decorId);

        /// A duplicate of a piece already unlocked, charged at its own price. `C:BUYCOPY` and
        /// `.housing buycopy` are the same call, not two implementations of it.
        HousingResult BuyDecorCopy(Player* player, uint32 decorId, uint32 count);

        /// Drop a piece in front of the player and snap it to the ground. The coordinates never
        /// leave the server: `C:PLACEAT` and `.housing placeat` both call this.
        HousingResult PlaceDecorInFront(Player* player, uint32 decorId, float distance,
            bool indoor, uint64& placementId);

        /// The placement gizmo's state. `C:BUILDMODE` and `.housing buildmode`.
        HousingResult SetBuildMode(Player* player, bool on, uint32 decorId);

        /// What a piece actually costs this player: the stock override, plus the smuggler's
        /// mark-up for a piece their faction cannot buy at home.
        uint32 GetPurchasePrice(Player* player, DecorInfo const& info) const;

        /// Whether the neighbourhood has passed the milestone that puts a piece on the shelf.
        bool IsStockAvailable(Player* player, DecorInfo const& info) const;

        bool HasUnlocked(uint32 accountId, uint32 decorId);
        void LoadCollection(uint32 accountId);

        // ---- access ----------------------------------------------------------

        HousingAccess CheckAccess(Player* player, HousingPlot* plot);
        /// The same question, asked of the yard or the interior.
        HousingAccess CheckSpace(Player* player, HousingPlot* plot, HousingSpace space);
        /// Which audiences this visitor belongs to, as HousingAudience bits.
        uint8 AudienceBits(Player* player, HousingPlot* plot);
        /// Owner-facing: set one space's audience mask.
        HousingResult SetAccess(Player* player, uint8 space, uint8 mask);
        HousingPlot* GetOwnedPlot(Player* player);
        /// Every house the account owns. Retail grants two, one per faction, and both belong
        /// to the account rather than to the character that bought them.
        std::vector<HousingPlot*> GetOwnedPlots(uint32 accountId);
        /// The house the Homestone goes to: the selected one when the account owns two, and
        /// the only one otherwise.
        HousingPlot* GetHomePlot(uint32 accountId);
        /// Choose that house.
        HousingResult SelectHouse(Player* player, uint32 housingId);
        HousingPlot* GetPlotForEdit(Player* player);
        PlayerState* GetPlayerState(Player* player);

        // ---- progression -----------------------------------------------------

        /// Level ladder, shells, rooms, dye variants, decor creatures, portals.
        void LoadProgression();

        uint32 GetXpPerNewDecor() const { return _xpPerDecor; }
        uint32 GetXpPerEndeavorTask() const { return _xpPerEndeavorTask; }

        uint8 GetLevelForHousing(uint32 housingId) const;
        uint32 GetXpForHousing(uint32 housingId) const;
        /// House XP. Learning a piece of decor pays *both* houses the account owns, which is
        /// what retail does; Endeavor XP pays only the house whose Endeavor was worked, so the
        /// two callers pass different answers.
        void GrantHouseXp(Player* player, uint32 xp, bool bothHouses = false);
        void GrantHouseXpToPlot(Player* player, HousingPlot* plot, uint32 xp);

        /// Grant every piece of decor whose `source_type`/`source_id` names this quest or
        /// achievement. A piece already owned is skipped, so a repeatable quest cannot be
        /// farmed for storage copies.
        void GrantSourceDecor(Player* player, uint8 sourceType, uint32 sourceId);

        /// Grant the decor a character has already earned achievements for. Runs at login,
        /// because this core has no achievement-complete hook.
        void SyncAchievementDecor(Player* player);

        // ---- the introduction chain, 900100-900103 ---------------------------
        //
        // These four quests were shipped as rows with no giver and no objective hook, so
        // nothing in the game could offer them and nothing could finish them. The Steward
        // hands over the one that is next and the Contractor accepts the turn-in; the
        // objective is a module event and is marked here.

        /// The quest the player should be offered next, or 0 when the chain is finished.
        uint32 GetIntroQuestFor(Player* player);
        bool OfferIntroQuest(Player* player, Creature* giver);
        bool TurnInIntroQuest(Player* player);

        /// Called from the events the chain is built on: a plot bought, a piece placed, a
        /// level reached. Marks the current quest complete when its condition is met.
        void CheckIntroQuestProgress(Player* player);
        HousingResult LevelUpWithGold(Player* player);
        HousingResult ApplyShell(Player* player, uint32 shellId);

        /// Point a shell row at a building object. The shells ship unbound because no name in
        /// `gameobject_template` means "the Human cottage" - see doc 06 sec.1.5 - so the object
        /// is chosen on the realm that owns it, by `.housing bindshell`, and validated here
        /// against the template table before the row is written.
        HousingResult BindShell(uint32 shellId, uint32 gameObjectEntry);

        /// The small shell that matches the character's racial kit: Human, Night Elf, Orc or
        /// Blood Elf (doc 01 sec.4). Applied when a plot is bought so a new house is dressed,
        /// instead of standing as a patch of grass until somebody picks an exterior by hand.
        ShellInfo const* GetStarterShell(Player* player) const;

        HouseLevelInfo const* GetHouseLevel(uint8 level) const;
        ShellInfo const* GetShell(uint32 id) const;
        std::map<uint32, ShellInfo> const& GetShells() const { return _shells; }
        RoomInfo const* GetRoom(uint32 id) const;
        std::vector<RoomInfo> GetRoomsForShell(uint32 shellId) const;
        DecorVariant const* GetVariant(uint32 decorId, uint8 group, uint8 color) const;
        PetBedInfo const* GetDecorCreature(uint32 decorId) const;
        HousingPortal const* GetPortal(uint32 id) const;
        std::vector<HousingPortal> const& GetPortals() const { return _portalList; }

        // ---- storage (retail: four chairs are four chairs) -------------------

        void LoadStorage(uint32 accountId);
        uint32 GetStorageCount(uint32 accountId, uint32 decorId);
        void AddStorage(uint32 accountId, uint32 decorId, uint32 count);
        bool ConsumeStorage(uint32 accountId, uint32 decorId);
        std::map<uint32, uint32> const& GetStorage(uint32 accountId);

        // ---- dyes ------------------------------------------------------------

        HousingResult DyeDecor(Player* player, uint64 placementId, uint8 group, uint8 color);

        /// Put the companion the player has out onto a bed, or take it off (mode 0). The pet is
        /// the one actually out - the journal is checked so a species the player does not own
        /// cannot be named, but the creature is taken from what is standing there.
        HousingResult AssignPet(Player* player, uint64 placementId, uint8 mode);

        /// The palette the realm's own gameobject names support. Loaded from `housing_dye`,
        /// which the bind script fills by looking at which colour words actually appear in
        /// front of a furniture name - there is no invented colour list here.
        std::map<uint8, std::string> const& GetDyes() const { return _dyes; }

        /// Every key LoadConfig read, with the value it ended up with after clamping. A setting
        /// that is only in a file is a setting nobody can check; this is what `.housing config`
        /// prints, and it is the same map the loader filled, not a second reading of the file.
        std::vector<std::pair<std::string, std::string>> const& GetConfigSnapshot() const { return _configValues; }
        std::string const* GetDyeName(uint8 id) const;

        // ---- travel, finder, visit, reservation ------------------------------

        std::vector<HousingNeighborhood*> GetPublicNeighborhoods(uint8 faction);
        HousingResult VisitNeighborhood(Player* player, uint32 neighborhoodId, uint8 plotIndex);
        HousingResult UsePortal(Player* player, uint32 portalId);
        HousingResult ReservePlot(Player* player, uint8 plotIndex);

        // ---- archive (the regret window) -------------------------------------

        bool HasArchive(uint32 accountId);
        HousingResult RestoreArchivedHouse(Player* player);

        // ---- blueprints ------------------------------------------------------

        HousingResult ExportBlueprint(Player* player, uint8 kind, std::string const& name, std::string& code);
        HousingResult ImportBlueprint(Player* player, std::string const& code);
        HousingResult ReadBlueprintCode(Player* player, std::string const& code, std::vector<DecorPlacement>& out,
            uint8& kind);

        // ---- endeavor glue ---------------------------------------------------

        uint32 GetPlayerNeighborhoodId(Player* player);
        void ApplyNeighborhoodPhase(Player* player, HousingNeighborhood* neighborhood);
        void SpawnNeighborhoodContent(HousingNeighborhood* neighborhood, Map* map);
        void EnforceBiomeRules(Player* player);

        /// Owner, or a guest with the edit bit. Place, move and remove all go through this.
        bool CanEdit(Player* player, HousingPlot* plot);

        /// The extended opcode set: finder, storage, endeavors, codes, dyes, portals, shells.
        bool HandleExtendedOp(Player* player, PlayerState& state, std::string const& op,
            std::vector<std::string> const& fields);

        // ---- lifecycle -------------------------------------------------------

        /// Eviction, grace and the population cap. Runs on its own timer: a house nobody has
        /// touched for Housing.Evict.Days goes back on the market, and a neighbourhood never
        /// holds more residents than Housing.Neighborhood.PopulationCap.
        void RunLifecycle();

        /// A private or guild neighbourhood that has held nobody for the grace period is
        /// dissolved: its plots are sold, its members are told, and its slot goes back.
        void RunDissolutionSweep();

        uint32 GetNeighborhoodPopulation(HousingNeighborhood* neighborhood) const;        /// Join a neighbourhood that is open to the player's faction. Refuses when it is full.
        HousingResult JoinNeighborhood(Player* player, uint32 neighborhoodId);
        HousingResult LeaveNeighborhood(Player* player);

        /// Found a neighbourhood: a private one for a handful of friends, or a guild one when
        /// the player leads a guild. The first plot is theirs only after they buy it.
        HousingResult FoundNeighborhood(Player* player);

        /// The founder plus every party member standing on the same map. doc 01 sec.6 says a
        /// neighbourhood is founded by a minimum number of *players*, not characters, so the
        /// count is of people who are actually there - not of alts, not of a roster.
        uint32 CountPresentFounders(Player* player) const;

        /// Membership administration. Rank 3 is the founder, 2 an officer, 1 a resident.
        HousingResult InviteToNeighborhood(Player* player, std::string const& name);
        HousingResult SetMemberRank(Player* player, std::string const& name, uint8 rank);
        HousingResult RemoveFromNeighborhood(Player* player, std::string const& name);

        /// The first character name on an account. Membership is stored by account, and a
        /// founder reading a list of account ids has been told nothing at all.
        std::string GetAccountCharacterName(uint32 accountId);

        /// The Steward's collection milestones - the seven conditions in doc 04 - as text,
        /// one line each, achieved or not. There is no achievement DBC entry on 8.3.7, so
        /// this is where the player reads them.
        std::string BuildMilestoneReport(Player* player);

        /// Move the house to another free plot in the same neighbourhood, keeping every piece
        /// of furniture, the level and the exterior. Retail's "move house" is free; so is this.
        HousingResult MoveHouse(Player* player, uint8 plotIndex);

        /// The Steward's gryphon: a paid, cooled-down flight to the player's own plot.
        HousingResult CallGryphon(Player* player);

        /// A guild that outgrows one neighbourhood gets a second one attached to it: same
        /// guild, own slot, own plots. Returns the wing's neighbourhood.
        HousingNeighborhood* GetOrCreateGuildWing(uint64 guildId, uint8 faction);

        /// Rooms bought at the Contractor: a room is an area of the interior and a budget.
        HousingResult CanBuyRoom(Player* player, uint32 roomId) const;
        /// Buy a shape and put it on the plan in the first cell that fits it.
        HousingResult BuyRoom(Player* player, uint32 roomId);
        std::vector<RoomInfo> GetRoomsForPlot(HousingPlot* plot) const;

        /// The floorplan (doc 06 sec.1.2). A room that is bought is placed, not counted: the
        /// plan is the record, `rooms_owned` is its cost, and every rule below is checked
        /// before a row is written.
        std::vector<PlacedRoom> const& GetPlacedRooms(uint32 housingId) const;
        void LoadPlacedRooms(uint32 housingId);
        /// Buy a shape and put it on an exact cell.
        HousingResult PlaceRoom(Player* player, uint32 roomId, int32 cellX, int32 cellY, uint8 floor);
        /// Take a placed shape off the plan, returning its slots to the budget.
        HousingResult RemovePlacedRoom(Player* player, uint32 roomId, int32 cellX, int32 cellY, uint8 floor);
        /// Slots the plan spends. The house's `rooms_owned` is this number.
        uint32 PlacedRoomCost(uint32 housingId) const;
        /// Drop the plan when the house is sold: the rooms go with the building.
        void ClearPlacedRooms(uint32 housingId);

        /// The weekly Arcanostorage Sphere: level 10, one roll per account per week.
        HousingResult OpenArcanostorage(Player* player);

        /// The Hearthlight Armillary: level 6, toggles the plot between day and night.
        HousingResult ToggleArmillary(Player* player);
        /// One of the five presets. Toggle is the cycle; this is the menu and the command.
        HousingResult SetArmillary(Player* player, uint8 preset);

        /// Endeavor favour: the neighbourhood's progress as a 0-1000 bar, sent as a worldstate.
        void SendFavor(Player* player, uint32 neighborhoodId);
        uint32 GetFavor(uint32 neighborhoodId) const;

        /// How many of the Endeavor's tasks must be done for one unit of progress, scaled by
        /// how many people actually live in the neighbourhood.
        uint32 ScaledRequirement(uint32 baseRequired, uint32 neighborhoodId) const;

        /// Spawn the Endeavor's themed props and NPCs in the neighbourhood's phase once it is
        /// complete, so a finished Endeavor changes what the town centre looks like.
        void SpawnEndeavorTheming(HousingNeighborhood* neighborhood, Map* map, uint32 endeavorId,
            uint8 milestone);

        // ---- maintenance -----------------------------------------------------

        void SaveAll();
        void ForceSellPlot(HousingPlot* plot);

        /// Park a house in the regret window before it is destroyed.
        ///
        /// Every path that takes a house away from its owner calls this first - a sale, an
        /// eviction, a removal from a street, a neighbourhood being dissolved - because the
        /// archive is what the warning letter promises and what `.housing restore` reads. The
        /// account is taken from the plot when 0 is passed, which is what the sweeps do: they
        /// have a plot and no player.
        void ArchiveHouse(HousingPlot* plot, uint32 accountId = 0);
        std::string BuildAuditReport();

        /// The plot standing at a world position, give or take a radius. A gameobject script
        /// knows nothing but where its object is, so the plot has to be found from that.
        HousingPlot* FindPlotAt(uint32 mapId, float x, float y, float radius = 30.0f);

    private:
        HousingMgr();
        ~HousingMgr() = default;

        // config
        bool _enabled;
        uint32 _housingMapId;
        uint32 _interiorMapId;
        Position _interiorAnchor;
        Position _townCenter;
        uint32 _saveInterval;
        uint32 _rateLimit;
        uint32 _homeCooldown;
        uint32 _refundPercent;
        bool _plotScopedPhasing;
        bool _nativeMap;
        uint32 _evictDays;          // 0 = eviction off
        uint32 _graceDays;          // days a house sits in the archive before it is sold
        uint32 _populationCap;      // residents per neighbourhood, 0 = no cap
        uint32 _lifecycleTimer;
        uint32 _gryphonCost;        // gold, paid to the Steward
        uint32 _gryphonCooldown;    // seconds
        uint32 _dyeCost;            // gold per dye change, 0 = free
        uint32 _restoreFeePercent;  // share of the archived price paid to restore it
        uint32 _reservationSeconds; // how long an inspection holds a plot
        uint32 _smugglerMarkupPercent;  // mark-up on the other faction's furniture
        float _endeavorScaleFloor;  // Endeavor requirements scale with residents, between these
        float _endeavorScaleCap;
        uint32 _xpPerDecor;         // house XP for learning a piece the house has never held
        uint32 _xpPerEndeavorTask;  // house XP for finishing one Endeavor task
        uint32 _maxFounded;         // founded neighbourhoods per account, 0 = unlimited
        uint32 _minFounders;        // players that have to be present to found a charter, 1 = off
        uint32 _minResidents;       // for dissolving a private/guild neighbourhood
        uint32 _dissolveGraceDays;
        uint32 _arcaneLevel;        // house level needed for the Sphere
        uint32 _arcaneCooldownDays;
        uint32 _armillaryLevel;
        bool _guildWings;
        uint32 _cornerstoneEntry;   // plot stone, 0 = no stone
        uint32 _petBedIndoorCap;    // beds per house, retail 100
        uint32 _petBedOutdoorCap;   // beds per plot, retail 25

        // world data
        std::map<uint32, BiomeInfo> _biomes;
        std::map<uint64, PlotTemplate> _plotTemplates;                 // key: (mapId << 8) | index
        std::map<uint32, std::vector<PlotTemplate>> _plotTemplatesByMap;
        std::map<uint32, DecorInfo> _decor;
        std::map<uint32, VendorInfo> _vendors;
        std::map<uint32, std::vector<VendorStockEntry>> _vendorStock;
        /// Does this footprint sit inside the grid, clear of every room on its floor, and
        /// touch the plan that is already there? A first room on floor 0 is the anchor and
        /// sits at 0,0; a room above floor 0 has to be reached by a stairwell below it.
        bool RoomFits(uint32 housingId, RoomInfo const& room, int32 cellX, int32 cellY,
            uint8 floor) const;
        /// Is every room on this plan reachable from the anchor over shared cell edges, and
        /// across a floor only where a stairwell stands under the room above.
        bool PlanIsConnected(std::vector<PlacedRoom> const& plan) const;
        /// The same question asked about a plan with one room taken out of it.
        bool PlanStaysConnected(uint32 housingId, PlacedRoom const& ignore) const;
        /// Find the first cell a shape fits in, scanning floor 0 outwards and then each floor
        /// a stairwell reaches. Returns false when the plan has no room for it.
        bool FindFreeCell(uint32 housingId, RoomInfo const& room, PlacedRoom& out) const;
        /// Recompute `rooms_owned` from the plan and write it to the character DB.
        void SyncRoomCost(HousingPlot* plot);

        std::map<uint32, AmenityInfo> _amenities;
        std::vector<AmenityInfo> _amenityList;
        std::vector<HuntInfo> _huntList;
        std::map<uint32, HuntInfo> _hunts;
        std::vector<VendorInfo> _vendorList;
        std::map<uint32, std::vector<std::string>> _namePool;           // faction -> names
        std::map<uint8, HouseLevelInfo> _levels;
        std::map<uint32, ShellInfo> _shells;
        std::map<uint32, RoomInfo> _rooms;
        std::map<uint32, std::vector<PlacedRoom>> _placedRooms;          // housingId -> floorplan
        std::map<uint8, std::string> _dyes;
        std::vector<std::pair<std::string, std::string>> _configValues;
        std::map<uint32, HousingPortal> _portals;
        std::vector<HousingPortal> _portalList;
        std::map<uint32, DecorVariant> _variants;                       // (decor << 16) | group << 8 | colour
        std::map<uint32, PetBedInfo> _decorCreatures;
        std::map<uint32, std::map<uint32, uint32>> _storage;            // accountId -> decorId -> count
        std::set<uint32> _storageLoaded;
        std::map<uint8, std::pair<uint32, uint32>> _reservations;       // plotIndex -> account, expiry
        std::map<uint32, uint32> _selectedHouse;                        // accountId -> housingId
        void LoadHouseSelection(uint32 accountId);
        std::map<uint32, uint32> _gryphonUsed;                          // accountId -> ms stamp
        std::map<uint32, uint32> _arcaneUsed;                           // accountId -> unix stamp
        std::map<uint32, uint32> _emptySince;                           // neighborhoodId -> unix stamp
        std::set<uint32> _contentSpawned;                               // neighborhoods whose static content is up

        // runtime state
        std::map<uint32, std::unique_ptr<HousingNeighborhood>> _neighborhoods;
        std::vector<bool> _usedSlots;
        std::map<uint64, PlayerState> _playerStates;                    // key: player guid counter
        std::map<uint32, std::set<uint32>> _collection;                 // accountId -> decor ids
        std::set<uint32> _collectionLoaded;                             // accounts already read
        std::map<uint32, std::map<uint32, uint8>> _guestCache;           // housingId -> accountId -> flags
        std::map<uint32, uint32> _houseViewers;                         // housingId -> players holding its phase

        uint64 _nextPlacementId;
        uint32 _nextHousingId;
        uint32 _nextNeighborhoodId;
        uint32 _saveTimer;
        uint32 _tick;

        // helpers
        static uint64 PlotKey(uint32 mapId, uint8 index);
        void LoadBiomes();
        void LoadPlotTemplates();
        void LoadDecor();
        void LoadVendors();
        void LoadAmenities();
        void LoadHunts();
        void LoadNamePool();
        void LoadLevels();
        void LoadShells();
        void LoadRooms();
        void LoadPortals();
        void LoadDyes();
        void LoadVariants();
        void LoadDecorCreatures();

        /// A name for a new neighbourhood, drawn from the faction's pool with a number
        /// appended so two foundations cannot collide.
        std::string GenerateNeighborhoodName(uint8 faction);

        /// Derive the spawn of a plot when the template has none (anchor mode).
        void BuildAnchorLayout();
        Position AnchorForPlot(uint8 plotIndex) const;

        /// The plot's front door, with the ground height filled in when the row asked for it.
        Position PlotSpawn(HousingPlot* plot, Player* player);
        Position AnchorForAmenity(uint32 index) const;
        Position AnchorForHunt(uint32 index) const;
        void SpawnPlotShell(HousingPlot* plot, HousingNeighborhood* neighborhood, Map* map);
        void SpawnCornerstone(HousingPlot* plot, HousingNeighborhood* neighborhood, Map* map);

        /// The stalls themselves: one creature per `housing_vendor` row that has an entry and
        /// a position, in the town centre phase. Without this the shops are rows in a table.
        void SpawnVendorStalls(HousingNeighborhood* neighborhood, Map* map);

        /// Tell everybody standing in the neighbourhood that a milestone moved.
        void BroadcastEndeavorMilestone(HousingNeighborhood* neighborhood, uint8 milestone);
        uint32 GetNeighborhoodPhaseOf(uint32 neighborhoodId) const;

        uint16 AllocateSlot();
        void ReleaseSlot(uint16 slot);

        void ResolvePlot(Player* player, PlayerState& state);

        /// Throw a player out of a plot they no longer have access to.
        void EnforceAccess(Player* player, PlayerState& state);
        void ClearPlotPhases(Player* player, PlayerState& state);
        void ApplyPlotPhase(Player* player, HousingPlot* plot);
        void ReleaseViewer(HousingPlot* plot);

        /// True while somebody is still inside the house's interior. Guards the despawn that
        /// follows the last viewer leaving: the outdoor plot and the interior are different
        /// maps, so a player looting the interior is not a "viewer" of the yard.
        bool IsHouseOccupied(uint32 housingId) const;

        void HandleHello(Player* player, PlayerState& state);
        void HandleNeighborhoodInfo(Player* player, PlayerState& state);
        void HandlePlotList(Player* player, PlayerState& state);
        void HandlePlotInfo(Player* player, PlayerState& state, uint8 plotIndex);
        void HandleDirectory(Player* player, PlayerState& state, uint32 page);
        void HandleDecorList(Player* player, PlayerState& state, std::string const& category,
            std::string const& subcategory, uint32 page);
        void HandlePlacedList(Player* player, PlayerState& state);
        void HandleGuestList(Player* player, PlayerState& state);
        void HandleAmenityList(Player* player, PlayerState& state);

        std::map<uint32, uint8> const& LoadGuests(uint32 housingId);
        uint8 GetGuestFlags(uint32 housingId, uint32 accountId);
        void AddGuest(uint32 housingId, uint32 accountId, uint8 flags);
        void RemoveGuest(uint32 housingId, uint32 accountId);

        void SendChunk(Player* player, std::string const& payload);
    };

} // namespace Housing

#define sHousingMgr Housing::HousingMgr::instance()

#endif // HousingMgr_h__
