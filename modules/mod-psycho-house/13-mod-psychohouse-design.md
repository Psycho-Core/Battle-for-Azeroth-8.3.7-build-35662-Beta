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
service bought so far; walking out despawns them. The Steward sells twelve services and the
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
creature:  map->SummonCreature(entry, pos, ...)   then PhasingHandler::AddPhase(...)
object:    GameObject::CreateGameObject(entry, map, pos, quat, 255, GO_STATE_READY)
           then PhasingHandler::AddPhase(...) then map->AddToMap(...)
```

Three consequences, all of them the reason for the choice:

- **there is nothing to orphan.** A persistent spawn needs a row, a guid, and a delete path
  that finds the row even when its grid is unloaded. That is a whole class of bug, and this
  draft has none of it.
- **the phase cannot be wrong for a frame.** The phase is applied before the object reaches
  the map, so no other player ever sees a service that is not theirs.
- **the cost is respawning.** Each entry into the area rebuilds the house, which is at most
  fifteen spawns. That is cheaper than a save-on-every-purchase path and infinitely cheaper
  than debugging a stale grid.

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

Two of those absences are the reason the draft looks unlike an older house module: the phase
mask and the per-player data map are gone, and everything that depended on them had to be
rebuilt on phase ids and on a table.

## 9. What was checked, and how

Four tools read the draft, and two of them were written for it.

```
check_cpp.py --src drafts/mod-psychohouse/src --sql drafts/mod-psychohouse/sql
  5 files, 3 classes, 25 declarations with 25 definitions
  braces, includes, fields[N] against SELECT column counts,
  every table named in a query created by this draft

check_psychohouse.py
  4 SQL files, 3 tables, 13 facilities, 3 build tables cross-checked
  INSERT and UPDATE columns against the table they name, value counts against column
  lists, kind/faction/price/is_object agreement, a resolver for every unbound facility,
  the phase range against uint16 and against the neighbourhood range, config keys in both
  directions, and the two arithmetic claims in the draft README against the catalogue

audit.py        ASCII, voice, links, counts over every document in this folder
verify_drafts.py / check_lua.py / check_protocol.py   unchanged, and still clean
```

Both new checks were mutation-tested rather than trusted:

- `check_cpp.py` on a copy with an unbalanced block and a bogus field index: both reported;
- `check_psychohouse.py` on a copy with seven injected faults - a misspelled build column, an
  unknown facility kind, an `is_object` that disagrees with its kind, a facility with no
  resolver, a resolver for a facility that does not exist, a phase base inside the
  neighbourhood range, a config key removed from the `.conf.dist`, and a README total that no
  longer matches the table: all seven reported.

## 10. What is still open, in order of how much it could hurt

1. **The SQL has never been applied to a MySQL server.** There is none in this workspace. What
   is checked is that every column exists in this build's own loader or schema; what is not
   checked is that MySQL parses it, and one statement - the two `REPLACE INTO
   creature_template_model` copies - depends on that table's primary key accepting a
   five-column insert.
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
