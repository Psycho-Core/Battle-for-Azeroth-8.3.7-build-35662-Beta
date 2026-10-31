# mod-psycho-house - build plan and full reference

This is my working set for turning the housing prototype into **mod-psycho-house**, a
self-contained player-housing system for BfA 8.3.7 build 35662 running on the
BfaCore Reforged tree. Client is unlocked, so client-side assets are on the table; the
client is still 8.3.7, so everything I build has to be expressed in systems 8.3.7
actually ships.

I read nine sources for this (listed in doc 01 sec.11) and I have been careful to keep
three kinds of statement apart:

- **Retail fact** - published by Blizzard or a guide, with a number attached. Quoted.
- **BfA capability** - something 8.3.7 genuinely supports. Verified against the tree.
- **My decision** - design calls, budgets, prices, scoring. Mine, not Blizzard's.

If a document says "unknown", it means I could not verify it and I am not going to
invent it. There are a lot of unknowns in retail's quest table because Blizzard does not
publish entry ids for the interior quests.

## The documents

| # | Document | What it answers |
| --- | --- | --- |
| 01 | [Retail reference](01-retail-housing-reference.md) | Every retail housing system with numbers: unlock, quests, costs, house levels 1-12 with the full budget table, rooms, exteriors, Endeavors, dyes, pet beds, blueprints, permissions, dashboard, travel, currencies, achievements. |
| 02 | [Feature matrix and port plan](02-feature-matrix-and-port.md) | Feature-by-feature: what retail does -> whether 8.3.7 can do it -> how I build it -> effort -> priority. The single most important document here. |
| 03 | [BfA asset catalogue](03-bfa-asset-catalogue.md) | Everything already in my source code that can be used as housing content: game object families, items, currencies, toys, achievements, quests, maps, creature models, and the exact SQL/scripts that enumerate them from *my* database. |
| 04 | [Content spec](04-content-spec-quests-endeavors.md) | The quest chain, Endeavors, treasure hunts, achievements and rewards I am actually shipping, with ids and acceptance criteria. |
| 05 | [Build plan](05-build-plan-mod-psycho-house.md) | Phases, milestones, file layout, migrations, test gates, effort and risks. How the prototype becomes mod-psycho-house. |
| 06 | [Blueprints, dyes and pet beds](06-blueprint-dye-petbed.md) | Deep design for the three systems that need real engineering rather than data. |
| 07 | [Client patching and assets](07-client-patching.md) | What an unlocked 8.3.7 client can and cannot be made to do, and the patch pipeline for housing art. |
| 08 | [Source asset appendix](08-source-asset-appendix.md) | Generated: 767 game objects and 221 creatures that already have ids in my source tree, with file and line, per family. |
| 09 | [Town Center register](09-town-center-register.md) | Every hub service, every service NPC, every one of the 39 neighborhood vendors with currency, coordinates and npc ids, and the **complete item list for 26 vendors (543 items)** with prices and unlock gates. Raw data in `data/town-center-stock.csv`. |
| - | [Generated index](generated/source-asset-index.md) | Auto-generated inventory (28 314 named id constants) plus the CSV. |

## Tools that ship with this

| File | Purpose |
| --- | --- |
| `tools/housing/harvest_decor.py` | Reads my world DB read-only and emits a ready-to-run `housing_decor` INSERT file from `gameobject_template`, scored and categorised by keyword, with duplicate handling. |
| `tools/housing/index_source_assets.py` | Walks `src/` and extracts every named id constant, classifying the furniture-shaped ones. Produces doc 08 and the CSV. |
| `tools/housing/fetch_vendor_stock.py` | Scrapes all 39 neighborhood vendor pages into a CSV (`vendor, item, price, currency, coords, npc_id`) ready for `psycho_house_vendor_stock`. |
| `docs/mod-psycho-house/vendor-slugs.txt` | The 39 vendor slugs the scraper consumes. |
| `sql/tools/psycho_house_harvest.sql` | The same harvest as pure SQL for people who do not want to run Python: candidate queries per furniture family, plus the report I use to spot gaps. |

## Executive summary

Retail housing is five systems wearing one hat: **ownership** (plots, warband, moving),
**building** (rooms, exteriors, budgets, house levels), **decor** (collection, placement,
dye, pet beds), **community** (neighborhoods, permissions, Endeavors), and **sharing**
(blueprints). 8.3.7 gives me usable raw material for four of the five almost intact, and
one place where I have to substitute hard: **the client has no housing UI and no
modular room geometry**, so rooms become "shells" and the UI becomes an addon.

What 8.3.7 already has that does the heavy lifting:

- **Phases** (`PhaseShift`, 16-bit ids) - the entire neighborhood/plot isolation model.
- **Instanced and non-instanced maps** - one map per neighborhood, one interior map.
- **Game objects** - every furniture piece, tree, brazier, wall, tent and cart already in
  the game is a GO template I can spawn, scale and rotate.
- **Battle pets** - pet beds are just creature spawns in a plot phase.
- **Scenarios + worldstates** - Endeavor progress and the intro questline.
- **Currencies + vendors + gossip** - the entire econ and shop layer.
- **The garrison tables and `GarrisonPlot` scaffolding** - proof this core can already do
  "player owns a plot on a private map", which is 80% of the hard part.

What it does not have, and what that costs me: no housing DB2, no room system, no dye, no
blueprint exchange, no Housing Dashboard. Those are additions, and docs 05 and 06 are
about paying for them in that order.

**The build order I am committing to** (detail in doc 05):

```
P0  ownership + travel + persistence            (prototype already does this)
P1  decor catalogue from my own GO templates    (harvest tool + vendor economy)
P2  house levels, budgets, rooms as shells      (retail-accurate numbers, apply via contractor)
P3  Endeavors as monthly BfA-content events     (worldstates + Phase 3.5 of my module)
P4  dyes (Alchemy/Inscription) + pet beds       (custom items + GO reshade)
P5  blueprints as import/export codes           (addon message + compressed blob)
P6  art pass: custom WMO/ADT housing zone       (client patch, replaces the anchor map)
```

Estimated 14-18 weeks of evenings for P0-P5 against a codebase that already compiles and
runs; P6 is art-bound, not code-bound.

## Audit pass, second revision

Every document was re-read and machine-checked, not skimmed. What the audit actually found
and fixed:

| # | Finding | Fix |
| --- | --- | --- |
| 1 | Two rows in doc 02 contained a literal pipe character inside inline code, which breaks the markdown table they sit in | Rewritten without the pipe |
| 2 | Doc 09 claimed "39 vendors" while its own tables summed differently, and Balen Starfinder was duplicated in the capture | Roster recounted from the source index; duplicate removed; every vendor count now compared against its published total |
| 3 | Doc 09 had a stray table row ("Jolli Maxiboom is Horde; see 3.2") left inside the Alliance table | Removed |
| 4 | Doc 01 said "you must visit the General Contractor", second person, in a document written in my voice | Rewritten |
| 5 | Doc 01's "300 starting budget" line looked like a contradiction of the level table | Now explicitly flagged as an early guide figure superseded by the shipped table |
| 6 | Endeavor milestone gating of vendor stock was **missing entirely** from docs 01, 02, 04 and 05 | Added: doc 01 sec.7.5, doc 02 schema + matrix rows, doc 04 sec.4.2b, doc 05 phase 3 |
| 7 | The "Deed of Patronage" entitlement and achievement-gated decor were unknown | Documented in doc 01 sec.7.5 and doc 09 sec.5.13 |
| 8 | Doc 09's coverage was implied complete when only 4 vendors had item data | Now states exact coverage: 26 of 39 vendors, 543 of 1 143 items, with the pending list named |
| 9 | All 21 cross-references between documents verified against actual section headings | 0 broken |
| 10 | Non-ASCII characters (em dashes, arrows, section signs) that render as garbage in some Windows encodings | All files folded to pure ASCII and verified byte by byte |

## Verification log

Everything numeric in these documents was re-checked against the published source at the
time of writing. Where two sources disagree I record both and say which I use.

| Claim | Check | Result |
| --- | --- | --- |
| House levels 1-12, XP thresholds, budgets | Every row compared against the housing progression table | Match: 0 / 10 / 1 200 / 2 400 / 3 700 / 5 700 / 7 900 / 10 300 / 12 900 / 15 750 / 18 850 / 22 200; interior 910 -> 5 975; rooms 19 -> 134 |
| Exterior budget 200 / 250 / 300 / 350 | Compared with the 12.0.7 change announcement | Match (was 250 cap pre-12.0.7) |
| Plots per neighborhood | Blizzard blog says "roughly 50"; neighbourhood guides count 55 | Using **55**; the blog phrasing is rounding |
| First house cost | Two independent guides | 1 000 gold, one time, no upkeep |
| Homestone cooldown | Two guides plus the neighbourhood page | 15 minutes |
| Dye model | Decor guide | 77 colours in 9 groups, one dye per slot, single use |
| Pet beds | Decor guide | 100 indoors / 25 outdoors, one pet per bed, indoor Roaming allowed |
| Community Coupons | Currency page | id 3363 |
| War Resources | Currency page | id 1560 (BfA currency, reused by Midnight vendors) |
| Argan Hammerfist stock | Vendor page, all 15 lines | Complete, prices included |
| Balen Starfinder stock | Two mirror pages | 53 vs 54 items; both printed, difference is one late addition |
| Neighborhood vendor count | Vendor index | 39 vendors stocking decor inside the two neighborhoods, out of 166 worldwide |
| Source asset counts | Own index run over `src/` | 28 314 named id constants, 988 decor-shaped |
| Vendor item counts | Each transcribed vendor compared to its published total | 26/26 exact: Faarden 69, Xiao Dan 65, Balen 54, Trevor 43, Boh'an 32, Den 27, Jolli 23, Holly 21, Hesta 18, Burlee 16, Timicky 16, Argan 15, Dovetail 15, Fel'orei 15, Griftah 15, Roshai 15, Taifa 15, Devin 13, Harlowe 13, Cordula 11, Perry 9, Agratha 9, Klasa 5, Lonomia 5, Unquestionably Griftah 3, Cousin Shortkaf 1 |
| Endeavor milestone gating | Six vendors cross-checked for "Reach the Nth milestone" conditions | Confirmed on 5 of 6 coupon vendors, 64 gated items total |
| Vendor npc ids | Each printed id traced to an NPC page | 15 confirmed, 12 deliberately left blank |

Open items I could not confirm and therefore did not print: the quest ids after 93057, the
Endeavor currency id, per-room placement costs, npc ids for vendors marked "not confirmed"
in doc 09, and the exact 12.1 exterior "Large" dimensions.
