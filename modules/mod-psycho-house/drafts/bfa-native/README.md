# Draft B - mod-psycho-house, built only from 8.3.7 build 35662

This is the version that needs nothing from retail. No Midnight item ids, no Midnight npc
ids, no imported art. Every id in the SQL already exists in my source tree or in the
8.3.7 world database, which is the point: it runs on a clean install the day it is
compiled.

## What is in it

| Piece | Count | Where |
| --- | --- | --- |
| Decor rows | 350 | `sql/world/03_decor_native.sql` |
| Vendors | 8 of my own | `sql/world/04_vendors_native.sql` |
| Vendor stock rows | 350, every native decor piece sold somewhere | same file |
| Currency | gold only | `currency = 0` on every row |
| Substitution for missing art | none needed | the pieces are real game objects |

## Where the 350 come from

`tools/housing/index_source_assets.py` walks the source tree and pulls out every named id
constant. 767 of those constants are game objects that look like furniture. This draft
takes those 767, drops the ones whose constant name marks them as doors, gates, traps,
spawn markers, spell visuals and instance props, and keeps 350 that can be dropped in a
yard without looking wrong. The list is in `../../generated/source-asset-index.csv`; the
filter is the `reject` tuple in `tools/housing/build_drafts.py`, so changing my mind about
what counts as decor is a one-line edit and a re-run.

Names in the SQL are the constant names, because that is what the source gives me. The
last statement in `03_decor_native.sql` fixes that the moment the world database is
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
`tools/sql/bind_native_vendors.sql` does - it picks existing 8.3.7 creature entries by
name and writes them into `housing_vendor.creature_entry`. Nothing is invented: a name
that is not in `creature_template` simply matches zero rows.

Decor is split across the eight stalls by `gameobject_entry % 8`, so each stall carries a
mixed bag rather than a category. That is on purpose: with 350 pieces a category split
would leave one stall with four items.

## Economy

Gold only, and the ladder is mine:

| Price | Pieces |
| --- | --- |
| 10g | lighting, storage, books, nature, accents |
| 25g | seating, rugs, wall art, holiday |
| 50g | structural, beds, pets |
| 75g | tables |

Budget cost follows retail bands: 1 for anything at or below 25g, 3 up to 75g, 5 above.
That keeps the retail placement budgets in doc 01 usable without translating prices.

## What it deliberately does not have

- No Community Coupons. 3363 is a real 8.3.7 currency id, but with no Endeavor system
  behind it there is nothing to earn it from in this draft.
- No Endeavor milestone gating. Every piece on every stall is buyable from the start.
- No dyes and no pet beds; those are phase 4 of doc 05.
- No retail house levels 1 to 12. Six levels, indoor budget 3000, outdoor 200.

## Install

```
sql/characters/01_characters_schema.sql
sql/world/01_schema.sql
sql/world/02_world_content.sql
sql/world/03_decor_native.sql
sql/world/04_vendors_native.sql
tools/sql/bind_native_vendors.sql        (optional, binds vendor models)
```

`../../tools/housing/install.py --draft bfa-native` puts the four trees where the build
expects them. `Housing.Map.Mode = "anchor"` is the default here because a clean 8.3.7
install has no housing map; point `Housing.Map.Id` at any map with room for the plot
anchors and author them in the world DB.
