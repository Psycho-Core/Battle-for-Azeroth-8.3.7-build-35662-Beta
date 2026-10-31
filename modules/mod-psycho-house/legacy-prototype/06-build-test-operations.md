# 06 - Build, test and operations

The practical half: how the module gets built, installed, fed with data and run. Written
so that I can follow it at 2 a.m. without re-reading the design docs.

## 1. Build wiring

Nothing needs to be added to any CMake file.

| What | How it gets picked up |
| --- | --- |
| `src/game/Housing/*.cpp` | `CollectSourceFiles()` in `src/server/game/CMakeLists.txt` globs the tree recursively |
| `src/scripts/Housing/*.cpp` | `GetScriptModuleList()` globs `src/server/scripts/*`; a directory named `Housing` becomes a script module automatically |
| Script entry point | `AddHousingScripts()` exported from `housing_script_loader.cpp` (the loader name is derived from the directory name - do not rename the file) |
| Game library symbols | Classes are marked `TC_GAME_API`, so the script module can link against `game` |

So the workflow is: drop the directories in, `cmake` again (so the globs re-run), build.

```
cmake --build build --target worldserver --config RelWithDebInfo
```

Things that will silently do nothing if I get them wrong:

- The `.cpp` files must sit directly under a globbed directory. A `src/game/Housing/impl/` subfolder is fine (recursive), but anything outside `src/server/game` or `src/server/scripts` is not.
- `AddHousingScripts()` must be a free function with C++ linkage, exactly as declared. If CMake's script graph shows `Housing` as `disabled`, the module is not linked - check the `SCRIPTS_*` variables printed at configure time.
- Re-running `cmake` after adding files is mandatory; the file glob is evaluated at configure time, not build time.

## 2. Core hook

One patch, `legacy-prototype/patches/0001-route-housing-addon-messages.md`, edits
`src/server/game/Handlers/ChatHandler.cpp` to hand `BFAH` addon messages to
`sHousingMgr->HandleAddonMessage()`. Apply it before building; without it the server half
works but the addon gets silence.

## 3. Install order

1. **World SQL** - `sql/world/2026_10_31_00_world_housing_module.sql`. Creates
   `housing_*` and seeds biomes, 55 plots, vendors, amenities and the name pool. The decor
   catalogue is created empty on purpose (sec.5).
2. **Character SQL** - `sql/characters/2026_10_31_00_characters_housing_module.sql`.
   Creates the six `character_housing_*` tables.
3. **Config** - append the `HOUSING` block from
   `src/server/worldserver/worldserver.conf.dist` to the live `worldserver.conf`, or just
   copy the keys. Every key has a safe default.
4. **Build**, then restart the worldserver.
5. **Check the log** for:
   ```
   Housing enabled: map 900 (interior 900, native mode)
   Loaded 6 biomes, 55 plots, 0 decor items, 12 vendors, 22 amenities
   Loaded 0 neighbourhoods, 0 owned plots
   ```
   If the first line is missing, `Housing.Enable` is 0 or the validation refused the map
   id (sec.8).

## 4. Configuration

| Key | Default | Meaning |
| --- | --- | --- |
| `Housing.Enable` | `0` | Master switch. Off  every entry point returns `Disabled`. |
| `Housing.Map.Mode` | `"native"` | `native` = real custom map, `anchor` = development fallback |
| `Housing.Map.Id` | `0` | Housing map id; **must exist in `Map.db2`** or the module disables itself |
| `Housing.Interior.Map` | `0` | Interior map; 0 = same as the housing map |
| `Housing.Interior.X/Y/Z/O` | `0` | Interior shell anchor |
| `Housing.TownCenter.X/Y/Z/O` | `0` | Town Center landing point; `0/0` disables `.housing leave` |
| `Housing.Phasing.PlotScoped` | `1` | Hide yard decor unless the player is inside that plot |
| `Housing.SaveInterval` | `30` | Deferred write interval, seconds |
| `Housing.RateLimit.Messages` | `20` | Addon messages per 10 s per player |
| `Housing.Homestone.Cooldown` | `900` | Homestone cooldown, seconds (retail value) |
| `Housing.Sell.RefundPercent` | `50` | Plot price refund on sale |

## 5. Authoring workflow

This is the part that turns the module from "compiles" into "playable". Do it in this
order, because every step depends on the previous one.

### 5.1 The map (sec.5 of doc 05 for the detail)

Author the terrain, add the `Map.db2` / `AreaTable.db2` rows through the `hotfixes`
database, generate `vmaps`/`mmaps`, build the client patch, then set `Housing.Map.Id` and
restart. The startup line must say `Housing enabled`.

### 5.2 Plot anchors

For each of the 55 rows in `housing_plot`:

```sql
UPDATE `housing_plot`
SET `spawn_x` = -1234.5, `spawn_y` = 5678.9, `spawn_z` = 123.4, `spawn_o` = 0.0,
    `yard_radius` = 45.0,
    `flags` = `flags` | 2          -- e.g. bridge-access only
WHERE `map_id` = 900 AND `plot_index` = 3;
```

In-game workflow I use: stand where the player should land, `.gps` (or `.debug position`),
copy the numbers. `yard_radius` is measured from that point to the plot boundary; decor is
clamped inside it, so err small rather than large until the terrain is final.

A plot with `spawn_x/y/z` still `NULL` is unbuyable and returns `InvalidPlot` - that is
the guard that stops the module selling plots it cannot place people in. The audit report
counts them:

```
.housing audit
```

### 5.3 Amenities and vendors

`.housing audit` does not cover these; they are cosmetic until authored. Use the same
`.gps` workflow against `housing_amenity` and `housing_vendor`. For vendors, also pick a
`creature_entry` that exists in the client (a human/night-elf civilian of the right look);
`0` means "not spawned", which is a valid intermediate state - the role and stock stay in
the data.

### 5.4 Authoring the decor catalogue

The catalogue ships empty. Rows need a `gameobject_entry` that actually exists in 8.3.7;
a wrong number spawns nothing and logs an error on every house load. To harvest
candidates:

```sql
-- world database
SELECT `entry`, `name`, `type`
FROM `gameobject_template`
WHERE `name` LIKE '%Chair%' OR `name` LIKE '%Bench%' OR `name` LIKE '%Table%'
   OR `name` LIKE '%Rug%'    OR `name` LIKE '%Tree%'  OR `name` LIKE '%Brazier%'
ORDER BY `name`
LIMIT 200;
```

Then insert with my cost bands - 1 for small props, 3 for ordinary furniture, 5 for
structures - and the surface/placement the piece needs:

```sql
INSERT INTO `housing_decor`
(`id`, `name`, `gameobject_entry`, `budget_cost`, `surface`, `placement`, `category`,
 `source_type`, `source_id`, `price`, `currency`, `faction_mask`, `scale_min`, `scale_max`, `flags`)
VALUES
(1001, 'Elwynn Apple Tree',  <entry>, 3, 0, 0, 'Nature',  0, 6, 75, 0, 1, 0.5, 2.0, 0),
(1002, 'Rugged Stool',       <entry>, 1, 0, 2, 'Furnishings', 0, 6, 25, 0, 1, 0.6, 1.8, 0),
(1003, 'Carved Wall Panel',  <entry>, 3, 1, 1, 'Architecture', 0, 7, 100, 0, 1, 0.5, 2.0, 0);
```

`source_id` is the vendor id from `housing_vendor` for `source_type = 0`, so 6 = Trevor
Grenner and 7 = Devin Slatesmith in the seeded data.

Then attach stock, which is what the vendor's gossip list reads:

```sql
INSERT INTO `housing_vendor_stock` (`vendor_id`, `decor_id`, `price`) VALUES (6, 1001, 75);
```

Reload without a restart:

```
.housing reload
```

### 5.5 Client addon

The harness in `addon/HousingUI/` is a normal addon folder - copy it to
`Interface/AddOns/` on the client. No patch needed. It is a transport/parser with slash
commands, not a UI; it is what I use to prove the server half before anyone draws frames.

## 6. Command reference

Player commands use permission `0`; GM commands use `rbac::RBAC_PERM_COMMAND_GM`.

| Command | Who | Effect |
| --- | --- | --- |
| `.housing list` | all | Plots in the current neighbourhood with owner/tier/price |
| `.housing info <plot>` | all | Plot detail: biome, price, spawn authored, budget, decor count |
| `.housing buy <plot>` | all | Purchase |
| `.housing sell` | all | Sell own plot (refund per config) |
| `.housing enter <plot>` | all | Teleport to a plot you may visit |
| `.housing home` | all | Homestone (cooldown) |
| `.housing leave` | all | Back to the Town Center |
| `.housing interior <enter\|exit>` | guest+ | Enter/leave a house interior |
| `.housing tier <1-5>` | owner | Upgrade |
| `.housing name <text>` | owner | Rename |
| `.housing lock <on\|off>` | owner | Lock/unlock visits |
| `.housing dir [page]` | all | Directory |
| `.housing decor [category] [page]` | all | Catalogue listing |
| `.housing buydecor <id>` | all | Unlock a decor item |
| `.housing placed` | owner | List placed decor with placement ids |
| `.housing place <decorId> <x> <y> <z> <o> <scale> <indoor>` | owner | Place |
| `.housing move <placementId> <x> <y> <z> <o> <scale>` | owner | Move |
| `.housing remove <placementId>` | owner | Remove |
| `.housing guest add <name> [flags]` / `guest del <name>` | owner | Guest list |
| `.housing audit` | GM | Invariant report |
| `.housing save` | GM | Flush deferred writes |
| `.housing reload` | GM | Reload config + world data + neighbourhoods |
| `.housing create <public\|guild\|private> <name>` | GM | Create a neighbourhood |
| `.housing dissolve <id>` | GM | Force-sell everything and delete |
| `.housing forcesell <plot>` | GM | Evict one plot |

## 7. Test matrix

I run these in this order after any change to the module. The "anchor" mode note matters:
several tests need authored plot spawns, so they run after sec.5.2.

| # | Test | Expected |
| --- | --- | --- |
| 1 | Start server with `Housing.Enable = 0` | No errors, module inert, `.housing list` says disabled |
| 2 | Enable with a bad `Housing.Map.Id` | One clear error, module disables itself, server still boots |
| 3 | Enable with the real map | Three startup log lines, `.housing audit` prints 55 plots |
| 4 | Buy a plot with plenty of gold | `ok`, money down by the price, plot shows as owned in `.housing list`, row in `character_housing` |
| 5 | Buy the same plot from a second account immediately | Second attempt gets `PlotTaken`; only one row in the DB |
| 6 | Buy with a second character on the same account | `TooManyHouses` |
| 7 | Buy second plot of the *other* faction | `WrongFaction` |
| 8 | Sell | Refund credited, decor rows gone, plot available again, house despawned |
| 9 | Enter the plot as owner | Decor spawns, phases applied (`.housing debug` or inspect phase shift) |
| 10 | Leave the plot | Decor despawns, viewer count drops to 0 |
| 11 | Second player walks in | Same decor, no duplicate spawn |
| 12 | Locked plot, stranger enters | No decor phased in, interior denied with `GuestRestricted` |
| 13 | Guest added, guest enters interior | Allowed; guest cannot place decor (`NotOwner`) |
| 14 | Place decor beyond budget | `BudgetExceeded`, nothing written |
| 15 | Place decor outside `yard_radius` | `OutOfBounds`, nothing written |
| 16 | Place decor with scale 5 | `BadScale` |
| 17 | Place decor the account does not own | `NotUnlocked` |
| 18 | Move + remove | Object despawned/respawned correctly, rows updated |
| 19 | Homestone twice within 15 min | Second gets `Cooldown` with remaining seconds |
| 20 | `.housing home` with no plot | `NoHouse` |
| 21 | Addon spam (50 messages) | After 20, `RateLimited`, no log flood, no crash |
| 22 | Restart the server mid-decor-edit | At most `SaveInterval` seconds of edits lost, no orphan rows |
| 23 | `.housing reload` with players inside plots | Phases cleared cleanly, no crash, players can re-enter |
| 24 | Two neighbourhoods, players in each | No cross-visibility of decor or players |
| 25 | `dissolve` a neighbourhood with players inside | Players moved or phased out without crash, rows gone, slot freed |
| 26 | Buy -> sell -> buy same plot | Same slot, new housing id, no leftover decor |
| 27 | `/reload` on the client mid-session | Addon re-handshakes, state re-renders |
| 28 | Malformed `BFAH` messages (empty, wrong field count, `..` paths, huge strings) | Dropped, no crash, no chat echo |

## 8. Failure modes I check in the log

| Log line | Meaning | Fix |
| --- | --- | --- |
| `Housing.Map.Id N does not exist in Map.db2` | Map row missing from the hotfix store | Add the map row, or fix the config |
| `housing_plot is empty` | World SQL not applied | Apply the world update |
| `Neighbourhood X initialised with N plots` | Plot templates missing for that map id | Check `housing_plot.map_id` matches the neighbourhood's `map_id` |
| `Decor N (...) points at missing gameobject entry X` | Bad catalogue row | Fix the entry or delete the row |
| `Failed to create decor object ...` | GO template exists but creation failed (bad displayid / model) | Fix the GO template |
| `Neighbourhood X has an invalid or duplicate slot` | Two rows share a phase slot | `.housing dissolve` one, or fix the slot |

## 9. Performance budget

| Item | Cost |
| --- | --- |
| `OnPlayerUpdate` when not on a housing map | One map id comparison per player update (the throttle check comes first) |
| `OnPlayerUpdate` on a housing map | 55 squared-distance checks every 500 ms |
| Decor spawn | One `GameObject` creation + one `AddToMap` per placement, on first entry to a plot |
| Decor despawn | Same in reverse when the last viewer leaves |
| Memory per fully decorated house | placement structs (64 bytes each) + GO objects only while spawned |
| DB writes | One `UPDATE` per dirty house per `SaveInterval`; placement writes are immediate |

The number I actually watch is **placed decor x simultaneously spawned houses**. With 55
plots at the tier-5 budget of 310 pieces each, a fully populated neighbourhood is ~17 000
game objects if every house is spawned at once. That cannot happen by accident (spawning
is viewer-driven), but it is the reason `Housing.Phasing.PlotScoped` exists and the reason
the hard cap `kMaxPlacedDecor` is 400.

## 10. Rollout order

| Phase | Scope | Gate |
| --- | --- | --- |
| 0 | Module + SQL + config, `Enable = 0` merged | Server boots, no behaviour change |
| 1 | Anchor mode on a dev realm, GM-only testing of buy/sell/place | Test matrix rows 1-28 minus the map-dependent ones |
| 2 | Custom map lands, plots + amenities + vendors authored, public neighbourhoods | Players can buy, visit, decorate; addon UI ready |
| 3 | Endeavors, dye, treasure hunts, Community Coupon stock, Horde mirror | Out of scope until 2 is stable |

## 11. Troubleshooting

| Symptom | Likely cause |
| --- | --- |
| Addon never receives `S:HELLO` | Patch 0001 not applied, or `CONFIG_ADDON_CHANNEL = 0` |
| Addon receives replies but buying does nothing | `Housing.Enable = 0`; check for `S:RESULT\|MODULE\|1` |
| `.housing list` shows every plot as available after a restart | `character_housing` rows missing (character DB not updated) |
| Decor spawns but is invisible | Plot phase not applied to the player, or `Housing.Phasing.PlotScoped` on with the player outside the yard radius |
| Players see furniture from another neighbourhood | Slot collision - `character_housing_neighborhood.slot` must be unique |
| Everything works until a crash, then houses are empty | Decor writes were pending; check `SaveInterval` and whether the crash happened during shutdown before `Unload()` |
| Player falls through the world on entering a plot | Custom map has no `vmaps`/height data at that coordinate |

## 12. Known limitations (deliberate, documented, not bugs)

- Yard decor is only visible from inside the plot when `PlotScoped` is on.
- Locking a plot denies the interior and the phasing but does not physically stop a
  determined player from walking onto the terrain; collision walls are a map-side task.
- One house per account, not per faction - with only the alliance neighbourhood
  implemented, the two rules are indistinguishable.
- No Endeavors, no dye, no crafting, no auction/trade involvement for decor.
- Interior bounds are not validated per shell yet; only implausible heights are rejected.
