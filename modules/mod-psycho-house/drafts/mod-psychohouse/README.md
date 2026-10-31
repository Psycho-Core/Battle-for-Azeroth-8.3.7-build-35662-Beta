# mod-psychohouse - one island house per character

A house on GM Island for each character that buys one, in a phase of its own. Not per guild,
not per account: two characters in the same guild own two houses in two phases, and neither
can walk into the other's. Written for BfA 8.3.7 build 35662.

The house is not a room builder. It is a key, a plot of island, and a Steward who furnishes
it: bank, auctioneer, mailbox, innkeeper, stable master, spirit healer, forge, anvil, barber
chair, guild vault, a repair vendor, and the portal that goes with the character's faction.
Twelve services and a door. Everything else in this folder - decor, budgets, rooms, dyes,
Endeavors, the addon - belongs to the neighbourhood module, not to this one.

## What it does

| Action | Where from | Price |
| --- | --- | --- |
| Buy the house | the Keeper NPC, or `.psychobuy` | 1 000 gold, config |
| Travel to it | the Keeper, or `.psychohome` | free |
| Furnish it | the Steward NPC, one service at a time | per row in `psychohouse_facility` |
| Sell it back | the Keeper, or `.psychosell` | half the house price, config |

Once the house is bought, walking into the island area puts the character into the house's
phase and spawns the Keeper, the Steward and everything bought so far. Walking out despawns
them again. Nothing is written to the `creature` or `gameobject` tables, so there is nothing
to orphan and no grid bookkeeping to get wrong.

## Install

```
1. Copy this folder to <source>/modules/mod-psychohouse        (name it exactly that)
2. copy the tree in src/ to <source>/src/server/scripts/PsychoHouse
   the folder name matters: the build generates Add<folder>Scripts(), so PsychoHouse
   without a hyphen is the only name that compiles
3. re-run cmake and rebuild, then copy conf/mod_psychohouse.conf.dist next to worldserver
4. apply, in order:
     drafts/mod-psychohouse/sql/world/01_schema.sql              world database
     drafts/mod-psychohouse/sql/world/02_creatures.sql           world database
     drafts/mod-psychohouse/sql/world/03_bind_facilities.sql     world database, then read its report
     drafts/mod-psychohouse/sql/characters/01_characters_schema.sql   characters database
5. restart, then .psychodiag to see the catalogue state
```

Step 4 is the one that needs a human. It ends with three SELECTs: the facility table, the
facilities still unbound, and the two NPC models. Any facility left at entry 0 is simply not
offered - the module never spawns an entry it does not have.

## The three numbers this folder cannot know

An entry for a banker, an entry for a mailbox, and a display id for a custom NPC are world
data. There is no world database in this folder, so none of them is typed in here. Instead
`03_bind_facilities.sql` reads them out of the realm's own tables:

| What | How it is found |
| --- | --- |
| banker, auctioneer, innkeeper, stable master, spirit healer, repair | the `npcflag` bit this build declares for that service |
| mailbox, barber chair, guild vault | the `gameobject_template.type` this build declares: 19, 32, 34 |
| forge, anvil | `gameobject_template.name`, because those two objects have no stable type |
| the two faction portals | `gameobject_template.name` of the portal to that capital |
| models for the two module NPCs | copied from an existing innkeeper and banker |

Lowest matching entry wins, so the result is stable. Any of them can be pinned by hand: the
resolver only fires where `entry` is still 0, so a hand-set entry is never overwritten.

## Tables

World: `psychohouse_facility`, 13 rows, offsets from the house anchor, price in copper, a
faction mask, an entry. Characters: `psychohouse_owner`, one row per house, and
`psychohouse_purchase`, one row per bought facility.

`psychohouse_owner.phase` is allocated at purchase, stored, and unique. The range is
50000-64999: 15 000 houses, inside the uint16 phase space, above every retail phase id this
build ships, and clear of the neighbourhood module's 10000-46374 range.

## The catalogue

| # | Facility | Copper | Gold | Faction | Type |
| --- | --- | --- | --- | --- | --- |
| 1 | Bank | 1 000 000 | 100 | both | creature |
| 2 | Auctioneer | 1 000 000 | 100 | both | creature |
| 3 | Mailbox | 250 000 | 25 | both | object |
| 4 | Innkeeper | 500 000 | 50 | both | creature |
| 5 | Stable Master | 500 000 | 50 | both | creature |
| 6 | Spirit Healer | 100 000 | 10 | both | creature |
| 7 | Forge | 250 000 | 25 | both | object |
| 8 | Anvil | 100 000 | 10 | both | object |
| 9 | Barber Chair | 500 000 | 50 | both | object |
| 10 | Guild Vault | 1 500 000 | 150 | both | object |
| 11 | Portal West | 1 000 000 | 100 | Alliance | object |
| 12 | Portal East | 1 000 000 | 100 | Horde | object |
| 13 | Repair | 250 000 | 25 | both | creature |

Furnishing every row once comes to 795 gold on top of the house; a character that can
only open its own faction's portal spends 695.

## The two NPCs

| Entry | Name | Script | Created by |
| --- | --- | --- | --- |
| 900200 | The Keeper | `psychohouse_keeper` | 02_creatures.sql |
| 900201 | The Steward | `psychohouse_steward` | 02_creatures.sql |

Both are level 120, friendly to everyone, gossip only. 78 columns of `creature_template` are
written for each - exactly the column list this build's template loader reads, so the rows
carry nothing the server ignores.

## Commands

| Command | Who |
| --- | --- |
| `.psychohouse` | status of the current character's house |
| `.psychobuy` / `.psychosell` | buy or sell the house |
| `.psychohome` | travel to it |
| `.psychodiag` | GM: catalogue state, bound and unbound facilities, loaded houses |
| `.psychoreload` | GM: re-read the conf and the catalogue table |

The commands exist because the Keeper is on an island: they are the same four actions with a
keyboard path, not a second feature set.

## What this draft deliberately does not do

- no decor, placement, budgets, rooms, dyes, Endeavors or blueprints - that is the
  neighbourhood module, and this one carries no part of it;
- no addon: the entire interface is two gossip menus and five commands;
- no guild involvement of any kind. A guild has no house here and cannot buy one;
- no visitors. A house's phase is entered by its owner only. Guest lists are the
  neighbourhood module's job, and doing them twice would mean two permission models.

## Verification

Checked here, mechanically, by two tools that read the files:

```
python3 tools/housing/check_cpp.py --src drafts/mod-psychohouse/src --sql drafts/mod-psychohouse/sql
  5 files, 3 classes, 25 methods declared and defined, braces balanced,
  includes resolved, every table named in a query created by this draft
python3 tools/housing/check_psychohouse.py
  4 SQL files, 3 tables, 13 facilities, 3 build tables cross-checked
  every INSERT column against its table, every value count against its column list,
  every unbound facility against a resolver, the phase range against the uint16 space
  and against the neighbourhood range, every config key against the code that reads it
```

Both were mutation-tested: seven deliberate faults - a misspelled build column, an unknown
kind, a facility with no resolver, a resolver for a facility that does not exist, a colliding
phase base, a missing config key, and a wrong `is_object` - were all caught by the second
tool, and an injected unbalanced block by the first.

Not checked, and not claimed:

- whether the SQL applies to a real MySQL server. No server here;
- whether the code compiles. No compiler, no boost, no MySQL client here. What is claimed
  is narrower and checkable: every core call it makes was read out of this tree and its
  signature matched, and the three calls this build does not have - the WotLK phase mask,
  the per-player data map, and the old config accessor - are not used anywhere in it;
- whether GM Island area 876 is still 876 in a given realm's content. That is why the zone
  and area are configuration;
- whether the resolved banker looks right. That is what the report at the end of
  `03_bind_facilities.sql` is for.

## Files

```
drafts/mod-psychohouse/
  README.md
  conf/mod_psychohouse.conf.dist
  sql/world/01_schema.sql              facility table and its 13 rows
  sql/world/02_creatures.sql           the two NPCs, every column the loader reads
  sql/world/03_bind_facilities.sql     resolvers for entries and models, then the report
  sql/characters/01_characters_schema.sql   owner and purchase tables
  src/mod_psychohouse.h                constants, records, the manager
  src/mod_psychohouse.cpp              config, catalogue, phases, spawn, buy, sell
  src/mod_psychohouse_gossip.cpp       the Keeper and the Steward
  src/mod_psychohouse_hooks.cpp        world, player and command scripts
  src/loader.cpp                       AddPsychoHouseScripts()
```
