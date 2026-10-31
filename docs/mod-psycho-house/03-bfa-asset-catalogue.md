# 03 - BfA 8.3.7 asset catalogue

Everything already inside my source code and client that mod-psycho-house can use as
housing content, and the exact procedure that turns my own database into a decor
catalogue.

## 0. Read this first: why there are no invented ids in here

The single most damaging thing I could do in this document is print a plausible-looking
`gameobject_template.entry` that does not exist in build 35662. A wrong entry spawns
nothing, logs an error on every house load, and poisons the catalogue. So this document
works the other way round:

1. It lists the **systems and families** that definitely exist in 8.3.7 (I can verify
   those against the tree and against published BfA data).
2. It gives the **harvest tooling** that reads *my* world database - which is the
   authoritative list of what my source code ships - and emits a ready-to-run
   `housing_decor` file with real entries, real names, and my budget scoring.
3. It names only the ids I can stand behind: quest ids, currency ids, achievement ids,
   item ids that are part of the retail record quoted in doc 01, and repo paths I read
   myself.

Everything else is "run this and the ids come out".

## 1. Systems in the tree that do the work

| System | Where | What housing uses it for |
| --- | --- | --- |
| Phase shift | `src/server/game/Phasing/PhaseShift.{h,cpp}`, `PhasingHandler` | Neighborhood slots, plot isolation, interior isolation. `PhaseShift::PhaseRef::Id` is `uint16`; custom ids not in `Phase.db2` fall back to `PhaseFlags::None`, which is why 10000+/30000+ are safe |
| Maps | `Maps/Map.cpp`, `MapManager` | One shared housing map, one interior map |
| Game objects | `Entities/GameObject/GameObject.{h,cpp}` | Every piece of decor. `CreateGameObject(entry, map, pos, quat, animProgress, GOState)` + `SetObjectScale` + `AddToMap` |
| Gossip | `Scripts`, `GossipDef` | Cornerstone, Steward, Contractor, vendors |
| Vendors | `npc_vendor` + `Player::GetSession()->SendListInventory` | Decor sales (or the module's own stock, which the prototype does) |
| Currencies | `Player::ModifyCurrency`, `DB2Stores` | War Resources / Community Coupons for themed vendors |
| Battle pets | `BattlePets/*`, `Player::GetSession()->GetBattlePetMgr()` | Pet beds |
| Scenarios | `Scenarios/*` | Optional framing for the intro questline |
| World states | `Map::SetWorldState`, `SendUpdateWorldState` | Endeavor Favor bars and completion flags |
| Conditions | `Conditions/ConditionMgr` | Decor unlock requirements (achievement, quest, reputation) |
| Loot | `Loot/Loot`, `LootHandler` | Endeavor Coffer, decor drops |
| Weather | `Weather/Weather` | Ambient mood in a plot |
| Area triggers | `AreaTriggerScript` | Plot boundaries, portals, the Endeavor hub |
| Guild | `Guilds/Guild`, `GuildScript` | Guild neighborhoods and roster-driven membership |
| Garrison scaffolding | `Garrison/GarrisonMap.{h,cpp}`, `GarrisonPlot.{h,cpp}`, `character_garrison*` | **Reference design**, not reused: a private map per player with plots, which is exactly the shape of housing minus the social layer |

`GarrisonPlot.h` in this tree is an empty stub and `GarrisonMap` is a thin `Map`
subclass - worth reading before writing any new map code, because it shows the pattern
the core already expects.

## 2. Currencies and money that already exist

| Asset | Id | Status | Use |
| --- | --- | --- | --- |
| Gold | - | in game | Plot purchase, standard decor, dye crafting cost |
| War Resources | **1560** | BfA currency, exists in 8.3.7 | Themed/Endeavor vendor currency, exactly how retail reuses old currencies (doc 01 sec.10) |
| Community Coupons | **3363** | Retail housing currency | Cannot be used on 8.3.7 unless added via `hotfixes`; keep the *name* in the UI copy and pay with War Resources |
| Polished Pet Charm | 163036 | BfA pet currency (verify id) | Pet bed vendors - thematically perfect |
| 7th Legion Service Medal / Honorbound Service Medal | verify | BfA | Alternate Endeavor currency if War Resources feels wrong |

Rule I am following: **never invent a currency id**. If a currency does not exist in
`currency_types` on my realm, the vendor pays in gold.

## 3. Item anchors that definitely exist in 8.3.7

These are used for *system hooks*, not as decor:

| Item | Id | Use |
| --- | --- | --- |
| Hearthstone | 6948 | Reference for Homestone cooldown feel; the Homestone itself is a command, not an item |
| Garrison Hearthstone | 110560 | Same |
| Dalaran Hearthstone | 140192 | Same |
| The "Housing Teleport" spell pattern | - | If a Homestone item is ever wanted, a spell with `SPELL_EFFECT_TELEPORT_UNITS` and a map/coords is cheaper than an item |

Decor in mod-psycho-house is **not** an item (doc 05 sec.2 of the architecture notes). The
collection is a table; the shop is a vendor; the placement is a GO. That decision removes
any need to touch `item_sparse`/`hotfixes` for furniture.

## 4. Map options

| Option | Notes |
| --- | --- |
| Custom housing map (target) | New map id + `Map.db2`/`AreaTable.db2` rows via the hotfix store, ADTs, vmaps/mmaps, client patch. Doc 07 |
| Existing unused BfA/Kul Tiras map (anchor mode) | Cheap to start: pick a zone with large empty terrain, hand-place 55 anchors. Query `map` for candidates rather than trusting memory |
| Garrison map as interior testbed | Alliance/Horde garrison maps already have interiors and plots; good for testing shell placement before art exists |

To list candidate maps instead of guessing ids:

```sql
SELECT `ID`, `Directory`, `Instanceable`, `MapType`, `ParentMapID`
FROM `map`
WHERE `Instanceable` = 0
ORDER BY `ID` DESC
LIMIT 100;
```

## 5. Creature models for the neighborhood cast

The retail NPCs do not exist in 8.3.7, so each vendor is "an existing creature template
wearing an existing model". The cast I need and the look I want:

| Role | Retail NPC | Look to reuse in 8.3.7 |
| --- | --- | --- |
| Steward (Alliance) | Lyssabel Dawnpetal | Human or night-elf civilian |
| Steward (Horde) | Tocho Cloudhide | Tauren civilian |
| General Contractor | Jorvan Longmoor (255104) | Human worker with hammer |
| Dye Crafter | Helmi Cooper (255126) / Haleth Turnwater (255125) | Alchemist / scribe civilian |
| Pet Decor | Perry Winkles (267795) / Agratha (267794) | Gnome / orc civilian |
| Last Architect | The Last Architect (248854 / 253596) | Ethereal (exists in BfA) |
| Elven decor | Argan Hammerfist (255218), Balen Starfinder (255216), Fel'orei Moonstrider | Night elf / blood elf civilians |
| Local + Stormwind | Faarden the Builder (255213), Xiao Dan | Human worker / pandaren |
| Plants | Trevor Grenner | Human, druid-ish |
| Woodwork | Devin Slatesmith (257332) | Dwarf woodworker |
| Cobwebs | Klasa | Undead or old woman in a cellar |
| Holiday | Holly Twinklebright | Gnome with festive tint |

Query for candidates:

```sql
SELECT `entry`, `name`, `subname`, `minlevel`, `faction`
FROM `creature_template`
WHERE `npcflag` & 1 = 1          -- gossip
  AND `minlevel` BETWEEN 1 AND 60
ORDER BY `name`
LIMIT 500;
```

## 6. The decor harvest - how I get "every item already in my source"

Two deliverables:

- `tools/housing/harvest_decor.py` - read-only MySQL client, scans
  `gameobject_template`, matches names against a keyword table derived from retail's seven
  categories (doc 01 sec.5), scores each hit, assigns budget cost by rule, dedupes by name
  family, and writes `sql/generated/housing_decor_<date>.sql`.
- `sql/tools/psycho_house_harvest.sql` - the same thing in SQL for people who prefer not
  to run Python: per-family candidate queries plus a summary report.

### 6.1 Category mapping (retail -> harvest keywords)

| Retail category | Keywords I match on `gameobject_template.name` |
| --- | --- |
| Furnishings > Seating | chair, stool, bench, seat, throne, sofa, couch |
| Furnishings > Tables and Desks | table, desk, counter, workbench, anvil (as a table-scale prop) |
| Furnishings > Storage | chest, crate, barrel, box, cabinet, bookshelf, bookcase, shelf, coffer |
| Furnishings > Beds | bed, cot, hammock, bunk, mattress |
| Accents > Ornamental | vase, urn, bowl, candle, goblet, trophy, banner, bannerpole, painting, map, statue |
| Accents > Wall Hangings | banner, rug, tapestry, curtains, drape, painting |
| Accents > Food and Drink | food, fruit, meat, bread, keg, bottle, mug, kettle, cauldron, feast |
| Accents > Floor | rug, carpet, mat, ground, plate, rune |
| Structural > Doors | door, gate, portcullis, archway |
| Structural > Walls and Columns | wall, pillar, column, fence, barricade, palisade |
| Structural > Windows | window, shutter, lattice, trellis |
| Structural > Large Structures | tower, gazebo, tent, pavilion, windmill, bridge, dock, well, fountain |
| Lighting | brazier, lantern, lamp, torch, campfire, candle, chandelier, light, beacon |
| Nature > Large Foliage | tree, oak, pine, willow, palm, ancient |
| Nature > Small Foliage | bush, shrub, fern, plant, flower, herb, vine, ivy, grass, moss |
| Nature > Ground Cover | leaves, flowers, grass, moss, mushrooms, snow, dirt |
| Functional > Utility | forge, anvil, oven, cauldron, chair (sittable), bed (rest), mailbox |
| Miscellaneous | everything that scored but fits nowhere else |

### 6.2 Budget cost rule (1-5)

| Rule | Cost |
| --- | --- |
| Very small props (candles, mugs, bowls, flowers) | 1 |
| Seating, small plants, wall hangings, small lights | 1-2 |
| Tables, beds, storage, medium plants, doors | 3 |
| Walls, columns, windows, large lights, large foliage | 4 |
| Large structures: towers, gazebos, tents, wells, fountains, trees | 5 |

The tool takes an override file so I can hand-correct the pieces I actually use. Retail's
real costs are 1-5 with the same shape, which is why the rule produces a usable economy
without hand-authoring thousands of rows.

### 6.3 Deduplication

`gameobject_template` in 8.3.7 contains the same chair a dozen times with suffixes
(`Wooden Chair`, `Wooden Chair 01`, `Chair, Wooden`). The tool normalises: lowercase,
strip digits and punctuation, collapse spaces, then keeps one row per normalised name  -
preferring the entry whose `type` is `GAMEOBJECT_TYPE_SPELL_FOCUS`-free and whose
`displayId` is non-zero. The chosen entry is recorded in the generated SQL comment so I can
overrule it.

### 6.4 What the generated file looks like

```sql
-- generated 2026-10-31 by tools/housing/harvest_decor.py
-- source: world.gameobject_template, 14 812 rows scanned, 638 candidates, 412 kept
INSERT INTO `housing_decor`
(`id`, `name`, `gameobject_entry`, `budget_cost`, `surface`, `placement`, `category`, `subcategory`, ...)
VALUES
(100001, 'Wooden Chair',        175xxx, 1, 0, 2, 'Furnishings', 'Seating', ...),   -- from 'Wooden Chair'
(100002, 'Small Wooden Table',  175xxx, 3, 0, 2, 'Furnishings', 'Tables and Desks', ...),
```

Ids start at **100000** so the hand-authored retail-parity rows (Pepe 25546 etc.) can keep
low numbers.

## 7. Items, quests and achievements that already exist and are worth wiring

### 7.1 Quests

The module ships its own intro chain (doc 04 sec.1) with ids in a free range. For Endeavor
tasks I do **not** create quests - I hook the completion of existing quests:

- Hook: `PlayerScript::OnQuestComplete` (or `QuestScript`) already gives me
  `(player, quest)`. Endeavor tasks of type `Quest` just match a list of quest ids.
- Selection: pick quests the population actually does, by zone, so a task never asks for a
  quest nobody can reach:

```sql
SELECT `ID`, `LogTitle`, `QuestLevel`, `QuestType`
FROM `quest_template`
WHERE `QuestLevel` BETWEEN 10 AND 60
  AND `QuestType` IN (0, 1)
ORDER BY `QuestLevel`
LIMIT 200;
```

### 7.2 Achievements

- Retail's housing meta is *Fully Furnished* (**61318**), which does **not** exist in
  8.3.7 - it is a Midnight achievement. My equivalent is a new achievement in a free id
  range, or (cheaper, no DBC work) a "collection milestones" gossip list on the Steward.
- BfA achievements that already exist are excellent Endeavor task targets, e.g. zone
  questing metas, dungeon metas, pet battle achievements. Query:

```sql
SELECT `ID`, `Title`, `Category`, `Points`
FROM `achievement`
WHERE `Category` IN (/* BfA zone + dungeon + pet categories */)
ORDER BY `ID`;
```

### 7.3 Items worth surfacing in the UI

| Use | Existing asset |
| --- | --- |
| Homestone icon | `ui_homestone` style icon, or reuse the Hearthstone icon (item 6948) |
| Dye icons | Spell/item icons for Alchemy and Inscription flasks and inks |
| Endeavor currency icon | War Resources currency icon (1560) |
| Plot pin icons | Homestone-style: mine / occupied / available / transport, as the retail map key uses (doc 01 of the Founder's Point notes) |

## 8. What the source tree itself contains

I ran an index over `src/` - every enum-style `NAME = <id>` constant, classified by kind
and then by furniture keyword. Results (regenerate with
`python3 tools/housing/index_source_assets.py --root .`):

| Kind | Named id constants in the tree |
| --- | --- |
| spell | 20 234 |
| creature | 4 912 |
| gameobject | 1 570 |
| quest | 1 018 |
| item | 460 |
| achievement | 61 |
| currency | 59 |
| **total** | **28 314** (24 606 distinct ids) |

Of those, **988** are shaped like decor after filtering out enums, triggers, invisibles and
non-placeable kinds: **767 game objects** and **221 creatures**.

| Category | Game-object candidates |
| --- | --- |
| Structural > Windows | 316 |
| Furnishings > Storage | 157 |
| Structural > Large Structures | 111 |
| Accents > Ornamental | 53 |
| Structural > Walls and Columns | 51 |
| Furnishings > Seating | 22 |
| Lighting > Small Lights | 15 |
| (remaining families) | 42 |

Two files carry the full list:

- **[08-source-asset-appendix.md](08-source-asset-appendix.md)** - readable, per family,
  top 15 with the exact file and line each id came from, plus 60 creatures and their
  suggested role (ambient life, prop creature, service NPC).
- **`generated/source-asset-index.csv`** - every row, machine readable, for import into a
  spreadsheet or straight into a migration.

The caveat that keeps this honest: **a constant name proves the id is used by code, not that
the template row exists in the world database.** The decor pool is the *intersection* of
this list and what `gameobject_template` actually ships, which is exactly what
`harvest_decor.py` computes. In practice the union of the two lists - DB templates (broad,
5 000-15 000 candidates) plus source constants (narrow, but visually verified by whoever
wrote the script) - is the right shop stock, and the source list is the one I would trust
first for anything irreplaceable.

## 9. Repo paths mod-psycho-house touches

| Path | Change |
| --- | --- |
| `src/server/game/PsychoHouse/` | New module (renamed from `Housing`), see doc 05 sec.3 |
| `src/server/scripts/PsychoHouse/` | Script module: world, player, gossip NPCs, commands, Endeavors |
| `src/server/game/Handlers/ChatHandler.cpp` | The one hook: route `PCHO` addon messages |
| `sql/updates/world/*psycho_house*` | World tables + content |
| `sql/updates/characters/*psycho_house*` | Character state |
| `src/server/worldserver/worldserver.conf.dist` | `PsychoHouse.*` config block |
| `addon/PsychoHouse/` | Client addon |
| `tools/housing/` | Harvest + build helpers |

Nothing else in the core is edited. That is a hard rule I keep from the prototype, because
it is what makes the module portable between BfA repacks - including to whatever build the
rest of my server ends up on.
