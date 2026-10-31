# 05 - 8.3.7 constraints

This document exists because the reference material in doc 01 describes a Midnight system
and I am implementing on BfA 8.3.7 build 35662. Every gap between the two is listed here
with the decision I made about it, so that "why doesn't this work like retail" has one
answer instead of forty.

## 1. What the client and core simply do not have

| Retail concept | State in 8.3.7 | Consequence |
| --- | --- | --- |
| Zone *Founder's Point* (`16105`) | Does not exist in `Map.db2`, `AreaTable.db2` or the client's world data | No geometry, no minimap, no zone name, no flight point |
| UI map `2352` | Does not exist | No world-map artwork to hang plot pins on |
| Housing DB2 stores (plots, decor, blueprints, dye) | Do not exist | No client-side catalogue, no native place/furnish flow |
| Housing UI | Does not exist | UI must be an addon |
| Decor items | Do not exist as items | Cannot be mailed, traded or stored as items |
| Warband | Does not exist | Account is the ownership unit |
| Neighbourhood/Endeavor system | Does not exist | Server-side invention |
| `HousingPlot`, `HousingDecor` etc. hotfix tables | Not in this build | All layout data lives in *my* world tables |

There is no partial support to leverage. The module is a full server-side
reimplementation with an addon front end.

## 2. Decor is a collection, not an item

The tempting design is "decor = item", which the rest of the core would handle for free
(mail, trade, auction, bank). It does not work here: a new item in 8.3.7 means rows in the
`hotfixes` database (`item_sparse`, `item`, `item_effect`, `item_appearance`, ...) *and* a
client-side patch for the icon and model resolution, and the moment a player loots one of
those items on a client without the patch, it renders as a question mark. That is a lot of
fragility for furniture.

So decor is an entry in `housing_decor` (world) plus an unlock row in
`character_housing_collection` (characters), priced in gold and delivered by a gossip
vendor. What a player "owns" is the right to place piece #1234 in their house, which is
exactly the retail model too - the item was only ever a delivery mechanism there.

Consequences I accept:

- Decor cannot be traded, mailed or refunded for gold. (Refund by removing the collection
  row is possible but I do not expose it.)
- The "collection" is account-wide, which is *better* than retail's per-item inventory and
  matches the warband-wide decor collection in Midnight.

## 3. The custom map is the one hard dependency

Without Founder's Point geometry there is nowhere to put the plots. Two operating modes
exist, and the module picks between them from configuration:

### 3.1 `native` mode (target)

A real custom map, with terrain, water, cliffs and the six biomes' landmarks.

Work required, in order:

1. **Terrain** authored in Noggit (or an equivalent ADT pipeline) at a map id we own,
   e.g. `900`. Six biome regions plus the Town Center, sized so 55 plots fit with the
   spacing described in doc 01 sec.6.
2. **`Map.db2` row** for the new map, added through the `hotfixes` database so the server
   accepts it (this core reads `Map.db2` from the hotfix store, and `.debug` map commands
   will confirm it).
3. **`AreaTable.db2` rows** for the zone and for each biome, so `GetZoneId()` returns
   something human-readable and the addon can name the biome the player is standing in.
4. **Liquid/height data + `vmaps`/`mmaps`** generated (`mapextractor`, `vmap4extractor`,
   `vmap4assembler`, `mmaps_generator`) or players fall through the world and NPC pathing
   breaks.
5. **A client patch** (MPQ/CASC) containing the ADT/`WDT`/`WMO`/textures/minimap. This is
   unavoidable: the server can invent maps, the retail client cannot render a map it does
   not have. Every player who wants housing must run the patch, so housing is opt-in at the
   launcher level on this realm.
6. **Plot anchors** measured in-game and written into `housing_plot` - `spawn_x/y/z/o`,
   `yard_radius`, `size_class`. This is the data nobody has published and the reason those
   columns ship empty (doc 01 sec.9).
7. **Amenity pins** measured the same way into `housing_amenity`.

### 3.2 `anchor` mode (fallback / development)

Runs the module against an existing 8.3.7 map area with hand-placed plot anchors - e.g. an
unused corner of Elwynn Forest or a dead instance map. Purpose: exercise buy/sell, decor,
phasing, protocol and persistence without waiting for the art pipeline.

`anchor` mode is **not** shippable content: plots sit in someone else's zone, the "biome"
distinctions are cosmetic, and there is no Town Center. It is a test harness, and the
config key says so (`Housing.Map.Mode = anchor`).

## 4. What replaces the missing client features

| Missing feature | Replacement |
| --- | --- |
| Housing UI | Addon (`BFAH` protocol, doc 04) |
| Decor catalogue in client DB2 | `C:DECOR` opcode, server-filtered by collection and faction |
| Placement gizmo | Addon-side placement mode with the server clamping position and scale |
| World map of the neighbourhood | Addon-drawn plot grid from `C:PLOTS` + amenity pins from `C:VAREA`; a real UI map needs client `UiMap.db2` rows and art, which I am not doing |
| Zone name | `AreaTable` entry from step 3 above; until then the addon falls back to "Founder's Point" from the biome row |
| Native house interior | Interior shell geometry placed off in a corner of the same map, separated by phase (doc 02 sec.4.2) |
| Warband | Account |
| Homestone item | `.housing home` command + addon button, 15 min cooldown, config-gated |
| Endeavors | Not implemented; columns reserved |
| Dye | Not implemented |

## 5. NPCs and vendors

The retail NPCs (Jorvan Longmoor, Faarden the Builder, Balen Starfinder, ...) are not in the
8.3.7 client, so their creature entries cannot be spawned as-is. Options considered:

| Option | Verdict |
| --- | --- |
| Create new creature templates for each vendor | Works, but every one needs a model that exists in 8.3.7 - I reuse existing human/night-elf models and set the name at spawn |
| Reuse existing NPC entries wholesale | Cheapest, but `.npc info` then lies about identity and gossip gets tangled with the original NPC's scripts |
| **Module-spawned vendor objects driven by `housing_vendor`** | **Chosen.** A row picks an existing creature template for appearance, the module spawns and names it, and gossip comes from our own `CreatureScript` |

The vendor's stock is `housing_vendor_stock`, and buying goes through our own gold check
plus a collection insert rather than `npc_vendor`. That keeps the housing economy entirely
inside the module and stops the retail vendor list from leaking 8.3.7 items into a
furniture shop.

Klasa stays in Brumewood Hollow's graveyard house basement and the smugglers stay at
Breakwater Bight, because vendor *placement* is data (`housing_vendor.biome_id` +
position), not code.

## 6. Client patch contents I expect to ship

| Asset | Needed for |
| --- | --- |
| ADT/WDT for the housing map | Terrain |
| WMO house shells (one per blueprint family, three footprints) | Interiors and exteriors |
| Minimap texture for the housing map | Addon map view |
| A couple of skybox/lighting overrides | Brumewood Hollow vs Gilded Oaks mood |
| Optional: flight-point taxi nodes | Six biome flight points |

Everything else - plot pins, the panel, the placement cursor - is addon-side, so it can be
iterated without a client patch.

## 7. Things I will not fake

- **No shadow economy.** Decor cannot be crafted, dropped or auctioned in v1. If demand
  exists, the right move is a profession recipe that unlocks a collection entry, not an
  item.
- **No fake zone.** I will not bolt `16105` onto an existing zone id in the world tables so
  that waypoints "work". Wrong zone ids poison every other system that reads them.
- **No client-side authority.** The addon never decides what a player owns; it renders what
  the server says. A patched addon gains nothing.
- **No silent teleports.** `C:ENTER` and `C:HOME` are server-executed teleports with the
  same validation as everything else; there is no client-side `SetMapToCurrentZone` trick.
