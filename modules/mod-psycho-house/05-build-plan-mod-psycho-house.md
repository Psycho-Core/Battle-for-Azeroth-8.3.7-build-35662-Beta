# 05 - Build plan: from prototype to mod-psycho-house

The engineering plan. Inputs: the prototype that already compiles and runs
(`src/game/Housing`, `src/scripts/Housing`, `addon/HousingUI`), the retail
spec in doc 01, and the port decisions in doc 02.

## 1. Definition of done

mod-psycho-house v1.0 is done when, **on a clean 8.3.7 build 35662 realm with the module
SQL applied and no GM intervention**:

1. A fresh account is offered the intro chain and ends with a house at level 2.
2. A player can buy, decorate, name, lock, move and sell a plot, and nothing is lost on
   restart, crash, or logout mid-edit.
3. Two neighborhoods run concurrently on the same map with zero cross-visibility of
   players or decor.
4. Decor is buyable from at least three vendors and placeable indoors and outdoors, with
   budgets enforced from the retail level table.
5. House levels 1-12 exist with retail numbers; XP comes from decor and Endeavors; the
   General Contractor applies levels.
6. One Endeavor can be run end to end, including vendor unlock and the coffer.
7. The addon is optional: every action has a `.house` or gossip path.
8. `.house audit` is clean on a populated realm (no invariant violations, no orphans).
9. No core file other than `ChatHandler.cpp` (one hook) is modified.

## 2. Phases

| Phase | Scope | Exit gate | Est. |
| --- | --- | --- | --- |
| P0 | Rename + restructure, config under `PsychoHouse.*`, keep behaviour | Prototype feature set still passes its own test matrix after rename; server boots | 2-3 evenings |
| P1 | Ownership parity + travel + level table + catalogue from harvest | Buy, move, sell, level 1-5 budgets, 3 vendors, 200+ decor entries, homestone, portal rooms | 2 weeks |
| P2 | Addon UI v1, permissions matrix, gossip paths, interior shells v1 | Playable without a GM; a stranger can be blocked from both plot and house independently | 3 weeks |
| P3 | Endeavors engine + the six Endeavors + coffer + themed vendors | One Endeavor completable on a small neighborhood; coffer once per warband | 4 weeks |
| P4 | Storage/duplicates, pet beds, weekly level-10 reward, guild neighborhoods | Duplicates enforced; pets live in plots; guild hoods by roster | 3 weeks |
| P5 | Dye system (limited), blueprint codes | Dyes work on variant-authored pieces; export/import round-trips | 3-4 weeks |
| P6 | Art pass: custom housing map, exteriors, rooms art | Founder's Point geometry shipped in a client patch | art-bound |

Total code effort to P5: **14-18 weeks of evenings**, assuming one developer and no art
dependency. P6 overlaps and is the only phase that cannot be compressed by working harder
at code.

## 3. Phase 0 - the rename and the skeleton

Do this as one commit before any feature work, so diffs stay readable.

### 3.1 Directory moves

```
src/game/Housing/               -> src/game/PsychoHouse/
src/scripts/Housing/            -> src/scripts/PsychoHouse/
addon/HousingUI/                -> addon/PsychoHouse/
```

Inside the housing folder those four trees are already in place. What has to move on a
live source tree is the copy into the build: `src/game/PsychoHouse` into
`src/server/game/PsychoHouse`, `src/scripts/PsychoHouse` into
`src/server/scripts/PsychoHouse` (or, on a tree that globs a root `modules/` directory,
the whole draft folder into `modules/`), the addon into `Interface/AddOns`, and the two
SQL files into `sql/updates/world` and `sql/updates/characters`. `tools/housing/install.py`
does all four moves.

`src/server/scripts/PsychoHouse/housing_script_loader.cpp` becomes
`psycho_house_loader.cpp` exporting **`AddPsychoHouseScripts()`** - the function name is
derived from the directory name by `ConfigureScriptLoader()` in
`src/server/scripts/CMakeLists.txt`, so renaming the directory without renaming the export
silently disables the whole module. CMake must be re-run (the glob is configure-time).

### 3.2 Symbol renames

| From | To |
| --- | --- |
| `namespace Housing` | `namespace PsychoHouse` |
| `sHousingMgr` | `sPsychoHouse` |
| `HousingMgr` | `PsychoHouseMgr` |
| `Housing::kAddonPrefix = "BFAH"` | `PsychoHouse::kAddonPrefix = "PCHO"` |
| Config `Housing.*` | `PsychoHouse.*` |
| Tables `housing_*` / `character_housing_*` | `psycho_house_*` / `character_psycho_house_*` |
| Protocol version 3 | 4 (breaking: new opcodes, renamed prefix) |

Table renames are painful on a live realm. Two options, both fine:

- **Greenfield realm**: drop the prototype tables, apply the new schema, move on.
- **Live realm**: keep the physical table names, only rename the C++ constants, and note the
  mismatch in the schema doc. Ugly, zero risk. I would take this for anything with data
  worth keeping.

### 3.3 What must not change

- The single core hook in `HandleChatAddonMessage` (new prefix, same shape).
- Account-as-warband ownership.
- Phases as the isolation mechanism.
- Server-authoritative placement validation.
- Deferred writes.

Those five decisions are the reason the prototype works; the port is additive.

## 4. Phase 1 - ownership parity and the catalogue

### 4.1 Schema work

Add (see doc 02 sec.4): `housing_house_level`, `housing_room`, `housing_shell`,
`character_housing_storage`, `character_housing_archive`. Change `character_housing.tier`
to `level` + `xp`.

Migration order:

```sql
-- 1. create new tables
-- 2. ALTER character_housing ADD level, xp; backfill level from tier (tier -> level, same 1..5)
-- 3. leave tier in place for one release, then drop
```

### 4.2 Level table

Insert retail's numbers verbatim (doc 01 sec.3.1) - this is the one place I copy retail exactly,
because budgets are what make builds feel like retail builds. `xp_total` is cumulative
(10, 1200, 2400 ...), `interior`, `rooms`, `exterior`, and an `unlocks` text field the UI
renders.

### 4.3 Catalogue generation

Run `tools/housing/harvest_decor.py` against the world DB, review the generated SQL, then:

```bash
mysql world < sql/generated/housing_decor_2026_11_01.sql
```

Then attach stock: for each vendor in `housing_vendor`, insert
`housing_vendor_stock` rows filtered by category  -
e.g. Trevor Grenner (`Nature`), Devin Slatesmith (`Structural`), Argan Hammerfist
(`Furnishings`), Xiao Dan (`Accents`), Klasa (`Accents` + cobwebs), Holly Twinklebright
(holiday), Brumewood smuggler (Horde families). Prices from the harvest's suggested price
band, +/-, tuned after a pass.

### 4.4 Portal rooms and Steward

- Two static GO portals (Stormwind Wizard's Sanctum, Orgrimmar Pathfinder's Den) with
  `AreaTriggerScript` or `GameObjectScript` -> `TeleportToNeighborhood`.
- Steward NPC at each portal: gossip *take me to Founder's Point*, *find me a plot*,
  *I want to found a neighborhood*.
- Both are DB rows + one small script file.

### 4.5 Cornerstone

One GO per plot (spawned by the module at plot anchor + offset, so it works even before the
map has art). Gossip states:

| State | Options |
| --- | --- |
| Unclaimed plot | *Buy this plot for 1 000g*, *Move my house here* (if already owning a plot in this neighborhood) |
| Own plot | *Rename*, *Lock/unlock*, *Move house*, *Relinquish* |
| Someone else's | *This plot is not for sale* (and the owner's name, if they have not locked it) |

## 5. Phase 2 - addon UI v1

The addon is the single biggest player-facing piece and the easiest to under-estimate. v1
is deliberately narrow:

- **Dashboard frame** (keybind `H`): house card (level, XP bar, budgets, homestone button),
  plot card (biome, address, lock state), neighbourhood card (name, residents count).
- **Catalogue**: filter by category/subcategory, search by name, "collected / not
  collected", buy button. No 3D previews (addons cannot render GO models) - use the GO's
  own icon where the client can resolve it, else a placeholder glyph by category.
- **Placement mode**: enter, move, rotate (3 axes), scale, place, undo last; budget meter
  live; red highlight when out of bounds (server tells the client after each move).
- **Finder**: list of neighborhoods with free plots, plot index + biome + price.
- **Permissions**: two matrices (plot / house) x audiences (anyone, neighbors, guild,
  party, friends) with instant apply.

Anything not in that list goes to P3+.

## 6. Phase 3 - Endeavors

Engine first, content second:

1. `psycho_house_endeavor` + task tables (doc 04 sec.4.1).
2. Progress storage: `character_psycho_house_endeavor` for claims; neighborhood-level
   progress in `psycho_house_neighborhood.endeavorProgress`.
3. Hook points, each one function call:
   - `PlayerScript::OnQuestComplete` -> `EndeavorMgr::OnQuest`
   - `PlayerScript::OnCreatureKill` -> `OnKill`
   - `PlayerScript::OnLoot`/loot hook -> `OnLoot`
   - `SpellScript`/`PlayerScript::OnSuccessfulSpellCast` (crafts) -> `OnCraft`
   - pet battle finish -> `OnPetBattle`
4. Scaling formula, Favor worldstate, hub vendor phase-in, coffer.
5. Milestone gating: `housing_endeavor_reward.milestone` (1-4) filters vendor stock, matching
   retail's "Reach the Nth milestone" gate on roughly a third of coupon-vendor items.
6. Then write the six Endeavor definitions as pure SQL. This is where the *time* goes, and
   it is content design, not engineering.

## 7. Phase 4 - storage, pets, guilds

- **Storage**: `character_psycho_house_storage(accountId, decorId, count)`. Placing decrements,
  removing increments, buying increments; the collection row only gates *permission*.
  This is the retail "four chairs are four chairs" rule and it doubles as a gold sink.
- **Pet beds**: bed GO with `spellclick`/gossip -> assign a pet; spawn the pet's creature in
  the plot phase with a small roam radius; despawn on unassign or house despawn. Caps 100
  indoors / 25 outdoors.
- **Guild neighborhoods**: `GuildScript::OnMemberRemove` -> evict + archive; GM-only
  creation gossip; ranks mapped to neighborhood ranks.

## 8. Phase 5 - dyes and blueprints

Both are specified in doc 06. Both are "nice to have" in retail terms; neither blocks v1.0
if the priority table in doc 02 is honest about P3.

## 9. Phase 6 - art

Doc 07. The module runs (in `anchor` mode) without any of it; art converts the system from
"works" to "shippable".

## 10. Test gates

Run the prototype's 28-row matrix (legacy docs, `06-build-test-operations.md`) plus:

| # | Gate | Phase |
| --- | --- | --- |
| 29 | 200+ catalogue rows load, no "missing gameobject entry" errors | P1 |
| 30 | Budget maths matches the level table exactly at every level | P1 |
| 31 | Homestone cooldown survives logout/login | P1 |
| 32 | Move house keeps every placement id and dyed colour | P1 |
| 33 | Stranger blocked from yard, allowed into house (independent permissions) | P2 |
| 34 | Permission change ejects a player within 1 tick | P2 |
| 35 | Addon reload mid-placement does not corrupt state | P2 |
| 36 | Endeavor completion unlocks vendors only in that neighborhood | P3 |
| 37 | Coffer cannot be looted twice by two characters of one account | P3 |
| 38 | Deleting a guild member archives their house and frees the plot | P4 |
| 39 | Two identical decor pieces must be owned to place two | P4 |
| 40 | Blueprint import cannot exceed budget or place out of bounds | P5 |

## 11. Risks and mitigations

| Risk | Impact | Mitigation |
| --- | --- | --- |
| GO templates in 8.3.7 are visually wrong for "furniture" (many are world props with odd pivot/origin) | Placed pieces float or clip | Placement offset per decor row (`offset_x/y/z`, `pivot_z`), authored/auto-tuned; the harvest tool emits a first guess |
| Addon channel throughput (255 byte messages) | Laggy catalogue, slow bulk operations | Chunking (already), plus server-side paging and a 20 msg/10 s rate limit |
| Custom map never lands | Housing lives in a borrowed zone | Anchor mode is a first-class mode, not a hack; keep it working forever |
| Dyes impossible for most pieces | Retail parity gap | Ship "variant" dyes; say so in the UI instead of pretending |
| Endeavor content is a treadmill | Player boredom after month 2 | Six at launch with monthly rotation and scaled targets; add one per patch |
| Table rename on live realm | Data loss | Option to keep physical names (sec.3.2) |
| One developer | Scope creep kills the project | The P0/P1/P2 ordering, and the "definition of done" list, are the contract |

## 12. What I do this week

1. Rename (P0) and re-baseline the test matrix.
2. Run the harvest tool, hand-review 100 rows, apply them.
3. Insert `housing_house_level` with retail numbers and switch the prototype's 5-tier
   budget code to read from it.
4. Spawn the two Steward NPCs and the portal GOs; wire the intro quest 1.
