# 04 - Content spec: quests, Endeavors, treasure hunts, rewards

What mod-psycho-house actually ships as *content*, as opposed to systems. Ids are proposed
and reserved by me in free ranges; anything that needs a client DBC edit is flagged
**[needs client patch]**.

## 1. The introduction chain

Retail uses four quests (*A House For You* -> *My First Home* -> *Welcome Home* -> *Time to
Decorate*, starting at quest 93057). I keep the same beats and names where they are
generic enough, with my own ids.

| # | Quest | Id | Giver | Objective | Reward |
| --- | --- | --- | --- | --- | --- |
| 1 | *A House For You* | 900100 | Automatic (login breadcrumb) | Speak with the faction Steward in the capital portal room | 2g, first Hearthstone-style hint |
| 2 | *My First Home* | 900101 | Steward | Travel to the neighborhood, walk a plot, click a **Cornerstone** | 1 000g stipend (so the first house is effectively free, like the retail pitch), 3 starter decor |
| 3 | *Welcome Home* | 900102 | General Contractor | Place 3 decor pieces, then return | 10 House XP tutorial pop, 5 starter decor |
| 4 | *Time to Decorate* | 900103 | General Contractor | Reach house level 2 | *Square Room (Medium)* shell, 250g |

Design notes:

- **The stipend matters.** Retail's "no exorbitant costs" pitch means the first house must
  not be a gold wall. On a private server 1 000g is real, so the quest pays it back.
- Auto-accept: `PlayerScript::OnLogin`, first login only, flag in
  `character_housing`. Never on a character that already owns a house.
- All four quests are **custom quest rows** (`quest_template`) - no client patch needed for
  quests as long as the quest text, title and objectives live in the DB. Icons are
  existing ones.
- Rewards use existing spells/items; the "house XP" reward is a module-side grant, not a
  spell.

## 2. House levels as content

The level table is retail's (doc 01 sec.3.1) and lives in `housing_house_level`. Content
implications I am committing to:

| Level | Content beat |
| --- | --- |
| 1 | Starter shell, 12 rooms, 910/19/200 budgets |
| 2 | +245 interior, +5 rooms - reachable in one evening (10 XP = one decor)* |
| 3-5 | Room unlocks, exterior budget steps, the "my house is taking shape" phase |
| 6 | Hearthlight Armillary as a *placeable ambience device* (not a dye-tier system) |
| 7 | The big jump (+830 interior, +18 rooms) - the "I built a manor" level |
| 8-9 | Medium exteriors, then the second big budget step |
| 10 | Weekly *Treasures from Distant Lands* random decor |
| 11-12 | Large exteriors, endgame prestige |

\* Retail's 10 XP for level 2 only works because every new decor item grants 10. With a
harvest catalogue of hundreds of pieces, levels 2-5 will fly by: exactly as intended, and
the reason Endeavor XP matters is the level 6+ grind.

## 3. Rooms and shells (what the player is buying at each level)

| Shell / room | Type | Placement cost | Level | Notes |
| --- | --- | --- | --- | --- |
| Square Room (Tiny / Small / Medium / Large) | Room | 1 / 2 / 3 / 5 | 1 / 1 / 2 / 5 | Four footprints, one art family |
| Octagon Room (Small / Medium / Large) | Room | 2 / 3 / 5 | 1 / 1 / 4 | |
| Circle Room (Daylight / Evening) | Room | 3 | 3 / 5 | |
| Cross / L / T shaped | Room | 3 | 1 | Connective geometry |
| Hallway / Closet | Room | 1 | 1 | |
| Stairwell Left / Right / Empty | Room | 2 | 1 | Vertical connectors |
| Small Human / Night Elf / Orc / Blood Elf House | Exterior | - | 1 | Faction-masked |
| Medium exteriors (4 racial) | Exterior | - | 8 | |
| Large exteriors (4 racial) | Exterior | - | 12 | |

Retail's 12 starter rooms and their shapes are listed in doc 01 sec.3.2; each becomes a GO/WMO
shell in the custom map's interior space. **Shell placement cost** is my number: retail
publishes that rooms have costs but not the values, so these are balanced against the 19
room budget at level 1: one large room plus a few small ones fits, anything bigger needs levels.

## 4. Endeavors

Six Endeavors at launch, each themed on BfA content so the task list is something players
already do on this server. Monthly rotation, one active per neighborhood.

### 4.1 Structure

```sql
housing_endeavor
  id, name, description, faction_mask, currency_id, favor_per_task, xp_per_task,
  theme_biome, prop_set, vendor_set, min_tasks, max_tasks

housing_endeavor_task
  endeavorId, taskType, targetId, required, weight, description

housing_endeavor_reward
  endeavorId, itemOrDecorId, cost, currencyId, requiresCompletion
```

`taskType` values: `Craft` (spell id), `Gather` (item id), `Quest` (quest id),
`Dungeon` (boss entry or instance map), `Raid`, `PetBattle`, `Kill` (creature entry),
`WorldQuest`-equivalent (daily), `Fish` (item id), `Archaeology` (project id).

### 4.2 The six

| Endeavor | Theme | Task examples | Vendor set unlocked |
| --- | --- | --- | --- |
| **Tides of Kul Tiras** | Drustvar / Tiragarde coastal | Fish 40x coastal fish; complete 3 Tiragarde quests; kill 25 pirates | Nautical: anchors, nets, crates, dock posts, fog lanterns |
| **Zandalari Bazaar** | Zuldazar markets | Craft 20x potions; loot 30x trade goods; 5 pet battles won | Tribal: rugs, urns, totems, market stands, hanging fabrics |
| **Mechagon Workshop** | Mechagon tinkering | Craft 15x engineering items; kill 30 mechagnomes; 2 dungeon runs | Gnome/Mech: gears, lamps, pipes, cog piles, mechanical chairs |
| **Nazjatar Depths** | Naga / ocean floor | Kill 40 naga; gather 25x herbs; complete 2 Nazjatar dailies | Oceanic: shells, coral, kelp, glowfish tanks, dark braziers |
| **Vulpera Caravan** | Vol'dun traders | Complete 10 Vol'dun quests; loot 20x caravan goods; 3 pet battles | Nomad: tents, cushions, campfires, saddlebags, desert plants |
| **Death Rising** | Scourge / Cult of the Damned | Kill 50 undead; complete 2 Northrend-era quests; 1 raid boss | Spooky: gravestones, cobwebs, candles, skulls, iron fences (pairs with Brumewood Hollow) |

Each has **20-34 tasks** in its list with a neighborhood-scaled requirement:

```
required = ceil(base * clamp(activeResidents / 10, 0.5, 3.0))
```

so a 3-house server still finishes an Endeavor and a 40-house server has to work for it.

### 4.2b Milestones, and why they are the real reward

Retail gates vendor stock on **Endeavor milestones**, not just on currency. Verified counts
from the live vendor tables: Brother Dovetail 11 of 15 items gated, Griftah 11 of 15, Harlowe
Marl 9 of 13, "Den" Nightshade 10 of 27, Hesta Forlath 14 of 18. The gate text is always
"Reach the Nth milestone of the endeavor", with N from 1 to 4.

So each Endeavor in mod-psycho-house ships **four milestones**:

| Milestone | Progress | Unlocks |
| --- | --- | --- |
| 1 | 25 % of tasks | First tier of themed stock becomes purchasable |
| 2 | 50 % | Second tier, plus the first NPC flavour dialogue |
| 3 | 75 % | Third tier, plus hub decoration stage 2 |
| 4 | 100 % | Fourth tier, the coffer appears, and the vendor's best pieces unlock |

Store this as `housing_endeavor_reward.milestone` (1-4) alongside the decor id, and check it
in the vendor's stock filter. That is the whole implementation: one integer per reward row and
one comparison in `GetVendorStock`. It turns a currency sink into a reason to log in.

### 4.3 Loop

1. Rotation timer (WorldScript) picks the next Endeavor per neighborhood, writes
   `endeavorId`, `rotationStart`, `rotationEnd` (30 days), zeroes progress.
2. Players see tasks in the addon and via `.house endeavor`; each character has an
   *active* Endeavor (per-character, per retail).
3. Task completions increment progress and pay Favor + House XP.
4. At 100 % the themed vendor NPCs spawn in the hub (their `phase` = neighborhood hub
   phase), a Coffer GO appears, and every resident can loot it once per warband:
   250 House XP + currency + one themed decor.
5. Milestones unlock vendor tiers as progress crosses 25 / 50 / 75 / 100 % (sec.4.2b).
6. Next month rotates. Public: server picks, never the same twice (retail's 12.1.5 fix).
   Private: leader picks from the list via Steward gossip.

## 5. Decor treasure hunts

Retail has per-neighborhood treasure-hunt quests with riddles (Founder's Point and
Razorwind Shores guides). Mod-psycho-house ships **10 riddle hunts** in Founder's Point,
each a custom quest with a GO objective:

| # | Riddle hook | Where | Reward |
| --- | --- | --- | --- |
| 1 | "Where the beer flows and the wheel turns" | Gilded Oaks beer garden | *Waterwheel Keg* decor |
| 2 | "The stone remembers the names" | Brumewood graveyard | *Weathered Gravestone* |
| 3 | "Under the ring of caps" | Mushroom ring | *Glowing Mushroom Cluster* |
| 4 | "The eye that never blinks" | Lighthouse | *Lighthouse Lantern* |
| 5 | "Grapes trodden, promises kept" | Mear's Farm stomping vats | *Wine Vat* |
| 6 | "Where gryphons sleep" | Small Aerie | *Gryphon Feather Bundle* |
| 7 | "The bridge that only one house crosses" | Stoneveil Ridge | *Ridge Bridge Lantern* |
| 8 | "Salt in the timbers" | Breakwater Bight smugglers | *Smuggler's Crate* |
| 9 | "Beneath the notice of neighbours" | Bulletin board (Pepe cameo) | *Pepe* decor |
| 10 | "The lamp the wind cannot kill" | Outer Banks cliff | *Storm Lantern* |

Mechanics: `quest_template` + `quest_objectives` with `GAMEOBJECT` type, plus a spawn of a
small "cache" GO per location. Rewards grant decor directly (`GrantDecor`, no item).

## 6. Achievements

8.3.7 achievement definitions are DBC, so anything here is **[needs client patch]** if I want
a real achievement panel entry. Cheap alternative first: a **Collection Milestones** list on
the Steward gossip, and titles only if a title DBC edit is acceptable.

| Proposed | Requirement | Reward |
| --- | --- | --- |
| *Fully Furnished* (parity name) | 500 decor of uncommon+ collected | Title **The Furnished** |
| *Landlord* | Reach house level 5 | Decor |
| *Estate Manager* | Reach house level 12 | Large exterior token |
| *Good Neighbour* | Contribute to 3 completed Endeavors | Decor |
| *Grand Tour* | Visit 20 different plots | Decor |
| *Master Builder* | 100 placed pieces at once | Decor, addon badge |
| *Hoarder* | Own 1 000 decor copies | Decor |

## 7. Rewards economy

| Reward channel | What drops | Guard |
| --- | --- | --- |
| Vendor (gold) | 60 % of the harvested catalogue | Gold is the sink |
| Endeavor vendors (War Resources 1560) | Themed sets, 16-30 pieces each | Only after completion |
| Endeavor coffer | Currency + 1 random themed decor + 250 House XP | Once per warband per Endeavor |
| Treasure hunts | 10 unique pieces | One-time quests |
| Dungeon/raid decor drops | Trophy pieces from bosses | Loot condition on boss kill |
| Profession decor | Crafted pieces (reciped) | Recipe learn + reagents |
| Weekly *Treasures from Distant Lands* (level 10+) | 1 random piece from a curated pool | 7-day cooldown per house |

## 8. Acceptance criteria per content block

| Block | Done when |
| --- | --- |
| Intro chain | New character on a fresh account is offered quest 1, can complete all four in one session without a GM, and ends with a level-2 house |
| Endeavors | On a 3-house test neighborhood, one Endeavor can be completed in 3 evenings; coffer loot is once-per-warband; vendor set only appears on completion |
| Treasure hunts | All 10 completable from the riddles alone by someone who knows the zone; rewards land in the collection, not the bag |
| Rewards | No reward path hands out decor that is not in the catalogue; no catalogue entry is unreachable |
| Levels | A player who only learns decor reaches level 5; reaching 6+ requires Endeavors (verified by simulation of the XP curve) |
