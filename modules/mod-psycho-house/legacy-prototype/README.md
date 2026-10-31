# Player Housing - Founder's Point (Alliance)

This is my working documentation for the housing module I'm building on top of the
BfaCore Reforged 8.3.7 (build 35662) tree. It is split into two halves: the design
reference I extracted from the live game, and the implementation notes for the server
module that lives in `src/game/Housing` and `src/scripts/Housing`.

Nothing in these pages is a retail-client port. The 8.3.7 client has no concept of
player housing, so everything below is an emulation built from primitives the core
already has - instanced/phased maps, dynamic game objects, gossip, and an addon
channel for the UI.

## Contents

| Document | What it covers |
| --- | --- |
| [01 - Founder's Point reference](01-founders-point-reference.md) | The alliance neighbourhood as it exists in the live game: zone layout, the six biomes, all 55 plots, amenities, vendors, travel. This is the source of truth for the data tables. |
| [02 - Module architecture](02-module-architecture.md) | Layer map, class responsibilities, ownership model, phasing strategy, lifecycle, and the single core hook the module needs. |
| [03 - Database schema](03-database-schema.md) | World and character tables, column semantics, indexes, and why each table is where it is. |
| [04 - Addon protocol](04-addon-protocol.md) | The `BFAH` wire format the client addon and the worldserver speak: framing, chunking, every opcode, error codes. |
| [05 - 8.3.7 constraints](05-client-and-data-constraints.md) | What the client cannot do, what I have to fake, and the exact list of custom assets/environment work the module depends on. |
| [06 - Build, test and operations](06-build-test-operations.md) | Build wiring, config keys, GM tooling, test matrix, and the rollout order I'm following. |

## Source material

- Wowhead, *Founder's Point: The Alliance Player Housing Neighborhood* - Paryah,
  updated 2026-10-19 (`/guide/player-housing/alliance-neighborhood-plots-biomes-maps-founders-point`).
- Wowhead, *Player Housing Neighborhoods in World of Warcraft* - Paryah,
  updated 2026-07-15 (`/guide/systems/player-housing-neighborhoods`).
- NPC/decor pages on Wowhead and the WoWDB housing hub, used only to cross-check
  NPC entry IDs and vendor coordinates.

Where the guides do not publish a number I actually need (per-plot world
coordinates, yard radii, plot polygon sizes), the value is marked `TODO(client)` in
the SQL seed and has to come out of the custom map once it exists. I would rather
ship an obviously empty column than a plausible-looking wrong coordinate.

## Status

| Area | State |
| --- | --- |
| Reference data (biomes, plots, vendors, amenities) | Complete |
| Domain classes, persistence, addon protocol | Implemented, compiles against the tree as of this branch |
| Commands and GM tooling | Implemented |
| Dynamic decor spawning on live maps | Implemented, needs the custom map to be meaningful |
| Client addon | Protocol harness only (dev build, not a shipped UI) |
| Endeavors, dye, house tiers from retail | Phase 3+ - see the roadmap in doc 06 |
