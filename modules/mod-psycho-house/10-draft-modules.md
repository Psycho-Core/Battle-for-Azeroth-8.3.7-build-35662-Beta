# 10 - The two draft modules

I built the housing system twice on purpose. One draft is the port: retail content, retail
prices, retail item ids, every piece of it traceable to a Midnight vendor page. The other
is the native draft: nothing from retail survives, and every id in it already exists in
8.3.7 build 35662 or in my own source tree. They share one C++ core and one schema, so
they are the same mod with two content packs, not two codebases.

```
drafts/
  midnight-port/     draft A - retail content on an 8.3.7 server
  bfa-native/        draft B - 8.3.7 content only, no port
```

## 1. What the two drafts share

Both drafts copy the same trees out of the module:

| Tree | Files | What it does |
| --- | --- | --- |
| `src/game/Housing/` | 14 | Plots, houses, decor placement, phasing, persistence, the addon protocol |
| `src/scripts/Housing/` | 4 | `.house` commands, player login hook, world config reload, loader |
| `addon/HousingUI/` | 2 | The client panel: plot list, catalogue, placement, blueprint codes |
| `drafts/*/sql/world/01_schema.sql` | 1 | Housing tables, identical in both drafts |
| `drafts/*/sql/world/02_world_content.sql` | 1 | Six biomes, 55 plot templates, 22 amenity pins, the name pool |
| `drafts/*/sql/characters/01_characters_schema.sql` | 1 | Ownership, placement, collection, guests, neighborhood membership |

`tools/housing/build_drafts.py` copies the code and the two shared SQL files into a draft
every time it runs, so the two `src/` trees cannot drift. If I change a C++ file, I re-run
the builder and both drafts get it.

## 2. Draft A - `midnight-port`

The retail port. This is the draft that answers "make it look like Midnight".

| Piece | Value |
| --- | --- |
| Vendors | 39, each with the retail npc id, faction and stock count in its row comment |
| Decor rows | 571, every one carrying a real 8.3.7 game object |
| Stock lines | 969 |
| Retail item ids on those rows | 450 rows carry one; between them they cover all 439 distinct ids in the catalogue |
| Currency | gold, Community Coupons 3363, and Tricky Treat item 33226 |
| Gated stock | 90 stock rows sit on six vendors that lock pieces behind Endeavor milestones: Den, Pascal, Griftah, Harlowe, Aeeshna and Hordranin. Hordranin's eight gates are recorded per item in the CSV note column; the other five carry a vendor-level count. The draft does not enforce gating yet - that is phase 3 |
| Unpriced lines | 87, listed as comments at the end of the stock file and deliberately not inserted |
| Unmapped pieces | none: `tools/housing/map_decor_go.py` maps every piece to a real entry and records how sure the match is |

Design rule: the spawn is a separate decision from the catalogue, which is why the mapping
lives in its own CSV with a confidence column instead of being baked into the SQL. A row
can be listed, priced and owned while its stand-in is still the third-best chair in the
game; the moment a better candidate exists, `data/town-center-go-map.csv` is edited and
the draft regenerated. Of the 618 mapped names, 189 are keyword hits, 171 are family hits
(chair to chair), 223 are category defaults and 35 are last-resort fallbacks.

## 3. Draft B - `bfa-native`

The from-scratch build. No retail ids at all.

| Piece | Value |
| --- | --- |
| Decor rows | 350 game objects that already exist in my tree |
| Vendors | 8 stalls of my own design |
| Stock lines | 350, every piece sold somewhere |
| Currency | gold only |
| Names | the constant in the SQL, replaced at load from `gameobject_template.name` |
| Vendor models | bound from `creature_template` by subname, never by a guessed npc id |

The 350 are what is left of the 767 decor-shaped game object constants after dropping
doors, gates, traps, spawn markers, spell visuals and instance props. The filter lives in
`tools/housing/build_drafts.py` as a single tuple of words, so the catalogue is a policy
decision I can change and re-generate in seconds.

## 4. Side by side

| Question | Draft A | Draft B |
| --- | --- | --- |
| Needs retail data | yes, by definition | no |
| Needs client patches | for art, yes | no |
| Runs on a clean 8.3.7 install | yes, every row already has an 8.3.7 stand-in game object | yes, immediately |
| Number of decor pieces | 571 | 350 |
| Number of vendors | 39 | 8 |
| Item ids referenced | 439 distinct retail ids on 450 rows | 0 |
| Currency | gold + 3363 + item 33226 | gold |
| Endeavor gating | yes, 4 milestones | no |
| House levels | 12, retail budgets | 6, my budgets |
| Dyes and pet beds | phase 4 | not planned |
| Map mode default | `native` | `anchor` |
| Best use | proving the retail experience end to end | shipping something today |

## 5. Installing one of them

```
python3 tools/housing/install.py --draft midnight-port --root /path/to/source
python3 tools/housing/install.py --draft bfa-native --root /path/to/source --dry-run
```

The installer detects which script layout the tree uses:

- **Layout A** - the tree globs a root `modules/` directory. The draft becomes
  `modules/PsychoHouse/`, and the root glob has to exist before CMake will look there, so
  run the CMake patch once:

  ```
  python3 tools/housing/patch_cmake.py --root /path/to/source --dry-run
  python3 tools/housing/patch_cmake.py --root /path/to/source
  ```

  It widens `GetScriptModuleList` in `cmake/macros/ConfigureScripts.cmake` to include
  `<source>/modules/*`, keeps the original as `.bak-cmake-patch`, and does nothing on a
  second run. The folder is `PsychoHouse`, not `mod-psycho-house`, because the loader
  generator builds `Add<folder>Scripts()` out of the directory name and a hyphen is not a
  legal identifier.
- **Layout B** - the BfaCore Reforged layout, where script modules are directories under
  `src/server/scripts/` and the loader function name has to match the directory name. The
  draft becomes `src/server/scripts/PsychoHouse/`, the loader is renamed to
  `psycho_house_loader.cpp`, and the export is renamed to `AddPsychoHouseScripts()`.

Both layouts place the six SQL files under `sql/updates/world/` and
`sql/updates/characters/` with a `2026_10_31_1<seq>` stamp, and both rename the addon's
`.toc` to match the folder name, since the client pairs them.

In both layouts `src/server/game/Housing/` gets the core classes, because a script module
cannot add game-layer classes on this core. That is the one part of the install that is a
core patch rather than a module drop, and doc 05 phase 0 says so.

Existing files are moved aside as `<name>.bak-install` rather than overwritten, so a bad
install is one `mv` away from undone.

## 6. Switching between drafts

Both drafts use the same table names and the same `id` spaces:

| Range | Owner |
| --- | --- |
| 1 - 99 | Shared world content: biomes, plots, amenities |
| 101 - 199 | Draft A vendors and decor |
| 201 - 208 | Draft B vendors |
| 1000 - 1999 | Draft A decor |
| 2000 - 2999 | Draft B decor |

The two content packs can therefore coexist in one database. Load both, set
`Housing.Enable = 1`, and the catalogue shows retail names next to native pieces; the
unmapped retail rows are flagged so a player can see which ones are still waiting on art.
To go back to one pack, delete the other pack's id range - the DELETE statements are
already at the top of each generated file.

## 7. Rebuilding after a data change

```
python3 tools/housing/build_drafts.py
```

That reads the three CSVs, rewrites six SQL files across the two drafts, and re-copies the
C++ and the addon. Everything it writes carries a generated-file header, so the rule is
simple: edit the CSVs or the C++, never the generated SQL.
