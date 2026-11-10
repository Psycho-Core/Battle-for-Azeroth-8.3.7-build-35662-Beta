# 15 - The BFAH protocol

The contract between `addon/HousingUI/` and the worldserver. Sixty-one client ops, thirty-nine
server ops, one prefix, one framing rule, one version number. Everything a second
implementation of either half needs is here; nothing here is inferred from a screenshot.

Two source files carry the implementation and neither of them is a specification:
`src/game/Housing/HousingMgrProtocol.cpp` reads requests and `HousingAddon.cpp` writes
answers, with the later features in `HousingMgrExtra.cpp`. This document is the third
description and the one that is checked - `tools/housing/check_protocol.py` fails if a `C:` or
`S:` op in either half is missing from the tables below, or if a table names an op that no
longer exists.

The prototype had a protocol document of its own (`legacy-prototype/04-addon-protocol.md`).
That one describes twenty-five ops including three replies this module no longer sends, and it
is kept as the record of what the prototype was. This is the shipped one.

## 1. Transport

| Item | Value |
| --- | --- |
| Prefix | `BFAH` (`kAddonPrefix`), registered by the addon with `C_ChatInfo.RegisterAddonMessagePrefix` |
| Client -> server | `C_ChatInfo.SendAddonMessage("BFAH", msg, "WHISPER", UnitName("player"))` |
| Server -> client | `WorldPackets::Chat::Chat` built with `CHAT_MSG_WHISPER`, `LANG_ADDON`, the player as both sender and receiver, and the prefix in the addon-name field |
| Server hook | `WorldSession::HandleChatAddonMessage`, which hands a `BFAH` message to `HousingMgr::HandleAddonMessage` before any chat routing and returns |
| Size | One addon message is at most 255 bytes in 8.3.7, which is why long payloads are chunked (sec.3) |

Whispering yourself is the one addon channel that needs no group, guild or channel membership.
The hook is before the routing switch, so a housing message never appears as a whisper to
anyone, and the core learns nothing about housing except that one prefix comparison.

Text is ASCII. A field that carries player-written text - a house name, a house's new name,
a guest's name - is stripped of control characters and of both framing characters before it is
stored (`HousingAddon::SanitiseText`), then truncated on a UTF-8 boundary. A name with a `|` in
it therefore cannot break the frame it travels in.

## 2. Framing

- `|` separates fields, `;` separates records inside one payload.
- Every record starts with a direction marker and an opcode: `C:NAME` client to server,
  `S:NAME` server to client, upper case.
- Numbers are decimal integers or `%.3f` floats. No locale-dependent formatting.
- Empty fields are legal (`||`), so a field count never shifts.
- Field **1** is the op token itself. The addon indexes `fields[1]` as the op and `fields[2]`
  as the first argument; the server splits the same way and calls the op `fields[0]`. The
  argument tables below use the addon's numbering, so "field 2" is the first argument.
- A payload is one or more records. There is no length prefix and no header: the client
  splits on `;` and dispatches each record.

Two shapes of reply:

**A result line.** `S:RESULT|<op>|<code>[|<extra>]`, where `<op>` is the request's opcode so
a client can match the answer to the question, `<code>` is a `HousingResult` (sec.7), and
`<extra>` is the id that was acted on when there is one - a placement id, a decor id, a
housing id, a Favor total. `Ok` is code 0 and `<extra>` is omitted when it would be 0.

**A list.** Zero or more entry records followed by a terminator:
`S:DECORENTRY`* `S:DECOREND`, `S:PLOT`* (no terminator), `S:DIRENTRY`* `S:DIREND`,
`S:PLACEDENTRY`* `S:PLACEDEND`, `S:GUEST`* `S:GUESTEND`, `S:VAREA`* `S:VAREAEND`,
`S:FINDER`* `S:FINDEREND`, `S:HOMES`* `S:HOMESEND`, `S:PORTAL`* `S:PORTALEND`,
`S:SHELL`* `S:SHELLEND`, `S:STORAGE`* `S:STORAGEEND`, `S:DYE`* `S:DYEEND`,
`S:ENDTASK`* `S:ENDTASKEND`, `S:HUNT`* `S:HUNTEND`.

The terminator counts whatever the client needs to page or to say "and nothing else": the
page number and whether another page exists for the two paged lists, a count for the house
list, nothing for the rest.

`S:PLOT` is the one list with no terminator, and it is a wart rather than a design: the plot
list is bounded by `kPlotCount` (55), so the client redraws from its own table when the last
entry arrives. A reader reimplementing the server does not need to send anything extra.

## 3. Chunking

A payload longer than 200 bytes (`kAddonMaxChunk`) is cut into fragments, and each fragment
travels as its own addon message:

```
S:CHUNK|<frame>|<seq>|<total>|<fragment>
```

| Field | Meaning |
| --- | --- |
| 2 | Frame number, a per-player counter that wraps at 256 |
| 3 | Fragment number, 1-based |
| 4 | Fragments in this frame |
| 5 | The fragment: a run of whole records, with no leading or trailing `;` added |

The server cuts on `;` boundaries, so a record is never split between two fragments unless it
is itself larger than 200 bytes - which only a malformed payload is - and then it is hard-split
and the client drops the frame because the reassembled text will not parse.

The client concatenates fragments 1..`total` for a frame and parses the result as one payload.
Reassembly is plain concatenation, which is why no fragment carries a separator of its own.

A frame whose fragments arrive out of order, or with a different `total`, is discarded and
restarted from that fragment. An incomplete frame older than 10 seconds is dropped by an
`OnUpdate` sweep, so a lost message costs one stale table entry rather than memory that never
comes back.

## 4. Versioning

`kAddonProtocolVersion` is **3**. The handshake is the first message in both directions:

```
C:HELLO|3
S:HELLO|<protocol>|<moduleVersion>|<enabled>|<hasHouse>
```

The client compares field 2 against its own 3 and prints a protocol-mismatch line rather than
half-working. `<enabled>` is always 1 on the wire: a disabled module answers
`S:RESULT|MODULE|1` and never reaches the handshake. `<hasHouse>` tells the client whether to
open on "buy" or "manage".

| Version | What changed |
| --- | --- |
| 1 | The prototype: neighbourhood info, plots, decor, placing. Replies included `S:ENTER`, `S:INTERIOR` and `S:LEAVE`, which are `S:RESULT` lines now |
| 2 | The prefix became `BFAH` and the op set was split into request and reply tables |
| 3 | The current one: lists carry terminators with paging state, op-specific replies (`S:HOME`, `S:BUYRESULT`, `S:PLOTINFO`) exist, and the later feature set - Endeavors, hunts, dyes, blueprints, storage, shells, rooms, the two-house selector - arrived |

Adding an op does not change the version; changing the meaning of a field or the shape of a
reply does. A client that meets a newer server keeps working as long as the ops it knows keep
their shapes, which is why new reply fields go at the end of the line.

## 5. Rate limiting

Twenty messages per ten seconds (`Housing.RateLimit.Messages`, `kRateLimitWindowMs` is 10000).
The count is per player and is checked before any lookup, so a flood costs the server one
comparison each. Over the limit the reply is `S:RESULT|RATE|22` (`RateLimited`) and the message
is dropped.

## 6. Client to server

Field 1 is the op. A field in `[brackets]` is optional; `|` inside a cell would break the
table, so alternatives are written `0 or 1`.

### 6.1 Handshake and the house itself

| Op | Fields | Reply | Notes |
| --- | --- | --- | --- |
| `C:HELLO` | 2: protocol version | `S:HELLO` | Sent once on login and on `/housing hello` |
| `C:PING` | - | `S:PONG` | Liveness and clock |
| `C:HOME` | - | `S:HOME` | Homestone. The reply carries the remaining cooldown on `Cooldown` |
| `C:LEAVE` | - | `S:RESULT LEAVE` | Teleport to the town centre |
| `C:NEIGHBORHOOD` | - | `S:NBR` or `S:RESULT NEIGHBORHOOD` | Which street the player is in |
| `C:PLOTS` | - | `S:PLOT`* | Every plot in that street |
| `C:PLOT` | 2: plot index | `S:PLOTINFO` or `S:RESULT PLOT` | One plot's card |
| `C:BUY` | 2: plot index | `S:RESULT BUY` | Free plot in the street the player is standing in |
| `C:SELL` | - | `S:RESULT SELL` | Refund is a percentage of the price paid |
| `C:TIER` | 2: tier 1-12 | `S:RESULT TIER` | The GM shortcut; the normal route is house XP |
| `C:NAME` | 2: text | `S:RESULT NAME` | House name, sanitised and truncated |
| `C:LOCK` | 2: 0 or 1 | `S:RESULT LOCK` | Locked admits nobody but the owner, a guest and a GM |
| `C:ENTER` | 2: plot index | `S:RESULT ENTER` | Teleport into a plot, subject to the guest rules |
| `C:INTERIOR` | 2: 0 or 1 | `S:RESULT INTERIOR` | 1 steps through the door, 0 steps back out |
| `C:HOMES` | - | `S:HOMES`* `S:HOMESEND` | The houses on the account and which one the Homestone uses |
| `C:SELECTHOME` | 2: housing id | `S:RESULT SELECTHOME` | `<extra>` is the id chosen |

### 6.2 Neighbourhoods

| Op | Fields | Reply | Notes |
| --- | --- | --- | --- |
| `C:DIR` | 2: page `[0]` | `S:DIRENTRY`* `S:DIREND` | Twenty owned plots per page |
| `C:VAREA` | - | `S:VAREA`* `S:VAREAEND` | The town centre's amenities |
| `C:FINDER` | - | `S:FINDER`* `S:FINDEREND` | Public streets of the player's faction |
| `C:JOIN` | 2: neighbourhood id | `S:RESULT JOIN` | `<extra>` is the id joined |
| `C:LEAVENB` | - | `S:RESULT LEAVENB` | Leave the street the player lives in |
| `C:FOUND` | - | `S:RESULT FOUND` | Found a new neighbourhood |
| `C:MOVEHOUSE` | 2: plot index | `S:RESULT MOVEHOUSE` | Move within the same street |
| `C:VISIT` | 2: neighbourhood id, 3: plot index | `S:RESULT VISIT` | Visit another street's plot |
| `C:RESERVE` | 2: plot index | `S:RESULT RESERVE` | Hold a free plot for 60 seconds |
| `C:FAVOR` | - | `S:RESULT FAVOR` | `<extra>` is the Favor total; also updates the two world-state fields |

### 6.3 Guest list

| Op | Fields | Reply | Notes |
| --- | --- | --- | --- |
| `C:GUESTLIST` | - | `S:GUEST`* `S:GUESTEND` | Guests by character name and access flags |
| `C:GUESTADD` | 2: name, 3: flags 0-7 | `S:RESULT GUESTADD` | Flags are the three audience bits |
| `C:GUESTDEL` | 2: name | `S:RESULT GUESTDEL` | |

### 6.4 Decor

| Op | Fields | Reply | Notes |
| --- | --- | --- | --- |
| `C:DECOR` | 2: category `[all]`, 3: page `[0]`, 4: subcategory `[all]` | `S:DECORENTRY`* `S:DECOREND` | Twenty rows per page, filtered by faction, category, subcategory and milestone availability |
| `C:BUYDECOR` | 2: decor id | `S:BUYRESULT` | Charged in the row's currency |
| `C:BUYCOPY` | 2: decor id, 3: count `[1]` | `S:STORAGE` then `S:RESULT BUYCOPY` | Count is clamped to 1-50. The `S:STORAGE` line reports the new total |
| `C:PLACED` | - | `S:PLACEDENTRY`* `S:PLACEDEND` | Every placement in the house |
| `C:PLACE` | 2: decor id, 3: x, 4: y, 5: z, 6: o, 7: scale, 8: indoor 0 or 1, 9: pitch `[0]`, 10: roll `[0]` | `S:RESULT PLACE` | `<extra>` is the new placement id. Tilt is optional so a client that stops at field 8 gets an upright piece |
| `C:MOVE` | 2: placement id, 3: x, 4: y, 5: z, 6: o, 7: scale, 8: pitch `[0]`, 9: roll `[0]` | `S:RESULT MOVE` | |
| `C:REMOVE` | 2: placement id | `S:RESULT REMOVE` | The copy returns to storage |
| `C:PLACEAT` | 2: decor id, 3: metres `[2]`, 4: indoor `[0]` | `S:RESULT PLACEAT` | The server computes the position from the player's own and snaps to the ground; `<extra>` is the placement id |
| `C:BUILDMODE` | 2: 0 or 1, 3: decor id `[0]` | `S:RESULT BUILDMODE` | The placement gizmo's state |
| `C:DYES` | - | `S:DYE`* `S:DYEEND` | The palette the dye station can mix |
| `C:DYE` | 2: placement id, 3: colour, 4: group `[0]` | `S:RESULT DYE` | Colour 0 strips; group 0 means the piece's own group |
| `C:PET` | 2: placement id, 3: mode 0, 1 or 2 | `S:RESULT PET` | 0 off, 1 asleep, 2 roaming |
| `C:ARMILLARY` | 2: preset 0-4 `[cycle]` | `S:RESULT ARMILLARY` | No argument cycles to the next preset |
| `C:ACCESS` | 2: space 0 (yard) or 1 (inside), 3: mask | `S:RESULT ACCESS` | The mask is the six audience bits |

### 6.5 Storage, shells and rooms

| Op | Fields | Reply | Notes |
| --- | --- | --- | --- |
| `C:STORAGE` | - | `S:STORAGE`* `S:STORAGEEND` | Owned copies by decor id |
| `C:SHELLS` | - | `S:SHELL`* `S:SHELLEND` | Exterior shells of the player's faction |
| `C:SHELL` | 2: shell id | `S:RESULT SHELL` | Applies the exterior, gated on house level |
| `C:ROOMS` | - | `S:ROOM`* then `S:ROOMS` | Each shape with its slot cost and level gate, then the slots spent |
| `C:ROOM` | 2: room id | `S:RESULT ROOM` | `<extra>` is the room id |

### 6.6 Progression, Endeavors and hunts

| Op | Fields | Reply | Notes |
| --- | --- | --- | --- |
| `C:LEVELUP` | - | `S:RESULT LEVELUP` | Pay the Contractor for the next level |
| `C:ARCANE` | - | `S:RESULT ARCANE` | The weekly Arcanostorage Sphere |
| `C:GRYPHON` | - | `S:RESULT GRYPHON` | The travel gryphon |
| `C:ENDEAVOR` | - | `S:ENDEAVOR` `S:ENDTASK`* `S:ENDTASKEND` | The active Endeavor, its tasks and its milestone |
| `C:DELIVER` | - | `S:RESULT DELIVER` | Hand in the delivery task |
| `C:CLAIM` | 2: milestone 1-4 | `S:RESULT CLAIM` | Claim a milestone reward |
| `C:HUNTS` | - | `S:HUNT`* `S:HUNTEND` | The ten riddle hunts and their state for this player |
| `C:HUNT` | 2: hunt id | `S:RESULT HUNT` | `<extra>` is the hunt id |

### 6.7 Portals, blueprints and the regret window

| Op | Fields | Reply | Notes |
| --- | --- | --- | --- |
| `C:PORTALS` | - | `S:PORTAL`* `S:PORTALEND` | Portal destinations bound on this realm |
| `C:GOPORTAL` | 2: portal id | `S:RESULT GOPORTAL` | |
| `C:BPEXP` | 2: kind 0 (build), 1 (room) or 2 (exterior) `[0]`, 3: name `[build]` | `S:BPCODE` then `S:RESULT BPEXP` | The code arrives first, then the result line |
| `C:BPIMP` | 2: code | `S:RESULT BPIMP` | |
| `C:RESTORE` | - | `S:RESULT RESTORE` | Recover a sold house from the archive |

## 7. Server to client

Field 1 is the op. Counts are of the fields after it.

| Op | Fields | Sent for | Notes |
| --- | --- | --- | --- |
| `S:CHUNK` | 2: frame, 3: seq, 4: total, 5: fragment | sec.3 | The only op the client handles before dispatch |
| `S:HELLO` | 2: protocol, 3: module version, 4: enabled, 5: has house | `C:HELLO` | Enabled is always 1 (sec.4) |
| `S:PONG` | 2: milliseconds | `C:PING` | `getMSTime()` |
| `S:RESULT` | 2: op, 3: code, 4: extra `[0]` | most requests | sec.2 |
| `S:HOME` | 2: code, 3: cooldown seconds | `C:HOME` | Where `S:RESULT HOME` would be, because the cooldown is the point |
| `S:BUYRESULT` | 2: code, 3: decor id | `C:BUYDECOR` | |
| `S:NBR` | 2: id, 3: type, 4: name, 5: slot, 6: owned plots, 7: total plots, 8: faction | `C:NEIGHBORHOOD` | |
| `S:PLOT` | 2: index, 3: biome, 4: state, 5: tier, 6: owner name | `C:PLOTS` | No terminator (sec.2) |
| `S:PLOTINFO` | 2: index, 3: biome, 4: price, 5: size class, 6: state, 7: tier, 8: owner name, 9: house name, 10: locked, 11: indoor budget, 12: outdoor budget, 13: indoor used, 14: outdoor used, 15: yard mask, 16: house mask, 17: Armillary preset | `C:PLOT` | The longest line in the protocol |
| `S:DIRENTRY` | 2: index, 3: owner name, 4: tier, 5: locked | `C:DIR` | |
| `S:DIREND` | 2: page, 3: more 0 or 1 | `C:DIR` | |
| `S:VAREA` | 2: type, 3: name, 4: x, 5: y, 6: biome | `C:VAREA` | Type is `AmenityType` (sec.8) |
| `S:VAREAEND` | - | `C:VAREA` | |
| `S:FINDER` | 2: id, 3: name, 4: owned plots, 5: total plots | `C:FINDER` | |
| `S:FINDEREND` | - | `C:FINDER` | |
| `S:HOMES` | 2: housing id, 3: plot index, 4: neighbourhood id, 5: neighbourhood name, 6: selected 0 or 1, 7: size class | `C:HOMES` | |
| `S:HOMESEND` | 2: count | `C:HOMES` | |
| `S:GUEST` | 2: character name, 3: flags | `C:GUESTLIST` | Guests are stored by account; the name is resolved for display |
| `S:GUESTEND` | - | `C:GUESTLIST` | |
| `S:DECORENTRY` | 2: decor id, 3: name, 4: budget cost, 5: placement, 6: surface, 7: owned 0 or 1, 8: price, 9: currency, 10: subcategory | `C:DECOR` | Placement, surface and currency are sec.8 |
| `S:DECOREND` | 2: page, 3: more 0 or 1 | `C:DECOR` | |
| `S:PLACEDENTRY` | 2: placement id, 3: decor id, 4: x, 5: y, 6: z, 7: o, 8: scale, 9: indoor, 10: pitch, 11: roll, 12: pet entry, 13: pet roaming | `C:PLACED` | The pet fields are 0 when the piece holds nothing |
| `S:PLACEDEND` | - | `C:PLACED` | |
| `S:DYE` | 2: colour, 3: name | `C:DYES` | |
| `S:DYEEND` | - | `C:DYES` | |
| `S:STORAGE` | 2: decor id, 3: count | `C:STORAGE`, `C:BUYCOPY` | |
| `S:STORAGEEND` | - | `C:STORAGE` | |
| `S:SHELL` | 2: id, 3: name, 4: size class, 5: level required | `C:SHELLS` | |
| `S:SHELLEND` | - | `C:SHELLS` | |
| `S:ROOM` | 2: id, 3: name, 4: slot cost, 5: level required, 6: buy result | `C:ROOMS` | Field 6 is a `HousingResult`, not a boolean: 0 means the server would sell this shape |
| `S:ROOMS` | 2: slots spent | `C:ROOMS` | The terminator of the room list |
| `S:PORTAL` | 2: id, 3: name, 4: level required | `C:PORTALS` | |
| `S:PORTALEND` | - | `C:PORTALS` | |
| `S:ENDEAVOR` | 2: neighbourhood id, 3: Endeavor id, 4: milestone, 5: completed tasks, 6: name, 7: ends (unix time) | `C:ENDEAVOR` | |
| `S:ENDTASK` | 2: task id, 3: progress, 4: required, 5: text | `C:ENDEAVOR` | |
| `S:ENDTASKEND` | - | `C:ENDEAVOR` | |
| `S:HUNT` | 2: hunt id, 3: state, 4: name, 5: riddle, 6: reward decor name, 7: quest id | `C:HUNTS` | State 0 not taken, 1 looking, 2 found |
| `S:HUNTEND` | - | `C:HUNTS` | |
| `S:BPCODE` | 2: code | `C:BPEXP` | The `PHP1:` code, sent before the result line |

## 8. The numbers on the wire

These are shared constants, mirrored in `addon/HousingUI/Core.lua`. Renumbering any of them is
a protocol break.

**`HousingResult`** - the code in `S:RESULT`, `S:HOME`, `S:BUYRESULT` and field 6 of `S:ROOM`.

| Code | Name | Code | Name | Code | Name |
| --- | --- | --- | --- | --- | --- |
| 0 | `Ok` | 13 | `BadScale` | 26 | `LevelTooLow` |
| 1 | `Disabled` | 14 | `TooManyDecor` | 27 | `StorageEmpty` |
| 2 | `NotInNeighborhood` | 15 | `WrongFaction` | 28 | `EndeavorIncomplete` |
| 3 | `InvalidPlot` | 16 | `TooManyHouses` | 29 | `BadCode` |
| 4 | `PlotTaken` | 17 | `NeighborhoodFull` | 30 | `ArchiveExpired` |
| 5 | `NotOwner` | 18 | `TierTooLow` | 31 | `NotGuild` |
| 6 | `NoHouse` | 19 | `TierMax` | 32 | `AlreadyKnown` |
| 7 | `NotEnoughMoney` | 20 | `NotFound` | 33 | `NoBattlePet` |
| 8 | `BudgetExceeded` | 21 | `GuestRestricted` | 34 | `NotReady` |
| 9 | `InvalidDecor` | 22 | `RateLimited` | 35 | `OnCooldown` |
| 10 | `NotUnlocked` | 23 | `Cooldown` | 36 | `NotInGuild` |
| 11 | `OutOfBounds` | 24 | `NotAllowedHere` | 37 | `GuildRankTooLow` |
| 12 | `WrongSurface` | 25 | `InternalError` | 38 | `WrongNeighborhood` |

**`PlotState`** - `S:PLOT` field 4 and `S:PLOTINFO` field 6: 0 available, 1 owned, 2 locked,
3 mine.

**`ArmillaryPreset`** - `C:ARMILLARY` argument and `S:PLOTINFO` field 17: 0 Default, 1 Natural
Light, 2 Unnatural Dark, 3 Evening Calm, 4 Midnight.

**`AmenityType`** - `S:VAREA` field 2: 0 bulletin board, 1 portal, 2 contractor, 3 flight
point, 4 gathering, 5 scenery, 6 vendor, 7 endeavor, 8 coffer.

**`SizeClass`** - `S:HOMES` field 7, `S:SHELL` field 4, `S:PLOTINFO` field 5: 0 small,
1 medium, 2 large.

**Access** - `C:ACCESS` field 3, `C:GUESTADD` field 3, `S:PLOTINFO` fields 15 and 16, the
space in `C:ACCESS`: six audience bits. 1 stranger, 2 neighbour, 4 guild, 8 party, 16 friend,
32 guest. The defaults are `0x3F` for a yard and `0x3E` for the house itself.

**Currency** - `S:DECORENTRY` field 9: 0 gold, 3363 Community Coupons, anything else a
currency id the realm has.

**Placement and surface** - `S:DECORENTRY` fields 5 and 6: placement 0 outdoor, 1 indoor,
2 either; surface the piece needs.

**Blueprint kind** - `C:BPEXP` field 2: 0 build, 1 room, 2 exterior.

## 9. What the server refuses to do

The rules a reimplementation has to keep, because each of them is a hole somebody has already
tried to climb through:

- An unknown opcode is dropped in silence. Echoing an error would turn the addon channel into
  an amplifier for anything that can send an addon message.
- A message whose first field is not `C:` is dropped. The check is on the third character, so
  a header shorter than three characters cannot be indexed past.
- Every numeric field is parsed by `HousingAddon::ParseUInt` or `ParseFloat`: digits only, a
  length cap, a complete parse, and a finite float. A failed parse answers with that op's
  "invalid" result and does not mutate anything.
- Every id is resolved against the loaded tables before use, and every plot index is checked
  against `kPlotCount`.
- Ownership is checked again at the point of mutation, not only where the window was drawn.
- Names are sanitised and truncated server-side, so a client cannot choose the length.
- The rate limit is checked before the first lookup (sec.5).

## 10. Reimplementing one half

The order that works, because each step is what the next one needs:

1. `C:HELLO` and `S:HELLO`, the version check, and `S:RESULT` with its op echo. Everything
   else is a request that answers through that one line.
2. `C:NEIGHBORHOOD`, `C:PLOTS`, `C:PLOT` and the three replies, which is enough to draw a
   street.
3. `C:DECOR`, `C:BUYDECOR`, `C:PLACED`, `C:PLACE`, `C:MOVE`, `C:REMOVE` - the loop that makes
   a house look like the player's.
4. `S:CHUNK` before any list with more than about four entries, or the decor catalogue will be
   truncated by the 255-byte chat limit.
5. Everything else in table order. The ops are independent of each other apart from the
   handshake.

A client that only reads what it knows stays compatible: every reply either keeps its field
order or adds fields at the end, and the two places that could not do that on their own -
`C:PLACE` and `C:MOVE`'s tilt - made the new fields optional and default them to upright.
