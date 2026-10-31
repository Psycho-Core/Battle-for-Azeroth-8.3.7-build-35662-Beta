# 13 - mod-guildhouse: read it, then decide what the fallback is

This is the backup idea, taken apart. `azerothcore/mod-guildhouse` is the oldest and most
installed Guild House module for AzerothCore. It is also the closest thing in the ecosystem
to what I am building, so the honest question is not "is it good" but "how much of my plan
survives if I have to fall back to it".

I read the whole repository, not the README: 3 076 lines across 12 files, every C++ line,
every SQL statement, the config, the loader. Then I checked every AzerothCore API it calls
against **my** tree, which is BfA 8.3.7 build 35662, because that is the only comparison that
decides anything. Section 8 is the result and it is the most important page here: the module
is written for WotLK and roughly a third of its API surface does not exist in my core.

The short version:

- it is a **service hub**, not a housing system: one island, one phase per guild, and a fixed
  list of 55 facilities bought from a butler at fixed coordinates. There is no furniture, no
  placement, no collection, no rooms, no dye, no blueprint, no UI beyond gossip;
- it is **not portable to 8.3.7 as written**. The phase model it is built on was deleted from
  the core before BfA;
- what is genuinely worth taking is the **purchase flow** and the **facility list**, and those
  map onto tables I already have. Section 12 is the mapping, section 14 is the fallback.

## 1. What the module actually does, in order

1. A GM places one NPC in the world by hand: `.npc add 500030`. Nothing spawns it otherwise.
2. A player who leads a guild talks to that NPC. The gossip offers *buy a guild house* only to
   the guild master, and only if the guild owns none.
3. Buying inserts one row into `guild_house` in the **characters** database. The row carries
   the phase, the map, and a fixed teleport position. The phase is not allocated: it is
   computed as `guildId + 10`.
4. Buying also spawns two things automatically inside that phase: a portal to Stormwind or
   Orgrimmar depending on team, and the butler NPC.
5. Any guild member can then reach the house by gossip teleport or by `.guildhouse tele`
   (alias `.gh tele`). Standing on GM Island puts the member into the guild's phase; leaving
   the zone restores the normal phase.
6. The butler sells facilities. Each purchase is a gossip confirmation with a copper price,
   which spawns one creature or one game object at a **fixed coordinate** from the world table
   `guild_house_spawns`, in that guild's phase, and saves it to `creature` / `gameobject`.
7. Selling the house back deletes the row and every spawn in that phase, and refunds half the
   house price.
8. When the guild disbands, `GuildScript::OnDisband` runs the same removal.

That is the entire feature set. Fourteen hundred and fifty-five lines of C++ for it, because the spawn, save,
reload-from-DB and grid-registration dance has to be written out twice (creature and game
object) and every facility needs a gossip branch.

## 2. The repository at a glance

| Property | Value |
| --- | --- |
| Stars / forks | 53 / 68 |
| Open issues | 19 |
| Created | 2019-03-25 |
| Last push to `master` | 2026-06-18 |
| Contributors | 20, top three 15 / 14 / 9 commits |
| License | **none in the repository** - no `LICENSE`, no license field on the project |
| Core requirement | AzerothCore (WotLK) at commit `77f1363` or later |
| Build check | GitHub Actions workflow compiling against AzerothCore, WotLK only |

The licence point matters more than it looks. The module is a derivative of GPLv2 code and
ships no license text of its own, so there is no explicit grant to reuse it. Reading it for
design is fine. Lifting its source verbatim into my module is a decision I would not make
without asking the authors first, and I would rather write the 200 lines I actually need.

Activity is real but slow: the last three commits are a localization PR pair from June 2026,
before that a run of schema-compatibility fixes across 2025 and 2026, all of them chasing
AzerothCore's own column renames (`id1` to `id`, `creature_template_model`, trainer fields).
That is the profile of a module maintained against a moving upstream, not one being rewritten.

The open issues are the useful part, because they are the module's own bug list:

| Issue | What it says, in short |
| --- | --- |
| 86 | spawned objects get no rotation from their orientation |
| 85 | players cannot see NPCs in Alliance territory |
| 83 | asks for "free spawns" and "spawn all" config options |
| 81, 54 | the `creature_template` SQL has broken twice on column changes |
| 76 | guild bank model renders invisible |
| 75 | asks for neutral-faction trainers and a GM Island sanctuary override |
| 71 | the SQL is not in the automated-import directory |
| 70 | teleport command has no cooldown |
| 68 | "guild house does not appear to be instanced" |
| 65, 60 | resets to 0, NPC spawn failures |
| 52 | how to add custom NPCs |
| 42 | profession trainers only reach Master/Artisan |
| 39, 37, 34 | compile issue, house disappears after restart, seller issues |

Two of those - 37 and 68 - are the same structural weakness I find in the code in section 9:
the house is a phase on a live world map, so it lives or dies with grid loading, and it is not
an instance no matter what it looks like.

## 3. The install, and what it asks of a server

| Step | Detail |
| --- | --- |
| Drop in `modules/` | re-run CMake, full rebuild |
| Config | copy `mod_guildhouse.conf.dist` to `mod_guildhouse.conf`, 13 keys |
| Characters SQL | `mod-guildhouse/data/sql/db-characters/2024_04_07_guildhouse.sql`, 11 lines, one table |
| World SQL | four files, 1 451 lines, three of them content |
| Locale SQL | two files, 1 324 lines, must run after the content or the gossip is blank |
| Manual step | `.npc add 500030` somewhere public |
| Client | nothing |

The SQL is not in the auto-import updates layout, which is issue 71: it has to be applied by hand
or moved. The dates in the filenames (2024_04_07) are older than the locale files
(2025_11_25), so the order is not alphabetical, and getting it wrong leaves an empty locale
table - which, since every gossip line goes through the locale lookup, produces a menu of
blank options rather than an error.

## 4. The data model: two tables that matter

`guild_house` in the characters database, one row per guild that bought a house:

| Column | Type | Meaning |
| --- | --- | --- |
| `id` | int, auto | row id |
| `guild` | int, unique | guild id, uniqueness is what stops a second house |
| `phase` | int | `guildId + 10`, stored, not derived on read |
| `map` | int | 1, Kalimdor |
| `positionX/Y/Z` | float | 16222.972 / 16267.802 / 13.136777 |
| `orientation` | float | 1.461173 |

`guild_house_spawns` in the world database, 55 rows, one per facility, `UNIQUE KEY entry`:

| Column | Type | Meaning |
| --- | --- | --- |
| `id` | int | row id |
| `entry` | int, unique | creature entry or game object entry |
| `posX/Y/Z`, `orientation` | float | the fixed anchor for that facility |
| `comment` | varchar(500) | human label, e.g. `Paladin Trainer` |

Third table, `mod_guildhouse_locale`, world database, `(Id, Locale)` primary key, `Text`.

That is all. Two facts follow immediately, and they are what separate this module from mine:

- **the anchor set is global, not per house.** Every guild that buys a Blacksmithing trainer
  gets it at `16220.5 / 16302.3`, because the coordinate comes from the shared world table.
  Houses differ only by which facilities were bought;
- **there is no per-house content at all.** No table stores position, rotation, scale or item
  for anything placed by a player, because a player never places anything.

## 5. The three NPCs and the ten portals it adds

Creatures, all entry `500030 + n`:

| Entry | Name | Subname | Script | Model |
| --- | --- | --- | --- | --- |
| 500030 | Talamortis | Guild House Seller | `GuildHouseSeller` | 25901 |
| 500031 | Xrispins | Guild House Butler | `GuildHouseSpawner` | 25901 |
| 500032 | Innkeeper Monica | - | `npc_innkeeper` | 18234 |

Game objects, all entry `500000 + n`, all type 22 (spell caster), `Data0` is the teleport
spell, `Data3 = 1`:

| Entry | Name | DisplayId | Spell |
| --- | --- | --- | --- |
| 500000 | Portal to Stormwind | 4396 | 17334 |
| 500001 | Portal to Darnassus | 4393 | 17608 |
| 500002 | Portal to Exodar | 6955 | 32268 |
| 500003 | Portal to Ironforge | 4394 | 17607 |
| 500004 | Portal to Orgrimmar | 4395 | 17609 |
| 500005 | Portal to Silvermoon | 6956 | 32270 |
| 500006 | Portal to Thunder Bluff | 4397 | 17610 |
| 500007 | Portal to Undercity | 4398 | 17611 |
| 500008 | Portal to Shattrath | 7146 | 35718 |
| 500009 | Portal to Dalaran | 8111 | 53141 |

Two more conventions worth noting because they cost nothing and I use the same ones:
`VerifiedBuild` in the template rows is `12340`, i.e. WotLK, and every creature is level 35,
faction 35, `unit_flags2 = 2048`. On 8.3.7 the build column has to say 35662 or the row is
flagged on import.

## 6. The 55 facilities, in full

Every row of `guild_house_spawns`, with the config key that prices it. Prices are copper as
shipped in the config file, with gold in brackets. `T` = the trainer keys, which are three
separate keys, not one.

| # | Entry | Facility | Price key |
| --- | --- | --- | --- |
| 1 | 26327 | Paladin Trainer | `GuildHouseTrainerCost` |
| 2 | 26324 | Druid Trainer | `GuildHouseTrainerCost` |
| 3 | 26325 | Hunter Trainer | `GuildHouseTrainerCost` |
| 4 | 26326 | Mage Trainer | `GuildHouseTrainerCost` |
| 5 | 26328 | Priest Trainer | `GuildHouseTrainerCost` |
| 6 | 26329 | Rogue Trainer | `GuildHouseTrainerCost` |
| 7 | 26330 | Shaman Trainer | `GuildHouseTrainerCost` |
| 8 | 26331 | Warlock Trainer | `GuildHouseTrainerCost` |
| 9 | 26332 | Warrior Trainer | `GuildHouseTrainerCost` |
| 10 | 500032 | Innkeeper Monica | `GuildHouseInnKeeper` |
| 11 | 30605 | Banker | `GuildHouseBank` |
| 12 | 29195 | Death Knight Trainer | `GuildHouseTrainerCost` |
| 13 | 2836 | Blacksmithing Trainer | `GuildHouseProf` |
| 14 | 8128 | Mining Trainer | `GuildHouseProf` |
| 15 | 8736 | Engineering Trainer | `GuildHouseProf` |
| 16 | 18774 | Jewelcrafting Trainer, Alliance | `GuildHouseProf` |
| 17 | 18751 | Jewelcrafting Trainer, Horde | `GuildHouseProf` |
| 18 | 18773 | Enchanting Trainer, Alliance | `GuildHouseProf` |
| 19 | 18753 | Enchanting Trainer, Horde | `GuildHouseProf` |
| 20 | 30721 | Inscription Trainer, Alliance | `GuildHouseProf` |
| 21 | 30722 | Inscription Trainer, Horde | `GuildHouseProf` |
| 22 | 19187 | Leatherworking Trainer | `GuildHouseProf` |
| 23 | 19180 | Skinning Trainer | `GuildHouseProf` |
| 24 | 19052 | Alchemy Trainer | `GuildHouseProf` |
| 25 | 908 | Herbalism Trainer | `GuildHouseProf` |
| 26 | 2627 | Tailoring Trainer | `GuildHouseProf` |
| 27 | 19184 | First Aid Trainer | `GuildHouseProf` |
| 28 | 2834 | Fishing Trainer | `GuildHouseProf` |
| 29 | 19185 | Cooking Trainer | `GuildHouseProf` |
| 30 | 8719 | Alliance Auctioneer | `GuildHouseAuctioneer` |
| 31 | 9856 | Horde Auctioneer | `GuildHouseAuctioneer` |
| 32 | 184137 | Mailbox | `GuildHouseMailbox` |
| 33 | 1685 | Forge | `GuildHouseObject` |
| 34 | 4087 | Anvil | `GuildHouseObject` |
| 35 | 500000 | Portal: Stormwind | free starter, Alliance |
| 36 | 500001 | Portal: Darnassus | `GuildHousePortal` |
| 37 | 500002 | Portal: Exodar | `GuildHousePortal` |
| 38 | 500003 | Portal: Ironforge | `GuildHousePortal` |
| 39 | 500004 | Portal: Orgrimmar | free starter, Horde |
| 40 | 500005 | Portal: Silvermoon | `GuildHousePortal` |
| 41 | 500006 | Portal: Thunder Bluff | `GuildHousePortal` |
| 42 | 500007 | Portal: Undercity | `GuildHousePortal` |
| 43 | 500008 | Portal: Shattrath | `GuildHousePortal` |
| 44 | 500009 | Portal: Dalaran | `GuildHousePortal` |
| 45 | 187293 | Guild Vault | `GuildHouseObject` |
| 46 | 28692 | Trade Supplies | `GuildHouseVendor` |
| 47 | 28776 | Tabard Vendor | `GuildHouseVendor` |
| 48 | 19572 | Food & Drink Vendor | `GuildHouseVendor` |
| 49 | 6491 | Spirit Healer | `GuildHouseSpirit` |
| 50 | 191028 | Barber Chair | `GuildHouseObject` |
| 51 | 29636 | Reagent Vendor | `GuildHouseVendor` |
| 52 | 29493 | Ammo & Repair Vendor | `GuildHouseVendor` |
| 53 | 28690 | Stable Master | `GuildHouseVendor` |
| 54 | 9858 | Neutral Auctioneer | `GuildHouseAuctioneer` |
| 55 | 2622 | Poisons Vendor | `GuildHouseVendor` |

Prices as shipped:

| Key | Copper | Gold | Note |
| --- | --- | --- | --- |
| `CostGuildHouse` | 10 000 000 | 1 000 | the house itself; refund on sale is half |
| `GuildHouseInnKeeper` | 1 000 000 | 100 | |
| `GuildHouseBank` | 1 000 000 | 100 | |
| `GuildHouseMailbox` | 500 000 | 50 | |
| `GuildHouseAuctioneer` | 1 000 000 | 100 | butler code default is 500 000, so the two disagree |
| `GuildHouseTrainerCost` | 1 000 000 | 100 | per class, eleven classes |
| `GuildHouseProf` | 500 000 | 50 | per profession trainer |
| `GuildHouseVendor` | 500 000 | 50 | |
| `GuildHouseObject` | 500 000 | 50 | |
| `GuildHousePortal` | 1 000 000 | 100 | butler code default is 500 000 |
| `GuildHouseSpirit` | 100 000 | 10 | |

Buying everything once comes to roughly 6 200 gold per house, on a 1 000 gold entry price.

The list is a Wrath list, and that is the second thing that does not carry over. Ammo and
repair, poisons vendors, weapon master trainers, a portal to Shattrath and one to Dalaran:
three of those are meaningless on 8.3.7 (ammunition was removed, poisons are a rogue spell
line, Shattrath is a faction hub nobody visits), and the trainers stop at Master/Artisan
because none of the NPCs picked have BfA ranks - which is issue 42 on their own tracker. A
faithful 8.3.7 version needs Kul Tiran and Zandalari trainers, the BfA profession rank
vendors, and the two capital portal rooms, not this list. The *shape* transfers. The ids do
not.

## 7. The phase decision, and why it is the thing that broke

The whole module rests on one primitive:

```
uint32 GetGuildPhase(Player* player) { return player->GetGuildId() + 10; }
```

Eleven call sites use it. It is passed as the third argument to `Creature::Create`,
`Creature::SaveToDB`, `GameObject::Create` and `GameObject::SaveToDB`, and read back out of
the `phaseMask` column when the house is sold or the guild disbands.

That primitive is the WotLK bitmask system: a `uint32` where bit N means "visible in phase N",
stored per spawn in a `phaseMask` column, applied to a player with `SetPhaseMask(value, true)`.

None of that exists in my tree. BfA replaced it with a set of phase ids and phase groups read
from `Phase.db2` and `PhaseXPhaseGroup.db2`, applied through `PhasingHandler`, and the spawn
column became `PhaseId` (`creature`) plus `PhaseId`, `PhaseGroup` and `phaseUseFlags`
(`gameobject`). There is no `phaseMask` column left to write to and no `SetPhaseMask` to call.

Two consequences:

1. **`guildId + 10` is not a safe id range on 8.3.7.** Phase ids in the low range are real
   content: retail's own phases live there, and `PhasingHandler::AddPhase` looks the id up in
   `Phase.db2` to decide whether it is `Cosmetic` or `Personal`. A guild house sharing an id
   with a live quest phase would borrow that phase's flags. My module starts its two ranges at
   10 000 and 30 000 for exactly this reason, and doc 03 sec.1 records the check that unknown
   ids degrade to `PhaseFlags::None` rather than failing.
2. **the removal path silently stops working.** `RemoveGuildHouse` queries
   `SELECT guid FROM gameobject WHERE map = 1 AND phaseMask = ...`. On 8.3.7 that statement
   fails on an unknown column, so a ported version would delete the `guild_house` row and
   leave every spawn behind as an orphan.

## 8. Every AzerothCore API it calls, checked against my tree

This is the section that decides the fallback. Each row was grepped in my source, not
remembered.

| Called by the module | In BfA 8.3.7 build 35662 | What replaces it |
| --- | --- | --- |
| `SetPhaseMask` | **absent**, zero hits in `src/server/game` | `PhasingHandler::AddPhase` / `RemovePhase`, `PhaseShift` |
| `GetPhaseByAuras` | **absent** | `PhasingHandler::OnAreaChange`, `Player::GetPhaseShift` |
| `Creature::Create(guid, map, phaseMask, entry, ...)` | different signature: `(guidlow, map, entry, x, y, z, ang, CreatureData const*, vehId)` | build a `CreatureData` and pass it in |
| `Creature::SaveToDB(mapid, spawnMask, phaseMask)` | `SaveToDB(mapid, std::vector<Difficulty> const&)` | phase travels in the `PhaseId` column |
| `GameObject::SaveToDB(mapid, spawnMask, phaseMask)` | same change | same |
| `Map::GetSpawnMode()` | present | unchanged |
| `sMapMgr->FindMap`, `Map::GenerateLowGuid`, `sObjectMgr->AddCreatureToGrid`, `AddGameobjectToGrid` | present | unchanged |
| `sObjectMgr->GetCreatureData`, `GetGameObjectData`, `GetGameObjectTemplate`, `sGameObjectDisplayInfoStore` | present | unchanged |
| `GlobalScript` + `OnBeforeWorldObjectSetPhaseMask` | **absent**, no `GlobalScript` class at all | no direct equivalent; combine-phase behaviour is per-phase data now |
| `PlayerScript::OnPlayerUpdateZone` | present, signature is `(player, newZone, oldZone, newArea)` - one more argument | adapt |
| `PlayerScript::OnPlayerLogin` | present | unchanged |
| `PlayerScript::OnPlayerBeforeTeleport` | **absent** | hook the zone change or the teleport call site |
| `Player::CustomData.GetDefault<GuildData>("phase")` | **absent**, no `DataMap::Base` in the tree | a table on my side, keyed by player |
| `Player::SetRestState` | present | unchanged |
| `Player::RemoveRestState` | **absent** | rest state is recalculated on zone change |
| `CreatureScript` with `OnGossipHello` / `OnGossipSelect` | present | unchanged |
| `AddGossipItemFor(..., popupText, popupMoney, coded)` | present | see the money note below |
| `GuildScript::OnCreate` / `OnDisband` | present | unchanged |
| `WorldScript::OnStartup`, `OnBeforeConfigLoad` | present | unchanged |
| `sConfigMgr->GetOption<int32>(key, def)` | **absent**, this is the newer config API | `GetIntDefault(key, def)` / `GetBoolDefault` |
| `TOTAL_LOCALES`, `GetLocaleByName`, `GetSessionDbLocaleIndex` | present | unchanged |
| `Player::ModifyMoney`, `Guild::BroadcastToGuild`, `ChatHandler::PSendSysMessage` | present | unchanged |

Ten incompatibilities, two of them structural (the phase primitive and the per-player data
map), one of them a rewrite of the spawn path. That is the whole answer to "can this be
dropped in": no. It is a port, and the port would be most of the 1 455 lines of C++.

One of those rows deserves its own note, because it is a trap in both directions. In this
core, `AddGossipItemFor` with a money popup makes the client show a price and the **server**
check `HasEnoughMoney` before dispatching the option - but the core never deducts the money.
The script must. mod-guildhouse does deduct in `SpawnNPC` and `SpawnObject`, which is correct,
and my module's vendor code does the same. The trap is believing the confirmation charges
twice; it does not.

## 9. What is broken in it as written

Nine things I found reading the code, separate from the issue tracker. They matter because a
fallback inherits them.

| Where | What |
| --- | --- |
| butler, `case 6` (Auctioneer) | sets no price. `cost` is a **file-scope global** that is only assigned in the branches that use it, so the faction auctioneer charges whatever the previous purchase happened to leave behind, and zero on the first click. The popup still promises `GuildHouseAuctioneer` |
| butler, `OnGossipHello` | `sGuildMgr->GetGuildById()` and `GetMember()` are both dereferenced with no null check. A player in a guild whose `Guild*` lookup fails crashes the world thread |
| `RemoveGuildHouse` | deletes only creatures and game objects the running map can `GetCreature` / `GetGameObject` for. Inside an unloaded grid they are skipped, the database row survives, and the house comes back. This is issue 37 |
| `RemoveGuildHouse` | `DELETE`/`SELECT` on `phaseMask`, a column the target core does not have |
| seller, `case 3` (sell) | the comment says the refund covers "the cost of the house and the cost of each purchase made". The code refunds `CostGuildHouse / 2` and nothing else, so every facility bought is lost |
| seller, `case 4` | a duplicate of `case 2`, reachable only from the menu entry that was commented out. Dead branch |
| butler, `case 10` | *PVP toggle*, an empty branch. Menu item exists on some builds, does nothing |
| `gameobject` spawn | `Create` is called with a rotation quaternion that never sees `orientation`, so bought objects face north. This is issue 86 |
| prices | the config file and the code defaults disagree for `GuildHouseAuctioneer` and `GuildHousePortal` (100 g vs 50 g). Reads the config, so the file wins, but a missing key changes the price |

Not one of these is hard to fix. The point is the failure mode: seven of the nine are
"silently wrong", not "loudly broken", which is the class of bug my checkers exist for and the
reason I would not take a dependency I cannot regression-test.

## 10. Localization, and why it is the one thing I am not taking

This one is a genuinely good pattern and a genuinely wrong fit.

The mechanism: a world table `mod_guildhouse_locale` keyed `(Id, Locale)`, loaded once at
`WorldScript::OnStartup` into
`std::unordered_map<uint32, std::array<std::string, TOTAL_LOCALES>>`, read through
`GetGuildHouseLocaleText(id, player)` which indexes by the player's session locale and falls
back to `enUS`. Every string in the module goes through it. `guildhouse.h` declares 128 ids
named `GUILDHOUSE_TEXT_*`, numbered in blocks: 1-31 for guild and house messages, 100-104 for
the seller menu, 200-239 for the butler main menu, 300-414 for the facility and submenu
entries. Two SQL files seed them: 198 rows for the 22 ids the seller uses across 9 locales,
then 954 rows for the remaining 106 ids, so the count checks out at 128 ids times 9 locales
(1 152 rows) in `enUS`, `frFR`, `deDE`, `koKR`, `zhCN`, `zhTW`, `esES`, `esMX`, `ruRU`.

Why it is the right pattern: it keeps every visible string out of the source, and a missing
key shows up in one place instead of being scattered through gossip calls.

Why I am not taking it: my whole set is sanitized to ASCII and written in English by choice,
and my player-facing text does not only live in gossip - it lives in the addon, where a server
table cannot reach. A locale table would localize half my strings and leave the other half in
`Menu.lua`. If that changes, this is the design to copy, and the id-block numbering scheme is
worth copying with it.

## 11. What the fallback would actually be

Naming it plainly: **F0, the service hub.** One shared map, one plot per owner instead of 55,
no placement, no collection, no budget, no dye, no blueprint, no addon window. A player buys
the plot, gets a house at a fixed anchor, and buys facilities from a butler.

Here is exactly what F0 keeps from my plan and what it throws away.

| My plan | F0 | Cost of the cut |
| --- | --- | --- |
| P0 ownership, travel, persistence | **kept**, unchanged | none - this already works |
| P1 decor catalogue, 739 item ids, 618 mapped game objects | **dropped** | the entire collection and shop layer, 1 056 stock lines, the town centre |
| P2 house levels, budgets, rooms as shells | **dropped** | 5 tiers, 3 840 combinations of level and surface |
| P3 Endeavors | **dropped** | 30+ task definitions, 12 vendor sets, the monthly loop |
| P4 dyes and pet beds | **dropped** | 9 dye groups, 77 colours |
| P5 blueprints and import codes | **dropped** | `PCH1` codes, the CRC32 path, export/import |
| P6 art pass | **dropped** | WMO/ADT work |
| addon `HousingUI` | **dropped**, gossip only | 1 324 lines of Lua, 24 `C:` opcodes |
| the 55 facilities | **added** - this is the whole point of F0 | needs the amenity work in section 12 |

So F0 is not "my module, smaller". It is my P0 plus a facility shop, which is the piece I do
not have yet and mod-guildhouse does. That is the real finding of this read: the backup is not
a substitute for my plan, it is **a shop for facilities my plan already has slots for**, and
the two are additive rather than alternative.

## 12. Feature-by-feature: what I can take, what I already have

| mod-guildhouse | My module today | Verdict |
| --- | --- | --- |
| Buy a service with gold, gossip confirm, `HasEnoughMoney` then `ModifyMoney` | `psycho_house_vendor_stock` sells decor items only | **take**: a service purchase path next to the decor path |
| Fixed facility anchors in a world table | `psycho_house_amenity` exists with `Pos`, `Radius`, `Flags`, but is map-authored, not purchasable | **partial**, and close: the table is already the right shape |
| Facility kinds | `AmenityType` has 8 values: bulletin board, portal, contractor, flight point, gathering, scenery, vendor, endeavor | **gap**: no bank, auctioneer, neutral auctioneer, mailbox, innkeeper, stable master, class trainer, profession trainer, spirit healer, forge, anvil, barber chair, guild vault |
| Faction-swapped NPC per facility (18774/18751) | `faction_mask` on vendor and stock rows | already have |
| Per-guild phase, `guildId + 10` | `PlotPhase`/`InteriorPhase`, `10000 + slot * 64 + plot`, `30000 + ...` | already have, with a safer id range |
| Rest area while inside | not set anywhere | **take**: `SetRestState(0)` on entry is one call and one line of value |
| `GuildScript::OnDisband` cleanup | **nothing**: a guild neighbourhood keeps its `guildId` after the guild is gone | **take, and this is a real hole in my module** |
| `GuildScript::OnCreate` welcome message | nothing | optional |
| Locale table with `enUS` fallback | no localization at all, by choice | not taken, see section 10 |
| Sell back at half price, delete every spawn | `Relinquish` exists; guild failure path does not | **take**: same shape as my archive timer |
| `.guildhouse tele` / `.gh tele` | `/housing` command tree exists | already have |
| Config-driven prices, 13 keys | costs are constants in `HousingDefines.h` | **take**: my prices should be `GetIntDefault`, not `constexpr` |
| SQL in six files with content before locale | my ordering is already correct | already have |

Two of those rows are the reason this exercise was worth doing. The guild-disband hole is a
bug in my module that I would not have found from my own documents, and the missing facility
kinds are the difference between a house that looks like a house and a house that is a home.

## 13. The fallback plan, in order

If per-player housing has to be cut, this is the sequence. Each step is small, and the first
three are worth doing whether or not the fallback ever happens.

1. **Add the facility amenity kinds to `AmenityType`** - bank, auctioneer, neutral auctioneer,
   mailbox, innkeeper, stable master, class trainer, profession trainer, spirit healer, forge,
   anvil, barber chair, guild vault. Append-only, because the enum is not on the wire; only
   `AmenityType` values that reach the addon as numbers are.
2. **Add a purchase flag to `psycho_house_amenity`** - `price`, `currency`, `faction_mask`,
   `required_level`, `required_rank`. The table already carries id, biome, name, type, pos,
   radius and flags, so this is six columns, not a new table.
3. **Write the purchase path** - one gossip NPC per neighbourhood, one menu built from the
   amenity rows not yet bought, the same `HasEnoughMoney` then `ModifyMoney` order, spawn
   through the existing decor spawner instead of a new one.
4. **Bind the guild hooks** - `GuildScript::OnDisband` releases any neighbourhood whose
   `guildId` matches, archives each plot to `character_housing_archive`, and deletes the
   per-house rows. `OnCreate` sends one line of text.
5. **Add `SetRestState(0)`** on entry to an owned house and drop it on exit.
6. **Collapse the plot count if needed** - `kPhasesPerSlot` is 64 with 55 plots used. F0 can
   run with 1 plot per neighbourhood and the same code, which is the cheapest possible way to
   trade "a village" for "a house".
7. **Stop there.** No placement, no catalogue, no budgets, no addon.

What step 7 costs in player terms: no buying furniture, no placing furniture, no walls or
rooms, no Endeavors, no dyes, no blueprints, no collecting anything. What remains: a house, a
teleport, a chest of services, and a reason to belong to a guild or start a neighbourhood.

**The decision gate.** F0 becomes the shipping plan if, by the end of P1, decor placement is
still not stable - specifically if a placed piece cannot survive a server restart and a
phase change in a repeatable test. That is one concrete, testable condition, not a feeling,
and it is the same condition under which the whole P2-P6 stack stops being worth building.

## 14. What I would change if I were writing it today

Not a review for its own sake - these are the five decisions I would make differently, and I
have made three of them already.

1. **Phase ids from a reserved high range, not `guildId + 10`.** Mine: 10 000 and 30 000.
2. **No file-scope price global.** Pass the price as an argument to the spawn call. The
   auctioneer bug cannot exist if the price is a parameter.
3. **Remove by row id, not by phase scan.** `guild_house_spawns` gives the anchor; the spawn
   itself should be recorded per house so deletion is a key lookup, not a grid walk. That one
   change kills issues 37 and 68 at once.
4. **Prices in config, positions in a table, both per house.** A shared anchor table means
   every house is the same house.
5. **Null-check the guild lookup.** It is two lines.

## 15. What I could not check

- whether the module compiles or runs today: no `cmake`, no boost, no MySQL in this sandbox,
  and it targets a core I do not have;
- whether issue 85 (invisible NPCs in Alliance territory) is a phase problem, a faction
  problem or a client problem - the code does not say, and reproducing it needs a live server;
- whether `Phase.db2` in build 35662 has content in the 10-1000 id range that would collide
  with `guildId + 10`. The core reads the store, but the store is not in this repository, so
  the collision is a well-founded inference from how BfA phasing is built, not a measurement.
  My own ranges are chosen to avoid the question entirely;
- whether GM Island area 876 is still walkable and still outside every city's phase on 8.3.7.
  The coordinates and the zone id are the WotLK ones; the island has shipped since vanilla,
  but it is not something I can confirm from source alone;
- what the module's authors would allow regarding reuse, since it ships no license.

## 16. Verdict

The research answers the question the backup idea was really asking. There is nothing here
that makes my housing system unnecessary, because mod-guildhouse is not a housing system: it
is a phased services room with a shop. There is also nothing here that threatens the plan,
because the parts worth having are the parts I can add in four evenings of work without
touching anything I have already built.

So the fallback stands, with one correction. F0 is not "give up and install the other module".
It is "ship P0 plus the facility shop, and postpone P1 to P6". That is a smaller, honest
retreat that keeps my ids, my phase maths, my tables and my command tree, and it is strictly
better than a port of code that cannot compile against this core.

The two things I am doing now, regardless of which way the gate falls:

- add the facility amenity kinds and the purchase columns to `psycho_house_amenity` (13.1,
  13.2);
- add `GuildScript::OnDisband` cleanup, because a guild neighbourhood outliving its guild is a
  bug in my module today (13.4).

Those are findings 63 and 64 in the README audit pass.
