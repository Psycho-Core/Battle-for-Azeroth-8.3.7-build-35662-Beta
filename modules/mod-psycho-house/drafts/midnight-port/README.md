# Draft A - mod-psycho-house, ported from Midnight

This is the version that reproduces retail housing content on 8.3.7. It is a draft: the
C++ core is the prototype I already had, the content is everything I pulled out of the
retail vendor pages, and the numbers are the retail numbers. I would rather ship retail
numbers on a stand-in model than my own numbers on perfect art - the numbers are what the
system is actually about.

## What is in it

| Piece | Count | Where |
| --- | --- | --- |
| Vendors | 39, all with name, npc id, faction and stock count | `sql/world/03_vendors_retail.sql` |
| Decor rows | 571, every one with a real 8.3.7 game object | `sql/world/04_decor_retail.sql` |
| Vendor stock rows | 969 | `sql/world/05_stock_retail.sql` |
| Retail decor item ids carried on those rows | 450 | same file, `item_entry` column |
| House levels | the core's five rungs (60/90/120/160/200 indoor, 25/45/65/85/110 outdoor). The retail 12-level ladder is reference only until phase 1 | `../../01-retail-housing-reference.md` and `HousingDefines.h` |
| Endeavor milestones | 4, with 59 stock rows on six vendors locked behind them. Not enforced yet - phase 3 | `../../04-content-spec-quests-endeavors.md` |
| Currency | Community Coupons 3363, plus Tricky Treat item 33226 for the Hallow's End stall | `04_decor_retail.sql`, `currency` column |
| Quest rows | 4, ids 900100-900103, money in the row and everything else module-side | `sql/world/06_quests.sql` |
| Client menu | Seven-tab housing window, `/house` or the House button by the minimap | `addon/HousingUI/Menu.lua` |

Every vendor row keeps the retail npc id in the table comment, so the mapping between a
Midnight npc and whichever 8.3.7 creature I end up using for the model is never lost.

## How the retail items got an 8.3.7 game object

A retail decor item id is not a game object entry. Midnight keeps decor in its own DB2
table, 8.3.7 has no such table, and no 8.3.7 client contains the Midnight model. So every
row carries two ids: `item_entry` is the retail item id, and `gameobject_entry` is the
8.3.7 game object I chose to stand in for it.

All 571 rows already carry one, so the file imports cleanly and every row is placeable the
moment it loads. The 571 pieces land on 187 distinct game objects, which is the honest part of the
substitution: a lamp, a lantern and a chandelier often share the nearest thing 8.3.7 has.
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

## What is not complete

- 317 of the 1 056 captured stock lines still have no retail item id, and 19 vendors have
  none at all. The reason is the same for all of them: a Wowhead vendor page prints 50
  rows and no more, and the vendors under 50 are the ones that resolved. The rows are
  still in the file with their names and prices, so an id can be filled in later without
  touching anything else.
- 87 stock lines have no published price. They are listed as comments at the end of
  `05_stock_retail.sql` and are not inserted, so they cannot come out free.
- `creature_entry` is 0 in the SQL on all 39 vendor rows. The retail npc id is in each
  row's comment, but a retail npc id is not an 8.3.7 creature entry. Run
  `tools/sql/bind_town_center_vendors.sql` after the import and it fills the column from
  `creature_template` by subname, one model per stall; it reports any stall it could not
  fill. Until that runs, the vendors are data the addon lists, and nothing stands in the
  town centre.
- The four story quests ship as `quest_template` rows with no `quest_objectives` rows, on
  purpose: their objectives are module events (walk a plot, claim a Cornerstone, place
  three pieces, reach level 2), so the player script completes them. That means the quest
  text and the money are real and the progress tracking is code, not data.
- Retail exterior shells (house skins) have no 8.3.7 equivalent. The draft uses the shell
  rooms from doc 06.

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
tools/sql/bind_town_center_vendors.sql   (optional, binds vendor models)
```

Then `Housing.Enable = 1`, `.house`, and the addon: copy `addon/HousingUI` into the client's
`Interface/AddOns/PsychoHouse` (the installer stages it in the source tree and prints the
path). `/house` opens the housing window; the House button sits next to the minimap.
