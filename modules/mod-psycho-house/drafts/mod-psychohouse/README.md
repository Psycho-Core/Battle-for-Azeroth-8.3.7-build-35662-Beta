# mod-psychohouse - one island house per character

I wrote this one third, and it is the smallest of the three on purpose. A house on GM Island
for each character that buys one, in a phase of its own. Not per guild and not per account:
two characters in the same guild own two houses in two phases, and a house opens only for the
character that bought it. Written for BfA 8.3.7 build 35662.

It is not a room builder. It is a key, a plot of island, and a Steward who furnishes it: bank,
auctioneer, mailbox, innkeeper, stable master, spirit healer, forge, anvil, barber chair, guild
vault, a repair vendor, and the portal that matches the owner's faction. Eleven services and
two portals, thirteen rows in one table. Decor, budgets, rooms, dyes, Endeavors and the addon
belong to the neighbourhood module, not to this one.

Nothing in here is a guessed id. The two NPCs are entries I made up because they are mine
(900200 and 900201); every other creature and object is resolved out of the realm's own world
database by a statement that says what it is looking for, and the run prints what it found.

## What it does

| Action | From | Price |
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
1. copy this folder anywhere convenient, for example <source>/modules/mod-psychohouse
2. copy src/ to <source>/src/server/scripts/PsychoHouse
   the folder name matters: the build generates Add<folder>Scripts(), so PsychoHouse
   without a hyphen is the only name that compiles
   (../../tools/housing/install.py --draft mod-psychohouse --root <source> does 2 and 5 as
   well, and picks modules/PsychoHouse or src/server/scripts/PsychoHouse by what it finds)
3. re-run cmake, rebuild, install
4. paste the keys from conf/mod_psychohouse.conf.dist into <config>/worldserver.conf,
   inside [worldserver], without the section header that file is not carrying anyway
5. apply, in order:
     sql/world/01_schema.sql              world database
     sql/world/02_creatures.sql           world database
     sql/world/03_bind_facilities.sql     world database, then read its report
     sql/characters/01_characters_schema.sql   characters database
6. restart, then .psychodiag to see the catalogue state
```

Step 4 is not a formality: this core reads exactly one configuration file and keeps only its
first section, so a `mod_psychohouse.conf` dropped next to the binary is never opened. The
keys have to live in `worldserver.conf`.

Step 5 is the one that needs a human. It ends with three SELECTs: the facility table, the
facilities still unbound, and the two NPC models. Any facility left at entry 0 is simply not
offered - the module never spawns an entry it does not have.

## The database this runs on

Written and checked against MariaDB 11.8.6. Nothing below is a preference; each line is a
decision that the SQL or the code would break without.

| Decision | Why |
| --- | --- |
| `ENGINE=InnoDB` on both new tables | the phase allocation depends on `UNIQUE KEY` being enforced immediately; MyISAM would hand two characters the same phase under a race |
| `utf8mb4` with `COLLATE=utf8mb4_general_ci` written out | MariaDB 11.8 defaults utf8mb4 to `utf8mb4_general_ci`, MySQL 8 to `utf8mb4_0900_ai_ci`; naming it makes the tables behave the same on both, and a table whose collation depends on the server is a table that behaves differently after an upgrade |
| `bigint unsigned` for the character key | this build's `characters.guid` is `bigint(20) unsigned` and `ObjectGuid::LowType` is `uint64`; a 32-bit key works on a young realm and truncates on an old one |
| every scalar subquery wrapped in `COALESCE(..., 0)` | the default `sql_mode` includes `STRICT_TRANS_TABLES`, so a resolver that matches nothing returns NULL, `entry` is `NOT NULL`, and the statement dies with error 1048 instead of leaving the row at 0 |
| no `INSERT IGNORE` | it turns a duplicate-key error into a warning. The phase allocation needs that error: the INSERT fails, the read-back misses, the next attempt takes the new maximum |
| no `ALTER TABLE ... ADD COLUMN IF NOT EXISTS` | that is MariaDB-only syntax; the schema is created whole and never patched in place, so it never needs it |
| no stored routines, no events, no triggers | nothing here has to be installed twice, and the character database stays readable to a plain SQL client |
| every file is safe to run twice | `CREATE TABLE IF NOT EXISTS`, `DELETE` before `INSERT` on the catalogue and on the NPC models, and resolvers that only fire where `entry` is still 0 |

Two things worth knowing before importing:

- the files are ordinary statements with `;` delimiters and no `DELIMITER` blocks, so they
  run from the `mariadb` client, from phpMyAdmin, or from the core's own SQL loader;
- if `sql_mode` has been changed on the realm, `STRICT_TRANS_TABLES` is the setting this was
  written for. Without it nothing breaks - the `COALESCE` calls simply become belt and braces.

What cannot be done here: no MariaDB server is installed in this environment, so the SQL has
never been executed. What was done instead is in the verification section, and it is static.

## The three numbers this folder cannot know

An entry for a banker, an entry for a mailbox, and a display id for a custom NPC are world
data. There is no world database in this folder, so none of them is typed in here. Instead
`sql/world/03_bind_facilities.sql` reads them out of the realm's own tables:

| What | How it is found |
| --- | --- |
| banker, auctioneer, innkeeper, stable master, spirit healer, repair | the `npcflag` bit this build declares for that service |
| mailbox, barber chair, guild vault | the `gameobject_template.type` this build declares: 19, 32, 34 |
| forge, anvil | `gameobject_template.name`, because those two objects have no stable type |
| the two faction portals | `gameobject_template.name` of the portal to that capital |
| models for the two module NPCs | copied from an existing innkeeper and banker |

Lowest matching entry wins, so the result is stable between runs. Any of them can be pinned by
hand: the resolver only fires where `entry` is still 0, so a hand-set entry is never
overwritten. The two model statements delete their row before inserting, so they are the
exception - change them in the file rather than in the table.

## Tables

World: `psychohouse_facility`, 13 rows, offsets from the house anchor, price in copper, a
faction mask, an entry. Characters: `psychohouse_owner`, one row per house, and
`psychohouse_purchase`, one row per bought facility.

`psychohouse_owner.phase` is allocated at purchase, stored, and unique. The range is
50000-64999: 15 000 houses, inside the uint16 phase space this build uses, above every retail
phase id it ships, and clear of the neighbourhood module's 10000-46374 range.

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

Furnishing every row once comes to 795 gold on top of the house; a character that can only
open its own faction's portal spends 695. Both figures are the table above, divided by ten
thousand, and a checker recomputes them.

## The two NPCs

| Entry | Name | Script | Created by |
| --- | --- | --- | --- |
| 900200 | The Keeper | `psychohouse_keeper` | sql/world/02_creatures.sql |
| 900201 | The Steward | `psychohouse_steward` | sql/world/02_creatures.sql |

Both are level 120 - the BfA cap, so a client shows them as current content rather than as a
leftover - friendly to everyone, gossip only, and neutral to both factions. 78 columns of
`creature_template` are written for each: exactly the column list this build's template loader
reads, so the rows carry nothing the server ignores.

Their models are the one thing in the file that is not a number chosen here. `02_creatures.sql`
takes a display id from an existing innkeeper and an existing banker on the realm, and the
report at the end of `03_bind_facilities.sql` prints what it found. An NPC with no model row is
invisible; the resolver statement is one line to repoint if the borrowed look does not suit.

Both are immune to players and both are re-created if the map loses them. That second part is
not decoration: everything this draft spawns is a temporary summon, and this core turns a
temporary summon with no duration into one that is removed when it dies. A vendor killed once
would otherwise stay dead for the rest of the visit. So a world tick every five seconds asks
the map for the guid of the keeper, the steward and every bought facility, and summons back
whatever is not there.

## Commands

| Command | Who |
| --- | --- |
| `.psychohouse` | status of the current character's house |
| `.psychobuy` / `.psychosell` | buy or sell the house |
| `.psychohome` | travel to it |
| `.psychodiag` | GM: catalogue state, bound and unbound facilities, loaded houses |
| `.psychoreload` | GM: re-read the conf and the catalogue table |

The commands exist because the Keeper is on an island: they are the same actions with a
keyboard path, not a second feature set.

## What this draft deliberately does not do

- no decor, placement, budgets, rooms, dyes, Endeavors or blueprints - that is the
  neighbourhood module, and this one carries no part of it;
- no addon: the entire interface is two gossip menus and six commands;
- no guild involvement of any kind. A guild has no house here and cannot buy one;
- no visitors. A house's phase is entered by its owner only. Guest lists are the neighbourhood
  module's job, and doing them twice would mean two permission models;
- no per-row price editing in game; the catalogue is a table, which is the point.

## Verification

Checked here by tools that read the files. None of them needs a database or a compiler, and
none of them claims the result was built:

```
python3 tools/housing/check_cpp.py --src drafts/mod-psychohouse/src --sql drafts/mod-psychohouse/sql
  5 files, 3 classes, 28 methods defined on the manager, braces balanced, includes resolved,
  every table named in a query created by this draft

python3 tools/housing/check_psychohouse.py
  4 SQL files, 3 tables, 13 facilities, 3 build tables cross-checked: every INSERT column
  against its table, every value count against its column list, every unbound facility
  against a resolver, the phase range against the uint16 space and against the neighbourhood
  range, every config key against the code that reads it

python3 tools/housing/check_psychohouse.py (continued, the MariaDB-facing part)
  printf dialect: every database call with arguments goes through PQuery/PExecute, carries no
  `{}` placeholder, and has as many conversions as arguments
  the house key is 64-bit in the header, in the character table and in the schema of
  `characters` itself
  every CREATE TABLE pins ENGINE, CHARSET and COLLATE
  no INSERT IGNORE, no bare scalar subquery in an UPDATE
  the .conf.dist carries no section header, because the core reads only the first section

python3 tools/housing/check_psychohouse.py (continued, the spawn-facing part)
  no PExecute on a table this draft owns, because a queued write followed by a read on the
  same tick is a race the read can win
  AddPhase(x, ..., false) only where AddToMap(x) follows, because a summon is already in the
  world and needs the visibility update that false suppresses
```

Mutation-tested: seven deliberate faults - a misspelled build column, an unknown kind, a
facility with no resolver, a resolver for a facility that does not exist, a colliding phase
base, a missing config key, and a wrong `is_object` - were all caught, as were five added this
round: a bare scalar subquery, a `[psychohouse]` header, a 32-bit house key, a `{}` placeholder
in a live call, and a table with no `COLLATE`. Two more were added after them: `PExecute` put
back at all five write sites, and one summon phased with `false`. Those five and that one were
reported.

Separately, and once, all four SQL files were parsed with a MySQL-dialect SQL parser
(sqlglot) to confirm they are statements rather than text that looks like statements. That is
a syntax check, not an execution.

Not checked, and not claimed:

- whether the SQL applies to a real MariaDB or MySQL server. There is no server here. No
  statement was executed, no table was created, no row was inserted;
- whether the C++ compiles. There is no compiler, no boost and no MySQL client here. What is
  claimed is narrower: every core call it makes was read out of this tree and its signature
  matched, and the calls this build does not have - the old integer phase mask, the
  per-player data map, the old config accessor, the pre-BfA rest call - do not appear in it;
- whether GM Island area 876 is still 876 in a given realm's content. That is why the zone
  and area are configuration;
- whether the resolved banker looks right. That is what the report at the end of
  `sql/world/03_bind_facilities.sql` is for.

## Files

```
drafts/mod-psychohouse/
  README.md
  conf/mod_psychohouse.conf.dist            keys to paste into worldserver.conf
  sql/world/01_schema.sql                   facility table and its 13 rows
  sql/world/02_creatures.sql                the two NPCs, every column the loader reads
  sql/world/03_bind_facilities.sql          resolvers for entries and models, then the report
  sql/characters/01_characters_schema.sql   owner and purchase tables
  src/mod_psychohouse.h                     constants, records, the manager
  src/mod_psychohouse.cpp                   config, catalogue, phases, spawn, buy, sell
  src/mod_psychohouse_gossip.cpp            the Keeper and the Steward
  src/mod_psychohouse_hooks.cpp             world, player and command scripts
  src/loader.cpp                            AddPsychoHouseScripts()
```
