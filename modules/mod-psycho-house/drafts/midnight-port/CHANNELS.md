# Channels: 0 of 34

This is the paragraph that was missing from the port. Everything else in `drafts/midnight-port`
puts furniture in a house. This page is about the other end of the product: a player talking to
their house from whatever chat application they already have open, and the machine in the
middle that carries the conversation.

The count is 0 of 34. None of the thirty-four integrations below is implemented, none has a
test, none has a credential in any store. What follows is the specification they will be built
against: the channel list, the adapter contract each one implements, the two OAuth routes every
OAuth channel needs, the callback rules that make an OAuth flow safe rather than decorative, and
the part that has nothing to do with chat - how to get the game data out of the retail client,
because half of these channels exist to show a picture of a house and the picture needs assets.

I am writing the spec before the code on purpose. Thirty-four integrations written one at a
time turn into thirty-four ways of storing a token, and the third one always looks reasonable
until the ninth contradicts it.

## 1. What a channel is

A channel is one conversation surface with three properties:

| Property | Meaning |
| --- | --- |
| Transport | how a message leaves the server and how an inbound message arrives: a webhook, a socket, a polled API, or an SMTP/IMAP pair |
| Identity | whose name the message carries, and what proves it: an app token per workspace, a bot account per guild, a phone number, an app password, or an OAuth grant |
| Envelope | what one message can carry: text length, attachments, buttons, threads, edits, reactions, and whether a reply can quote another |

Every adapter below normalises those three into one internal shape, so the conversation layer
never asks which channel it is on. A button that says "open the blueprint" is the same button on
Slack, Telegram and IRC; only the rendering differs, and the adapter owns the difference.

## 2. The thirty-four

The status column is the honest one: `none` means no code, no test and no credential exists.
`spec` means this document is the whole of it. Nothing in this table is further along than that,
and the table is the thing to edit when one of them moves.

| # | Channel | Auth | Inbound | Outbound | Status |
| --- | --- | --- | --- | --- | --- |
| 1 | Slack | OAuth 2.0, bot scopes per workspace | Events API over HTTPS, signed | Web API `chat.postMessage` | none |
| 2 | Discord | OAuth 2.0 `bot` + `applications.commands` | Gateway socket, or interactions over HTTPS | REST, per-channel webhook | none |
| 3 | Telegram | Bot token from BotFather | Long poll or webhook | `sendMessage`, `editMessageText` | none |
| 4 | WhatsApp Cloud API | Meta app, system user token, phone number id | Webhook, `X-Hub-Signature-256` | Graph API messages endpoint | none |
| 5 | Signal | Linked device (`signal-cli` / `signald`), no cloud API | Local daemon socket | Local daemon socket | none |
| 6 | iMessage | Apple Business Messages or a Mac bridge; no public API | Bridge daemon | Bridge daemon | none |
| 7 | Matrix | Access token, or OAuth for homeservers that offer it | `/sync` long poll | `PUT /rooms/{id}/send` | none |
| 8 | Mastodon | OAuth 2.0 per instance, registered app | Streaming API or webhook | `POST /api/v1/statuses` | none |
| 9 | Reddit | OAuth 2.0, script or installed app | Polled inbox | `POST /api/comment`, chat | none |
| 10 | X | OAuth 2.0 with PKCE, or OAuth 1.0a for some endpoints | Filtered stream | `POST /2/tweets` | none |
| 11 | Bluesky | App password, or OAuth for the newer flow | `com.atproto.sync` firehose subset | `com.atproto.repo.createRecord` | none |
| 12 | Threads | Meta app, long-lived token | Webhook | Graph API | none |
| 13 | Facebook Messenger | Meta app, page token | Webhook | Send API | none |
| 14 | Instagram DM | Meta app, page token | Webhook | Send API | none |
| 15 | Google Chat | Service account, or OAuth | App endpoint over HTTPS | `spaces/{id}/messages` | none |
| 16 | Microsoft Teams | Entra app, bot framework registration | Bot framework HTTPS endpoint | Bot framework replies | none |
| 17 | Rocket.Chat | Personal access token, or OAuth | Realtime API socket | REST `chat.postMessage` | none |
| 18 | Mattermost | Bot token per server | WebSocket events | REST `posts` | none |
| 19 | Zulip | Bot email + API key | Event queue long poll | `POST /messages` | none |
| 20 | Guilded | Bot token | WebSocket gateway | REST | none |
| 21 | Revolt | Bot token | WebSocket | REST | none |
| 22 | IRC | SASL or server password | Socket, per network | Socket | none |
| 23 | XMPP | JID + password, or SASL EXTERNAL | Socket, per account | Socket | none |
| 24 | Twitch chat | OAuth (chat scopes) or anonymous read | IRC over WebSocket | IRC over WebSocket | none |
| 25 | YouTube live chat | OAuth 2.0 (Google) | `liveChatMessages` poll | `liveChatMessages.insert` | none |
| 26 | Viber | Bot token from the partner panel | Webhook | `send_message` | none |
| 27 | LINE | Channel access token | Webhook, signature header | Push message API | none |
| 28 | WeChat | App id + secret, per official account | Webhook with signature | Customer service message API | none |
| 29 | KakaoTalk | REST API key, channel | Webhook | `talk/memo/default/send` | none |
| 30 | Zalo | OA access token | Webhook | OA message API | none |
| 31 | VK | Community access token | Callback API | `messages.send` | none |
| 32 | Feishu | App id + secret, tenant token | Event subscription | `im/v1/messages` | none |
| 33 | Email | IMAP + SMTP; OAuth2 for Gmail and Microsoft 365, app password otherwise | IMAP IDLE or poll | SMTP, or the provider's send API | none |
| 34 | SMS | Provider API key (Twilio or equivalent) | Provider webhook | Provider messages endpoint | none |

Nine of these are named in the brief and twenty-five are not; the table is the count, and the
count is 34.

Three groups matter more than the numbering:

* **OAuth channels** - 1, 2, 7, 8, 9, 10, 11, 15, 17, 24, 25, 33. They need the two routes
  in section 4. Two more support OAuth where the deployment offers it - 3 by webhook token and
  7 by homeserver - and both work without it, which is why they are counted with the protocols
  and tokens rather than here.
* **Token channels** - 3, 5, 6, 12, 13, 14, 16, 18, 19, 20, 21, 26, 27, 28, 29, 30, 31, 32,
  34. They need a token store, a rotation plan and a test that proves the token is not the
  thing being logged.
* **Protocol channels** - 22, 23. They need neither: a socket, a password, and a reconnect
  policy that does not hammer a network after a ban.

## 3. The `channel_agent`

`channel_agent` is the row that binds an agent to a channel. It is not a bot, and it is not a
conversation: it is the record that says *this agent speaks on this channel, in this workspace,
as this identity, with these permissions, under these limits*.

### 3.1 The record

| Field | Type | Why it exists |
| --- | --- | --- |
| `id` | uuid | the binding's own identity; every message, log line and metric refers to it and never to a channel name |
| `channel` | enum, 1-34 | which adapter. Changing it is a new binding, never an edit |
| `agent_id` | uuid | which agent answers. One agent may be bound to many channels |
| `external_id` | text | the workspace, guild, server, phone number, room or account the binding lives in |
| `credential_id` | uuid | a reference into the secret store. The secret itself is never in this row, never in a log and never in an export |
| `display_name` | text | the name the channel shows |
| `locale` | text | the language the agent answers in, independent of the operator's |
| `scopes` | text[] | what this binding may do, in the adapter's own vocabulary |
| `allow_list` | text[] | which external users may speak at all. An empty list means nobody, not everybody |
| `rate_limit` | integer | messages per minute per conversation |
| `state` | enum | `pending`, `active`, `paused`, `revoked`, `failed` |
| `created_at`, `updated_at`, `last_seen_at` | timestamps | `last_seen_at` is what makes a dead binding visible |
| `failure_count`, `last_error` | integer, text | a binding that has failed a hundred times in a row is not active whatever the state says |

Two rules are load-bearing. First, `credential_id` and not the credential, because a table
that holds tokens ends up in a backup, an export and a support ticket. Second, `allow_list` is
empty by default and empty means nobody: an integration that answers the whole workspace the
moment it is installed is a support incident waiting for a reason.

### 3.2 The contract every adapter implements

```
interface ChannelAdapter {
    // Lifecycle
    Authorize(binding) -> url | token instructions
    CompleteAuthorization(binding, code, state) -> credential
    Revoke(binding) -> ok | error
    Health(binding) -> { state, last_seen, notes }

    // Inbound
    VerifyInbound(request) -> bool             // signature, timestamp window, replay
    ParseInbound(request) -> [Envelope]        // one request may carry several events
    Acknowledge(request) -> response           // the fast 2xx, before any work

    // Outbound
    Send(binding, conversation, Message) -> MessageRef
    Edit(binding, MessageRef, Message) -> ok | error
    Delete(binding, MessageRef) -> ok | error
    Render(Message) -> channel-specific payload  // buttons, threads, attachments

    // Identity
    Describe() -> { transport, capabilities, limits, scopes }
}
```

`VerifyInbound` runs before `ParseInbound`, and `Acknowledge` runs before any queue write. Those
two orderings are the difference between an integration that survives a retry storm and one that
answers a hundred times to the same message.

### 3.3 What the binding does with a message

```
inbound request
  -> VerifyInbound        reject unsigned, stale or replayed requests
  -> Acknowledge          2xx within the channel's own tolerance (Slack 3s, others less)
  -> dedupe by event id   a retried delivery must not be answered twice
  -> allow_list check     an unknown external user is dropped and logged, not answered
  -> rate limit           per conversation and per binding
  -> normalise            Envelope -> { channel_agent, conversation, author, text, attachments }
  -> conversation layer   the same handler the addon and the web page call
  -> Render               Message -> channel payload
  -> Send                 with retry, and a stored MessageRef for edits
```

### 3.4 Capability table the conversation layer reads

The layer above the adapters asks for capabilities instead of asking which channel it is. A
channel that cannot edit a message gets a new message where an edit was intended; a channel with
no threads gets a flat reply; a channel that allows 200 characters gets the short form of the
house summary and a link.

| Capability | True for | Behaviour when false |
| --- | --- | --- |
| `edit` | 1, 2, 3, 4, 7, 21, 23, 25, 28, 34 | post a new message |
| `threads` | 1, 2, 8, 12, 16, 18, 19, 32 | reply in the main conversation |
| `buttons` | 1, 2, 3, 4, 15, 16, 26, 27, 28, 30, 32 | numbered text options |
| `attachments` | 1, 2, 3, 4, 7, 8, 13, 14, 15, 16, 18, 19, 21, 26, 27, 28, 32, 33 | a link to the image |
| `reactions` | 1, 2, 3, 7, 8, 18 | acknowledgements in text |
| `long_text` (2000+) | 1, 2, 3, 7, 8, 19, 33 | chunked or short form |

## 4. The OAuth routes

Two routes per OAuth channel, plus two endpoints that are not channels at all. All of them are
mounted under the adapter's prefix, so adding a channel adds six paths and no edits to the router.

```
GET  /channels/{channel}/oauth/start
GET  /channels/{channel}/oauth/callback
POST /channels/{channel}/oauth/refresh
POST /channels/{channel}/oauth/revoke
GET  /channels/{channel}/webhook          some channels verify with a GET challenge
POST /channels/{channel}/webhook
GET  /channels/{channel}/health
```

### 4.1 Start

```
GET /channels/slack/oauth/start?binding=<uuid>&return=<relative path>
```

Behaviour, in this order:

1. the caller must be an authenticated operator with `channel_agent:authorize` on the binding;
   an unauthenticated start is a 401 and nothing else happens;
2. the binding must exist, be in `pending` or `failed`, and be for this channel;
3. generate `state`: 32 bytes from a CSPRNG, stored server-side against the binding with a
   10-minute expiry, single use, and bound to the session that started it;
4. generate a PKCE verifier where the channel supports it (1, 10, 11, 25, 33), and store only
   its SHA-256 challenge in the pending row;
5. redirect to the channel's authorize URL with the channel's own scope set, the exact
   registered redirect URI, and `state`;
6. write one audit row: who started, which binding, which channel, when. No token, no code.

### 4.2 Callback

```
GET /channels/slack/oauth/callback?code=<code>&state=<state>
```

Behaviour, in this order, and every step is a rejection rather than a repair:

1. `state` must exist, be unexpired, unused and belong to the same session. Consume it first,
   then continue: a state consumed after the token exchange is a state that can be replayed;
2. the channel must echo the same `redirect_uri` it was given; a mismatch is a 400 and a
   logged event;
3. exchange `code` (with the PKCE verifier where used) for tokens over the channel's token URL;
4. read the identity the token belongs to, and compare `external_id` with the binding: a token
   for a different workspace does not activate a binding, it fails it;
5. store the credential in the secret store, encrypted, keyed by `credential_id`; store the
   refresh token separately with its own expiry; never log either;
6. set `state = active`, `last_seen_at = now`, `failure_count = 0`;
7. redirect to `return`, with `channel=<n>&binding=<uuid>&result=ok`.

Failure leaves the binding `pending`, increments nothing, and redirects with
`result=<error_code>`. The codes are fixed, because an error page that says what happened is
half a support ticket: `state_expired`, `state_replayed`, `session_mismatch`, `redirect_mismatch`,
`code_rejected`, `identity_mismatch`, `store_unavailable`, `scope_missing`.

### 4.3 Refresh, revoke, and the three channels that are not OAuth

`POST /oauth/refresh` is called by a scheduler, never by a request path: a token that expires
during a message becomes a message the player never receives. Refresh runs at 80% of token
lifetime, and three consecutive failures set `state = failed` and stop the scheduler for that
binding.

`POST /oauth/revoke` exists because uninstalling an integration must actually revoke. It calls
the channel's own revoke endpoint where one exists, then marks the credential dead locally
whether or not the call succeeded, and writes an audit row either way.

Telegram, Signal, iMessage and the token channels in section 2 have no start route: they have a
paste-a-token screen, a test call, and the same credential store. The absence of OAuth is not the
absence of a lifecycle.

### 4.4 What the routes must never do

| Never | Because |
| --- | --- |
| put a token or a code in a URL that is rendered | it lands in browser history, referrers and logs |
| accept `state` from a cookie alone | the cookie is the session, not the binding; both are required |
| reuse a `state` | a replayed callback is the one flow that turns a token exchange into an account takeover |
| store a refresh token in the same row as the binding | one backup of one table is then a credential leak |
| answer a webhook with a 500 because the queue was full | the channel retries, the retries multiply, the queue stays full |
| trust `X-Forwarded-For` for the allow list | it is a header, not a fact |

## 5. Webhooks and the channels without them

Twelve channels deliver inbound messages over HTTPS (4, 12, 13, 14, 15, 26, 27, 28, 30, 31,
32, 34), each with its own signature scheme:

| Channel | Signature | Replay window |
| --- | --- | --- |
| WhatsApp, Messenger, Instagram, Threads | `X-Hub-Signature-256`, HMAC-SHA256 of the raw body | 5 minutes |
| Slack | `X-Slack-Signature` over `v0:timestamp:body` | 5 minutes |
| Google Chat | `Authorization: Bearer` with a Google-signed JWT | token lifetime |
| Teams | Bot framework JWT | token lifetime |
| LINE | `X-Line-Signature`, Base64 HMAC-SHA256 | 5 minutes |
| Viber | `X-Viber-Content-Signature`, HMAC-SHA256 | 5 minutes |
| WeChat | `signature` over the token, timestamp and nonce | 5 minutes |
| KakaoTalk, Zalo, VK, Feishu | per-channel token or HMAC header | 5 minutes |

The rule is the same everywhere: verify over the **raw body bytes**, before any JSON parse,
because a re-serialised body is a different body and the signature is over the bytes.

The other twenty-two are polled or socketed. Each needs three things and no webhook: a cursor
that survives a restart, a backoff that respects the channel's own limits, and a health check
that distinguishes "quiet" from "disconnected".

## 6. What the conversation layer actually offers on a channel

The point of the integration is not to chat. It is to reach the house from a phone without the
client. The vocabulary is the same one the addon and the commands use, which is what keeps the
three surfaces honest:

| Intent | What comes back |
| --- | --- |
| `status` | plot, level, budget spent of total, pieces placed, next Endeavor |
| `catalogue <category>` | the pieces that category has, with prices, as a paged list |
| `place <piece> <x> <y> <z>` | a confirmation, or the same failure code the addon sees |
| `blueprint list` / `blueprint load <name>` | the named snapshots |
| `travel` | the same teleport the key does |
| `render` | one image of the plot from the last capture, with a cache key |
| anything else | the agent's own answer, within the binding's scopes |

A channel that can carry an image gets `render` as a picture; one that cannot gets a link. Both
are the same capture, produced by the same job, which is the subject of the next section.

## 7. Getting all the data out of the retail client

Every picture above, every icon in the catalogue, and every mesh a ported piece needs comes out
of a retail client install. That extraction is separate from the channels, but the channels are
the reason it has to be repeatable rather than a one-off evening.

### 7.1 What is on disk, and what is not

A modern retail install is one `Data/` folder of CASC archives plus an install manifest. Nothing
in it is a file with a name until a listfile says which FileDataID maps to which path. So the
first step is never extraction; it is identification:

| Piece | Where it comes from | Notes |
| --- | --- | --- |
| build number | `.build.info`, or the launcher | 8.3.7 is 35662; a Midnight build is a different number and a different content set |
| CDN config and encoding | `.build.info` | the EKey and the encoding file are what make the archives readable |
| listfile | community-maintained, per build | without it every file is a number; with it the names are known |
| TACT keys | community key list for the encrypted archive indices | several DB2 tables are encrypted per build |
| install manifest | generated from the CDN config | needed for a clean install, not for extraction |

### 7.2 The pipeline, in order

1. **Copy, do not work in place.** A retail client that is patched mid-extraction yields half
   of one build and half of another, and no error.
2. **Read the build number** and pin it. Every id written later is annotated with it.
3. **Load the listfile and the keys** for that exact build.
4. **Extract by FileDataID.** Two families matter here: display data and file data.
   `ItemAppearance`, `ItemModifiedAppearance`, `ItemSparse`, `CreatureDisplayInfo`,
   `GameObjectDisplayInfo`, `ModelFileData`, `TextureFileData`, `ManifestInterfaceData`,
   `AreaTable`, `Map`, `LightData`, `UiTextureAtlas`, `UiTextureAtlasMember`.
5. **Convert DB2 to something queryable.** A DB2 dump is a CSV plus a schema; the schema is
   not in the file, so the conversion needs the structure definition for that build. Two DB2s
   with the same name across builds routinely have different column counts.
6. **Resolve display ids to files.** `GameObjectDisplayInfo.FileDataID` and
   `CreatureDisplayInfo.ModelID` are the joins that turn a game id into an M2, and an M2 into a
   set of textures. Nothing else in the pipeline is as fragile as this step, because a display
   row may name a `.mdx` that the client has long since replaced.
7. **Extract the art**: `.m2` (and `.skin` for BfA, `.skel`/`.anim` for later builds), `.blp`,
   `.wmo`, `.adt`, `.anim`.
8. **Extract the icons**: the interface atlases and the item icons, either as a `.blp` or as
   the atlas member coordinates plus one atlas texture.
9. **Write a manifest**: build number, listfile hash, key set hash, extraction time, file count,
   total bytes, and one row per extracted file with its FileDataID and SHA-256. That manifest is
   what makes a second extraction comparable with the first.
10. **Keep the retail files out of the repository.** They are large, they are not mine, and
    nothing in the module ships them.

### 7.3 What each part is for

| Data | Feeds | In this repository today |
| --- | --- | --- |
| `ItemSparse` + `ItemAppearance` | the retail item id behind a decor piece, its name and its quality | `data/town-center-items.csv` holds 739 ids; 450 pieces in the port carry one |
| `GameObjectDisplayInfo` + `ModelFileData` | an 8.3.7 stand-in for a Midnight piece, and later the real mesh | 177 stand-in game objects are already chosen; doc 12 sec.4 is the route to the real ones |
| `CreatureDisplayInfo` | vendor models and pet beds | bound at install from the realm's own tables |
| `UiTextureAtlas` | the icons in the addon's fifteen tabs | the addon currently uses client-native icon paths only |
| `ManifestInterfaceData` | which interface file a UI element comes from | doc 07 sec.4, for the patched menus |
| `AreaTable`, `Map` | zone and map names for a custom housing map | doc 12 sec.4.8 |
| `LightData` | the night set's ambience | `bind_night_variants.sql` binds a second entry where the realm has one |
| `.adt` / `.wmo` | a custom plot, if a computed ring is ever not enough | not started, and doc 12 sec.4.8 says why it is last |

### 7.4 What this repository can and cannot do about it

There is no client in this workspace, no CASC reader and no listfile. The extraction above has
never been run here, and this document does not pretend it has. What this repository does hold
is the shape of the result: `generated/midnight-port-worklist.csv` has one row per ported piece
and six empty columns - display id, M2, BLP, icon, patch, status - that the extraction fills in.
A piece is ported when those columns are filled and the manifest says which build they came from.

Two practical notes, both from having got them wrong on paper first:

* **FileDataIDs are per build.** A decor piece extracted from one build and a mesh extracted from
  another will not join, and the failure looks like a missing model rather than a version
  mismatch. Pin the build in the manifest and refuse to mix.
* **The client is not the server.** A display id that exists in the retail client does not exist
  in an 8.3.7 server's `gameobject_template` until a row is written for it. That row - entry,
  type, display id, name, size - is the five-column DB2 work in doc 12 sec.4, and it is what
  turns an extracted mesh into a piece a player can place.

## 8. How this will be checked when it exists

Nothing here is testable today, so the checks are stated as the conditions that will make it
testable, and each one is written down with the file it will live in:

| Check | What it proves | Where |
| --- | --- | --- |
| adapter contract | every adapter implements the interface in section 3.2, including the ones with no OAuth | a per-adapter test that runs against a fixture transport |
| signature verification | a body edited after signing is rejected, and a valid body is accepted | fixed vectors per channel, in the repository |
| state handling | a consumed `state` is rejected the second time, and an expired one is rejected | the OAuth test, with a controllable clock |
| secret hygiene | no token, code or refresh token appears in a log line, an audit row or an export | a log scan over the test run's own output |
| dedupe | the same event id delivered twice produces one answer | the fixture transport, replaying one delivery |
| capability fallback | a channel without `edit` posts instead of editing, and the text still makes sense | one test per capability, run across all thirty-four descriptors |
| extraction manifest | two extractions of the same build produce the same file hashes; two different builds do not | the extractor's own `--verify` mode |
| worklist completion | `port_worklist.py --check` passes with the six port columns filled, in a fillable format | the tool that already exists |

The last row is the only one that can be run today, and it can only be run empty: it reports 571
pieces, 450 with a retail item id, and every port column blank. That is the honest status of the
whole page - a specification with a count of zero and a worklist waiting for a manifest.
