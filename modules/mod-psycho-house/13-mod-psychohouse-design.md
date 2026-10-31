# 13 - mod-psychohouse: one island house per character

This is the third draft, and the smallest one that could be called finished: a house on GM
Island for each character that buys one, in a phase of its own, furnished from a thirteen row
catalogue. It exists because the neighbourhood module is a large build and there has to be
something that ships before it, and because a per-character house is a different product from
a per-neighbourhood one - not a cut-down version of it.

The draft is in `drafts/mod-psychohouse/`. Everything below is about that folder: what it
does, why each decision is the BfA-compatible one, and what was checked against this tree
rather than assumed.

## 1. The shape, in one paragraph

A character buys a key from a Keeper NPC for a thousand gold. The purchase allocates a phase,
writes one row, and gives nothing else: the house itself is the island plus the phase. Walking
into the area puts the character in that phase and spawns the Keeper, the Steward and every
service bought so far; walking out despawns them. The Steward sells eleven services and the
faction portal one at a time, each for its own price and its own confirmation. Selling the key
back deletes the row, the purchases, and every spawn, and refunds a share of the price.

## 2. Why per character, and not per guild or per account

Three models were on the table. A house owned by a guild is cheaper to run - one phase per guild, one
purse - but it puts the house behind a rank check and makes a player's home depend on
somebody else's guild. An account house matches a warband and costs nothing extra. A character
house is the one that behaves like a house: it belongs to one character, it is bought with
that character's gold, and it dies with that character.

The cost of the choice is real and stated in the draft's README: a player with eight
characters can own eight houses. That is a deliberate trade - the phase range holds 15 000
houses, and a realm that finds that too generous can gate purchases on a reputation, a level,
or a per-account counter without touching the schema.

## 3. Phases: 50000-64999, allocated and stored

The isolation primitive in this build is a set of phase ids applied through `PhasingHandler`,
stored per spawn and per player, with the flags read out of `Phase.db2` at runtime. Four
decisions follow from that, and each one is written into the draft:

- **ids start at 50000.** Retail content occupies the low range, and a house must not land on
  a live quest phase and inherit its `Cosmetic` or `Personal` flag. 50000-64999 is clear of
  the neighbourhood module's 10000-46374 as well, so both drafts can exist on one realm.
- **the id is allocated, not derived.** `phase = 50000 + n` with `n` read from
  `MAX(phase) + 1` at purchase, guarded by `UNIQUE KEY phase`. A guild id or a character id
  modulo something would collide eventually and the collision would be silent. The worst case
  here is that the INSERT is a no-op and the next attempt reads the new maximum.
- **the phase is stored.** A house whose phase had to be recomputed from the character id
  could change identity if the id model ever changed.
- **`uint16` is the ceiling.** 64999 is inside it with room to spare, and 15 000 houses is
  what that range holds.

The draft refuses the 15001st purchase with a clear message rather than wrapping.

## 4. Spawns are transient, and that is the design

Nothing this module spawns is written to `creature` or `gameobject`. Both are created from
templates at run time, phased, and added to the map:

```
object:    GameObject::CreateGameObject(entry, map, pos, quat, 255, GO_STATE_READY)
           then PhasingHandler::AddPhase(object, phase, false) then map->AddToMap(object)
creature:  map->SummonCreature(entry, pos, ...)
           then PhasingHandler::AddPhase(creature, phase, true)
```

The two paths are not symmetric, and the asymmetry is in the third argument of `AddPhase`.
`CreateGameObject` hands back an object that is not in the world yet, so the phase goes on
first and the visibility update is pointless - hence `false`, followed by `AddToMap`.
`Map::SummonCreature` puts the creature in the world itself, before returning, so the phase
goes on afterwards and the visibility update is the only thing that makes the NPC appear -
hence `true`. Getting it the other way round is invisible in review and produces an NPC that
exists, answers gossip when its guid is known, and is not on screen.

Three consequences, all of them the reason for the choice:

- **there is nothing to orphan.** A persistent spawn needs a row, a guid, and a delete path
  that finds the row even when its grid is unloaded. That is a whole class of bug, and this
  draft has none of it.
- **no other player ever sees a service that is not theirs.** The phase is set before the
  object reaches the map, and before the summon's visibility update goes out.
- **the cost is respawning.** Each entry into the area rebuilds the house, which is at most
  fifteen spawns. That is cheaper than a save-on-every-purchase path and infinitely cheaper
  than debugging a stale grid.

A temporary summon is also a creature that can die. `TempSummon::InitStats` turns
`TEMPSUMMON_MANUAL_DESPAWN` with a duration of 0 into `TEMPSUMMON_DEAD_DESPAWN`, which means
the corpse is removed and nothing brings the NPC back - a vendor lost to a stray area effect
would stay lost until the character left the area and came back. Two answers, both cheap: the
spawned NPCs are flagged `UNIT_FLAG_IMMUNE_TO_PC` the way a city vendor is, and a world tick
every five seconds asks the map for each guid a house believes in and re-spawns whatever is
missing.

What the draft does persist is the decision: one row per purchase in `psychohouse_purchase`.
Data is durable, objects are not.

## 5. The catalogue, and where the numbers come from

Thirteen rows in `psychohouse_facility`: bank, auctioneer, mailbox, innkeeper, stable master,
spirit healer, forge, anvil, barber chair, guild vault, two faction portals, and a repair
vendor. Prices run from 10 gold for a spirit healer to 150 for a guild vault, 795 gold for the
lot, against 1 000 for the house.

The prices are mine, not retail's - Blizzard has never sold a mailbox - and they are set so a
house is an evening's play rather than a month's. They live in a table, not in code, so a
realm can reprice one service with one UPDATE.

Positions are **offsets from the house anchor**, two rows of six and seven within ten yards of
it. Absolute coordinates in a table are how a whole village ends up inside a rock when the
anchor moves; offsets mean the anchor is one config value.

## 6. Content: what is 8.3.7 about it

The draft carries no Wrath content. Every one of these is a decision, and every one of them
was checked against this tree:

| Decision | Why |
| --- | --- |
| the two module NPCs are level 120 | 120 is the 8.3.7 cap. A level 35 NPC on a BfA realm is a leftover, and the client treats low level NPCs as old content |
| no class trainers, no ammo vendor, no poisons vendor | ammunition is gone, poisons are a rogue mechanic, and class trainers do nothing in BfA |
| profession trainers are resolved, not shipped | a BfA trainer needs a `creature_trainer` binding to current-rank spells; a table this folder cannot know. Resolving one from a realm that has them is honest; inventing one is not |
| nineteen service objects and creatures are resolved from the realm | the alternative is typing entry numbers from memory, and a wrong entry is an invisible NPC or a mailbox that is a chair |
| the two NPCs write 78 columns of `creature_template` | that is exactly the column list this build's template loader reads, so the rows cannot carry a field the server ignores, and cannot miss one it needs |
| zone and area are configuration | area 876 is content. A realm that has moved GM Island should not need a rebuild |
| `GetIntDefault` / `GetFloatDefault`, not a templated key lookup | this build's config object has no template form. The newer API does not exist here |

## 7. The two NPCs

| Entry | Name | Script | Purpose |
| --- | --- | --- | --- |
| 900200 | The Keeper | `psychohouse_keeper` | buy, travel, sell |
| 900201 | The Steward | `psychohouse_steward` | buy facilities |

They are the only creature entries the draft invents, and it invents them because they are
its own NPCs - 900200 and 900201 belong to this module the way 900100-900103 belong to the
quest chain. Everything else it spawns is an entry that already exists in the realm.

Their models are the one number the draft cannot supply: a `CreatureDisplayID` is world data.
So `03_bind_facilities.sql` copies one from an existing innkeeper and one from an existing
banker. To give either NPC a different look, change one mask in one statement and re-run it.

## 8. What this build gives a house module, and what it does not

The reason this draft is written the way it is sits in this table. Every row was grepped in
`src/server/game` this round, not remembered, because three of these are the difference
between a module that builds and one that does not.

| Capability | In 8.3.7 build 35662 | Used by the draft as |
| --- | --- | --- |
| phase ids and groups, `PhasingHandler::AddPhase` / `RemovePhase` | present | the isolation primitive |
| the old integer phase mask (`SetPhaseMask`, a `phaseMask` column, `GetPhaseByAuras`) | **absent**: no column, no method, no call anywhere in the tree | not used at all |
| per-player custom data (`CustomData`, `DataMap::Base`) | **absent** | a characters table instead |
| `GlobalScript` and its pre-phase-set hook | **absent** | not needed: a house owns its own phase |
| `PlayerScript::OnPlayerBeforeTeleport` | **absent** | the zone-change hook |
| `PlayerScript::OnUpdateZone(player, newZone, oldZone, newArea)` | present, four arguments | entry and exit |
| `Creature::Create` / `SaveToDB` with a phase mask argument | **absent**: the spawn methods take `(guid, map, entry, x, y, z, ang, CreatureData const*, vehId)` and `SaveToDB(mapid, std::vector<Difficulty> const&)` | transient spawns, so neither is called |
| `Map::SummonCreature`, `GameObject::CreateGameObject`, `Map::AddToMap`, `GameObject::Delete`, `Creature::DespawnOrUnsummon` | present | the whole spawn path |
| `SetRestState(0)` | **absent** | `GetRestMgr().SetRestFlag(REST_FLAG_IN_TAVERN)`, which is the BfA rest API |
| a templated config accessor | **absent** | `GetIntDefault` / `GetFloatDefault` / `GetBoolDefault` |
| `CreatureScript` gossip, `AddGossipItemFor` with a money popup, `GuildScript` | present; the core checks the price but never takes it | the Steward takes the copper itself, once, after every other check |
| `TOTAL_LOCALES`, `GetLocaleByName` | present | unused: this draft is English only, by choice |
| `DatabaseWorkerPool::Execute` / `PExecute` | present, and **asynchronous**: they enqueue on a worker and return | only where nothing reads the row afterwards |
| `DatabaseWorkerPool::DirectExecute` / `DirectPExecute` | present, and synchronous | every write whose row is read back on the same tick |
| `WorldScript::OnUpdate(uint32 diff)` | present, called on every world tick | the five-second repair pass for missing spawns |
| `UNIT_FLAG_IMMUNE_TO_PC` | present (`UnitDefines.h`) | keeps housing NPCs from being killed by players |
| `TempSummon::InitStats` | `MANUAL_DESPAWN` with duration 0 becomes `DEAD_DESPAWN` | the reason the repair pass exists |

Two of those absences are the reason the draft looks unlike an older house module: the phase
mask and the per-player data map are gone, and everything that depended on them had to be
rebuilt on phase ids and on a table.

## 9. What was checked, and how

Four tools read the draft, and two of them were written for it.

```
check_cpp.py --src drafts/mod-psychohouse/src --sql drafts/mod-psychohouse/sql
  5 files, 3 classes, 28 methods defined on the manager, all declared in the header
  braces, includes, fields[N] against SELECT column counts,
  every table named in a query created by this draft

check_psychohouse.py
  4 SQL files, 3 tables, 13 facilities, 3 build tables cross-checked
  INSERT and UPDATE columns against the table they name, value counts against column
  lists, kind/faction/price/is_object agreement, a resolver for every unbound facility,
  the phase range against uint16 and against the neighbourhood range, config keys in both
  directions, and the two arithmetic claims in the draft README against the catalogue

check_psychohouse.py, added after the MariaDB pass
  the printf dialect of this build: every database call that carries arguments goes through
  PQuery/PExecute, contains no `{}` placeholder, and has as many conversions as arguments;
  the house key is 64-bit in the header, in the character table, and in the build's own
  `characters` schema; every CREATE TABLE pins ENGINE, CHARSET and COLLATE; no INSERT IGNORE;
  no bare scalar subquery in an UPDATE; the .conf.dist carries no section header

check_psychohouse.py, added after the spawn-and-write pass
  no `PExecute` on a table this module owns, because a queued write followed by a read on
  the same tick is a race the read can win; `AddPhase(..., false)` only on an object that is
  followed by `AddToMap`, because a summon is already in the world and needs `true`

audit.py        ASCII, voice, links, counts over every document in this folder
verify_drafts.py / check_lua.py / check_protocol.py   unchanged, and still clean
```

Both new checks were mutation-tested rather than trusted:

- `check_cpp.py` on a copy with an unbalanced block and a bogus field index: both reported;
- `check_psychohouse.py` on a copy with seven injected faults - a misspelled build column, an
  unknown facility kind, an `is_object` that disagrees with its kind, a facility with no
  resolver, a resolver for a facility that does not exist, a phase base inside the
  neighbourhood range, a config key removed from the `.conf.dist`, and a README total that no
  longer matches the table: all seven reported;
- and again with five faults added in the MariaDB pass - a bare scalar subquery, a
  `[psychohouse]` section header, a 32-bit house key, a `{}` placeholder in a live call, and a
  table with no `COLLATE`: all five reported;
- and once more with two faults added in the spawn pass - `DirectPExecute` turned back into
  `PExecute` at all five write sites, and one summon phased with `false`: five findings and
  one finding respectively.

## 9a. MariaDB 11.8.6, and what changed because of it

The realm runs MariaDB, not MySQL, and the two differ in exactly the places a house module
touches. Six decisions were taken because of that, and every one of them is enforced by a
check rather than by memory:

| Decision | What breaks without it |
| --- | --- |
| `COLLATE=utf8mb4_general_ci` written on both new tables | the server default differs between MariaDB and MySQL, so a table created today can behave differently after a server move |
| `COALESCE((SELECT ...), 0)` around every resolver subquery | with the default `sql_mode` (`STRICT_TRANS_TABLES`), a subquery that matches nothing yields NULL, `entry` is `NOT NULL`, and the statement stops with error 1048 |
| no `INSERT IGNORE` | it demotes the duplicate-key error the phase allocation depends on to a warning, and two characters could end up sharing a phase |
| `bigint unsigned` for the character key | `characters.guid` is `bigint(20) unsigned` and `ObjectGuid::LowType` is `uint64`; a 32-bit key is correct until it silently is not |
| no `ADD COLUMN IF NOT EXISTS`, no stored routines, no events | `ADD COLUMN IF NOT EXISTS` is MariaDB-only; the other two make an install unrepeatable |
| every file run twice without damage | `IF NOT EXISTS`, `DELETE` before `INSERT` on the catalogue and on the model rows, resolvers that only fire on `entry = 0` |

What was not done, and is not claimed: no statement was executed. There is no MariaDB server in
this workspace. The SQL was checked three ways instead - by the column and width rules above,
by a MySQL-dialect parser, and by reading. That is enough to believe the statements are
statements; it is not the same as having run them.

## 9b. Three defects the second read found

The MariaDB pass read the SQL closely and the C++ by signature. Reading the C++ a second
time, against `DatabaseWorkerPool.h`, `Map::SummonCreature` and `TemporarySummon.cpp` rather
than against the header, found three things that would have shipped:

| Defect | What it would have done | Fix |
| --- | --- | --- |
| the owner row was written with `PExecute` | `Execute`/`PExecute` **enqueue** on a worker thread and return; `PQuery` runs on the calling thread. The write was followed on the next line by a read-back that fetches the allotted phase. When the read won that race the character was charged for a house and given none, and the allocation loop had already burned a second phase id looking for the row | all five writes to the module's own tables are `DirectPExecute` |
| summons were phased with `AddPhase(..., false)` | `Map::SummonCreature` has already added the creature to the map, and `false` means "do not refresh visibility". The keeper, the steward and every creature facility would exist and be invisible | `true` for every summon; `false` stays on the gameobjects, which are phased before `AddToMap` |
| nothing replaced a summon that died | a temp summon with duration 0 becomes `TEMPSUMMON_DEAD_DESPAWN`, so one death removes a vendor for the rest of the visit | `UNIT_FLAG_IMMUNE_TO_PC` on every spawned NPC, plus a five-second `WorldScript::OnUpdate` pass that asks the map for each guid and re-spawns what is missing |

Two smaller things came out of the same read. `SpawnKeeper` and `SpawnSteward` are now their
own methods, because the repair pass needs them separately from the whole house. The refund
is computed in `uint64`: the expression multiplied a `uint32` price by a `uint8` percentage
before dividing, which is safe at the 1 000 gold default (10 000 000 copper, so the product
peaks at 500 million) but wraps at a price past 8 589 gold with a 50 percent rate. A realm
that reprices should not also have to check the arithmetic.

Both new rules are enforced rather than remembered: `check_psychohouse.py` reports `PExecute`
on any table the module owns, and reports `AddPhase(x, ..., false)` unless `AddToMap(x)`
follows it. Both were mutation-tested - see sec. 9.

## 10. What is still open, in order of how much it could hurt

1. **The SQL has never been applied to a server.** There is none in this workspace. What is
   checked is that every column exists in this build's own loader or schema and that a
   MySQL-dialect parser accepts every statement; what is not checked is execution, and two
   statements - the model rows for 900200 and 900201 - depend on `creature_template_model`
   accepting a five-column insert with `Idx` 0.
2. **The C++ has never been compiled.** No compiler, no boost, no MySQL client here. What is
   claimed is narrower: every core call was read out of this tree and its signature matched,
   and the calls this build does not have are absent from the code. `check_cpp.py` covers the
   things a reader can see without a compiler.
3. **The two `npcflag` masks and the three `gameobject_template` types** are read from this
   build's headers (`UnitDefines.h`, `GameObjectData.h`) and are correct for this build. They
   are not correct for every build of the game, and the draft's SQL says which values it
   assumes so a different tree is a one-line change rather than a mystery.
4. **Whether GM Island area 876 is 876 on a given realm.** Configurable, on purpose.
5. **Whether the resolved services look right on the island.** That is what the report at the
   end of `03_bind_facilities.sql` is for, and it is a five-minute read, not a build.

## 11. Why this one ships first

The neighbourhood module is the product: plots, decor, budgets, Endeavors, dyes, blueprints,
an addon. This draft is the thing that can be finished in an evening and played with
tomorrow, and it is also the layer the neighbourhood module needs anyway - doc 05 sec.4.6
schedules the same facility catalogue there. So this is not a throwaway: the phase maths, the
transient spawn path, the resolver SQL and the two NPCs all move across unchanged.

If per-player placement ever becomes too expensive to finish, what is already here is a
complete, honest, playable answer - and that is the point of building it first.
