# 14. Settings and commands

The module has two halves and each half keeps its own settings. The server half is a conf file
read once at start and again on `.housing reload`. The client half is the addon, which has no
conf file and therefore keeps its preferences in `HousingUIDB`, the SavedVariables table the
`.toc` already declares.

Neither half is allowed to be a mystery. `.housing config` prints every server setting with the
value `LoadConfig` actually kept after clamping, `.housing` with no argument prints every
command the server dispatches, and the Settings tab shows every client preference with the
buttons that change it. The three lists are generated from the same sources the code uses, and
`check_reference.py` fails if this file and the code disagree.

## 1. The server settings

Every key is optional, and there are 33 of them. The default below is the value that applies
when the key is absent, and the clamping in the fourth column is applied by
`HousingMgr::LoadConfig` before the value is used, so a nonsense value degrades to a usable
one instead of falling through.

| Key | Default | What it changes | Clamp / when it applies |
| --- | --- | --- | --- |
| `Housing.Enable` | `0` | master switch; 0 leaves every hook in place but inert | reload |
| `Housing.Map.Mode` | `"native"` | native expects the map in Map.db2; anchor runs on an existing map with computed anchors | reload |
| `Housing.Map.Id` | `900` | map id of the neighbourhood; must exist in Map.db2, or housing disables itself | reload |
| `Housing.Interior.Map` | `0` | interior map; 0 means the same map as Housing.Map.Id | reload |
| `Housing.Interior.X` | `0.0` | interior anchor, X | reload |
| `Housing.Interior.Y` | `0.0` | interior anchor, Y | reload |
| `Housing.Interior.Z` | `0.0` | interior anchor, Z | reload |
| `Housing.Interior.O` | `0.0` | interior anchor, orientation | reload |
| `Housing.TownCenter.X` | `0.0` | Town Center landing point, X; 0/0 disables the leave teleport | reload |
| `Housing.TownCenter.Y` | `0.0` | Town Center landing point, Y | reload |
| `Housing.TownCenter.Z` | `0.0` | Town Center landing point, Z | reload |
| `Housing.TownCenter.O` | `0.0` | Town Center landing point, orientation | reload |
| `Housing.Evict.Days` | `0` | days without an edit before a house goes back on the market; 0 is off | reload |
| `Housing.Evict.GraceDays` | `7` | part of that window spent as a warning; the owner is mailed when it starts | reload |
| `Housing.Neighborhood.PopulationCap` | `0` | residents a neighbourhood may hold, by owned plots; 0 is as many as there are plots | reload |
| `Housing.Neighborhood.MinResidents` | `0` | below this many residents, with no owned plots, a non-public neighbourhood is dissolved; 0 is off | reload |
| `Housing.Neighborhood.DissolveGraceDays` | `30` | days an empty neighbourhood stands before it is dissolved; never below 1 | reload |
| `Housing.Phasing.PlotScoped` | `1` | hide a plot's yard decor unless a player stands in that plot | reload |
| `Housing.PetBed.IndoorCap` | `100` | beds allowed indoors per house; 0 turns the cap off | reload |
| `Housing.PetBed.OutdoorCap` | `25` | beds allowed outdoors per plot; 0 turns the cap off | reload |
| `Housing.SaveInterval` | `30` | seconds between deferred decor and name writes; never below 5 | reload |
| `Housing.RateLimit.Messages` | `20` | addon messages accepted per ten second window per player; never below 1 | reload |
| `Housing.Homestone.Cooldown` | `900` | homestone cooldown in seconds; 0 removes the wait | reload |
| `Housing.Sell.RefundPercent` | `50` | percentage of the price refunded on sale; clamped 0 to 100 | reload |
| `Housing.Gryphon.Cost` | `5` | gold the Steward charges for a ride to the house door; 0 is free | reload |
| `Housing.Gryphon.Cooldown` | `300` | seconds between rides, per account; 0 removes the wait | reload |
| `Housing.Dye.Cost` | `0` | gold charged for a dye change on one piece; re-applying the colour it already has is free, and 0 makes every change free | reload |
| `Housing.Restore.FeePercent` | `10` | share of the archived plot price charged to restore a sold or evicted house; 0 makes restoring free, clamped 0 to 100 | reload |
| `Housing.Guild.Wings` | `1` | give a guild that outgrows one neighbourhood an extra wing instead of turning members away | reload |
| `Housing.Arcanostorage.Level` | `10` | house level before the Arcanostorage Sphere will turn; never below 1 | reload |
| `Housing.Arcanostorage.CooldownDays` | `7` | days between turns of the Sphere, per account; never below 1 | reload |
| `Housing.Armillary.Level` | `6` | house level before the Hearthlight Armillary can be lit; never below 1 | reload |
| `Housing.Plot.CornerstoneEntry` | `0` | gameobject entry spawned on every free plot so it can be clicked; must carry go_housing_cornerstone; 0 spawns nothing | reload, and new spawns take effect on the next plot view |

`reload` in the last column means `.housing reload` re-reads the key: the command calls
`Unload`, `LoadConfig`, `LoadHousingData` and `LoadNeighborhoods`, in that order, so a changed
value applies without a restart. What a reload does not do is respawn content that is already
standing in the world; a cornerstone entry changed from 0 to an id appears the next time a
player looks at a free plot, not in the middle of a visit.

## 2. `.housing config`

A GM command, filtered by substring:

```
.housing config                 all 31, in the order LoadConfig reads them
.housing config Evict           just the eviction keys
```

The output is the loader's own record, not a re-reading of `worldserver.conf`. A key that is
missing from the output was not read at all, which is the only way to tell a setting that is
being applied from a setting that is being ignored.

## 3. Every command

48 commands, of which 41 are player commands and 7 are GM commands. `.housing` with no argument
prints this list from the command table itself, so the in-game help cannot drift from what the
server dispatches.

| Command | Arguments | Who | What it does |
| --- | --- | --- | --- |
| `.housing list` | `-` | player | every plot in the neighbourhood: state, tier, owner |
| `.housing info` | `<plot>` | player | price, biome, size class, spawn, and for an owned plot the tier, lock and budgets |
| `.housing buy` | `<plot>` | player | buy a free plot in the neighbourhood the player is in |
| `.housing sell` | `-` | player | sell the house back; the refund is Housing.Sell.RefundPercent of the price |
| `.housing enter` | `<plot>` | player | teleport into a plot, subject to the guest rules |
| `.housing home` | `-` | player | homestone: teleport to the house door, on the Housing.Homestone.Cooldown |
| `.housing leave` | `-` | player | teleport back to the Town Center landing point |
| `.housing interior` | `<enter|exit>` | player | step through the door into the interior or back out |
| `.housing tier` | `<1-12>` | player | set the house level directly (a GM shortcut; the normal route is GrantHouseXp) |
| `.housing name` | `<text>` | player | rename the house, up to 64 characters |
| `.housing lock` | `<on|off>` | player | lock or unlock the plot; a locked house admits nobody but the owner, a guest and a GM |
| `.housing access` | `<yard|house> <mask|names>` | player | who may stand on the yard and who may come inside, as six audience bits or as names (`anyone neighbours guild party friends guests none`); no argument prints both masks |
| `.housing armillary` | `[0-4]` | player | the Hearthlight Armillary: prints the five presets and cycles to the next one, or sets the preset named |
| `.housing decor` | `[category] [page] [subcategory]` | player | the catalogue, filtered by faction, category and subcategory, paged |
| `.housing buydecor` | `<id>` | player | buy a decor item with gold or the row's currency |
| `.housing placed` | `-` | player | every placement in the house with id, decor, position and scale |
| `.housing place` | `<decorId> <x> <y> <z> <o> <scale> <indoor> [pitch] [roll]` | player | place a piece at a typed coordinate; the two tilt angles are optional and in radians |
| `.housing move` | `<placementId> <x> <y> <z> <o> <scale> [pitch] [roll]` | player | move, rescale or tilt a placement |
| `.housing remove` | `<placementId>` | player | take a piece down; the copy goes back to storage |
| `.housing tilt` | `<placementId> <pitch deg> [roll deg]` | player | tilt one piece without retyping its position; the easier half of `.housing move` |
| `.housing pet` | `<placementId> <stationary|roaming|clear>` | player | put the companion that is currently summoned on a pet bed, asleep or wandering, or take it off |
| `.housing dir` | `[page]` | player | the neighbourhood directory: owned plots and their owners |
| `.housing finder` | `-` | player | public neighbourhoods on the player's faction, with the owned/total count |
| `.housing visit` | `<neighbourhood> <plot>` | player | visit another neighbourhood's plot, subject to its guest rules |
| `.housing reserve` | `<plot>` | player | hold a free plot for 60 seconds while it is bought |
| `.housing storage` | `-` | player | every decor copy in storage, with counts |
| `.housing levelup` | `-` | player | pay the Contractor to reach the next level without the experience |
| `.housing shell` | `[id]` | player | list the exterior shells, or choose one by id |
| `.housing dye` | `<placement> <colour> [group]` | player | dye one placed piece; colour 0 strips it, and the group defaults to whichever one this piece keeps that colour in |
| `.housing bp` | `[kind] [name]` | player | export a build code, PHP1 format |
| `.housing bpimport` | `<code>` | player | import a build code |
| `.housing restore` | `-` | player | recover a sold or evicted house from the archive |
| `.housing portal` | `<id>` | player | use one of the house portals |
| `.housing endeavor` | `-` | player | the active Endeavor, its tasks and its milestone |
| `.housing deliver` | `-` | player | hand in the Endeavor delivery task |
| `.housing claim` | `<milestone>` | player | claim a milestone reward once the milestone is reached |
| `.housing choose` | `<endeavor>` | player | choose the neighbourhood Endeavor in a private neighbourhood |
| `.housing guest` | `add <name> [flags] | del <name> | list` | player | the plot's guest list; flags are the access bits |
| `.housing neighbor` | `invite <name> | rank <name> <1-3> | del <name>` | player | neighbourhood membership; no subcommand lists the residents with their ranks |
| `.housing dyes` | `-` | player | the colours the dye station can mix, from housing_dye |
| `.housing milestones` | `-` | player | the seven collection milestones and whether each is met |
| `.housing audit` | `-` | GM | counts every loaded table and every owned plot, for an operator |
| `.housing save` | `-` | GM | flush deferred writes now |
| `.housing reload` | `-` | GM | re-read the conf and reload every world table, then the neighbourhoods |
| `.housing config` | `[filter]` | GM | every setting LoadConfig read and the value it kept, optionally filtered by substring |
| `.housing create` | `<public|guild|private> <name>` | GM | found a neighbourhood |
| `.housing dissolve` | `<id>` | GM | dissolve a neighbourhood; only when it is empty |
| `.housing forcesell` | `<plot>` | GM | sell a plot on behalf of its owner |

Two of those rows are parents: `.housing guest` and `.housing neighbor` have their own
subcommands, listed in the Arguments column.

## 4. The client settings

Six preferences, all in `HousingUIDB.settings`, all editable on the Settings tab, and all written
the moment they change rather than at logout.

| Key | Default | What it changes |
| --- | --- | --- |
| `snapEnabled` | `true` | snap positions, angles and scale before they are sent |
| `grid` | `0.5` | snap size in yards; 0.25 to 2.0 in steps of 0.25 |
| `angle` | `15 degrees` | rotation step; 5 to 45 degrees in steps of 5 |
| `scaleStep` | `0.1` | scale step; 0.05 to 0.5 in steps of 0.05 |
| `frameScale` | `1.0` | window scale; 0.8 to 1.3 in steps of 0.05 |
| `framePoint` | `-` | where the window was last left: anchor, relative anchor and offsets |

`/housing snap on` and `/housing snap off` still work; they write the same `snapEnabled` value the
Settings tab does, and the change survives a reload. `/housing settings` prints the five visible
values in one line for anyone who would rather read than click.

The snap is client-side by design: the server clamps and confirms every position it is given, so
a player who turns snapping off sends precise numbers and a player who leaves it on sends
rounded ones, and the server cannot tell the difference.

There is a seventh client-side control and it is not a SavedVariable, because the client keeps
it: `Bindings.xml` declares one binding, "Toggle the housing window", in Key Bindings under
Addons. Retail's dashboard opens on `H` and an addon cannot claim a key by itself, so the
binding exists and the key is the player's to set - which is the closest an addon can come to
the retail default without taking a key nobody agreed to.

## 5. The permission model

Two spaces, six audiences, stored as one byte each per house: `plotMask` and `houseMask` on
`character_housing`. The bits are `HousingAudience`; the spaces are `HousingSpace`.

| Bit | Value | Audience |
| --- | --- | --- |
| `AudienceAnyone` | `0x01` | no relationship needed |
| `AudienceNeighbors` | `0x02` | a member of the neighbourhood |
| `AudienceGuild` | `0x04` | same guild as the owner |
| `AudienceParty` | `0x08` | same group as the owner |
| `AudienceFriends` | `0x10` | on the owner's friend list, or the owner on theirs |
| `AudienceGuests` | `0x20` | the explicit per-house guest list |

Defaults are `0x3F` for the yard (a yard is public ground) and `0x3E` for the interior
(everything but a stranger). `locked` is not a seventh audience: it is a blanket denial that
only the owner, a guest and a GM get past, and it is checked before the mask.

Three details worth writing down, because each is a decision rather than a mechanic:

- **Guests are always in the mask.** `SetAccess` sets the bit whether the caller asked for it
  or not. A house whose mask excluded guests would be a house nobody could be invited to.
- **Friends while the owner is offline.** The owner's own friend list cannot be read when they
  are not logged in, so the visitor's list stands in. That is more permissive than retail in
  one direction, and the alternative is no friend audience at all during the hours when
  visitors actually turn up.
- **A change applies on the same half second.** `SetAccess` calls `EnforceAccess` directly
  instead of waiting for the tick, so closing the yard moves the people standing in it out
  straight away.

## 6. What is a table and not a setting

Five numbers that live in the world database rather than the conf file, because they change
without a restart and because an operator may want to edit one row rather than the whole file:

| Table | What it holds |
| --- | --- |
| `housing_house_level` | levels 1-12, XP thresholds, indoor, room and outdoor budgets |
| `housing_shell` | exteriors, sizes, level requirement, faction |
| `housing_room` | rooms, their shell, level requirement, budget cost |
| `housing_plot` | plot prices, size classes, yard radius |
| `housing_vendor_stock` | prices, currency, Endeavor milestone gating |

`housing_dye` is in that list too: the dye station's palette is data, so a realm that wants a
seventh colour inserts a row rather than patching the addon. The addon asks for the palette with
`C:DYES` and rebuilds its Dyes tab from the answer.

The permission masks are not settings and not a table either: they are per-house data, edited
by the owner, and they live on `character_housing` next to the tier and the name.

## 7. What has no setting, and why

Three constants stay in the header rather than the conf file, because a realm that changes them
changes the shape of the content rather than its numbers:

- the plot count (55) and the ring layout that places them, because the addon's map and the
  server's anchors have to agree on both;
- the phase bands (plot, interior, neighbourhood), because a saved placement records nothing
  about the phase it was spawned under;
- the opcode names, because they are a protocol and not a preference.

Everything else that a realm might reasonably want to change is either in the conf file or in
the database. A setting that exists in one and not the other is a defect, which is what the
reference checker looks for.
