# 16. Using the native draft

`drafts/bfa-native` is the version of the module that needs nothing from retail: no Midnight
item ids, no imported art, no patched client. Every id in its SQL is already in a BfA 8.3.7
build 35662 world database or in the source tree, every piece is a game object that ships with
the game, and it installs on a clean realm the day it compiles.

This is the page for somebody who has the draft in front of them and wants a furnished house
this evening. It is written as a walk through, in the order the steps happen, and it ends with
the things that can go wrong and what each one means.

## 1. What is in the box

| Folder | What it holds |
| --- | --- |
| `sql/world/` | the catalogue, the world content, the vendors, the quests, the progression, the hunts |
| `sql/characters/` | the character-side schema: placements, blueprints, houses |
| `drafts/bfa-native/sql/bind/` | statements that read the realm's own `creature_template`, `gameobject_template` and `quest_template` and fill the zeros; each ends with a report |
| `src/game/Housing/` | the module itself, compiled into the core |
| `addon/HousingUI/` | the client window: fifteen tabs, one per part of the catalogue |
| `drafts/bfa-native/conf/mod-psycho-house.conf.dist` | the 42 settings, as a file to paste into `worldserver.conf` |

A house in this draft is a plot in a neighbourhood, on maps the client already loads. There is
no custom terrain, no custom model and no client patch: the plots are computed as rings around
`Housing.TownCenter`, and every piece placed on one is a real game object. Doc 07 sec.3 covers
the three ways a plot can get a position, including the anchor file.

## 2. Install, in order

```
1. copy drafts/bfa-native/src/game/Housing/  ->  <source>/src/game/Housing/
   copy drafts/bfa-native/addon/HousingUI/   ->  <client>/Interface/AddOns/HousingUI/
   (tools/housing/install.py --draft bfa-native --root <source> does the first line and
    reports what it wrote; the addon is a plain folder copy)
2. re-run cmake, rebuild, install
3. paste every key from conf/mod-psycho-house.conf.dist into <config>/worldserver.conf,
   inside [worldserver], without a section header - then edit two of them, because the file
   ships a realm that is not this one:
     Housing.Enable = 1        the file ships 0, and a module with Enable 0 does nothing
     Housing.TownCenter.X/Y/Z  a land point on Housing.Map.Id. The file ships 0/0, which
                               leaves every plot on a ring around the world origin and makes
                               the arrival call refuse with "not allowed here"
4. characters database:   sql/characters/01_characters_schema.sql
5. world database, in this order:
     sql/world/01_schema.sql              the module's own tables
     sql/world/02_world_content.sql       town centre, biomes, amenities, pets
     sql/world/03_decor_native.sql        the catalogue: 706 pieces
     sql/world/04_vendors_native.sql      eight stalls and their stock
     sql/world/06_quests.sql              four quest rows
     sql/world/07_progression.sql         levels, shells, rooms, portals, dyes, Endeavors
     sql/world/08_hunts.sql               ten riddle hunts
6. world database, the bind pass, in this order:
     sql/bind/bind_native_vendors.sql     vendor creature entries
     sql/bind/bind_decor_variants.sql     dye variants
     sql/bind/bind_night_variants.sql     the night set
     sql/bind/bind_housing_content.sql    the scripts behind the town centre
     sql/bind/bind_town_center_vendors.sql  the captured stock, where it applies
7. restart, then in game: .housing diag
```

Two rules matter more than they look:

* **Order.** The bind files read tables the earlier files have just filled. Running
  `bind_decor_variants.sql` before `03_decor_native.sql` is not an error - it simply matches
  nothing and leaves the catalogue with no dye variants, silently.
* **Re-running.** Every file is safe to run twice: the generated ones `DELETE` their own id
  band before inserting, and the bind files write only into cells that are still zero or empty.
  Re-running them after a restart is a normal thing to do, not a rescue.

Every bind file ends with a `SELECT` that reports what it found. Those reports are the
install's own answer to "did this work", and they are worth reading rather than scrolling past:
"vendors bound: 0" is a realm whose `creature_template` has none of the names the resolver
looks for, and that is a one-line edit in the file rather than a mystery in game.

## 3. The first hour, in game

| Step | How | What to expect |
| --- | --- | --- |
| Look at the realm | `.housing diag` | one line per subsystem: catalogue size, vendors bound, plots free, settings the module actually read |
| Find the town centre | `.housing tele` then the taxi, or the teleport NPC in the capital | eight amenity objects and the vendor stalls, standing where `Housing.TownCenter` says |
| Buy a plot | the housing clerk at the town centre, or `.housing buy` | a plot index allocated, one row in the character table, a key in the bag |
| Travel to it | `.housing home`, or the key | the plot's phase, the house shell for the current level, the amenity slots |
| Furnish it | the vendor stalls, then the window | a piece bought becomes a decor entry the window can place |
| Place something | the addon's Build tab, or `.housing place <id> <x> <y> <z> <o>` | the object spawns, the budget goes down, the placement is written on the next save |
| Save a look | the Blueprint tab, or `.housing blueprint save <name>` | a named snapshot of every placement on the plot |

The addon is not required for any of it. Every action in the window has a command behind it,
which is deliberate: a client with the addon missing can still play, and a player who prefers
typing never has to open a window.

## 4. Where the 706 pieces come from, and how to change the list

The catalogue is generated from `generated/source-asset-index.csv`, which
`tools/housing/index_source_assets.py` writes by walking the source tree and pulling out every
named id constant:

```
python3 tools/housing/index_source_assets.py          # rebuild the index from the source tree
python3 tools/housing/build_drafts.py                 # regenerate both drafts from it
python3 tools/housing/verify_drafts.py                # recompute every id and count and compare
```

767 of the index's rows are game objects that look like furniture, and they are 748 distinct
entries. The builder drops entries whose constant name marks them as traps, triggers, spawn
markers, spell visuals, sound emitters, boss markers or instance props; 702 survive, and the
four pet beds make 706 catalogue rows. Doors, gates and portcullises are no longer dropped -
they are 393 distinct entries and they are why Structural is by far the largest tab in the catalogue.
Collision objects, sky walls and raid encounter objects are dropped, because none of the three
is furniture: a `COLLISION_*` object exists to block movement and has no model at all, a
`SKYWALL` is a zone ceiling, and an `ENCOUNTER_*` object is a raid door or an invisible
trigger that opens when a boss dies.

Three things follow from that, and each one is a rule rather than a preference:

* **An id is a position, not a hash.** A decor id is the entry's rank among all 748, so the
  range 2000-2999 has gaps where a filter dropped a piece, and a filter change never renumbers
  the catalogue. A decor id is what a placement, a stock row and a saved blueprint point at.
* **The filter is one tuple.** It lives in `build_native()` in `tools/housing/build_drafts.py`
  as a list of words matched against the constant name. Removing a word and re-running is the
  whole edit; adding one is the same edit in reverse, and the ids do not move either way.
* **A piece with no model is a piece to delete.** `03_decor_native.sql` ends with a report that
  lists every catalogue row whose `gameobject_template` row is missing or has `displayId` 0. It
  is expected to return nothing. A row there is a piece that would stand in a yard invisible and
  still count against the budget, so delete it from the table and add its entry to the reject
  list so the next regeneration leaves it out.

To add a piece the index has never seen - a custom object, a seasonal one, one of the four pet
beds' cousins - write the row by hand above the merge band:

```sql
INSERT INTO `housing_decor`
  (`id`, `name`, `gameobject_entry`, `budget_cost`, `surface`, `placement`, `category`,
   `source_type`, `source_id`, `price`, `currency`, `faction_mask`, `scale_min`, `scale_max`, `flags`)
VALUES
  (2954, 'Winter Veil Sleigh', 180797, 3, 3, 2, 'Holiday', 0, 208, 50, 0, 3, 0.5, 3, 0);
```

2954 and up is deliberate: 2000-2899 belongs to the generated catalogue and 2950-2959 to the
pet beds, so a hand-written row above them survives the next regeneration. `source_id` is the
vendor, and a stock row is what makes the piece buyable:

```sql
INSERT INTO `housing_vendor_stock` (`vendor_id`, `decor_id`, `price`) VALUES (208, 2954, 50);
```

## 5. What the catalogue covers, and what it does not

| Part of a house | In this draft |
| --- | --- |
| Walls, floors, roofs | 12 shells, four kits in three sizes, bought as a level |
| Rooms inside the shell | 17 room shapes, each with a budget cost and a footprint |
| Doors, gates, columns, windows | 433 Structural pieces: doors, gateways, portcullises, columns, walls, bridges, towers |
| Furniture: seating and storage | 23 seating and 152 storage pieces, from the same catalogue |
| Furniture: tables and beds | nothing - the 8.3.7 tree names no table, desk, shelf or bed constant at all |
| Lighting, accents, wall art | 25 lighting, 24 accent and 34 wall-art pieces, 4 books |
| Rugs | nothing - no rug or carpet constant exists; a rug-shaped piece takes a banner |
| Nature: foliage, rocks | 7 pieces; the tree has trees, one bush and no flower |
| Pets and pet beds | four beds, plus the pet system |
| Dyes | six colour rows; a piece is dyable when its name has a colour word and a sibling exists |
| Night set | a second entry per piece for the dark Armillary, where the realm has one |
| Music | nothing yet: `DecorFlagJukebox` and a sound id are still reserved, and no amenity plays anything |
| Mounts and mannequins | nothing yet: the two flags are reserved and no row sets them |

What it does not carry: custom art, custom terrain, imported Midnight pieces, and any item id
from retail. That is the point of this draft rather than a gap in it - the port draft next to it
exists for the pieces this one cannot show.

## 6. Things that go wrong, and what each one means

| Symptom | Cause | What to do |
| --- | --- | --- |
| `.housing diag` says the catalogue is 0 | `03_decor_native.sql` was never run, or ran into a schema that predates it | run `01_schema.sql` then `03_decor_native.sql` again; the first `DELETE` makes it repeatable |
| A stall has no stock | `04_vendors_native.sql` ran before the catalogue, or `vendor_id` does not match a stall | re-run it; it deletes 201-208 first and re-inserts |
| A vendor is invisible or cannot be clicked | its `creature_entry` is still 0 - `bind_native_vendors.sql` found no name it recognises | read the report at the end of that file, then set one entry by hand |
| A placed piece is invisible | the entry has no model on this realm | run the report at the end of `03_decor_native.sql`; delete the row |
| Pieces place but nothing saves | the characters schema was not applied, or the character database name in `worldserver.conf` differs from where it was applied | `.housing diag` names the tables it cannot read |
| The window is empty but the commands work | the addon folder is not in the client, or its interface number does not match the client build | this draft's addon is `## Interface: 80300`; the commands are the fallback by design |
| A house has no phase | the plot row exists and its phase column is 0 | `.housing diag` lists plots without a phase; `.housing repair` re-allocates them |

## 7. How this draft is checked, and what is not checked

```
python3 tools/housing/verify_drafts.py       every id, every count, every documented number
python3 tools/housing/simulate_install.py    the file order, executed in memory: 11 files,
                                             31 tables, 1 653 rows, nothing lost
python3 tools/housing/audit.py --only drafts  the catalogue read as data: stock on a piece
                                             its vendor's faction can buy, both factions can
                                             buy a pet bed, every quest target exists
python3 tools/housing/check_mariadb.py       the MariaDB rules every file obeys
python3 tools/housing/audit.py               twenty checks, both addons run in Lua 5.1
```

Not checked, and not claimed: the SQL has never been applied to a server, the C++ has never
been compiled, and no client has loaded the addon. The install simulation is an interpreter for
this SQL, not MariaDB; it proves the order and the row counts, not the server's acceptance.
