# 03 - Database schema

Two databases are involved, split the way the core already splits them:

- **World** - everything that describes Founder's Point as *content*: biomes, plot
  layout, the decor catalogue, vendor placement, amenity pins. Read once at startup,
  reloadable with `.housing reload`.
- **Characters** - everything that is *state*: neighbourhoods, who owns which plot,
  what is standing in their house, guest lists, membership, the decor collection.

DDL lives in `sql/world/2026_10_31_00_world_housing_module.sql` and
`sql/characters/2026_10_31_00_characters_housing_module.sql`.

Naming: I use `housing_*` for world and `character_housing_*` for character tables, which
is the convention the surrounding tables already use (`character_garrison*`,
`character_battleground_data`, ...). No table touches an existing row of any core table;
the module is installable on a live database.

## 1. World tables

### 1.1 `housing_biome`

One row per biome. Uniquely identifies a faction's biome and carries the map handle plus
the flight point that every biome is supposed to have.

| Column | Type | Notes |
| --- | --- | --- |
| `id` | `INT UNSIGNED` PK | 1-6 Founder's Point, 100-series reserved for Razorwind Shores |
| `name` | `VARCHAR(64)` | "Brumewood Hollow" |
| `description` | `TEXT` | Flavour text shown in the addon panel |
| `faction` | `TINYINT UNSIGNED` | 0 alliance, 1 horde |
| `zone_id` | `INT UNSIGNED` | Retail zone id (`16105`) |
| `ui_map_id` | `INT UNSIGNED` | Retail UI map id (`2352`) |
| `map_id` | `INT UNSIGNED` | **Emulated** map id the module actually uses |
| `flight_x/y/z/o` | `FLOAT` | Biome flight point |
| `center_x`, `center_y` | `FLOAT` | Biome centroid, for the addon map view |
| `flags` | `INT UNSIGNED` | Bit 0: sanctuary, bit 1: no-fly |

Only `map_id` has any effect on the 8.3.7 client; the retail ids are kept for reference
and for any future tooling that wants to line data up with Wowhead.

### 1.2 `housing_plot`

The 55 plot templates. This is the table that doc 01 sec.6 seeds.

| Column | Type | Notes |
| --- | --- | --- |
| `map_id` | `INT UNSIGNED` | Part of the PK |
| `plot_index` | `TINYINT UNSIGNED` | 0-54, part of the PK |
| `biome_id` | `INT UNSIGNED` | -> `housing_biome.id` |
| `size_class` | `TINYINT UNSIGNED` | 0 small, 1 standard, 2 large - my pricing split, not a retail value |
| `price` | `INT UNSIGNED` | Gold |
| `spawn_x/y/z/o` | `FLOAT` | Plot "front door" anchor, where the player lands |
| `yard_radius` | `FLOAT` | Radius of the yard trigger and of the decor bound |
| `flags` | `INT UNSIGNED` | Bit 0: island plot, bit 1: bridge access only, bit 2: no outdoor decor |
| `comment` | `VARCHAR(255)` | Free text, e.g. "backs onto the apple orchard" |

`PRIMARY KEY (map_id, plot_index)`, `KEY (biome_id)`. Composite PK rather than an
auto-increment id, because the plot index is meaningful - it is the index the player sees
on the map and the index the addon protocol uses.

`spawn_*` and `yard_radius` are seeded as `NULL`/`0` because the retail guides do not
publish per-plot coordinates. They must be authored on the custom map (doc 05). Until
then the module runs in `anchor` mode, which refuses to open plots whose spawn point is
unset rather than teleporting anybody to the origin.

### 1.3 `housing_decor`

The catalogue. One row per decor item - this is my equivalent of retail's housing decor
DB2.

| Column | Type | Notes |
| --- | --- | --- |
| `id` | `INT UNSIGNED` PK | Module-internal decor id |
| `name` | `VARCHAR(128)` | Display name |
| `gameobject_entry` | `INT UNSIGNED` | GO template spawned when placed |
| `budget_cost` | `TINYINT UNSIGNED` | 1, 3 or 5 - mirrors retail cost bands |
| `surface` | `TINYINT UNSIGNED` | 0 floor, 1 wall, 2 ceiling, 3 any |
| `placement` | `TINYINT UNSIGNED` | 0 outdoor, 1 indoor, 2 both |
| `category` | `VARCHAR(32)` | "Furnishings", "Nature", "Accents", "Architecture" |
| `source_type` | `TINYINT UNSIGNED` | 0 vendor, 1 quest, 2 achievement, 3 drop, 4 treasure hunt, 5 GM only |
| `source_id` | `INT UNSIGNED` | NPC / quest / achievement entry for display |
| `price` | `INT UNSIGNED` | Gold, when `source_type = 0` |
| `currency` | `INT UNSIGNED` | 0 = gold, otherwise currency id (`3363` = Community Coupons) |
| `faction_mask` | `TINYINT UNSIGNED` | Bit 0 alliance, bit 1 horde |
| `scale_min`, `scale_max` | `FLOAT` | Clamp for the client's scale slider |
| `flags` | `INT UNSIGNED` | Bit 0: interactive (Pepe-style), bit 1: no floor clipping, bit 2: hide in catalogue |

Deliberately **not** an item. Decor in my module is an unlock in
`character_housing_collection`, not an item in a bag. That decision removes the need to
touch the `hotfixes` DB2 tables entirely (see doc 05 sec.2) and it matches how retail treats
decor: it is a collection, not inventory.

### 1.4 `housing_vendor` and `housing_vendor_stock`

The retail vendor NPCs do not exist in 8.3.7 - Faarden the Builder and friends are
Midnight-era creatures. So a vendor row is "an NPC I want, at this spot, selling this
list, wearing this existing creature template's appearance".

| `housing_vendor` | Type | Notes |
| --- | --- | --- |
| `id` | `INT UNSIGNED` PK | Module vendor id |
| `name` | `VARCHAR(64)` | "Faarden the Builder" |
| `biome_id` | `INT UNSIGNED` | 0 = Town Center, else the biome (Klasa is biome 1) |
| `role` | `TINYINT UNSIGNED` | 0 decor vendor, 1 contractor, 2 steward, 3 smuggler, 4 pet, 5 holiday |
| `creature_entry` | `INT UNSIGNED` | Existing 8.3.7 creature template used for appearance; 0 = gossip only, not spawned |
| `x/y/z/o` | `FLOAT` | World position |
| `retail_name` | `VARCHAR(64)` | Name of the retail counterpart, for the record |
| `comment` | `VARCHAR(255)` | e.g. "basement of the graveyard house" |

| `housing_vendor_stock` | Type | Notes |
| --- | --- | --- |
| `vendor_id` | `INT UNSIGNED` | Part of PK |
| `decor_id` | `INT UNSIGNED` | Part of PK, -> `housing_decor.id` |
| `price` | `INT UNSIGNED` | Overrides `housing_decor.price` when non-zero |

`PRIMARY KEY (vendor_id, decor_id)`. Stock is therefore explicit (a decor can be sold by
two vendors at different prices) while the item's own default price stays in
`housing_decor`.

### 1.5 `housing_amenity` and `housing_neighborhood_name`

- `housing_amenity` (`id`, `biome_id`, `name`, `type`, `x/y/z/o`, `radius`, `flags`)  - 
  the map pins from doc 01 sec.7: bulletin board, portal, beer garden, lighthouse, moonwell,
  nightwell, mushroom ring, graveyard, aerie, stage, windmills, vineyards. `type` uses the
  same enum the addon renders pins from.
- `housing_neighborhood_name` (`id`, `faction`, `name`) - the pool public neighbourhoods
  are named from, e.g. "Elwynn Rest", "Hollowmere", "Oakhollow". Names are drawn without
  replacement until the pool is exhausted, then suffixed with a number.

## 2. Character tables

### 2.1 `character_housing_neighborhood`

| Column | Type | Notes |
| --- | --- | --- |
| `id` | `INT UNSIGNED` PK | Module-allocated, see sec.2.7 |
| `type` | `TINYINT UNSIGNED` | 0 public, 1 guild, 2 private |
| `name` | `VARCHAR(64)` | |
| `faction` | `TINYINT UNSIGNED` | |
| `map_id` | `INT UNSIGNED` | |
| `slot` | `SMALLINT UNSIGNED` | **Phase slot, `UNIQUE`** - the identity of the phases |
| `guildId` | `BIGINT UNSIGNED` | 0 for non-guild |
| `ownerAccount` | `INT UNSIGNED` | Charter owner for private, 0 otherwise |
| `created` | `INT UNSIGNED` | Unix time |
| `endeavorId` | `INT UNSIGNED` | Reserved, Phase 3 |
| `endeavorProgress` | `INT UNSIGNED` | Reserved, Phase 3 |

`UNIQUE KEY (slot)` is the important one: two neighbourhoods sharing a slot would share
phases and see each other's furniture. The manager also keeps the slot set in memory, but
the unique key is what protects against a restart with a stale in-memory set.

### 2.2 `character_housing`

One row per owned plot - and, because a house only exists on an owned plot, one row per
house.

| Column | Type | Notes |
| --- | --- | --- |
| `id` | `INT UNSIGNED` PK | The "housing id" used by decor, guests and the protocol. **Module-allocated**, not `AUTO_INCREMENT` |
| `neighborhoodId` | `INT UNSIGNED` | -> `character_housing_neighborhood.id` |
| `plotIndex` | `TINYINT UNSIGNED` | 0-54 |
| `ownerAccount` | `INT UNSIGNED` | The actual owner; warband  account |
| `ownerGuid` | `BIGINT UNSIGNED` | Character that bought it, for display |
| `ownerName` | `VARCHAR(32)` | Cached for the directory, so it does not need a join |
| `tier` | `TINYINT UNSIGNED` | 1-5 |
| `blueprint` | `INT UNSIGNED` | Shell / style id |
| `name` | `VARCHAR(64)` | Player-chosen plot name, escaped on write |
| `locked` | `TINYINT UNSIGNED` | 1 = strangers cannot enter the yard |
| `budget_indoor` | `INT UNSIGNED` | Cached, recomputed on load |
| `budget_outdoor` | `INT UNSIGNED` | Cached, recomputed on load |
| `purchase_time` | `INT UNSIGNED` | |
| `last_edited` | `INT UNSIGNED` | |

`UNIQUE KEY (neighborhoodId, plotIndex)`, `KEY (ownerAccount)`, `KEY (ownerGuid)`.

The unique key on the plot is the double-buy guard (doc 02 sec.6, invariant 2). "One plot
per account per faction" is *not* a schema constraint because the faction lives two tables
away; it is enforced in `HousingMgr::CanBuy` and audited by `.housing debug audit`, which
is the right split - the schema guarantees the hard invariant, the code guarantees the
policy one.

### 2.3 `character_housing_decor`

| Column | Type | Notes |
| --- | --- | --- |
| `id` | `BIGINT UNSIGNED` PK | Placement id, **module-allocated** (see below) |
| `housingId` | `INT UNSIGNED` | -> `character_housing.id` |
| `decorId` | `INT UNSIGNED` | -> `housing_decor.id` |
| `x`, `y`, `z`, `o` | `FLOAT` | Plot-relative-to-world coordinates |
| `scale` | `FLOAT` | Clamped on write |
| `surface` | `TINYINT UNSIGNED` | Copied from the catalogue at placement time |
| `indoor` | `TINYINT UNSIGNED` | 0 yard, 1 interior |
| `placed` | `INT UNSIGNED` | Unix time |

`KEY (housingId)` - every read is "give me this house's furniture".

### 2.4 `character_housing_collection`

`PRIMARY KEY (accountId, decorId)`, plus `source_type`, `source_id`, `acquired`.

Account-scoped, not character-scoped, because decor is a warband-wide collection in
retail: buying a rug once means all your characters can place it. One row per unlocked
decor item per account.

### 2.5 `character_housing_guest`

`PRIMARY KEY (housingId, accountId)`, plus `flags` (bit 0 enter, bit 1 edit, bit 2
manage-guests) and `added`.

Guests are per-house, not per-neighbourhood: permission to walk into my living room does
not grant permission to walk into my neighbour's.

### 2.6 `character_housing_neighbor_member`

`PRIMARY KEY (neighborhoodId, accountId)`, plus `rank` (0 member, 1 officer, 2 leader)
and `joined`.

Only guild/private neighbourhoods need rows here; public neighbourhoods are open and are
not enumerated. The neighbourhood object loads its member list on creation and keeps it
in memory.

## 2.7 Why the ids are module-allocated

`character_housing.id`, `character_housing_neighborhood.id` and
`character_housing_decor.id` are **not** `AUTO_INCREMENT`. The module needs the id
*after* the insert, in the same call, and this core has no `GetLastInsertId()` on the
pooled `DatabaseWorkerPool` - `LAST_INSERT_ID()` would have to run on the same connection
as the insert, which the async pool does not guarantee.

So `HousingMgr` keeps three counters (`_nextHousingId`, `_nextNeighborhoodId`,
`_nextPlacementId`), initialises each from `SELECT MAX(id) + 1` at startup, and writes the
id explicitly. All allocation happens on the world thread, so there is no race, and the
counter survives restarts because it is derived from the table.

If you ever add rows by hand, use an id above the current maximum or the next allocation
will collide with your `PRIMARY KEY`.

## 3. Size and growth expectations

| Table | Row count on a busy server | Growth |
| --- | --- | --- |
| `housing_plot`, `housing_biome`, `housing_amenity`, `housing_neighborhood_name` | ~70 rows total | static |
| `housing_decor` | low hundreds, grows with content patches | slow |
| `character_housing_neighborhood` | 1 per ~40 houses | linear with population |
| `character_housing` | 1 per house | linear |
| `character_housing_decor` | budget-capped at 310 rows per house (200 indoor + 110 outdoor) | bounded |
| `character_housing_collection` | decor rows x active accounts | the biggest table; expect millions on a large realm |

The only table whose growth worries me is `character_housing_collection`, and it is a
narrow PK with two extra small integers, so it stays cheap. If it ever matters, the
`acquired` column can be dropped and the PK kept.

## 4. Migrations I am deliberately not doing

- **No `ALTER TABLE` on core tables.** No new columns on `characters`, no new `rbac_*`
  rows (the GM commands reuse `RBAC_PERM_COMMAND_GM`, player commands use permission `0`),
  no touching `creature_template`.
- **No archive tables in v1.** Selling a plot deletes its house and decor rows. An
  `character_housing_archive` copy would be the obvious next step for "I want my old
  layout back" support, and the schema is designed for it (decor rows are self-contained
  apart from `housingId`).
