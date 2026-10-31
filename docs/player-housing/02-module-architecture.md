# 02 - Module architecture

How the housing module is put together, what each class owns, and why I chose the
mechanisms I did. Doc 01 says what the feature *is*; this one says how it runs on a
BfA 8.3.7 core.

## 1. Ground rules

Four constraints drove every decision here.

1. **Additive.** The module must not fork core files. The only edit to existing code is
   a handful of lines in `WorldSession::HandleChatAddonMessage` that hand `BFAH`-prefixed
   addon messages to the module. Everything else lives in new directories and registers
   itself through the existing `ScriptMgr` hooks.
2. **Data-driven.** Plot layout, biome flavour, prices, vendors and the decor catalogue
   are rows in the world database. Nothing about Founder's Point is hardcoded in C++.
   Adding the Horde neighbourhood should be an SQL exercise plus a map, not a rebuild of
   the module.
3. **No client module.** The 8.3.7 client has no housing UI, no housing DB2 tables and no
   Founder's Point geometry. The player-facing surface is a custom addon talking over the
   addon channel, plus gossip from NPCs, plus a chat command for the fallback paths.
4. **Cheap when idle.** A server with 400 houses must not pay for 400 houses' worth of
   game objects when nobody is standing in them.

## 2. Layer map

```
src/server/scripts/Housing/          registration only
  housing_script_loader.cpp          AddHousingScripts()
  housing_player_script.cpp          login / logout / update / zone-change hooks
  housing_world_script.cpp           startup load, shutdown save, config reload
  housing_commands.cpp               .housing ... (player + GM)

src/server/game/Housing/             the module
  HousingDefines.h                   enums, limits, phase maths, value structs
  HousingAddon.h/.cpp                BFAH wire format: encode, decode, chunking
  HousingNeighborhood.h/.cpp         one village: 55 plots, type, name, members
  HousingPlot.h/.cpp                 one plot: owner, biome, house, state machine
  HousingHouse.h/.cpp                tier, blueprint, budget, decor list, game objects
  HousingDecor.h/.cpp                catalogue entry + placed instance
  HousingMgr.h/.cpp                  singleton: data load, lookup, buy/sell, teleport,
                                     protocol dispatch, persistence, tick
```

`CollectSourceFiles()` in `src/server/game/CMakeLists.txt` globs recursively, so new files
under `src/server/game/Housing` are compiled without touching any CMake file.
`src/server/scripts/*` directories are picked up automatically by
`GetScriptModuleList()`, and a directory named `Housing` needs exactly one exported
`AddHousingScripts()` in a file inside it.

## 3. Class responsibilities

| Class | Owns | Does not own |
| --- | --- | --- |
| `HousingMgr` (singleton) | World data (biomes, plot templates, decor catalogue, vendors), the neighbourhood registry, per-player session state, all persistence, the update tick | Any single plot's decor objects |
| `HousingNeighborhood` | Identity (id, type, name, faction, guild), phase slot, 55 `HousingPlot` objects, member list, plot -> owner index | Game objects, packets |
| `HousingPlot` | `plotIndex`, biome, template ref, state (`Available` / `Owned`), owner account + character, purchase time, price paid, back-pointer to its `HousingHouse` | Whether the plot is currently phased in |
| `HousingHouse` | Tier, blueprint id, budget accounting, placed decor list, spawned `GameObject*` handles, interior phase | Ownership (that is the plot's job) |
| `HousingDecor` | Catalogue definition (one per row of `housing_decor`) and placement value type | Budget rules, which live in `HousingHouse` |
| `HousingAddon` | Serialising/deserialising `BFAH` messages, splitting oversize payloads | Any game logic; it never mutates state |

`HousingMgr` is a singleton with the usual `instance()`/`sHousingMgr` pattern used
everywhere else in this tree, initialised from `WorldScript::OnStartup` and drained in
`OnShutdown`.

## 4. Why phases and not instanced maps

Retail neighbourhoods are persistent map instances: each neighbourhood is its own
`Map`, and each house interior is another. On this core that would mean a `Map`
subclass, a `MapManager::CreateMap` hook, per-instance grid loading, and a pile of
instance-lifetime edge cases.

Instead I put **every neighbourhood on one map and separate them by phase**:

| Criterion | Instanced maps | Phases (chosen) |
| --- | --- | --- |
| Core edits required | `MapManager::CreateMap` + a `Map` subclass + instance id bookkeeping | none |
| Memory / grid cost per neighbourhood | Full grid set per instance | One shared grid set for all neighbourhoods |
| Cross-neighbourhood visiting | Teleport + instance id | Phase swap, no loading screen |
| Ceiling | Instance-id space (large) | 16-bit phase ids, 2 ranges of 64 per slot |
| Risk if the custom map is missing | Fatal for that instance only | Fatal for all housing |
| Debuggability | `.instance` tooling | `.housing debug phases` prints the shift |

The practical difference is that all neighbourhoods share one copy of the terrain and
one creature/GO grid, and separation is a `PhaseShift` on the player. `PhasingHandler`
tolerates phase ids that do not exist in `Phase.db2` - `GetPhaseFlags()` falls back to
`PhaseFlags::None` - so custom ids are safe, and `PhaseShift::PhaseRef::Id` is a
`uint16`, which caps us at `65535`.

The trade-off I accept: **all neighbourhoods must occupy the same physical map.** Two
populations in slot 3 and slot 7 stand in the same world coordinates in different
phases. That is fine for housing (there is nothing to collide with across
neighbourhoods, and no cross-phase visibility), but it does mean I can never let two
neighbourhoods share a slot, and it means a GM free-cam sees overlapping villages if
they toggle phases.

### 4.1 Phase allocation

```c
plotPhase(slot, index)     = 10000 + slot * 64 + index;   // 10000 .. 26374
interiorPhase(slot, index) = 30000 + slot * 64 + index;   // 30000 .. 46374
```

- `slot` is the neighbourhood's slot in the registry, `0 .. 255`.
- `index` is the plot index, `0 .. 54`, so 64 phases per slot leaves 9 spare.
- `HOME_PHASE` (169) is the core default and stays untouched; nothing in the module
  ever runs with an empty phase shift.

A player's phases while in housing:

| Situation | Phases held |
| --- | --- |
| In the open neighbourhood, in no plot | base phase only |
| Inside their own plot's yard | base + `plotPhase(slot, own)` |
| Inside a neighbour's plot with permission or as a guest | base + `plotPhase(slot, theirs)` |
| Inside a house interior | `interiorPhase(slot, plotIndex)` only |

Decor objects are spawned with exactly the plot (or interior) phase attached, so they are
invisible to everyone who is not standing in that plot. That is the whole visibility
mechanism - I do not filter object visibility per player anywhere.

**The seam this creates:** a neighbour's yard decor is not visible from the street, only
from inside their plot boundary. I consider that an acceptable cost, and I mitigate it
with a generous yard trigger radius (default 1.35x the authored yard radius) so the
load-in happens before you can see the difference. `Housing.Phasing.PlotScoped = 0`
turns the plot phase off entirely and makes all yard decor permanently visible - useful
for screenshots, expensive with 55 decorated plots in one grid.

### 4.2 Interiors

Interiors cannot be phases at the same coordinates as the yards, or the house interior
geometry would sit inside the yard. So interiors live on a **second map** (`Housing.Interior.Map`,
default the same custom map, in an unused corner of the terrain). Interiors are grouped
by **shell id**:

- All houses on the same shell (same blueprint family and tier) share one physical
  interior location.
- Houses on different shells get different anchor points.
- Within a shell, each house is separated by its `interiorPhase`.

This means N houses on one shell cost one copy of the shell geometry and N phases of
furniture, which is exactly the trade I want.

## 5. Ownership and permissions

| Concept | Retail | Here |
| --- | --- | --- |
| Owner | Warband, one alliance + one horde house | **Account**, one plot per faction per account |
| Editor | Any warband character | Any character on the owning account |
| Recorded buyer | - | `ownerGuid` / `ownerName` in `character_housing`, display only |
| Guest | Invited player of any faction | Row in `character_housing_guest` with flags |
| Neighbourhood membership | Guild/private charters | `character_housing_neighbor_member` |

Permission checks funnel through exactly one function, `HousingMgr::CheckAccess`, which
returns a `HousingAccess` enum: `Owner`, `Guest`, `Neighbor`, `Stranger`, `Gm`. Every
mutation path takes the access level as an argument and refuses below the required
level, so there is one place to audit.

| Action | Required access |
| --- | --- |
| Enter yard | Anyone while the plot is unlocked; `GuestRestricted` while locked unless guest or better |
| Enter interior | Guest or better |
| Move/place/delete decor | Owner (or `Gm` with `.housing gm on`) |
| Change house tier/blueprint | Owner |
| Set plot name / lock plot | Owner |
| See the yard's decor | Access at least `Neighbor` (the plot phase is only applied then) |
| Sell plot | Owner, or `Gm` force |
| Create/dissolve neighbourhood | Guild master / community leader, or `Gm` |

Faction gating is separate and checked first: an alliance character may not buy in a
horde neighbourhood and vice versa, matching the retail rule. Guests are exempt, which
is also the retail rule ("any member of your warband can visit and modify any house you
have bought, regardless of faction").

## 6. Plot lifecycle

```
        buy                 build tier 1
AVAILABLE  OWNED  UPGRADED (tier 2..5)
                                                
                   sell / force-evict           
    
                   (decor despawned, rows archived)
```

Invariants enforced in code, checked by `.housing debug audit`:

1. A plot index belongs to exactly one neighbourhood slot, and a slot to exactly one
   neighbourhood row.
2. `character_housing.neighborhoodId + plotIndex` is unique - a plot cannot have two
   owners even under a race between two buyers (the insert is `INSERT ... ON DUPLICATE
   KEY UPDATE` on a unique key, and the in-memory state re-reads the row before it
   announces success).
3. A house exists if and only if its plot is `Owned`, and a plot without an authored
   spawn point cannot be sold at all.
4. `decor_budget_used == SUM(budget_cost)` over that house's placed decor. Budget is
   recomputed from the rows on load, never trusted from the client.
5. Every spawned `GameObject*` in `HousingHouse` has a matching placement row, and vice
   versa after a load/despawn cycle.
6. Selling a plot despawns the decor before the row is deleted, and clears the account's
   "has house" flag before the transaction commits.

## 7. Decor: catalogue, budget, placement

The catalogue (`housing_decor`, world DB) mirrors retail decor: an id, a display name, a
game object entry to spawn, a **budget cost** (retail pieces cost 1, 3 or 5), a surface
(`Floor`, `Wall`, `Ceiling`), an indoor/outdoor flag, and where you get it (vendor, quest,
achievement, treasure hunt). Budget costs in the seed are taken from the retail values I
saw while researching: small props are 1, ordinary furniture 3, structures 5.

Budget per house tier (my numbers, tuned so a tier-1 house is a room and a tier-5 house
is the whole plot):

| Tier | Indoor budget | Outdoor budget | Unlock cost |
| --- | --- | --- | --- |
| 1 | 60 | 25 | plot purchase |
| 2 | 90 | 45 | 5 000g |
| 3 | 120 | 65 | 15 000g |
| 4 | 160 | 85 | 40 000g |
| 5 | 200 | 110 | 100 000g |

Placement flow (all server-authoritative):

1. Client sends `C:DECOR` to fetch the catalogue slice it is allowed to see.
2. Player drags a piece; client sends `C:PLACE` with decor id, position, rotation, scale.
3. Server validates: owned plot, access, decor exists, surface matches, cost fits in
   budget, position within plot radius and above terrain, scale in `[0.5, 3.0]`,
   rate limit.
4. Server inserts the row, spawns the `GameObject` in the plot phase, returns `S:PLACE`
   with the new placement id. The client never gets to decide the id.
5. `C:MOVE`, `C:REMOVE` follow the same validation path; removal despawns and deletes.

Spawns are lazy: when the first player holding a plot phase enters the plot,
`HousingHouse::EnsureSpawned()` walks its placement list and creates the objects; when
the last such player leaves, they are despawned. `HousingHouse::_spawned` is the single
source of truth for whether that has happened.

## 8. Neighbourhood lifecycle

- **Registry.** `HousingMgr::m_neighborhoods` keyed by `uint32 neighbourhoodId`, plus
  `m_freeSlots` as a bitset of 256 slots. Slot assignment happens at creation and is
  persisted, because it *is* the identity of the phases.
- **Public pool.** On demand: when a player asks to buy and no public neighbourhood has a
  free plot, the manager creates one (`INSERT` row, take a slot, name it from a name list
  in `housing_neighborhood_name`). Names are deterministic-ish for tidiness, e.g.
  "Elwynn Rest 14".
- **Guild / private.** Created by the Steward gossip from a guild (`guildId` set) or a
  charter (`ownerAccount` set). Minimum occupancy is checked at creation and re-checked
  by a periodic sweep; a private neighbourhood below minimum occupancy for
  `Housing.Private.GraceDays` days is dissolved and its plots freed.
- **Dissolution.** Plots are force-sold (full refund, decor despawned), players inside are
  teleported to the town center, row deleted, slot returned to the pool.
- **Empty slot reuse is safe** because phase ids are functions of the slot and every
  spawned object is despawned during dissolution.

## 9. Player lifecycle

| Event | Hook | What the module does |
| --- | --- | --- |
| Login | `PlayerScript::OnLogin` | Ensure a `HousingSession` exists; load owned plot id + access list lazily on first housing interaction (not on login - most logins never touch housing) |
| Update | `PlayerScript::OnUpdate` (throttled to 500 ms) | If on a housing map, resolve which plot the player is standing in and add/remove phases |
| Zone change / teleport | `PlayerScript::OnUpdateZone` + `OnMapChanged` | Clear all plot phases; if the destination is a housing map, re-resolve |
| Logout | `PlayerScript::OnLogout` | Clear phases, release the plot's `spawned` refcount, flush pending decor writes for that account |
| Addon message | `HandleChatAddonMessage` hook | `BFAH` prefix -> `HousingMgr::HandleAddonMessage` |
| Chat command | `CommandScript` | `.housing` fallback for everything the addon can do |

The plot-resolution tick is deliberately boring: for each of the 55 plots whose template
has a spawn point, compare squared distance to the player against the yard radius
squared. 55 float comparisons every 500 ms per player is nothing, and it avoids needing
area-trigger rows for plots that do not exist in the client yet.

## 10. Persistence strategy

- Reads: one `SELECT` per account at first housing interaction, one for the
  neighbourhood on slot load, one for a house's decor when the house is first spawned.
- Writes: **deferred and coalesced**. `HousingHouse::MarkDirty()` sets a flag and a
  timer; the manager flushes dirty houses every `Housing.SaveInterval` seconds (default
  30) and unconditionally on logout, shutdown and `.housing save`. A player dragging
  furniture around generates one write per interval, not one per drag.
- All statements are built against the pooled `CharacterDatabase` / `WorldDatabase` with
  `PExecute`-style formatting and `CharacterDatabase.EscapeString()` for any string that
  could contain a player-supplied value (plot names, neighbourhood names, owner names).

## 11. Failure handling

| Failure | Behaviour |
| --- | --- |
| Custom map missing / wrong map id | Module loads but every housing entry point returns `HousingResult::Disabled`; one loud error at startup |
| Decor catalogue row points at a missing GO entry | Row is skipped at load with an error log; placement of that id is refused |
| A spawned GO fails to create | Placement stays in the DB, object is skipped, error logged once per placement id, retried on next spawn |
| Player teleports out mid-edit | Tick clears phases; pending placement writes are flushed before the phase is dropped |
| Two clients equip the same decor id beyond budget | Budget is recomputed server-side before each write; second write gets `HousingResult::BudgetExceeded` |
| Neighbourhood row vanishes under a live session | All operations return `NotInNeighborhood`; the player is teleported to the town center |
| Server crash mid-session | Worst case is losing `SaveInterval` seconds of decor edits; ownership changes are written synchronously and cannot be lost |

## 12. Security notes

- Every value from the client is treated as hostile: decor ids are looked up in the
  catalogue, coordinates are range-checked against the plot, scale is clamped, and
  placement ids must belong to the requesting house.
- The addon channel is unauthenticated by nature, so the protocol does **no** trust: a
  hand-crafted `BFAH` message can do exactly as much as the UI could, no more. The opcode
  table has no "set owner" or "grant permission" entry.
- Rate limiting: 20 messages per 10 seconds per player; over the limit the request is
  answered once with `RateLimited` and then dropped (spamming must not create log spam or
  crash).
- GM commands are behind `rbac::RBAC_PERM_COMMAND_GM`; player-facing commands use
  permission `0` and are therefore available to everyone.

## 13. Deliberately out of scope for v1

- Endeavors (retail's neighbourhood-wide activity) - Phase 3.
- Dye (retail recolours decor) - Phase 3.
- Decor acquisition from quests/drops/achievements - v1 only sells from vendors and
  grants via GM command; the catalogue has a `source` column so the rest can be filled in
  without a schema change.
- Homestone as an *item*. V1 uses a command and the addon button with the same 15 minute
  cooldown. Making it a real item needs a `hotfixes` DB2 insert (`item_sparse` +
  `item_effect`), which is a separate piece of work and is documented in doc 05.
- Horde mirror. Everything is data-driven, so Razorwind Shores is "seed more rows"; the
  one thing that cannot be data is its map.
