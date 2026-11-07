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

The level table is retail's (doc 01 sec.3.1) and belongs in `housing_house_level`, which
is phase 1 work - the table is not in the draft yet. Until it lands the core uses the
five-rung ladder compiled into `HousingDefines.h`, so the beats below are what each retail
level is *for*, not what the draft currently charges. Content implications I am
committing to:

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

Four Endeavors at launch, each themed on content this realm already has so the task list is
something players already do. Monthly rotation, one active per neighborhood.

### 4.1 Structure

```sql
housing_endeavor
  id, name, description, duration_days, base_required, flags

housing_endeavor_task
  id, endeavor_id, type, target, target_name, required, text

housing_endeavor_reward
  endeavor_id, milestone, item_entry, item_count, currency_id, currency_amount, xp
```

`type` is a number, not a word: 0 kill, 1 boss, 2 quest, 3 loot, 4 craft, 5 pet battle,
6 deliver. `target` is the creature, gameobject, quest or spell id the hook compares
against, and 0 means the row is not bound yet; `target_name` is the name the bind statement
in `tools/sql/bind_housing_content.sql` searches for, so a row can be authored before the
id is known. `required` is scaled per neighbourhood by `GetRequired`, not used raw: an
Endeavor in a street of twelve asks for more than the same one in a street of two.

`character_housing_endeavor` holds the running one - `neighborhoodId`, `endeavorId`,
`milestone`, `started`, `ends` - and per-task progress lives beside it, so a server restart
mid-rotation does not reset a street's work.

### 4.2 The four that ship

Retail rotates themed Endeavors - one per neighbourhood theme, six in the 12.0 set. What the
module ships is four, and they are deliberately small: every task is a real 8.3.7 creature,
quest or system, because a task that names something the realm does not have is a bar that
never fills.

| Endeavor | Premise | Tasks | Rows |
| --- | --- | --- | --- |
| **Harvest Home** | The neighbourhood fills its larder for the season | Kill 10 Kobold Vermin, 5 Harvest Golems, 10 Boars; complete *A House For You*; win 3 pet battles; deliver 5 loads at the Coffer | 6 |
| **The Hunters' Gate** | Something is taking the livestock at night | Kill 10 Wolves, Hogger, 10 Defias Pillagers; defeat Edwin VanCleef; complete *First Home* and *Welcome Home* | 6 |
| **Winter Veil Warmth** | The neighbourhood decorates for the holiday | Defeat the Abominable Greench; complete *Time to Decorate*; win 5 pet battles; deliver 10 loads; kill 10 Winterfall Den Watchers and 10 Frostsabers | 6 |
| **Deep Places** | The old things under the rock are stirring again | Defeat Onyxia; kill 10 Faceless Ones; win 8 pet battles; deliver 5 loads | 4 |

Twenty-two task rows, in `07_progression.sql`. The six retail themes - Tides of Kul Tiras,
Zandalari Bazaar, Mechagon Workshop, Nazjatar Depths, Vulpera Caravan, Death Rising - are
written down in doc 01 as the shape to grow into; adding one is four definition rows plus its
task rows, with no code change.

Each task's requirement scales with how many people live in the street:

```
required = max(1, base * clamp(residents / 10, 0.5, 3.0))
```

`ScaledRequirement`, called through `GetRequired`, so a three-house street still finishes an
Endeavor and a forty-house street has to work for it.

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

1. Rotation timer (`EndeavorMgr::Update`, driven by the world script) finishes an expired
   Endeavor and starts the next one: it writes `endeavorId`, `started` and `ends`
   (`duration_days`, 30 in the shipped rows) and zeroes the task progress.
2. Players see tasks in the addon and via `.house endeavor`; each character has an
   *active* Endeavor (per-character, per retail).
3. Task completions increment progress; progress is what moves the milestone. Favor and
   House XP are paid at the milestone claim, not per task.
4. At 100 % the themed vendor NPCs spawn in the hub (their `phase` = neighborhood hub
   phase), a Coffer GO appears, and every resident can loot it once per warband:
   250 House XP + currency + one themed decor.
5. Milestones unlock vendor tiers as progress crosses 25 / 50 / 75 / 100 % (sec.4.2b).
6. Next month rotates. Public: server picks, never the same twice (retail's 12.1.5 fix).
   Private: leader picks from the list via Steward gossip.

## 5. Decor treasure hunts

Retail has per-neighborhood treasure-hunt quests with riddles. The module ships ten of them:
ten rows in `housing_hunt`, ten quests 900110-900119, ten reward pieces and ten caches. A
hunt is a line of verse, a cache hidden in the neighbourhood, and the piece the cache holds.

| # | Riddle | Quest | Cache object | Reward piece (game object entry) |
| --- | --- | --- | --- | --- |
| 1 | Where the beer flows and the wheel turns | 900110 | Suspicious Crate (190094) | Bar Keg (170607) |
| 2 | The stone remembers the names | 900111 | Crate of Cellblock Rations (206996) | Iron Gravestone (220864) |
| 3 | Under the ring of caps | 900112 | Stout Crate of Mantid (221802) | Blue Cluster Display (7364) |
| 4 | The eye that never blinks | 900113 | Massive Crate of Mogu (221803) | Harbour Brazier (195424) |
| 5 | Grapes trodden, promises kept | 900114 | Massive Crate of Mantid (221804) | Wine Keg (171941) |
| 6 | Where gryphons sleep | 900115 | Lightweight Crate of Mantid (221806) | Aerie Banner (178205) |
| 7 | The bridge that only one house crosses | 900116 | Lightweight Crate of Mogu (221807) | Ridge Fire (177417) |
| 8 | Salt in the timbers | 900117 | Lightweight Crate of Mantid 2 (221808) | Smuggler's Crate (190094) |
| 9 | Beneath the notice of neighbours | 900118 | Lightweight Crate of Mantid 3 (221809) | Plagued Crate (190095) |
| 10 | The lamp the wind cannot kill | 900119 | Cache of Storms (194312) | Storm Brazier (195403) |

The piece is named after the object it is made of, not after the retail guide entry it
stands in for. None of the ten riddle sites in retail - the beer garden, the graveyard, the
mushroom ring, the lighthouse, the stomping vats, the aerie, the ridge, the breakwater, the
notice board, the cliff - has a dedicated 8.3.7 object, so each piece is a real object from
the build's own asset index chosen to sit where the retail piece would: a bar keg for the
beer garden, an iron tomb for the graveyard, a cluster display for the mushroom ring, a
brazier for the lighthouse, a keg trap for the vats, a banner for the aerie, an Elune fire
for the ridge, a suspicious crate for the breakwater, a plagued crate for the notice board
and a second brazier for the cliff.

A piece called "Wine Vat" whose object is an idol is worse than a piece that says what it
is, which is why the names are the objects. The reward pieces are authored `housing_decor`
rows, ids 1900-1909 in draft A and 2900-2909 in draft B, source type 4 (`TreasureHunt`)
with the hunt id as the source.

`check_coverage.py` walks this table too: every reward id has to be a row in the same
draft's catalogue, every cache and every reward object has to be a game object entry the
source tree names, and the ten quests have to sit inside the module's reserved range. Seven
of the ten reward objects are furniture-shaped by the index's own keyword rule and three
are not - a tomb, a cluster display and a fire - so the rule the checker applies is that
the entry exists, not that a keyword agrees it is a chair.

Mechanics:

- **The table.** `housing_hunt` carries `id`, `name`, `riddle`, `quest_id`, `reward_decor`,
  `cache_entry`, `x`, `y`, `z` and `faction_mask`.
  `HousingMgr::LoadHunts` reads it and stays quiet on an install that has not imported it
  yet: a missing table is a log line and no hunts, not a failed start.
- **The cache.** One game object per hunt, spawned in the neighbourhood's phase by
  `SpawnHuntCaches` as part of `SpawnNeighborhoodContent`. A row with authored coordinates
  is used as written; a row of zeroes lands on the hunt ring, sixty-two yards out from the
  town centre at half a step from the amenity angles, so no cache sits inside a stall or a
  wall. `go_housing_huntcache` is the script, bound by `gameobject_template.ScriptName`,
  and it finds its row from the entry plus the distance to that anchor - a game object
  cannot carry a database id.
- **Taking the riddle.** The Hunts tab sends `C:HUNTS`, gets one `S:HUNT` line per hunt
  with the state the player is in, and sends `C:HUNT <id>` to take one. The Steward offers
  the same list as gossip, and reads the verse out loud when the option is taken. A hunt
  with `faction_mask` bit 0 clear is not offered to an Alliance character, and the reverse.
- **Finishing it.** The quest's objective is the cache, objective type 2 in
  `quest_objectives`, so the quest log says "find the cache and open it". The click is
  intercepted by the object script before the core credits the objective, so
  `HousingMgr::CompleteHunt` hands over the credit with `AreaExploredOrEventHappens`, adds
  the piece to `character_housing_collection` together with one copy in storage, and rewards
  the quest. The piece goes into the collection rather than the mail because a decoration
  is a `housing_decor` row, not an item: no `item_template` row is needed.
- **The SQL.** `tools/housing/build_hunts.py` writes `08_hunts.sql` into both drafts: the
  twenty reward rows, the ten `housing_hunt` rows, the ten `quest_objectives` rows and the
  ten `quest_template` rows. It is a generator because the two drafts use different id
  ranges for the reward pieces, and a hand-written file would drift between them.

The four introduction quests in section 1 have a giver now: the Steward offers the next one
in the chain and takes the turn-in, and the objective is marked by the module event that
satisfies it. They were rows with no giver and no objective hook before that, which meant
nothing in the game could start or finish them.

## 6. Achievements

8.3.7 achievement definitions are DBC, so anything here is **[needs client patch]** if I want
a real achievement panel entry. What ships instead, and what the table below is checked
against: `HousingMgr::BuildMilestoneReport` reads all seven conditions out of state the module
already keeps, and the list is printed on the Steward and by `.housing milestones`. Titles
still need a DBC edit, so there are none.

Where each condition is read from is in the middle column: no milestone table exists, because
a table that can disagree with the state it describes is worse than none.

| Proposed | Requirement (and where it is read from) | Reward |
| --- | --- | --- |
| *Fully Furnished* (parity name) | 500 decor collected (`character_housing_collection`) | Title **The Furnished** |
| *Landlord* | House level 5 (`character_housing.tier`) | Decor |
| *Estate Manager* | House level 12 (same column) | Large exterior token |
| *Good Neighbour* | 3 completed Endeavor tasks (`character_housing_endeavor_task` progress) | Decor |
| *Grand Tour* | 20 different plots stood on (the housing tick's plot resolution) | Decor |
| *Master Builder* | 100 pieces placed at once (`character_housing_decor`) | Decor, addon badge |
| *Hoarder* | 1 000 copies in storage (`character_housing_storage`) | Decor |

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
