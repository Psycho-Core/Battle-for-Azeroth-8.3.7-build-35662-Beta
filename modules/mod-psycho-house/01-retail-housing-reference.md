# 01 - Retail housing, complete reference

Everything I know about the shipped housing system, with numbers. This is the
specification mod-psycho-house is measured against; doc 02 then decides what of it is
buildable on 8.3.7 and doc 04 decides what of *that* ships.

Confidence markers are the ones I use everywhere: **[F]** published fact with a number,
**[?]** uncertain or unpublished, **[D]** my decision.

## 1. Unlock and getting started

| Item | Value |
| --- | --- |
| Expansion required | Midnight **[F]** - the system is gated on expansion ownership |
| Can non-owners collect decor? | Yes, decor can be collected without the expansion **[F]** |
| Intro quest | *A House For You*, quest id **93057** - auto-offered on first login **[F]** |
| Quest chain | *A House For You* -> *My First Home* -> *Welcome Home* -> *Time to Decorate* **[F]** |
| Remaining quest ids in the chain | **unknown** - not published anywhere I could find **[?]** |
| Starting cost of a house | **1,000 gold**, one time, no upkeep, no lottery **[F]** |
| Houses per warband | **2** - one per faction neighborhood **[F]** |
| Buyer requirement | The first purchase must be made by a character of the matching faction **[F]** |
| After purchase | Any warband character may visit, decorate and live in either house, any faction **[F]** |
| House slots per plot | 55 plots per neighborhood **[F]** (Blizzard's dev blog says "roughly 50") |
| Starter exterior | *Small Human House* (Alliance), *Small Orc House* (Horde) **[F]** |
| Extra starter exteriors | *Small Night Elf House* / *Small Blood Elf House* **[F]** |
| Alts | Warband-wide decor collection and XP from learning decor **[F]** |

## 2. Travel

| Route | Detail |
| --- | --- |
| Alliance portal room | *Wizard's Sanctum*, Stormwind -> Founder's Point portal, attended by Steward **Lyssabel Dawnpetal** **[F]** |
| Horde portal room | *Pathfinder's Den*, Orgrimmar -> Razorwind Shores portal, attended by Steward **Tocho Cloudhide** **[F]** |
| Homestone | Housing Dashboard button, **15 minute cooldown**, teleports to the *selected* house **[F]** |
| Dashboard keybind | **H** by default **[F]** |
| Routing from other hubs | Dornogal / Valdrakken / Oribos -> capital portal -> housing portal **[F]** |
| Moving house | Free, instant, keeps interior and exterior exactly as built **[F]** |
| Relinquishing | House state is saved and can be "unpacked" on a later purchase; a regret timer allows a move back before the plot is resold **[F]** |
| Eviction | Being removed from a private neighborhood packs the house up too **[F]** |

## 3. House leveling

House levels run 1-12 and behave like Renown. XP sources:

| Source | XP |
| --- | --- |
| Learning a **new** decor item (first time only, not shop/promo, not duplicates) | **10** **[F]** |
| Endeavor tasks | varies **[F]** |
| Looting the Endeavor Coffer when the neighborhood Endeavor completes | **250**, once per warband per Endeavor **[F]** |

XP from learning decor applies to **both** houses; Endeavor XP only to the house whose
Endeavor the earning character has active **[F]**. A level-up is not automatic: it is
applied by visiting the **General Contractor** **[F]**.

### 3.1 The full level table

Interior decor budget, room budget, exterior budget and unlocks per level. Exterior
values are the post-12.0.7 numbers (200 / - / 250 / - / 300 / - / 350 ...).

| Lvl | XP req. | Interior | Rooms | Exterior | Unlocks |
| --- | --- | --- | --- | --- | --- |
| 1 | 0 | 910 | 19 | 200 | Initial Room Collection, exterior decoration |
| 2 | 10 | 1155 | 24 | - | Square Room (Medium) |
| 3 | 1 200 | 1450 | 30 | 250 | Daylight Circle Room, +50 exterior |
| 4 | 2 400 | 1745 | 36 | - | Octagon Room (Large) |
| 5 | 3 700 | 2050 | 43 | 300 | Square Room (Large), Evening Circle Room |
| 6 | 5 700 | 2360 | 50 | - | **Hearthlight Armillary** |
| 7 | 7 900 | 3180 | 68 | 350 | +10 rooms, +500 decor, +50 exterior |
| 8 | 10 300 | 3500 | 76 | - | **Medium-size house exteriors** |
| 9 | 12 900 | 4340 | 95 | - | +10 rooms, +500 decor |
| 10 | 15 750 | 4675 | 104 | - | *Treasures from Distant Lands* - weekly Arcanostorage Sphere from the Last Architect |
| 11 | 18 850 | 5545 | 124 | - | +10 rooms, +500 decor |
| 12 | 22 200 | 5975 | 134 | - | **Large-size house exteriors** |

Total interior growth: 910 -> 5975. Room budget: 19 -> 134. Exterior: 200 -> 350.

### 3.2 Initial Room Collection (12 rooms, all at level 1)

Square Room (Small), Square Room (Tiny), Hallway, Closet, Octagon Room (Medium), Octagon
Room (Small), Cross-Shaped Room, L-Shaped Room, T-Shaped Room, Stairwell Left, Stairwell
Right, Stairwell Empty **[F]**.

Later unlocks: Square (Medium) @2, Daylight Circle @3, Octagon (Large) @4, Square (Large)
and Evening Circle @5. Every room has its own **room placement cost**; bigger rooms cost
more **[F]**.

### 3.3 Exterior sizes

Small exteriors at level 1, **Medium at level 8**, **Large at level 12** **[F]**.

## 4. Customization systems

| System | Behaviour |
| --- | --- |
| **Basic Mode** | Snap-to-grid placement for fast layouts **[F]** |
| **Advanced Mode** | Free placement, free rotation on three axes, scaling (some pieces cannot scale) **[F]** |
| **Customize Mode** | Dye UI, pet bed assignment, other per-item settings **[F]** |
| Exterior customization | Barber-shop style: base + roof style, roof colour, windows, wall fixtures, towers, facades **[F]** |
| Exterior kits | Human, Night Elf (Alliance only); Orc, Blood Elf (Horde only); neutral kits in either **[F]** |
| Interior vs exterior geometry | **Fully detached** - "houses are dimensionally transcendental"; interior footprint is not the exterior footprint **[F]** |
| Floorplan | Rooms tile together with rules about what fits and what must remain reachable **[F]** |
| Decor limits | Interior and exterior budgets are separate; decor cannot be placed outside the plot or floorplan **[F]** |

## 5. Decor

| Item | Value |
| --- | --- |
| Placement cost per piece | **1 to 5** **[F]** |
| Starting budget (launch-era guide text) | 300, raised by leveling **[F]** - an early figure; the shipped per-level table in sec.3.1 is what the game uses |
| Categories | Furnishings (Seating, Tables & Desks, Storage, Beds, Misc), Accents (Ornamental, Wall Hangings, Food & Drink, Floor, Misc), Structural (Doors, Walls & Columns, Windows, Large Structures, Misc), Lighting (Large, Wall, Ceiling, Small, Ambient, Misc), Nature (Large Foliage, Small Foliage, Bushes, Ground Cover, Misc), Functional (Utility, Misc), Miscellaneous **[F]** |
| Collection scope | **Warband-wide** **[F]** |
| Copies | Decor items are individual objects - four chairs are four chairs; a piece placed in the Alliance house is not available to the Horde house **[F]** |
| Duplicates | Usually repurchasable from a vendor once unlocked; raid/dungeon drops usually are not **[F]** |
| Where decor comes from | Quests, dungeons/raids, reputations, achievements, vendors, professions, events, in-game shop, promotions, PvP, treasure hunts **[F]** |
| Rarity tiers used by guides | Commodities (everyday), Investment (animated/particle pieces needing an unlock), Trophies (prestige) **[F]** |
| Achievement | *Fully Furnished* (id **61318**) for 500 uncommon+ decor, title **The Furnished** **[F]** |
| Collection storage | Owned pieces sit in a decor storage; the catalogue shows how many copies the player holds **[F]** |

### 5.1 Functional decor

**Hearthlight Armillary** - decor id 14583. Interior-only ambience control with five
presets: Default, Natural Light, Unnatural Dark, Evening Calm, Midnight. Set it while
placed, remove it to make the choice stick, or leave it placed for a session-only mood.
Sold additionally by **The Last Architect** (Founder's Point npc **248854**, Razorwind
Shores npc **253596**) **[F]**.

**Pet Beds** - assign a collected battle pet to a bed; Stationary or Roaming indoors,
Stationary only outdoors. Limits: **100 beds inside, 25 outside**, one pet per bed, no
duplicate pet ownership required. Starter items *Westfall Pet Cage* (**277142**) and
*Crossroads Pet Cage* (**277144**). Additional styles from **Perry Winkles** (267795) and
**Agratha** (267794) **[F]**.

### 5.2 Dyes

| Item | Value |
| --- | --- |
| Colours | **77**, in **9** dye groups **[F]** |
| Model | One dye of a group colours one dye slot with any colour in that group (e.g. one Black dye -> Dark Iron, Dark Obsidium, Darkwood, Obsidium Black or Stormheim Gray) **[F]** |
| Consumption | Dyes are **single use** per dye slot **[F]** |
| Slots | Each dyable piece has one or more dye slots; preview freely, Apply to commit **[F]** |
| Not dyable | Some decor cannot be dyed; pet beds are not dyable because their UI is the customization menu **[F]** |
| Acquisition | Crafted by **Alchemy** and **Inscription**; also drops and the Auction House; tradeable, not soulbound **[F]** |
| NPC teachers | **Helmi Cooper** (255126) / **Haleth Turnwater** (255125), Dye Crafters standing at the Dye Station in every neighborhood **[F]** |

## 6. Neighborhoods

| Item | Value |
| --- | --- |
| Zones | **Founder's Point** (Alliance), **Razorwind Shores** (Horde) **[F]** |
| Plots | **55** per neighborhood, same size, one house each **[F]** |
| Plot layout | Solo hermits, 2-3 house cul-de-sacs, larger groupings **[F]** |
| Persistence | Persistent instances; neighbours stay neighbours unless someone moves **[F]** |
| Type: Public | Created as needed by servers, semi-procedural names, game picks Endeavors, anyone can buy **[F]** |
| Type: Guild | GM creates; roster drives membership; ranks can be given management rights; guild neighborhoods get **attached extra instances** so every guild member can have a plot regardless of guild size **[F]** |
| Type: Charter | Player-created, player-managed membership and permissions, hard-capped at 55 plots **[F]** |
| Minimums | Minimum players (not characters) to found, and a minimum occupancy to keep running **[F]** |
| PvP | Sanctuaries - no PvP **[F]** |
| Services | No bank, no auction house **[F]** |
| Neighborhood NPCs | Steward, General Contractor, Dye Crafter, Lumberjack, decor vendors, smugglers **[F]** |
| Endeavor Coffer | Physical chest in the hub, looted on completion **[F]** |
| Damage to neighbors | Plots are spaced so yards cannot intrude on each other **[F]** |

## 7. Permissions

Independent, frequently-re-evaluated permissions for **plot** and **house**, settable per
audience: **anyone, neighbors, guildmates, party members, friends** **[F]**. Changing them
immediately ejects anyone who no longer qualifies **[F]**. Permissions are per-house, so two
houses can have different rules **[F]**. The settings UI lives behind the gear next to
*Edit House* while standing on the plot **[F]**.

## 7.5 How vendor gating actually works

Pulled from the live vendor tables, not from a guide, so this is the part most write-ups miss.
A neighborhood vendor's stock sits behind **three different gate types**, and they stack:

| Gate | Example | What it means |
| --- | --- | --- |
| Currency | Community Coupons (3363) | Pay the neighborhood currency |
| **Endeavor milestone** | "Reach the third milestone of the endeavor" | The item is unbuyable until the neighborhood's Endeavor passes that milestone. Roughly a third of coupon-vendor stock is gated this way |
| Entitlement | "Requires a Deed of Patronage", or an achievement such as *Ever Painting* | A one-time entitlement, not a currency |

Concrete cases: Brother Dovetail has **11 of 15** items milestone-gated, Griftah **11 of 15**,
Harlowe Marl **9 of 13**, "Den" Nightshade **10 of 27**, Hesta Forlath **14 of 18**. Hesta
also sells eight "Unframed Painting" pieces at **1 500g each** that additionally require a
**Deed of Patronage**, and a 150g cushion that requires the achievement *Ever Painting*.

The design reading: Endeavors are not just a currency faucet, they are the **unlock track for
the whole neighborhood economy**. A neighborhood that never completes Endeavors can look at
the vendor and buy almost nothing of value. My port keeps that, because it is what makes
Endeavors matter (doc 04 sec.4).

## 8. Endeavors

| Item | Value |
| --- | --- |
| Cadence | Roughly **monthly** **[F]** |
| Who picks | Public: the game. Private: the owners/managers, from a selection **[F]** |
| Tasks | Cross-content: crafting, gathering, questing, dungeons, raids, pet battles **[F]** |
| Scaling | Required tasks scale with neighborhood size, and down for less active neighborhoods **[F]** |
| Rewards | Endeavor currency, **Neighborhood Favor**, House XP, themed decor, new vendor NPCs, visual changes to the hub **[F]** |
| Themed NPCs | As tasks complete, NPCs from the Endeavor's culture move in and sell themed decor to *anyone* in the neighborhood, including visitors **[F]** |
| Coffer | Looted once when the Endeavor completes, **250 House XP** plus bonus currency **[F]** |
| Named Endeavors (12.1) | *Candle Culture* (Kobolds, 34 activities, 16 decor), *Every Bakar Has Its Day* (Maruuk Centaur pet training, 23 decor), *Knock-Off Amani* (Amani traders, extra vendor), *Vacation Season* (Tortollan, underwater dive-bar decor) **[F]** |
| Player choice | Each *character* sets which neighborhood's Endeavor is "active"; can be switched at any time, any faction **[F]** |

## 9. Blueprints (patch 12.1)

| Item | Value |
| --- | --- |
| What | Save/share/import a **single room**, a **full build**, or an **exterior** **[F]** |
| How | System-generated codes, not files **[F]** |
| Restrictions | Faction rules for exteriors and full builds; single rooms are faction-agnostic and limited only by the importer's decor budget **[F]** |
| Stairwells | Connected stairwells are exported together; imports can attach to existing rooms on any floor if they do not collide **[F]** |
| Dye handling | Export lists only the dyes that will actually be consumed **[F]** |

## 10. Currencies and economy

| Currency | Id | Used for |
| --- | --- | --- |
| Gold | - | Plots (1 000), most vendor decor (10-150g typical, up to ~500g), dyes, duplicates |
| Community Coupons | **3363** | Endeavor/neighborhood vendor stock, e.g. Pepe copies (10), Torquewrench Tractor (30) **[F]** |
| War Resources | **1560** | Reused by Midnight-era decor vendors (e.g. Fenwir 10-150, Hoarder Jena 50-300) **[F]** |
| Endeavor currency | not published as an id **[?]** | Themed Endeavor vendors |

The interesting part for me: **retail reuses old expansion currencies for housing**, which
means an 8.3.7 port can legitimately pay for decor with War Resources, 7th Legion Service
Medals, Coalescing Visions and friends.

## 11. Sources

| Source | Used for |
| --- | --- |
| Wowhead, *Player Housing Overview* (Zvez, 2026-08-21) | Dashboard, leveling, permissions, travel, warband rules |
| Wowhead, *Player Houses: Unlocking, Leveling, Customizing* (Paryah, 2026-08-15) | Quest chain, level rewards, rooms, exteriors, blueprints |
| Wowhead, *Player Housing Decor Items* (Zvez, 2026-08-21) | Categories, budget costs, dyes, armillary, pet beds, Fully Furnished |
| Wowhead, *Housing and Decor Updates 12.1.5* (Paryah, 2026-10-26) | Blueprint improvements, crafting cost cuts, patch decor, 12.0.7/12.1 systems |
| Wowhead, *Founder's Point* (Paryah, 2026-10-19) | Plots, biomes, vendors, amenities, Pepe, treasure hunts |
| Blizzard, *Midnight: Come Home to Azeroth with Housing* | Neighborhood types, Endeavors concept, exteriors, warband |
| Blizzard, *WoW Housing: It's Only Neighborly* | Public vs private, permissions, Endeavor scaling, moving/regret timer |
| Icy Veins, *Player Housing Guide* (Gogogadgetkat, 2026-08-10) | Quest chain names, 1 000g, portal NPCs, moving, blueprints, addons |
| WoWDB Housing Hub, *Housing Progression* (PTR data) | The L1-12 budget table |
| Epiccarry, *WoW Player Housing Guide* (2026-10-22) | Portal rooms, Endeavor names, decor tiers, 12.2 roadmap |

Numbers I could **not** verify and will therefore not hardcode without testing: Endeavor
currency id, the quest ids after 93057, exact per-room placement costs, exact dye group
composition (77 colours / 9 groups is all that is published), and the 12.1 Exterior
"Large" sizes.
