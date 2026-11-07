# 06 - Blueprints, dyes, pet beds and room shells

These are the four systems I have to design properly rather than tabulate. I checked all four against the tree: every API named here exists in this 8.3.7 source, and I have not named one that does not.

## 1. Rooms as shells

Retail tiles arbitrary rooms into a floorplan. 8.3.7 has no room renderer, so a
"room" in mod-psycho-house is a **shell**: a pre-authored piece of interior geometry
(WMO or a cluster of GOs) with a fixed footprint that the server positions on a grid.

### 1.1 Data

```sql
housing_room
  id, shell_id,              -- the shell object it places
  name,
  x, y, z,                   -- offset from the interior anchor
  radius,                    -- the room the player must stand in to decorate it
  budget_cost,               -- rooms come out of the room budget
  level_required
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

That is the published retail floorplan rule ("what rooms fit and ensure each room can be accessed") rewritten as a graph problem on a small grid. I chose that shape because it is cheap to validate and deterministic, so two servers given the same blueprint build the same house.

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

So I only sell a dye where a **recoloured display already exists** in the client. Lots of
furniture does: same model, several ID variants (wood tones, cloth colours, metal tints).
My catalogue can therefore expose dyes on exactly those families, and honestly refuse them
elsewhere - which is also retail's behaviour ("not all decor can be dyed").

### 2.2 Data

```sql
housing_dye                  -- the colour words a dyable piece's name starts with
  id, name

housing_decor_variant        -- one row per decor, group and colour
  decor_id, dye_group, dye_color, dye_name, gameobject_entry
```

The variant row is the honest mechanism: a colour is offered for a piece only when the
variant table has a game object entry for it. Where no row exists the colour is not listed
for that piece at all, rather than silently doing nothing.

The palette is therefore a word list, not a copy of retail's. Retail has 77 colours in nine
groups, because its client has a dye system; here a colour exists when the realm's game
objects carry that word in front of the piece's own name. The draft seeds six words - Red,
Blue, Green, Yellow, White, Black - and `bind_decor_variants.sql` fills the variant table from
whatever words are in `housing_dye`, so a realm that adds a seventh word gets its variants
without editing the script. Six is not seventy-seven, and the sensible reading is that the
palette is as wide as the build's own furniture names are.

### 2.3 Applying

1. Player picks a colour in the addon -> `C:DYE|<placementId>|<colour> [group]`.
2. Server validates: the piece has a variant row for that group and colour, and the caller
   may edit the plot. No dye item is consumed - `housing_dye` is a word list, not a
   consumable, and charging one would need an item id per colour that the source data does
   not carry.
3. Server writes `dye_group`, `dye_color` and `variant_entry` to
   `character_housing_decor`, then despawns the standing object and respawns it as the
   variant entry. A dye that only took effect on the next login would read as a failure.
4. Persisted per placement, exported in blueprints.

### 2.4 Acquisition

A dye change costs gold - `Housing.Dye.Cost`, default 0. Re-applying the colour the piece
already has is free, because charging for a click that changes nothing is a tax on the player
who read the menu twice.

Retail's dyes are crafted by **Alchemy** and **Inscription**, tradeable, and sold on the
auction house. That economy is the one part of the dye system this port does not have, and
the reason is an id rather than an opinion: an 8.3.7 dye item needs an `item_template` row and
a `spell_template` row per colour, and the build has no housing dyes to reuse. Inventing
seventy-seven item ids and seventy-seven spell ids so a player can hand over a reagent is a
worse trade than a price the realm sets, so the sink is gold and the bookkeeping is one
column.

The alternative - no sink at all - is what most servers would ship, and it makes dyeing a
free action a player does once per piece and never thinks about again. A price, even a small
one, is what turns it into a decision.

### 2.5 What I will say in the UI

"Colours available for this piece: 5 of 5 in Black; 0 of 6 in Green." Not a fake colour
picker that does nothing.

## 3. Blueprints (import/export codes)

Retail generates a code client-side for a room, a build or an exterior and imports it
within budget. Since the client is 8.3.7, **the server generates and parses the code**, and
the addon is a dumb transport.

### 3.1 Wire format

```
PHP1:<base62 payload>
```

| Field | Values |
| --- | --- |
| `PHP1:` | magic + version, in ASCII, followed by a colon |
| payload | every byte of section 3.2, base62-encoded as one big integer |
| checksum | the last byte of the payload, `(sum * 31 + byte) & 0xFF` over the bytes before it |

The payload carries the kind and the faction as its second and third bytes, so the code does
not repeat them in text. Import and export are both implemented and both server-side:
`BlueprintCodec::Encode` / `Decode` in `HousingBlueprint.cpp`, reached by `C:BPEXP` and
`C:BPIMP` and by `.housing bpexp` / `.housing bpimp`. The addon validates the envelope -
header, alphabet, length - and leaves the contents to the server, because a format with two
implementations is a format with two bugs.

`H.Crc32` and `H.Base62` in `addon/HousingUI/Core.lua` were the earlier client-side checksum
idea; they remain for the CRC32 reference value (CRC32 of `123456789` is `CBF43926`) and are
no longer on the blueprint path.

### 3.2 Payload layout

Version 2, which is what `HousingBlueprint.cpp` writes. A version 1 code still decodes; the
tilt simply comes back as zero, because a code a player saved before the tilt existed is not a
mistake.

```
header:   u8 version,     // 2
          u8 kind, u8 faction,
          u16 placementCount
repeat placementCount:
          varint decorId,
          i16 x, i16 y,        // centimetres, relative to the build anchor
          i16 z,
          u8  rotZ,            // 1.40625 degrees per step
          u8  scale,           // 0..255 -> 0.1..5.1
          u8  flags,           // indoor bit, surface in bits 1-2
          u8  tilt             // version 2 only: pitch in the low nibble, roll in the high
```

The tilt nibbles are 12 degrees apart over -90 to +90, so both axes fit in the byte the piece
already had room for and a 300-piece build grows by 300 bytes. The decoder requires the whole
record - ten bytes per piece on version 2, nine on version 1 - which it did not always do: the
earlier guard counted only the three coordinates and then read six more bytes.

No compression: a 300-piece build is about 3.3 KB of base62, which is 17 addon messages of
200 bytes. Acceptable, and it means a truncated paste is a length error rather than a
half-inflated stream.

### 3.3 Import validation

Same validation path as a live placement, per piece, plus:

- Every decor id must exist and be owned by the importer.
- Total cost must fit the importer's budgets (interior vs exterior).
- All coordinates must land inside the plot or floorplan.
- Faction rule for `B`/`E`; `R` codes are faction agnostic.
- Dyes listed in the code must be owned or the import refuses with the list of missing dyes
  (retail 12.1.5 behaviour).

### 3.4 Storing and sharing

`character_housing_blueprint(id, accountId, name, kind, faction, code, created)`. The code is the
canonical artefact; sharing is just pasting the string in chat, Discord or mail. Because it
is deterministic text, it also survives database wipes - a player's build is portable
outside the realm, which retail's codes are not (they are account-bound).

## 4. Pet beds

Retail: 100 beds indoors, 25 outdoors, one pet per bed, Stationary or Roaming indoors,
Stationary only outdoors. 8.3.7 can do all of that with creature spawns, and this is what the
module now does - the caps are `Housing.PetBed.IndoorCap` and `Housing.PetBed.OutdoorCap`,
enforced when a bed is placed.

### 4.1 Implementation

1. A bed is **decor with a creature attached**, not a table of its own: the row in
   `housing_decor_creature` says which creature the piece shows when nobody has chosen, and
   the placement row carries `pet_entry` and `pet_roaming` for the companion that was.
2. On assignment, spawn a `Creature` from the pet's `creature_entry` (from
   `BattlePetSpecies`, which 8.3.7 has) in the plot's phase. `HousingMgr::AssignPet` takes the
   creature the player currently has summoned, then confirms through
   `sBattlePetSpeciesStore` and `Player::GetBattlePetCountForSpecies` that the species is in
   the player's journal - a creature entry alone proves nothing, because a spell can summon
   anything. The mode and the entry are stored on the placement rows (`pet_entry`,
   `pet_roaming`), so the bed comes back the way it was left:

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

Two independent spaces, six audiences, evaluated on the housing tick:

```
plotMask  : who may stand on the yard     default 0x3F (everyone)
houseMask : who may enter the interior    default 0x3E (everyone but a stranger)
audience  : anyone | neighbors | guild | party | friends | guests
```

Stored as two `TINYINT` masks on `character_housing`, one bit per audience. Evaluation order:
owner -> GM -> guest list -> `locked` -> audience match. Re-evaluated every 500 ms on the
housing tick; on loss of permission the player is teleported to the plot's front door (yard)
or outside the house (interior). That reproduces retail's "ejected, no cooldown, checked
frequently", and `SetAccess` also calls the check directly so a change made in the UI applies
before the next tick.

Two asymmetries are deliberate and are written down in doc 14 sec.5: the guest bit is always
in the mask because a mask without it is a house nobody can be invited to, and the friends
audience falls back to the visitor's own list when the owner is offline, because the owner's
list cannot be read then.

The prototype's per-house guest table survives as the explicit "guests" audience, which is
how Pepe-style invites and "book club at 8" work in practice.

## 6. The Hearthlight Armillary

Retail's Armillary is a level-6 device that changes a house's ambience: five presets, and the
lighting of every placed piece moves with them. A server can act on two axes - whether a
piece spawns as its lit variant, and whether the sky is forced clear for the player - so the
five presets are mapped onto those two axes and the mapping is written down rather than
implied:

| Preset | Client number | Pieces | Sky |
| --- | --- | --- | --- |
| Default | 0 | as authored | as the realm has it |
| Natural Light | 1 | as authored | clear |
| Unnatural Dark | 2 | lit variants | as the realm has it |
| Evening Calm | 3 | lit variants | clear |
| Midnight | 4 | lit variants | as the realm has it |

Unnatural Dark and Midnight differ only in the number the client receives. That is a
limitation of what `Weather` exposes: `Weather::SetWeather` takes a type and a grade, but a
`Weather` object belongs to a zone and is owned by the world updater, which resets it on its
next tick. The one state a script can hand a single player without fighting the updater is
fine weather, through the static `Weather::SendFineWeatherUpdateToPlayer`. Inventing a third
axis to pretend otherwise would be worse than a documented collision.

- **State**: `character_housing.armillary`, one tinyint, so the preset survives a relog and is
  the same for everyone who walks in. `Housing.Armillary.Level` gates it, and `CanEdit`
  decides who may change it.
- **Lit pieces**: `housing_decor.night_entry`. When a dark preset is active, `SpawnPlacement`
  sets the placement's night entry and the piece spawns as that game object. A dye wins over
  a lit variant: a dyed piece is a colour the player chose, and a lit twin for that exact
  colour does not exist, so a dyed piece stays dyed when the Armillary goes dark.
- **Binding the twins**: `tools/sql/bind_night_variants.sql` pairs a piece with the realm's
  own game object of the same name and type with `Lit ` in front (or behind). Nothing is
  guessed; a catalogue whose realm has no lit twins keeps `night_entry` at 0, and the dark
  presets then change the world state only.
- **Telling the client**: world state 61202 carries the preset, `S:PLOTINFO`'s seventeenth
  field carries it as well so the window lights the right button, and `.housing armillary
  <0-4>` sets one directly.
