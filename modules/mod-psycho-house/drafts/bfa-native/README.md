# Draft B - mod-psycho-house, built only from 8.3.7 build 35662

This is the version that needs nothing from retail. No Midnight item ids, no Midnight npc
ids, no imported art. Every id in the SQL already exists in my source tree or in the
8.3.7 world database, which is the point: it runs on a clean install the day it is
compiled. I built this one second and it took far less time than the port, which is the
argument for starting from what 8.3.7 already has rather than from what Midnight invented.

## What is in it

| Piece | Count | Where |
| --- | --- | --- |
| Decor rows | 350 | `sql/world/03_decor_native.sql` |
| Vendors | 8 of my own | `sql/world/04_vendors_native.sql` |
| Vendor stock rows | 350, every native decor piece sold somewhere | same file |
| Currency | gold only | `currency = 0` on every row |
| Quest rows | 4, the same ids and beats as the port | `sql/world/06_quests.sql` |
| Riddle hunts | 10 hunts, quests 900110-900119, ten reward pieces ids 2900-2909, ten caches | `sql/world/08_hunts.sql` |
| Client menu | The same fifteen-tab window - both drafts ship one core | `addon/HousingUI/Menu.lua` |
| Substitution for missing art | none needed | the pieces are real game objects |
| House levels | 12, the same ladder the port uses, from the same generator | `sql/world/07_progression.sql` |
| Exteriors, rooms, portals | 6 shells, 18 rooms, 10 portal destinations | same file |
| Endeavors | 4, 24 tasks, 16 reward rows | same file |

## Where the 350 come from

`tools/housing/index_source_assets.py` walks the source tree and pulls out every named id
constant. 767 of those constants are game objects that look like furniture. This draft
takes those 767, drops the ones whose constant name marks them as doors, gates, traps,
spawn markers, spell visuals and instance props, and keeps 350 that can be dropped in a
yard without looking wrong. The list is in `../../generated/source-asset-index.csv`; the
filter is the `reject` tuple in `tools/housing/build_drafts.py`, so changing my mind about
what counts as decor is a one-line edit and a re-run.

Names in the SQL are the constant names with the `GO_` prefix stripped and the words
title-cased, because the constant is all the source gives me. The last statement in
`03_decor_native.sql` replaces them with the real name the moment the world database is
present:

```sql
UPDATE `housing_decor` hd JOIN `gameobject_template` gt
  ON gt.`entry` = hd.`gameobject_entry`
SET hd.`name` = gt.`name`
WHERE hd.`id` BETWEEN 2000 AND 2999 AND gt.`name` <> '';
```

So the catalogue shows real game object names without me typing any of them.

## Vendors

Eight stalls of my own design: Housing Steward, Groundskeeper, Stonewright, Lamplighter,
Stores Overseer, Pet Handler, Tradesman, Festival Steward. Their `creature_entry` is 0
until they are bound to a creature template that exists, which is what
`sql/bind/bind_native_vendors.sql` does - it picks existing 8.3.7 creature entries by
name and writes them into `housing_vendor.creature_entry`. Nothing is invented: a name
that is not in `creature_template` simply matches zero rows.

Decor is split across the eight stalls by `gameobject_entry % 8`, so each stall carries a
mixed bag rather than a category. That is on purpose: with 350 pieces a category split
would leave one stall with four items.

## Economy

Gold only, and the ladder is mine:

| Price | Pieces |
| --- | --- |
| 10g | lighting, storage, books, accents |
| 25g | seating, rugs, wall art, nature, holiday |
| 50g | structural, beds, pets |
| 75g | tables |

Budget cost follows retail bands: 1 for anything at or below 25g, 3 up to 75g, 5 above.
That keeps the retail placement budgets in doc 01 usable without translating prices.

## What it deliberately does not have

- No Community Coupons in the shops. 3363 is a real 8.3.7 currency id and the Endeavor
  rewards pay it, but no stall in this draft prices anything in it: gold is the whole
  economy here, which is the point of the native draft.
- No milestone gating on the stalls. Every native piece is buyable from the first visit;
  `housing_vendor_stock.milestone` is 0 on every row. The gating code is the same one the
  port uses, so turning it on is a data edit, not a rebuild.
- No retail numbers. The 12-level ladder and its XP curve come from the shared generator,
  but the prices and budgets are calculated from this draft's gold prices rather than
  copied from retail - the ladder is the shape, the numbers are mine.
- No retail item ids anywhere, and therefore no `item_entry` column values to port later.
  A piece that gets a retail equivalent later is a row edit in `03_decor_native.sql`.

- What answers when clicked. Eight scripts are selected by `ScriptName`, never by entry, so
  no creature or object is created and none is overwritten - every binding is an `UPDATE ...
  WHERE ScriptName = ''`:
  `npc_housing_steward`, `npc_housing_contractor`, `npc_housing_coffer`,
  `npc_housing_vendor` in `HousingNpc.cpp`; `go_housing_cornerstone`,
  `go_housing_coffer`, `go_housing_bulletin`, `go_housing_portal`, `go_housing_huntcache` in
  `HousingGo.cpp`.
  `02_world_content.sql` binds the two contractors, the Last Architect as the coffer, the
  stewards where a realm has them, and every `housing_vendor` row into the vendor script in
  one join. A creature the realm does not have stays unbound and the command does the same
  job, which is why nothing here depends on the retail ids.

## Install

```
sql/characters/01_characters_schema.sql
sql/world/01_schema.sql
sql/world/02_world_content.sql
sql/world/03_decor_native.sql
sql/world/04_vendors_native.sql
sql/world/06_quests.sql
sql/world/07_progression.sql
sql/world/08_hunts.sql               (the ten riddle hunts)
sql/bind/bind_native_vendors.sql        (binds the 8 stall models)
sql/bind/bind_housing_content.sql       (binds amenities, shells, pet beds, portals, Endeavor targets)
sql/bind/bind_decor_variants.sql        (optional, fills the dye variant table)
```

`../../tools/housing/install.py --draft bfa-native` puts the four trees where the build
expects them, and stages the addon - copy it into the client's `Interface/AddOns/PsychoHouse`.
`/house` opens the housing window; the House button sits next to the minimap. `Housing.Map.Mode = "anchor"` is the default here because a clean 8.3.7
install has no housing map; point `Housing.Map.Id` at any map with room for the plot
anchors and author them in the world DB.
