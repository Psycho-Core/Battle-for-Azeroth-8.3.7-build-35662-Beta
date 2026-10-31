# 04 - Addon protocol (`BFAH`)

The 8.3.7 client cannot render a housing UI, so the UI is an addon. This document is the
contract between that addon and the worldserver, and it is the part I expect to change
most often, so it is versioned independently of the module.

## 1. Transport

| Item | Value |
| --- | --- |
| Addon prefix | `BFAH` (registered with `RegisterAddonMessagePrefix`) |
| Client -> server | `SendAddonMessage("BFAH", msg, "WHISPER", UnitName("player"))` |
| Server -> client | `WorldPackets::Chat::Chat` with `LANG_ADDON`, `CHAT_MSG_WHISPER`, the player as both sender and receiver, and the prefix set |
| Server hook | `WorldSession::HandleChatAddonMessage`, before the normal switch: if `prefix == "BFAH"`, hand off and **return** - nothing about a housing message should ever reach guild/party/whisper chat |

Whispering yourself is ugly but it is the one addon channel that is always available
(no group, no guild, no channel membership required). The server intercepts the prefix
before any routing happens, so the message never actually appears as a whisper to anyone.

The interceptor accepts the prefix on *any* chat type, so a future UI can use `PARTY`
for group-scoped operations without a server change.

## 2. Framing

- ASCII only, `|` separates fields, `;` separates records inside one payload.
- Every message starts with a direction marker: `C:` (client -> server) or `S:` (server ->
  client), followed by an opcode in upper case.
- Numeric fields are decimal integers or `%.3f` floats. No locale-dependent formatting  - 
  the addon formats floats with `string.format("%.3f", v)`.
- Empty string fields are legal (`||`), so fixed field counts are safe.
- Unknown opcodes are ignored by both sides, and a malformed message is dropped with a
  log line - never an error to the client, because error echoes on an untrusted channel
  are an attack surface.

Example:

```
C:PLOT|13
S:PLOTINFO|13|2|250000|1|3|1|Thordekk|Orchard Rest|0|120|65|37|22
```

## 3. Chunking

A single addon message payload is capped at 255 bytes by the client. Server payloads that
can exceed that (plot lists, directories, catalogues) are split:

```
S:CHUNK|<msgId>|<seq>|<total>|<fragment>
```

- `msgId` is a `uint8` counter per player, so the client can tell two interleaved
  responses apart.
- `seq` is 1-based, `total` is the number of chunks.
- Each fragment is at most **200** bytes and is always cut on a `;` boundary, so
  reassembly is plain concatenation.
- The client reassembles by `msgId` and dispatches the joined payload through the same
  parser as an unchunked message. If `total` chunks do not arrive within 10 seconds the
  buffer is discarded.

Nothing larger than 200 bytes is ever a single un-framed message, so the client never has
to handle both cases for the same opcode.

## 4. Opcodes

### 4.1 Handshake

| Client | Server | Notes |
| --- | --- | --- |
| `C:HELLO\|<proto>` | `S:HELLO\|<proto>\|<moduleVersion>\|<enabled>\|<hasHouse>` | `enabled` is 0 when the custom map is missing or the module is switched off; the addon then hides the whole UI. `hasHouse` lets the addon show "buy" vs "manage" on login. |
| `C:PING` | `S:PONG\|<secondsSinceStartup>` | Liveness and clock check. |

Protocol version is currently **3**. The client refuses to talk if the major version it
receives is lower than it expects, and shows a "housing addon out of date" line instead of
failing silently.

### 4.2 Neighbourhood and plots

| Client | Server | Notes |
| --- | --- | --- |
| `C:NEIGHBORHOOD` | `S:NBR\|<id>\|<type>\|<name>\|<slot>\|<owned>\|<total>\|<faction>` | Summary of the neighbourhood the player is currently in. |
| `C:PLOTS` | `S:PLOT\|<index>\|<biome>\|<state>\|<tier>\|<owner>` x55 | Chunked. `state`: 0 available, 1 owned, 2 locked, 3 mine. |
| `C:PLOT\|<index>` | `S:PLOTINFO\|<index>\|<biome>\|<price>\|<size>\|<state>\|<tier>\|<owner>\|<name>\|<locked>\|<budgetIn>\|<budgetOut>\|<usedIn>\|<usedOut>` | |
| `C:BUY\|<index>` | `S:RESULT\|BUY\|<code>` | |
| `C:SELL` | `S:RESULT\|SELL\|<code>` | |
| `C:TIER\|<tier>` | `S:RESULT\|TIER\|<code>` | Upgrade request, priced server-side. |
| `C:NAME\|<text>` | `S:RESULT\|NAME\|<code>` | Truncated to 64 bytes server-side, escaped for SQL. |
| `C:LOCK\|<0\|1>` | `S:RESULT\|LOCK\|<code>` | |
| `C:ENTER\|<index>` | `S:ENTER\|<code>` | Server teleports to the plot's yard; the client never moves the player itself. |
| `C:INTERIOR\|<0\\|1>` | `S:INTERIOR\|<code>` | Enter (1) or leave (0) the house interior of the plot the player is standing in. |
| `C:HOME` | `S:HOME\|<code>\|<cooldownRemaining>` | The Homestone. `cooldownRemaining` in seconds when `code == 23` (on cooldown). |
| `C:LEAVE` | `S:LEAVE\|<code>` | Return to the Town Center. |
| `C:DIR\|<page>` | `S:DIRENTRY\|<plot>\|<owner>\|<tier>\|<locked>` xN, then `S:DIREND\|<page>\|<more>` | Bulletin board / neighbourhood directory. 20 entries per page. |

### 4.3 Decor

| Client | Server | Notes |
| --- | --- | --- |
| `C:DECOR\|<category>\|<page>` | `S:DECORENTRY\|<id>\|<name>\|<cost>\|<placement>\|<surface>\|<unlocked>\|<price>\|<currency>` xN, then `S:DECOREND\|<page>\|<more>` | Catalogue slice, already filtered to what the account can use and buy. |
| `C:PLACED` | `S:PLACEDENTRY\|<placementId>\|<decorId>\|<x>\|<y>\|<z>\|<o>\|<scale>\|<indoor>` xN, then `S:PLACEDEND` | Everything currently in the player's house. |
| `C:PLACE\|<decorId>\|<x>\|<y>\|<z>\|<o>\|<scale>\|<indoor>` | `S:RESULT\|PLACE\|<code>\|<placementId>` | |
| `C:MOVE\|<placementId>\|<x>\|<y>\|<z>\|<o>\|<scale>` | `S:RESULT\|MOVE\|<code>` | |
| `C:REMOVE\|<placementId>` | `S:RESULT\|REMOVE\|<code>` | |
| `C:BUYDECOR\|<decorId>` | `S:BUYRESULT\|<code>\|<decorId>` | Spends gold, unlocks in the collection. |

Placement coordinates are **world** coordinates on the housing map, not plot-relative.
I chose world coordinates because it keeps the server validation trivial (one bounding
sphere per plot) and keeps the addon free to compute positions with its own map texture
without a coordinate transform the server would have to mirror.

### 4.4 Guests

| Client | Server | Notes |
| --- | --- | --- |
| `C:GUESTLIST` | `S:GUEST\|<name>\|<flags>` xN, then `S:GUESTEND` | Character names, not account ids - accounts are never exposed on the wire. |
| `C:GUESTADD\|<name>\|<flags>` | `S:RESULT\|GUESTADD\|<code>` | Flags: bit 0 enter, bit 1 edit, bit 2 manage guests. |
| `C:GUESTDEL\|<name>` | `S:RESULT\|GUESTDEL\|<code>` | |
| `C:VAREA` | `S:VAREA\|<type>\|<name>\|<x>\|<y>\|<biome>` xN, then `S:VAREAEND` | Amenity pins for the map view. |

## 5. Result codes

One enum, `HousingResult`, mirrored in the addon's localisation table. The numbers are
part of the protocol and must not be renumbered.

| Code | Name | Player-facing meaning |
| --- | --- | --- |
| 0 | `Ok` | - |
| 1 | `Disabled` | Housing is switched off on this realm |
| 2 | `NotInNeighborhood` | You are not standing in a neighbourhood |
| 3 | `InvalidPlot` | No such plot |
| 4 | `PlotTaken` | That plot already has an owner |
| 5 | `NotOwner` | Not yours to change |
| 6 | `NoHouse` | You do not own a house here |
| 7 | `NotEnoughMoney` | - |
| 8 | `BudgetExceeded` | Decor budget full |
| 9 | `InvalidDecor` | Unknown decor item |
| 10 | `NotUnlocked` | You have not collected that decor |
| 11 | `OutOfBounds` | Outside the plot |
| 12 | `WrongSurface` | Not placeable on that surface |
| 13 | `BadScale` | Scale out of range |
| 14 | `TooManyDecor` | Hard cap on placed pieces |
| 15 | `WrongFaction` | Your faction cannot buy here |
| 16 | `TooManyHouses` | You already own a house for this faction |
| 17 | `NeighborhoodFull` | No free plots |
| 18 | `TierTooLow` | Upgrade prerequisites not met |
| 19 | `TierMax` | Already at maximum tier |
| 20 | `NotFound` | Generic missing row |
| 21 | `GuestRestricted` | The plot is locked |
| 22 | `RateLimited` | Slow down |
| 23 | `Cooldown` | Homestone on cooldown |
| 24 | `NotAllowedHere` | Wrong map / interior for that action |
| 25 | `InternalError` | Something went wrong, logged server-side |

## 6. Server-side validation rules

Applied to every message, in this order, before any state changes:

1. Prefix, direction marker and opcode parse; unknown -> drop.
2. Rate limit: 20 messages per 10 s per player (`Housing.RateLimit.Messages`). Over the
   limit -> `RateLimited`, and the message is dropped rather than queued.
3. Module enabled and player on a housing map (except `HELLO`, `PING`, `HOME`).
4. Access check through `HousingMgr::CheckAccess` for the target plot/house.
5. Field validation: ids must resolve; floats must be finite and inside the plot's
   bounding sphere; scale clamped to the decor's `scale_min/scale_max`; strings truncated
   and escaped.
6. Business rules: budget, tier, money, collection ownership.
7. Only then mutate state and reply.

The reply always comes after the state change has been accepted, so `S:RESULT|Ok` implies
"the server did it", not "the server will think about it".

## 7. Client harness

`addon/HousingUI/` in this tree is the protocol harness I use for testing: it implements
framing, chunking, the parser and a slash-command plus a raw log frame, but no real UI.
It exists so I can verify the server half without a designed interface, and so that the
person doing the UI work has a working transport to build on.

```
/housing ping
/housing plots
/housing plot 13
/housing place 1001 0.5 1 0.2
/housing proto        -- dump the parsed opcode table and version
```
