# Draft A - mod-psycho-house, ported from Midnight

This is the version that reproduces retail housing content on 8.3.7. It is a draft: the
C++ core is the prototype I already had, the content is everything I pulled out of the
retail vendor pages, and the numbers are the retail numbers.

## What is in it

| Piece | Count | Where |
| --- | --- | --- |
| Vendors | 39, all with name, npc id, faction and stock count | `sql/world/03_vendors_retail.sql` |
| Decor rows | 555, every one with a real 8.3.7 game object | `sql/world/04_decor_retail.sql` |
| Vendor stock rows | 830 | `sql/world/05_stock_retail.sql` |
| Retail decor item ids carried on those rows | 434 | same file, `item_entry` column |
| House levels, budgets, rooms, exteriors | retail values, 1 to 12 | `../../01-retail-housing-reference.md` |
| Endeavor milestones | 4, with 82 stock rows on vendors that gate pieces behind them. Not enforced yet - phase 3 | `../../04-content-spec-quests-endeavors.md` |
| Currency | Community Coupons 3363, plus Tricky Treat item 33226 for the Hallow's End stall | `04_decor_retail.sql`, `currency` column |

Every vendor row keeps the retail npc id in the table comment, so the mapping between a
Midnight npc and whichever 8.3.7 creature I end up using for the model is never lost.

## Why `gameobject_entry` is 0

A retail decor item id is not a game object entry. Midnight stores decor in its own DB2
table; 8.3.7 has no such table, and no 8.3.7 client contains Midnight models. So each
decor row carries the retail item id in `item_entry` and leaves `gameobject_entry` at 0
until the piece is mapped to a spawn that exists in the 8.3.7 client. A row with a 0
gameobject entry is collectable and visible in the catalogue but cannot be placed; the
core logs it once and skips it. That is deliberate - a guessed game object entry spawns
nothing and logs an error on every load.

How to fill it: pick the closest existing game object from
`../../generated/source-asset-index.csv`, put its entry in `gameobject_entry`, and the row
becomes placeable with retail art substituted by the nearest 8.3.7 model. Doc 03 lists
the families worth starting with.

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

- 87 stock lines have no published price. They are listed as comments at the end of
  `05_stock_retail.sql` and are not inserted, so they cannot come out free.
- 13 vendors have a name, an npc id and an item count but no item names yet: "High Tides"
  Ren 146, "Len" Splinthoof 117, Gronthul 83, "Yen" Malone 68, Jehzar Starfall 54. Those
  five are the last ones; the rest of the 39 are captured.
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
```

Then `Housing.Enable = 1`, `.house`, and the addon from `addon/HousingUI`.
