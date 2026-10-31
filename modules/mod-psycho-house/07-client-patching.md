# 07 - Client patching and assets

My client is unlocked, so this is permitted territory. What follows is what I think is *worth* doing, in what order, and where the real limits are, because "unlocked client" is not the same thing as "anything is possible".

The item-by-item sequence - extract a Midnight item, convert the model, write the DB2 rows,
package the patch, verify it in game - is doc 12. This document is the budget and the
policy; doc 12 is the procedure.

## 1. What an unlocked 8.3.7 client can do

| Capability | Supported | Notes |
| --- | --- | --- |
| Add custom addons | Yes | Standard, no patch needed. The primary UI path |
| Add rows to DB2 via the server's `hotfixes` store | Yes | Maps, `AreaTable`, `Phase`, creatures, quests, items, spell names. This is how a custom map gets named and how custom items appear client-side |
| Ship new MPQ/CASC patch with ADT/WMO/M2/BLP | Yes, with an MPQ-based loader or a CASC patch | The art pipeline; heavy |
| Add new textures/icons | Yes | Cheap and high value: 20-30 icons for categories, dyes, Endeavors |
| Add new models (furniture, shells) | Yes | Requires modelling; this is the P6 bottleneck |
| Change existing model colour | Only by adding a new model/texture | See dye discussion in doc 06 sec.2 |
| Add new UI frames without an addon | No, not worth it | Addon is strictly better |
| Add new DB2 *structures* (rooms, decor catalogue) | Possible but large | Not doing it; the server-side table is enough |

## 2. The server-side hotfix path

Adding a custom map that the client can display needs, in this order:

1. **ADT/WMO** assets for the terrain and buildings.
2. `Map.db2` row (map id, directory name matching the asset directory).
3. `AreaTable.db2` row(s) for the zone and subzones (districts), because `AreaTable` drives
   the zone text, the map name, and `/who` zone labels.
4. Server-side `map`, `area_trigger`/`areatable` mirrors in the world DB.
5. `Server-side vmaps/mmaps` extraction for collision and line of sight.

The `hotfixes` table pattern in this tree (`HotfixDatabase`, `DB2Manager`) lets the server
push 2-4 through the client on login without shipping a client patch, which is how most
custom content servers add maps. Steps 1 and 5 still need real assets and the extractor.

## 3. Anchor mode: shipping without any art

This is the operational point I care most about. I designed the module so the **map is a config value** rather than an assumption:

```
PsychoHouse.Enable          = 1
PsychoHouse.MapId           = 900
PsychoHouse.InteriorMapId   = 900
PsychoHouse.AnchorMode      = 1     # place plots on hand-chosen coordinates
PsychoHouse.AnchorFile      = data/psycho_house/anchors_founders_point.txt
```

In anchor mode, plot anchors, the amenity hub and the vendor positions come from a text
file, so housing works on **any** map I pick - a borrowed zone, a garrison map, an empty
test map - with correct phasing and correct persistence. The custom map then swaps in
later by changing three config values and deleting one file.

Practical plan:

| Stage | Map | Why |
| --- | --- | --- |
| Dev | Any flat test map | Fast iteration on placement maths |
| Beta | Borrowed zone, hand-anchored 12-20 plots | Playable, no art dependency |
| v1 | Custom Founder's Point WMO/ADT + 55 anchors | Shippable |

## 4. Art budget

| Asset | Count for a credible Founder's Point | Effort |
| --- | --- | --- |
| Terrain ADT (one tile, edited from an existing zone) | 1 | Low - reuse and resculpt |
| Exterior shells (small/medium/large x 4 races) | 12 | High - each needs an interior floor plan |
| Room shells (18) | 18 | Medium - mostly modular walls/floors/ceilings |
| Amenity props (stage, beer garden, moonwell, bulletin board, mailbox stand) | ~10 | Low - many exist in 8.3.7 already |
| Custom furniture families (if the harvest pool is not enough) | 0-20 | Only if needed; the harvested catalogue is the plan |
| Icons | 20-30 | Low, very high visual value |
| **Total new models** | **~30-40** | The entire art risk sits here |

Everything else - decor, trees, tents, carts, braziers, fences - comes from the harvested 8.3.7 game object catalogue, which is the whole reason doc 03 exists.

## 5. Addon packaging

`addon/HousingUI/` (renamed to PsychoHouse when copied in) with:

| File | Contents |
| --- | --- |
| `PsychoHouse.toc` | `## Interface: 80300`, SavedVariables, file list |
| `Core.lua` | Transport, message dispatch, slash commands. **Ships today** |
| `Menu.lua` | The housing window: House, Catalogue, Placed, Neighbours, Endeavors, Blueprints, Guide tabs. **Ships today** |
| `Placement.lua` | Placement mode and gizmo - the Placed tab lists and removes, it does not drag pieces yet |
| `Permissions.lua` | Two matrices |
| `Localisation/enUS.lua` | Strings |

Rules that keep the addon debuggable:

- Every packet is a single line, pipe-delimited, `<Op>|<fields>`, never nested.
- The addon never trusts its own state: any mutation is answered by an authoritative
  refresh packet.
- Every addon action has a `.house` command equivalent, printed by `.house help`, so a
  player without the addon loses nothing.
- Rate limit is server-side (20 messages / 10 s); the addon also self-throttles to be kind
  to it.

## 6. What I will not do to the client

- No memory patching to fake a housing UI. Addons exist.
- No custom DB2 **structures** (the room catalog, the decor catalog) - the server table plus
  addon UI achieves the same result with a fraction of the fragility.
- No texture swaps on existing models to fake dye on arbitrary furniture. It breaks on
  every other use of that model in the world.
