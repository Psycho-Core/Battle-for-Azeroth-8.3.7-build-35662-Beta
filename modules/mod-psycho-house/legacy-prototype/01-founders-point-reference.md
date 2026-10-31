# 01 - Founder's Point reference

Everything I know about the alliance neighbourhood as it exists in the live game,
written down in the shape I need it for the module's data tables. This is the
document I check against before I seed a table or argue about a design decision.

Primary source: Wowhead's *Founder's Point: The Alliance Player Housing Neighborhood*
(Paryah, updated 2026-10-19) and *Player Housing Neighborhoods in World of Warcraft*
(Paryah, updated 2026-07-15). NPC entries cross-checked on Wowhead's NPC pages and the
WoWDB housing hub. Anything not published there is called out as an assumption.

## 1. Identity

| Fact | Value |
| --- | --- |
| Zone name | Founder's Point |
| Faction | Alliance |
| Wowhead zone id | `16105` |
| UI map id (used by `/way`) | `2352` |
| Horde counterpart | Razorwind Shores |
| Aesthetic reference | Elwynn Forest, Westfall, Duskwood |
| Plot count | 55, numbered `0` - `54` |
| PvP | Sanctuary - no PvP anywhere in the neighbourhood |
| Services absent | No bank, no auction house, no profession trainers |

Founder's Point is the alliance half of the retail housing system. The whole zone is a
persistent, shared village instance: your neighbours are the same people every time
you log in unless somebody moves out. It is deliberately *not* a capital city - it is
a residential district with vendors, a flight point, a Stormwind portal and a social
hub, and that is it.

The two id numbers matter to me: `16105` is the Wowhead/DB2 zone handle, `2352` is the
client UI map handle. In my seed data I keep them in separate columns because the
emulated zone will not have the same values as retail - I need the retail values
recorded to build waypoints and to look things up later, not because the 8.3.7 client
will ever see them.

## 2. Neighbourhood fundamentals

These are the rules that come from the neighbourhoods overview guide, not the zone
guide, and they drive most of the module's model:

- **Neighbourhoods are persistent instances.** Your house sits in a village that keeps
  its population between sessions. New public neighbourhoods are spun up by the server
  as existing ones fill.
- **Three administration types:** *Public* (server-created, anyone may buy, server names
  it, server picks Endeavors), *Guild*, and *Private* (chartered by a community; group
  membership required to buy, minimum group size and minimum occupancy, leader names it,
  players mostly choose the Endeavors).
- **One house per warband per faction.** A warband can own at most one alliance house
  and one horde house, so two houses total across two neighbourhoods. Any character in
  the warband may visit and *modify* any house the warband owns, regardless of faction.
  Guests of any faction may be invited to visit.
- **Your house is its own instance inside the neighbourhood instance.** The yard is part
  of the plot; the interior is separate.
- **Each neighbourhood has 55 plots** of broadly similar size, each holding one house and
  a decoratable yard.
- **Every neighbourhood carries the support cast:** a Steward, a General Contractor, a
  Dye Crafter, a Lumberjack, various decor vendors and Smugglers.
- **Endeavors** are the rotating group activity - a shared, neighbourhood-wide objective
  track with rewards.

The warband rule is the one I map most directly onto 8.3.7: warband  **account**. One
alliance plot per account, any character on that account can edit it, plus an explicit
guest list for other accounts. That is the closest available analogue and it preserves
the social contract of the original system.

## 3. Getting there

| Route | Detail |
| --- | --- |
| Stormwind portal | Permanent portal in the Stormwind portal room. This is the "walk in for the first time" route. |
| Homestone | From the Housing Dashboard. Teleports you to your own plot. **15 minute cooldown.** If you own both an alliance and a horde house you must select the right one in the dashboard first. |
| House Finder | Dashboard tool listing public neighbourhoods plus the private/guild neighbourhoods you are eligible for. Also how you *switch* neighbourhoods after buying. |
| View Houses | Right-click a friend's name -> **View Houses**. Visiting somebody else's neighbourhood. |
| Breadcrumb | Quest *A House For You* (`93057`) leads new characters to the neighbourhood the first time. |
| Flight points | One per biome (six in total). |

The Homestone's 15 minute cooldown is worth copying exactly - it is the mechanism that
stops housing from becoming a free second hearthstone for the whole server.

## 4. Town Center

The central hub. Everything here is a service, not decoration.

| Amenity | Function | Emulation hook |
| --- | --- | --- |
| Bulletin Board | Neighbourhood directory - who lives where | Gossip NPC + `C:DIR` addon opcode |
| General Contractor - **Jorvan Longmoor**, entry `255104` | Levels up your house | Gossip menu -> house tier purchase |
| Stormwind portal (north side) | Exit to Stormwind | Static portal GO at the town center |
| Endeavors area | Turn-in point for the active Endeavor, plus its vendors | Phase 3 |
| Steward | Founds guild/private neighbourhoods, next to the bulletin board | Gossip menu -> `HousingNeighborhood` creation |

Map icon key published by the guide: portal to Stormwind, bulletin board, general
contractor, flight point, plus the "your house" / "occupied house" / "available house" /
"local transportation" map symbols. For the emulated UI I reproduce the four state
symbols (mine / occupied / available / transport) as addon map-pin textures because
that is the information a player actually needs at a glance.

## 5. Vendors

Neighbourhood vendors need no reputation and no unlock; they mostly take gold. Some
stock is only available here - that is what makes the neighbourhood worth visiting
after you have your house.

| NPC | Entry | Location | Coordinates | Stock |
| --- | --- | --- | --- | --- |
| Jorvan Longmoor | `255104` | Town Center | - | General Contractor, house upgrades |
| Argan Hammerfist | `255218` | Town Center | 52.2 / 37.8 | Elven furniture - tables, benches, barrels, weapon racks (~15 items) |
| Balen Starfinder | `255216` | Town Center | 52.2 / 37.8 | Bel'ameth architecture - platforms, interior walls/doorways, pillows, rugs, bath, bookshelves (~53 items) |
| Faarden the Builder | `255213` | Town Center | 52.0 / 38.4 | Local + Stormwind decor - tents, market stands, fountains, gryphon roost, signposts (~69 items) |
| Xiao Dan | *not captured* | Town Center | 52.0 / 38.4 | Pandaren/local furnishings (~65 items) |
| Trevor Grenner | *not captured* | Town Center | 52.0 / 38.4 | Plants and nature - Elwynn apple trees, ivy, bellflowers, boulders (~43 items) |
| Devin Slatesmith | `257332` | Town Center | 52.0 / 38.4 | Carved wood and stone interior surfaces - floor/wall tiles, staircases (12 items) |
| Klasa | *not captured* | **Brumewood Hollow** - basement of the house by the graveyard entrance | - | Cobwebs and spooky accents (Ceiling/Pillar/Tented/Small Dangling Cobweb, ~10 gold each) |
| Perry Winkles | `267795` | Town Center | - | Pet decor, including extra copies of Pepe |
| Fel'orei Moonstrider | *not captured* | Town Center | - | Elven / nightborne decor |
| Holly Twinklebright | *not captured* | Town Center | - | Holiday decor (Hallow's End stock observed) |
| Breakwater Bight smugglers | - | Brumewood Hollow waterfront | - | Horde-oriented decor sold to alliance players (the "purified" Horde pieces) |

Two details I do not want to lose:

- **Klasa is the only vendor outside the Town Center.** She is in the basement of the
  house by the graveyard entrance in Brumewood Hollow. If I implement vendor placement
  as "all in the hub" I lose a deliberate exploration hook, so vendor location is a
  first-class column in the data, keyed by biome.
- **Smugglers.** Breakwater Bight is where alliance players buy Horde furniture, which
  is a faction-flavour loophole by design. The Horde mirror of the same vendor is
  `"Len" Splinthoof` in Razorwind Shores at 39.9 / 73.3 - same stock, different faction.

Currencies observed on these vendors:

| Currency | Id | Used for |
| --- | --- | --- |
| Gold | - | Nearly everything (prices observed between 10g and 150g per piece) |
| Community Coupons | `3363` | Pepe copies (10 coupons) |

**Pepe.** A decor version of Pepe is unlockable: own either the *Trans-Dimensional Bird
Whistle* (`122293`) or the *Festive Trans-Dimensional Bird Whistle* (`209859`), find
Pepe perched on the neighbourhood billboard, click him to put him on your head, and he
enters your decor collection. Extra copies cost 10 Community Coupons from Perry Winkles
(*Mechanically Indistinguishable Pepe*, `276246`). Once placed, clicking the placed Pepe
applies the 60-minute *Pepe* buff (`188246`) so he can sit on your head again. The whole
chain is a nice "why is there a bird on the notice board" moment and I want to keep it,
because it is the cheapest possible example of a decor item that is also interactive.

## 6. The six biomes

The neighbourhood is split into six named sub-zones. Each biome has its own terrain,
its own flight point and at least one landmark. All 55 plots are distributed across
them as follows.

| Biome | Plots | Count | Terrain reference | Landmarks |
| --- | --- | --- | --- | --- |
| Brumewood Hollow | 0, 2, 6, 8, 10, 15, 17, 18, 20, 21, 33, 38, 41 | 13 | Duskwood, with a dose of Drustvar | Graveyard, nightwell, mushroom ring, Breakwater Bight (smugglers) |
| Gilded Oaks | 1, 5, 13, 14, 26, 30, 35, 36, 37 | 9 | Westfall's autumn palette without the wheat fields | Beer garden with water wheel, windmills, lighthouse, ocean view |
| Mear's Farm | 7, 9, 27, 34, 39, 42, 44, 45 | 8 | Elwynn farmland | Vineyards with grape-stomping vats, orchards, cows |
| Small Aerie | 11, 16, 46, 47, 48, 49, 51, 53, 54 | 9 | Elwynn, green and lush | Gryphon aerie, flight point |
| Stoneveil Ridge | 3, 22, 23, 31, 32 | 5 | Pastoral scree and mountain | Sheep pasture, private bridges, best views |
| The Outer Banks | 4, 12, 19, 24, 25, 28, 29, 40, 43, 50, 52 | 11 | Gothic coastline | High cliffs, beachfront, private islands, strong wind |

Sum: 13 + 9 + 8 + 9 + 5 + 11 = **55**. Every index from 0 to 54 appears exactly once  - 
I verified that before writing the seed, because a missing index in a data-driven plot
system is a silent crash waiting to happen.

### 6.1 Brumewood Hollow (13 plots)

The dark side of alliance living. Heavily forested, inspired by Duskwood with a
Drustvar edge. Contains the graveyard that "any undead would feel at home in", a
nightwell and a mushroom ring. The smuggler camp at Breakwater Bight sits on its
waterfront, and Klasa's basement shop is tucked into the house by the graveyard
entrance.

Mechanically this is the biome that justifies having a *biome-scoped vendor list* at
all - two of my vendor hooks (Klasa, smugglers) exist only here.

### 6.2 Gilded Oaks (9 plots)

Autumnal, ocean-facing. Beer garden with a water wheel, windmills, the lighthouse.
Commentary from players confirms plot 13 specifically: it backs onto the apple orchard,
is secluded, ends just before the cliff edge (so no overhanging builds), and looks out
over the bay. That matches the terrain claim in the guide and is a useful sanity check
when I place the plot anchor points on the custom map.

### 6.3 Mear's Farm (8 plots)

Elwynn pastoral: green fields, cows, vineyards with grape-stomping vats, orchards.
This is the biome where "yard decor" matters most - the plots are meant to look like
working smallholdings, so the decor budget for outdoor pieces is the one I will tune
first here.

### 6.4 Small Aerie (9 plots)

Green, lush, Elwynn charm, and the gryphon aerie plus its flight point. Plots 48 and 53
are adjacent enough that two players can run decor across the shared border and read as
one large estate - players actively recommend the pair for couples. I want that to keep
working, so plots never get a hard visual wall between neighbours: the plot boundary is
a permission and phasing boundary, not a fence.

### 6.5 Stoneveil Ridge (5 plots)

The quietest biome - sheep grazing, mountain backdrop, and some plots reachable only by
their own bridge. Five plots total, the smallest biome. Good candidate for the "large
plot" pricing tier because seclusion is the product.

### 6.6 The Outer Banks (11 plots)

High cliffs, strong wind, gothic-romance atmosphere, beachfront and private-island
plots. Second-largest biome. The private islands are the one case where a plot is not
contiguous with the rest of the neighbourhood, which matters for the phasing design: a
player must be phased into their island plot *before* they land on it, not after.

## 7. Amenities and public spaces

Shared, unowned spaces that make the zone worth walking around. These are static world
objects in my implementation, not per-player content.

| Amenity | Biome | Type | Notes |
| --- | --- | --- | --- |
| Town Center | central | hub | Bulletin board, contractor, portal, Endeavors area |
| Stormwind portal | Town Center (north) | transport | One-way exit |
| Bulletin Board | Town Center | interactable | Directory + Pepe perch |
| General Contractor | Town Center | interactable | House upgrades |
| Beer garden | Gilded Oaks | gathering | Water wheel |
| Windmills | Gilded Oaks | scenery | - |
| Lighthouse | Gilded Oaks | scenery / landmark | Coast marker |
| Stage | central / social | gathering | Public events |
| Moonwell | central | scenery | Night elf flavour |
| Nightwell | Brumewood Hollow | scenery | - |
| Mushroom ring | Brumewood Hollow | scenery | - |
| Graveyard | Brumewood Hollow | scenery | Klasa's basement is by its entrance |
| Vineyards + stomping vats | Mear's Farm | scenery | - |
| Orchards | Mear's Farm | scenery | Plot 13 backs onto the apple orchard |
| Gryphon aerie + flight point | Small Aerie | transport | - |
| Sheep pasture | Stoneveil Ridge | scenery | - |
| Bridges | Stoneveil Ridge | traversal | Some plots are bridge-access only |
| Private islands / beach | The Outer Banks | plot terrain | Non-contiguous plots |
| Six flight points | one per biome | transport | Guide states each biome has one |

## 8. What this means for the module

Short version of the mapping from lore to code; the full reasoning is in doc 02.

| Retail fact | Emulation decision |
| --- | --- |
| Neighbourhood is a persistent instance | One `Map` plus a phase group per neighbourhood; plots are sub-phases |
| 55 plots per neighbourhood, fixed indices 0-54 | `housing_plot` world table keyed by `plotIndex`, seeded from sec.6 |
| Six biomes with distinct flavour | `housing_biome` world table; biome drives plot price, terrain anchor and vendor scope |
| One house per warband per faction | One plot per **account** per faction, enforced in `HousingMgr::CanBuy` |
| Any warband character can edit | Ownership is account-scoped; `ownerGuid` records the buyer for display only |
| Interior is its own instance | Second phase on the interior map, allocated per house |
| Homestone, 15 min cooldown | Player-level cooldown on the `.housing home` command and dashboard button |
| Bulletin board directory | `C:DIR` opcode + gossip list |
| Vendors mostly gold, community coupons for some | Vendor entries seeded with gold prices; coupon stock deferred to Phase 3 |
| Pepe on the notice board | Interactive GO with a one-shot collection flag, Phase 2 |
| Endeavors | Phase 3, out of scope for the first cut |
| Sanctuary, no PvP | `Housing.Zone.Sanctuary` toggles PvP flags off on entry |

## 9. Data confidence

I keep this table so that nobody - including me in three months - mistakes an
assumption for a fact.

| Item | Confidence | Basis |
| --- | --- | --- |
| Plot index -> biome mapping | High | Explicit per-biome plot lists in the guide |
| Plot count (55, 0-54) | High | Guide, stated twice |
| Vendor names, entries 255104/255213/255216/255218/257332/267795 | High | Wowhead NPC pages |
| Xiao Dan / Trevor Grenner / Klasa / Fel'orei Moonstrider / Holly Twinklebright entries | **Missing** | Only names, locations and stock published; entries must be read off the client DB2 |
| Vendor coordinates in the Town Center | High | Wowhead `/way` pins |
| Vendor coordinates for Klasa | **Unknown** | Basement of the graveyard house, no pin published |
| Per-plot world coordinates, yard radius, polygon | **Unknown** | Not published; must be authored on the custom map |
| Plot sizes ("similarly sized") | High that they are similar, **low** on exact dimensions | Guide; my 3-class pricing split is my own invention |
| Biome flight point positions | **Unknown** | Only "each biome has one" is published |
| Endeavor mechanics | Low | Separate guide, deliberately out of scope until Phase 3 |
| "Last Architect" / Cursed Keepsake acquisition flow | Low | Guide section exists and cursed->purified Horde decor is visible in the data, but the flow is not documented step by step in what I have |

The two rows that will bite anyone touching this module are the per-plot coordinates
and the missing NPC entries. Both are flagged as `TODO(client)` in the SQL seed.
