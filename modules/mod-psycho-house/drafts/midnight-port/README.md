# Draft A - mod-psycho-house, ported from Midnight

This is the version that reproduces retail housing content on 8.3.7. It is a draft: the
C++ core is the prototype I already had, the content is everything I pulled out of the
retail vendor pages, and the numbers are the retail numbers. I would rather ship retail
numbers on a stand-in model than my own numbers on perfect art - the numbers are what the
system is actually about.

## What is in it

| Piece | Count | Where |
| --- | --- | --- |
| Vendors | 39, each with its retail name, npc id, faction and captured stock count in a row comment | `sql/world/03_vendors_retail.sql` |
| Decor rows | 571, every one with a real 8.3.7 game object, on 177 distinct objects | `sql/world/04_decor_retail.sql` |
| Vendor stock rows | 972 | `sql/world/05_stock_retail.sql` |
| Retail decor item ids carried on those rows | 450 rows carry one; between them they cover all 439 ids the capture has | `04_decor_retail.sql`, `item_entry` column |
| Faction split | 240 Horde only, 99 Alliance only, and 232 are stocked by both neighbourhoods at the same price | `faction_mask` column |
| Currency | gold, Community Coupons 3363, and Tricky Treat item 33226 for the Hallow's End stall | `04_decor_retail.sql`, `currency` column |
| House levels | 12, retail's ladder: XP and the three decor budgets per level | `sql/world/07_progression.sql` |
| Exteriors | 12 shells, four kits in three sizes, gated 1 / 8 / 12 | same file |
| Rooms | 17, each with its budget cost, level requirement and floorplan footprint | same file |
| Portals | 10 destinations, bound to the realm's own `game_tele` rows by name | same file |
| Amenities | 23: 8 in the town centre and 15 across the six biomes | `sql/world/02_world_content.sql` |
| Endeavors | 4, 24 tasks across them, 16 reward rows; 18 stock lines ship gated behind their milestones | `07_progression.sql` |
| Dyes | 6 colour rows, one per dye group | same file |
| Pet beds | 4, `housing_decor_creature` kind 0 with no bound creature - the companion is the player's | same file |
| Quest rows | 4, ids 900100-900103, money in the row and everything else module-side | `sql/world/06_quests.sql` |
| Riddle hunts | 10 hunts, quests 900110-900119, ten reward pieces ids 1900-1909, ten caches | `sql/world/08_hunts.sql` |
| Client menu | Fifteen-tab housing window, `/house` or the House button by the minimap | `addon/HousingUI/Menu.lua` |

Every vendor row keeps the retail npc id in the table comment, so the mapping between a
Midnight npc and whichever 8.3.7 creature I end up using for the model is never lost.

## How the retail items got an 8.3.7 game object

A retail decor item id is not a game object entry. Midnight keeps decor in its own DB2
table, 8.3.7 has no such table, and no 8.3.7 client contains the Midnight model. So every
row carries two ids: `item_entry` is the retail item id, and `gameobject_entry` is the
8.3.7 game object I chose to stand in for it.

All 571 rows already carry one, so the file imports cleanly and every row is placeable the
moment it loads. The 571 pieces land on 177 distinct game objects, which is the honest part
of the substitution: a lamp, a lantern and a chandelier often share the nearest thing 8.3.7
has. The 187 in `data/town-center-go-map.csv` is the same question asked of all 618 mapped
names, including the ones whose price never resolved and that therefore ship no row.
`tools/housing/map_decor_go.py` produced those choices and records its reason in the
`match` column of `data/town-center-go-map.csv`:

| Reason | Names | What it means |
| --- | --- | --- |
| category | 223 | no name match, so the piece took the default stand-in for its category |
| keyword | 189 | the decor name and the game object name share a distinctive word |
| family | 171 | the names share a family stem, such as `WALL` (18) or `CRATE` (15) |
| fallback | 35 | nothing matched, not even a family stem; last resort default |

`category` and `fallback` are the two buckets I would revisit first, and both are cheap to
fix: change `gameobject_entry` on that row, or give the map tool another keyword. A row
whose entry is not in `gameobject_template` fails cleanly at placement instead of spawning
nothing, which is why I would rather ship a rough stand-in than a zero.

The retail item id is still there in `item_entry`, so when a piece is later given a proper
8.3.7 model nothing else in the row has to change.

## The systems behind the data

The content above is only worth what the core does with it, so this is the list of what the
draft actually implements rather than what it stores.

| System | What happens |
| --- | --- |
| House levels | 1-12 on retail's XP curve. XP comes from a piece learned for the first time, from Endeavor tasks and from Endeavor milestones; the level raises the three decor budgets and the room budget. `Housing.Level.GoldSkip` lets a player pay to skip a level instead |
| Decor storage | A purchase is a copy in storage, not a permanent unlock: placing consumes the copy and removing returns it. Four chairs are four purchases, which is retail's rule and the reason a catalogue entry is not the same thing as an owned piece |
| Endeavors | Per neighbourhood, on a clock. Every player in the neighbourhood contributes to the same 24 tasks, the milestone (1-4) is the fraction of tasks completed, and a stock row whose `milestone` the neighbourhood has not reached is not on the shelf - the vendor window, the price it quotes and `BuyDecor` all ask the stall's own row |
| Blueprints | `/housing export` turns the house into `PHP1:` plus a base62 payload (kind, faction, piece count, then a varint id and five packed bytes per piece) with a checksum; `/housing import` validates the checksum before it touches anything, and refuses a code from the other faction |
| Dyes | A dye is a game object swap, stored on the placement as `dye_group`, `dye_color` and `variant_entry`. `sql/bind/bind_decor_variants.sql` fills the variant table from colour pairs in `gameobject_template`; a piece with no variant row is not dyable and says so |
| Pet beds | A bed is a `housing_decor_creature` row of kind 0 with no creature of its own: the player assigns the companion they have out, the species is checked against the journal, and the creature spawns and despawns with the placement |
| Town centre | 23 amenities and 39 vendor stalls spawn in the neighbourhood's own phase (`60000 + slot`), so two neighbourhoods on the same map share the coordinates and see none of each other's objects. Amenities stand where their row says; a row with no position takes the biome anchor, not the hub ring, so the Brumewood graveyard is in Brumewood |
| House Finder | `/housing finder` lists public neighbourhoods with their resident count, `/housing visit` and `/housing reserve` move to one and hold a plot while the player buys it |
| Regret window | A sold house is packed into `character_housing_archive` for seven days and can be restored for 10 % of its price, with its decor, its level and its XP |
| Inactivity | Off by default. `Housing.Evict.Days` plus `Housing.Evict.GraceDays` gives a house back to the market after a letter and a grace period, archiving the contents first |
| Guests | Three bits, all enforced: enter, edit, manage. A guest with edit can place, move and remove; a guest with manage can add and remove other guests; a guest with neither can only walk around |

## Prices

Prices are gold unless `currency` says otherwise:

- `currency = 0` - gold.
- `currency = 3363` - Community Coupons. 3363 is a real currency id in 8.3.7, so this
  works without a client patch.
- `currency = 33226` - the cost is paid in Tricky Treat. 33226 is an item, not a
  currency. The rule the core applies: gold at 0, Community Coupons at 3363, and any
  other non-zero value is an item id paid through `HasItemCount` / `DestroyItemCount`.
  The rule is `Housing::IsItemCost` in `src/game/Housing/HousingDefines.h`, and
  `HousingMgr::BuyDecor` is the only caller.

## Horde and Alliance

The two neighbourhoods are faction hubs, not shared ground: Razorwind Shores is Horde and
Founder's Point is Alliance, and each vendor row's comment says which side it stands on.

`faction_mask` carries that into the catalogue. 240 of the 571 pieces are Horde only, 99 are
Alliance only, and 232 are stocked by both neighbourhoods at the same price and carry both
bits. The mask is not decoration: `HousingMgr::BuyDecor` refuses a row whose mask does not
carry the buyer's bit, and the shop listing filters on the same field, so a player never
sees the other side's stock. Every stock line sits on a row its own vendor's faction can
buy, which is a rule `tools/housing/verify_drafts.py` checks in both directions.

That matters most where both hubs sell the same piece at different prices. Ten names are
sold twice, once per side, and the two prices are not close: Autumn Leaf Pile is 50 gold in
Razorwind Shores and 150 in Founder's Point, and the same holds for Charming Laurel Tree
(75/150), Creeping Corner Ivy (50/150), Creeping Lattice Ivy (50/150), Duskberry Bush
(25/150), Founder's Point Blooming Grass Patch (10/150), Gift of Gilneas (25/75), Gloomrose
(25/75), Granite Cobblestone Long Path (75/50) and Granite Cobblestone Path (50/75). Without
the mask the cheap Horde copy would be the best price in the Alliance shop, and the Alliance
vendor would never be used.

Those two Granite paths are also the case where a third vendor sells the same piece at the
same price as one of the two: Trevor Grenner stocks both at 50 and 75 in Founder's Point,
so those two rows carry both bits and there is one row per price. A Horde player still sees
each path twice, at two prices, and buys the cheaper one.

## What is not complete

- 234 of the 1 056 captured stock lines have no retail item id, and 15 vendors have none at
  all. The reason is the same for all of them: a Wowhead vendor page prints 50 rows and no
  more, and the vendors under 50 are the ones that resolved. The rows are still in the file
  with their names and prices, so an id can be filled in later without touching anything
  else.
- 87 captured lines carry no price on their vendor page. Three of them are priced on their
  own item page - Elegant Elven Bathtub, Empty Elegant Elven Bathtub and Elegant Elven Water
  Well, all 125 gold - and ship at that price. The other 84 are listed as comments at the
  end of `05_stock_retail.sql` and are not inserted, so they cannot come out free. Sixteen
  of them are Burlee Jinglepocket's whole stock and sixteen are Roz Copperpinch's, so those
  two stalls import empty and stay empty until a price is filled in. Nine of the other
  stalls lose between one and seventeen lines; the rest ship everything their page printed.
- Retail gates 59 lines behind the milestones; 18 of them ship gated. The other 41 stay ungated, listed as a comment
  at the end of the stock file with the count per vendor. Eighteen lines - Aeeshna's ten and
  Hordranin's eight - state which milestone they need, so those are shipped gated; the
  vendor pages for the other four print one count for the whole set and I will not invent
  which of Pascal's seventeen are his.
- Hesta Forlath's 14 gated lines are gated on the Deed of Patronage, which is a different
  gate with no id in the 8.3.7 record. They sell ungated and the file says so where the
  count is written down.
- `creature_entry` is 0 in the SQL on all 39 vendor rows. The retail npc id is in each
  row's comment, but a retail npc id is not an 8.3.7 creature entry. Run
  `sql/bind/bind_town_center_vendors.sql` after the import and it fills the column from
  `creature_template` by subname, one model per stall; it reports any stall it could not
  fill. Until that runs, the vendors are data the addon lists, and nothing stands in the
  town centre.
- The same applies to everything else the realm owns: the 23 amenity objects and NPCs, the
  12 shell exteriors, the 10 portal destinations, the 24 Endeavor targets and the dye
  variants all ship with a name in the row and a 0 in the id.
  `sql/bind/bind_housing_content.sql` fills them from `gameobject_template`,
  `creature_template`, `item_template`, `quest_template` and `game_tele`, then prints a count
  of what stayed at 0. A name the realm does not have stays unbound rather than being
  guessed, and an unbound Endeavor task never completes, which is the safe failure.
- `housing_decor_variant` ships empty. A dye needs two game objects for the same piece and
  8.3.7 has few; `sql/bind/bind_decor_variants.sql` finds the ones it has (a piece whose
  name starts with a colour word and whose colourless name matches another catalogue entry)
  and reports 0 when there are none. The dye path itself is finished - `/housing dye` reports
  "no dyed appearance" for a piece with no variant row, which is honest rather than silent.
- Plot spawns are authored for a map whose origin is the town centre. `Housing.Map.Mode =
  "anchor"` ignores them and lays the 55 plots out on concentric rings around
  `Housing.TownCenter`, which is how the draft runs on a borrowed map before the custom one
  exists. In native mode a row whose `spawn_z` is NULL takes the ground height from the map
  on first entry, so a plot row can be authored in X and Y before the terrain is final.
- The four story quests ship as `quest_template` rows with no `quest_objectives` rows, on
  purpose: their objectives are module events (walk a plot, claim a Cornerstone, place
  three pieces, reach level 2), so the player script completes them. The ten riddle hunts do
  ship an objective each, because their quest is a kill or a loot the core can count.
- Retail exterior shells (house skins) have no 8.3.7 equivalent. The draft uses the shell
  rooms from doc 06.

- What answers when clicked. Ten scripts are selected by `ScriptName`, never by entry, so
  no creature or object is created and none is overwritten - every binding is an `UPDATE ...
  WHERE ScriptName = ''`:
  `npc_housing_steward`, `npc_housing_contractor`, `npc_housing_coffer`,
  `npc_housing_vendor` and `npc_housing_dyecrafter` in `HousingNpc.cpp`;
  `go_housing_cornerstone`, `go_housing_coffer`, `go_housing_bulletin`, `go_housing_portal`
  and `go_housing_huntcache` in `HousingGo.cpp`.
  `02_world_content.sql` binds the two contractors, the Last Architect as the coffer, the
  stewards and dye crafters where a realm has them, and every `housing_vendor` row into the
  vendor script in one join. A creature the realm does not have stays unbound and the
  command does the same job, which is why nothing here depends on the retail ids.

## Install

`../../tools/housing/install.py --draft midnight-port` copies the four trees to the right
places on a source checkout, and `--build` runs CMake afterwards. The same script works
whether the tree globs a root `modules/` directory or the older `src/server/scripts/`
layout.

SQL order:

```
sql/characters/01_characters_schema.sql
sql/world/01_schema.sql
sql/world/02_world_content.sql
sql/world/03_vendors_retail.sql
sql/world/04_decor_retail.sql
sql/world/05_stock_retail.sql
sql/world/06_quests.sql
sql/world/07_progression.sql
sql/world/08_hunts.sql               (the ten riddle hunts)
sql/bind/bind_town_center_vendors.sql    (binds the 39 stall models)
sql/bind/bind_housing_content.sql        (binds amenities, shells, beds, portals, Endeavor targets)
sql/bind/bind_decor_variants.sql         (optional, fills the dye variant table)
```

Then `Housing.Enable = 1`, `.house`, and the addon: copy `addon/HousingUI` into the client's
`Interface/AddOns/PsychoHouse` (the installer stages it in the source tree and prints the
path). `/house` opens the housing window; the House button sits next to the minimap.
