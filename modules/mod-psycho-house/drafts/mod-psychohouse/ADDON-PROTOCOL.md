# The PHSH channel

The island house has two interfaces. The first is gossip: walk up to the Keeper to buy the
house and to the Steward to buy a facility, and the menu is drawn by the server. The second is
this channel, which lets the bundled addon show the same state in a window and place objects by
clicking instead of typing coordinates. Both call the same functions in the module; neither can
do anything the other cannot.

## How a message travels

An addon message is a whisper with the addon prefix attached. The client sends it with
`C_ChatInfo.SendAddonMessage("PHSH", text, "WHISPER", playerName)`, addressed to the character
sending it; the core hands any message carrying the `PHSH` prefix to the module before the
generic whisper, group and channel routing sees it, and the module answers on the same channel.
The reply is a `CHAT_MSG_WHISPER` with language `LANG_ADDON` and the same prefix, constructed
by the module itself, so nothing a player types can be mistaken for a reply.

Two consequences worth stating plainly:

* The channel is per character and goes nowhere else. Nothing in this protocol is broadcast,
  and no record carries another character's data.
* The core has to know the prefix exists. That is one declaration and one early return in
  `src/server/game/Handlers/ChatHandler.cpp`, shipped as
  `patches/0001-phsh-addon-route.patch`. Without the patch the module, the gossip menus and the
  commands all work and the addon window stays silent - which `.psychodiag` reports, so a silent
  window is a diagnosis rather than a mystery.

## Shape of a record

Records are ASCII, `|`-separated, and end at the end of the message. Every field is mandatory
unless the table says otherwise, and every numeric field is parsed strictly on the server: an
integer field is digits only, a float must parse completely and be finite, and a missing field
is a bad request rather than a zero.

Client records begin `C:`, server records begin `S:`. The first field is always the record name.
The field count below does not include the name.

## Records the client sends

| Record | Fields | Payload | Meaning |
| --- | --- | --- | --- |
| `C:HELLO` | 0 | - | The addon has loaded. Answered with `S:HELLO`. |
| `C:PING` | 0 | - | Round-trip check. Answered with `S:PONG`. |
| `C:STATUS` | 0 | - | House state. Answered with one `S:STATUS`. |
| `C:FACILITIES` | 0 | - | The whole facility catalogue. Answered with one `S:FAC` per row and one `S:FACEND`. |
| `C:BUY` | 0 | - | Buy the house. Answered with `S:RESULT`. |
| `C:SELL` | 0 | - | Sell the house. Answered with `S:RESULT`. |
| `C:HOME` | 0 | - | Travel to the house. Answered with `S:RESULT`. |
| `C:BUYFAC` | 1 | facility id | Buy one facility. Answered with `S:RESULT`. |
| `C:OBJECTS` | 0 | - | The standing objects. Answered with one `S:OBJ` per piece and one `S:OBJEND`. |
| `C:PLACE` | 6 | entry, x, y, z, o, scale | Put a piece at absolute coordinates. Answered with `S:RESULT`. |
| `C:PLACEHERE` | 1 | entry, o, scale | Put a piece where the character is standing. `o` and `scale` are optional and default to 0 and 1. The client cannot read world coordinates, so this is the placement the window offers. Answered with `S:RESULT`. |
| `C:REMOVE` | 1 | slot | Remove one piece by slot. Answered with `S:RESULT`. |

An unrecognised record is answered with `S:RESULT` carrying `UNKNOWN` and result 12. It is
answered rather than dropped because a client that sends into silence is the hardest kind of
fault to trace from the outside.

## Records the server sends

| Record | Fields | Optional | Payload | Meaning |
| --- | --- | --- | --- | --- |
| `S:HELLO` | 3 | 0 | protocol, object limit, has house | The answer to `C:HELLO`. A window whose protocol differs from this number stops drawing and says so. |
| `S:PONG` | 1 | 0 | milliseconds | The module's uptime in milliseconds. |
| `S:STATUS` | 7 | 0 | has house, phase, facilities bought, facilities in catalogue, objects standing, object limit, house price | Everything the header line needs, in one record. |
| `S:FAC` | 7 | 0 | id, kind, name, price, owned, entry, is object | One facility row. `kind` is the module's `FacilityKind`; `is object` is 1 when the facility is a game object rather than a creature. |
| `S:FACEND` | 1 | 0 | rows sent | Closes the facility run. A run of zero rows is `S:FACEND|0` with no `S:FAC` before it. |
| `S:OBJ` | 7 | 0 | slot, entry, x, y, z, o, scale | One standing object. Two decimals on the position, four on the orientation, two on the scale. |
| `S:OBJEND` | 2 | 0 | objects standing, object limit | Closes the object run. |
| `S:RESULT` | 2 | 1 | op, result code, echoed field | The answer to any action. `op` is the record that caused it - `BUY`, `SELL`, `HOME`, `BUYFAC`, `PLACE`, `PLACEHERE`, `REMOVE`, or `UNKNOWN`. The echoed field is the facility id, the entry or the slot, whichever the request carried. |

## Result codes

`S:RESULT` carries the module's `HouseResult` enum, in declaration order. The numbers are
appended to only, so a shipped addon keeps understanding a realm that has grown a new failure:

| Code | Meaning |
| --- | --- |
| 0 | done |
| 1 | housing is switched off on this realm |
| 2 | no house on this character yet |
| 3 | the character already has a house |
| 4 | not enough gold |
| 5 | no free phase left on this realm |
| 6 | the character has to be inside the house |
| 7 | no such facility |
| 8 | that facility is already bought |
| 9 | the facility is not for this character's faction |
| 10 | level too low |
| 11 | storage is full |
| 12 | the action did not work |
| 13 | the house is full - remove something first |
| 14 | that is not a placeable object |
| 15 | too far from the house |
| 16 | no object in that slot |

## Limits that are settings, not protocol

The object limit, the radius and the scale band come from the configuration:
`PsychoHouse.Objects.Max`, `PsychoHouse.Objects.Radius`, `PsychoHouse.Objects.Scale.Min` and
`PsychoHouse.Objects.Scale.Max`. The limit is sent to the client in `S:HELLO`, `S:STATUS` and
`S:OBJEND` so a window can draw "7 of 20" honestly, and a realm that changes the setting is
reflected the next time the window asks. The server re-reads the limit on every placement, so a
client with a stale number cannot place one piece more than the realm allows.

## What the addon never does

It does not decide anything. Every button sends one record and waits for the answer, every
round trip re-asks for state, and nothing is cached between sessions except the window's screen
position. A house can change while the window is closed - a sale, a reload, a piece removed
from another client - and a stale list is worse than no list.

## Testing the two sides together

`tools/housing/check_psychohouse.py` reads this document, the switch in
`src/mod_psychohouse_addon.cpp` and the handler table in `addon/PsychoHouse/core.lua`, and
fails when the three disagree about which records exist or how many fields they carry.
`tools/housing/check_psychohouse_addon.py` runs the addon itself in a Lua 5.1 interpreter
against the model of the client in `tools/housing/wow_env.lua`, feeds it one line per record
above, clicks every button and reports what it did. Both run without a server and without a
client; neither proves the module compiles.
