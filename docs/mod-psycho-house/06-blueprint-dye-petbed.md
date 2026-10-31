# 06 - Blueprints, dyes, pet beds and room shells

The four systems that need real design rather than a table of numbers. All four are
verified against the tree: every API named here exists in this 8.3.7 source.

## 1. Rooms as shells

Retail lets you tile arbitrary rooms into a floorplan. 8.3.7 has no room renderer, so a
"room" in mod-psycho-house is a **shell**: a pre-authored piece of interior geometry
(WMO or a cluster of GOs) with a fixed footprint that the server positions on a grid.

### 1.1 Data

```sql
psycho_house_room
  id, name, go_entry,        -- the shell object
  foot_x, foot_y,            -- footprint in yards
  cost,                      -- room placement budget cost
  level_required,
  offset_x, offset_y, offset_z,   -- where the shell's origin sits relative to its grid cell
  wall_style, floor_style,
  connects_north/south/east/west  -- doorway mask
```

### 1.2 Placement rules (server-side, identical for UI and commands)

1. Every room occupies an integer grid cell (4x4 yards) or a set of cells.
2. A new room must touch at least one existing room on a side whose `connects_*` bit is set
   on **both** rooms, or be the first (anchor) room.
3. Rooms may not overlap cells.
4. Total room placement **cost** must fit the level's room budget (19 at level 1 -> 134 at
   level 12, doc 01 sec.3.1).
5. Every room must remain reachable from the anchor (flood fill over the adjacency graph).
6. Rooms can be removed only if removal does not orphan another room.

That is roughly the retail floorplan rule set as described publicly ("what rooms fit and
ensure each room can be accessed"), expressed as a graph problem on a small grid - cheap
and deterministic.

### 1.3 What the player experiences

- Choosing a room shows its footprint and cost.
- Once placed, the shell spawns as a GO in the interior phase for that house.
- Interior decor is placed in world space like any other decor; the shell is just scenery.
- A "stairwell" shell teleports the player to a second interior band (a different
  `interiorAnchor` z), giving multi-floor buildings without engine support.

### 1.4 The honest limitation

Because shells are pre-authored, two players' houses will share silhouettes. Mitigations:
several shells per shape and size (Square Tiny/Small/Medium/Large, Octagon, Circle,
Cross, L, T, Hallway, Closet, 3 stairwells = 18 starter shells, retail's 12 plus my own),
wall/floor style variants per shell (`SetDisplayId` on a style GO), and decor doing the rest.
That is the same trick retail players use with interior structures anyway.

## 2. Dyes

### 2.1 The constraint, stated plainly

A game object's colour comes from its model and texture. 8.3.7 cannot recolour a model at
runtime. What it **can** do is change which display the object uses:

```cpp
// src/server/game/Entities/GameObject/GameObject.h:289
void SetDisplayId(uint32 displayid);
```

So a dye is only real where a **recoloured display already exists** in the client. Lots of
furniture does: same model, several ID variants (wood tones, cloth colours, metal tints).
My catalogue can therefore expose dyes on exactly those families, and honestly refuse them
elsewhere - which is also retail's behaviour ("not all decor can be dyed").

### 2.2 Data

```sql
psycho_house_dye            -- 9 groups, retail parity
  id, name, icon, item_entry, craft_spell_alchemy, craft_spell_inscription

psycho_house_dye_color       -- 77 colours across the 9 groups
  id, dye_id, name, display_ids   -- comma list of GO displays this colour can switch to
```

`display_ids` is the honest mechanism: a colour is only offered for a decor piece if the
piece's family has a display for it. Where no display exists, the colour is greyed out with
"not available for this piece" rather than silently doing nothing.

### 2.3 Applying

1. Player picks a colour in the addon -> `C:DYE|<placementId>|<colorId>`.
2. Server validates: piece is dyable, colour exists, family supports it, player has a dye
   item (or the dye is in the collection), budget unaffected.
3. Server consumes one dye, updates `character_housing_decor.dye_color`, and calls
   `SetDisplayId(newDisplay)` on the live object (or respawns it).
4. Persisted per placement, exported in blueprints.

### 2.4 Acquisition

Dyes are crafted by **Alchemy** and **Inscription**, exactly as retail. On 8.3.7 that is a
custom recipe per dye group: spell + `spell_recipe`/`skill_line_ability` + reagents. 9
recipes is a small patch. Dyes are tradeable (no soulbound flag), which gives the AH a new
commodity - probably the single best economy side-effect in this whole project.

### 2.5 What I will say in the UI

"Colours available for this piece: 5 of 5 in Black; 0 of 6 in Green." Not a fake colour
picker that does nothing.

## 3. Blueprints (import/export codes)

Retail generates a code client-side for a room, a build or an exterior and imports it
within budget. Since the client is 8.3.7, **the server generates and parses the code**, and
the addon is a dumb transport.

### 3.1 Wire format

```
PCH1-<kind>-<faction>-<payload>-<checksum>
```

| Field | Values |
| --- | --- |
| `PCH1` | magic + version |
| `kind` | `R` room, `B` build, `E` exterior |
| `faction` | `0` any, `1` alliance, `2` horde (builds and exteriors only) |
| `payload` | base62 of a zlib-packed bitstream |
| `checksum` | 4 base62 chars, CRC32 of the payload |

### 3.2 Payload layout

```
header:   u8 roomCount, u8 shellId, u16 worldAnchorX, u16 worldAnchorY
repeat roomCount:
          u8  roomId,          // index into psycho_house_room
          i8  cellX, i8 cellY  // relative to anchor, signed
repeat placementCount:
          u16 decorIdDelta,    // delta-encoded vs previous, sorted ascending
          i16 x, i16 y,        // centimetres, relative to the build anchor
          i16 z,
          u8  rotZ,            // 0..255 -> 0..360
          u8  scale,           // 0..255 -> 0.5..3.0
          u8  flags,           // indoor bit, dye slot present bits
          u8  dyeColor         // optional
```

Delta encoding + 2-byte coordinates keeps a 300-piece build at roughly 3-4 KB raw,
**~1.2-1.6 KB compressed**, which is 8-10 addon messages of 200 bytes. Acceptable.

### 3.3 Import validation

Same validation path as a live placement, per piece, plus:

- Every decor id must exist and be owned by the importer.
- Total cost must fit the importer's budgets (interior vs exterior).
- All coordinates must land inside the plot or floorplan.
- Faction rule for `B`/`E`; `R` codes are faction agnostic.
- Dyes listed in the code must be owned or the import refuses with the list of missing dyes
  (retail 12.1.5 behaviour).

### 3.4 Storing and sharing

`psycho_house_blueprint(id, accountId, kind, faction, code, name, created)`. The code is the
canonical artefact; sharing is just pasting the string in chat, Discord or mail. Because it
is deterministic text, it also survives database wipes - a player's build is portable
outside the realm, which retail's codes are not (they are account-bound).

## 4. Pet beds

Retail: 100 beds indoors, 25 outdoors, one pet per bed, Stationary or Roaming indoors,
Stationary only outdoors. 8.3.7 can do all of that with creature spawns.

### 4.1 Implementation

1. A bed is a **decor-like object** of its own kind (`psycho_house_pet_bed` rows) placed
   through the normal placement flow, with a `petEntry` field.
2. On assignment, spawn a `Creature` from the pet's `creature_entry` (from
   `BattlePetSpecies`, which 8.3.7 has) in the plot's phase:

```cpp
Creature* pet = map->SummonCreature(entry, pos, nullptr, 30000, ownerGuid);
pet->SetHomePosition(pos);
pet->GetMotionMaster()->MoveRandom(6.0f);   // Roaming
```

   Stationary: skip `MoveRandom`, and freeze facing.
3. Cleanup on: unassign, bed removal, house despawn, plot sell, logout of the last viewer.
4. Caps enforced per house: 100 indoor, 25 outdoor.
5. Pets are affected by the plot phase, so a neighbour cannot see another player's cats  -
   which is also true in retail.

### 4.2 Edge cases I have to handle

| Case | Handling |
| --- | --- |
| Pet is also a hunter/warlock pet | Player's own summoned pet is a different system; never touch it |
| Pet creature has combat/AI | Spawn with no faction hostility inside a sanctuary map, passive AI, no loot, no XP |
| Duplicate named pets | Allowed, matches retail |
| Bed removed while pet is out | Despawn the creature first |
| Realm restart | Pets are not persisted as creatures; they respawn from the bed+assignment rows on next load |

## 5. Permissions matrix (supporting design for doc 02 sec.2.5)

Two independent spaces, five audiences, evaluated on the housing tick:

```
plotMask  : who may stand on the yard
houseMask : who may enter the interior
audience  : anyone | neighbors | guild | party | friends | guests
```

Stored as two `TINYINT`-per-audience bitfields on `character_housing`. Evaluation order:
owner -> GM -> guest list -> audience match. Re-evaluated every 500 ms on the housing tick;
on loss of permission the player is teleported to the plot's front door (plot) or outside
the house (interior). That reproduces retail's "ejected, no cooldown, checked frequently".

The prototype's per-house guest table survives as the explicit "guests" audience, which is
how Pepe-style invites and "book club at 8" work in practice.
