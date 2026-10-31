# 11 - Verification ledger

Every number in this folder had to come from somewhere, and this is the page that says
where. It is also the page that says what I could not check, because a document that only
lists its passes is not a verification record.

Run the mechanical part any time:

```
python3 tools/housing/audit.py
```

Seven checks, one line each, exit code 1 on a finding:

| Check | What it enforces |
| --- | --- |
| `ascii` | Every tracked text file is pure ASCII, byte for byte. No smart quotes, no Cyrillic or Chinese left over from source comments, no stray section signs |
| `voice` | No first person plural, no second person address, no tooling talk. Quoted material - a site error message, a quest name, a code sample - is stripped before the scan so a quoted string cannot mask a real slip |
| `links` | Every relative markdown link and every backticked `data/`, `docs/`, `sql/`, `src/`, `addon/`, `tools/` path resolves on disk. Globs are expanded; paths into the game source tree and the post-install `PsychoHouse` names are listed as intentional |
| `counts` | The headline numbers in `README.md` equal the CSVs: 739 item ids, 1 056 stock rows, 87 blank prices, 618 mapped decor names |
| `csv` | Item ids are unique per vendor, every stock vendor exists in `data/vendor_npc_ids.csv`, and any vendor without stock rows is named rather than skipped |
| `sql` | No blank field and no unbalanced quote in any generated `INSERT` tuple |
| `mapping` | Every decor row has a non-zero game object entry, a known constant name, and one of the five match classes |

Current result:

```
ascii    ok
voice    ok
links    ok
counts   ok
csv      ok
sql      ok
mapping  ok

0 finding(s) across 7 check(s)
```

This is the first pass where I have nothing open in the mechanical checks. What is still incomplete is incomplete in **volume**, not in consistency: the rows past the 50 row cap, counted exactly in section 5.

## 1. Where each number comes from

| Number in the docs | How it was obtained | Checked against |
| --- | --- | --- |
| 39 vendors, 27 + 12 split by neighborhood | UmbraMarket vendor index, cross read against the Wowhead zone pages for Founder's Point (16105) and Razorwind Shores (15524) | `data/vendor_npc_ids.csv` row count |
| 1 155 catalogue items | Sum of the per-vendor item counts on the same index | The per-vendor table in doc 09 |
| 1 056 stock rows | Transcribed from live vendor pages, then merged through `merge_capture.py`; the pages that render were transcribed whole, not sampled | `wc -l data/town-center-stock.csv` |
| 739 item ids | Wowhead "Sold by NPC" listings and NPC sell tabs, vendor by vendor; names diffed against the stock CSV before a single id was written | `data/town-center-items.csv`, `source` column |
| 439 distinct ids | Duplicates are items several vendors sell (the Orgrimmar and Stormwind furniture runs, Wooden Chamberstick, the Hallow's End set) | `data/town-center-items.csv`, sorted by id |
| 87 blank prices | Stock rows whose vendor page shows no cost. Blank means unpublished, never free | doc 09 sec.7.6 lists every one by vendor |
| 618 mapped decor names | Draft A's decor count, one mapping row per name | `data/town-center-go-map.csv` |
| 906 candidate game objects | Rows of `generated/source-asset-index.csv` where `kind = gameobject`, enum constants rejected | `tools/housing/map_decor_go.py --stats` |
| 571 decor rows with an entry | Generated SQL, one tuple per mapped name | `tools/housing/audit.py` mapping check |
| 350 native decor rows | Index rows whose constant and category look like housing decor | `drafts/bfa-native/sql/world/03_decor_native.sql` |

## 2. What was checked by reading, not by script

The mechanical checks catch format and arithmetic. They cannot catch a wrong claim, so I re-read the sources myself for these:

- Phase numbering and cross references across docs 02 to 09: every "doc N sec M" points at
  a section that exists, with the claimed content.
- The four retail quest names and the 93057 starting id, against the two Blizzard posts and
  the Wowhead quest chain.
- Every gameplay constant that goes into code: budget bands, the 4x4 grid, `kMaxPlacedDecor`
  at 400, the 15 minute Homestone cooldown, the 10000 + slot * 64 + index phase scheme on
  map 900.
- The `housing_decor` column order in doc 03 against `HousingDecor.cpp`'s column list, which
  is the one place where a reordering would silently load garbage.
- The 8.3.7 API claims: the script loader's glob and `Add<Dir>Scripts()` naming, the
  `ChatCommand` initialiser layout, `PhasingHandler` as the only phase entry point,
  `GameObject::CreateGameObject` taking a quaternion, `CharacterDatabase.EscapeString` not
  returning `std::string`.

## 3. What could not be verified here

| Not verified | Why | What it means for the drafts |
| --- | --- | --- |
| The C++ compiles | The sandbox has no boost, MySQL or OpenSSL headers, so not even the core headers parse | Every file is written against the signatures in this tree, and `check_cpp.py` checks what a compiler check would catch first: blocks balance, every definition matches a declaration, every table and column in a query string exists in the module schema, no `fields[N]` runs past the end of its SELECT, every include resolves |
| The SQL parses | No MySQL server or grammar library available | Field counts per tuple are checked mechanically; the schema itself is ordinary `CREATE TABLE` and is reviewed by eye |
| The CMake patch actually configures | No `cmake` binary in the sandbox | The patch only adds a `file(GLOB)` and one early return, both standard CMake; the script prints exactly what it changes and keeps a backup |
| Retail values behind `[?]` | Not published by any source I could reach | Left as `[?]` on purpose. Nothing in the drafts depends on those numbers |
| Client behaviour of the addon | The addon is a transport harness, not a UI; the patched client is not present here | The protocol, the message prefix and the SavedVariables names are in doc 07 with the exact frames to log |

## 4. Per-document status

| Document | Claims | Status |
| --- | --- | --- |
| 01 retail reference | Prices, quests, budgets, zones, Endeavor scaling | Facts marked `[F]` are sourced, `[?]` are open. Two open items: the remaining quest ids in the introduction chain, and the exact Endeavor plot bonus percentage |
| 02 feature matrix | Which retail feature maps onto which 8.3.7 mechanism | Complete as a mapping; every row names a mechanism that exists in this tree |
| 03 asset catalogue | 28 314 named constants, 1 570 game objects, 988 decor shaped | Recounted from the index this round: 1 570, 460 item, 4 912 creature constants |
| 04 content spec | Quest ids 900100-900103, Endeavor tables, reward bands | Complete; the four quests are now rows in `06_quests.sql` in both drafts, and the two agree with this spec line for line |
| 05 build plan | Phase 0 to phase 6, file lists per phase | Complete; the file lists match the shipped `src/` tree |
| 06 blueprint, dye, pet bed | `PCH1` payload format, checksum, dye channels | Complete. The checksum is no longer only specified: `Crc32`, `Base62`, `Checksum` and `ValidateBlueprint` are in `Core.lua`, called by the Blueprints tab and by `/housing bp`, and the CRC matches the standard check value |
| 07 client patching | `.toc`, `## Interface: 80300`, message prefix, DB2 notes | Complete; the addon ships as `addon/HousingUI/` and is renamed on install |
| 08 source appendix | Pointer to the generated index | Regenerated from `build_appendix.py`; header, 14 sections and the per-section totals agree with the CSV |
| 09 town centre register | 39 vendors, 1 056 rows, 739 ids, 87 blanks | Current. The 99 rows still behind the page cap are named in doc 09 sec.7.4 |
| 10 draft modules | What each draft contains, how to install and switch | Row counts re-read from the generated SQL; the mapping split was wrong before (156/365/29) and now reads 223/189/171/35. Quest rows and vendor bindings added this pass |
| 11 this ledger | Where every number comes from | Written this round |
| 12 Midnight item port | IDs, extraction, model and texture conversion, the five DB2 tables, icons, packaging, verification order, and how the housing menus are rebuilt | Written this round. The procedure is checked against this folder's own data; the client file formats could not be checked here, and the document says which parts those are. Every path it names was checked to exist this round |
| 13 mod-psychohouse design | The per-character draft: phase maths, transient spawns, the 13 row catalogue, the two NPCs, and the table of what build 35662 provides and what it dropped | Written with the draft itself. Every capability row in sec.8 was grepped in this tree, not remembered, and the three that are absent are named as absent. Sec.10 separates the five open items from the twenty-odd things the two checkers do settle. |

## 5. Open items, with exact counts

1. **99 catalogue items are not captured yet**, every one behind the 50 row window and
   named in doc 09 sec.7.4: Ren 34 + 16, Len 19, Gronthul 26, Jehzar 3. The second window trick
   (`items?filter=129;0;<npc>` orders the same set differently) bought seven of Gronthul's.
2. **317 of the 1 056 transcribed rows have no item id** - the same 317 as before, all on
   the vendors captured in earlier rounds. The wall is documented in doc 09 sec.7.2: 50
   rows per Wowhead listing with a JavaScript pager that ignores the query string, and two
   12.1.7 NPCs that the "Sold by NPC" filter does not index at all.
3. **87 stock rows have no published price.** Not free, just unpublished.
4. **One npc id unconfirmed**: Throska, a service NPC outside the vendor list, so the
   register still prints no id for it. Every one of the 39 vendors is confirmed.
5. **Vendor models are not bound in the SQL.** `creature_entry` is 0 in all 39 rows and
   in the 8 native stalls, because no 8.3.7 creature is called "Faarden the Builder". Both
   drafts ship a bind script that fills the column from `creature_template` by subname and
   prints whatever it could not fill; running it needs the world database, which is not in
   this workspace.
6. **223 of the 618 decor mappings are category fallbacks.** They are valid game objects and
   they place correctly, but the piece will not look like its retail original. The map is a
   CSV for exactly this reason: replacing one is a one-line edit and a rebuild. When a real
   `gameobject_template` export exists, `map_decor_go.py --go-names` takes it and raises the
   family match count.
7. **Compile, SQL parse and client test** are listed in section 3 and are the reason the
   drafts are called drafts. What that check *can* do is done: every column the generated
   SQL writes is a column this build's own update files write to, which is what
   `check_sql_columns.py` harvests and checks.

## 6. The two drafts, checked as SQL

`python3 tools/housing/verify_drafts.py` reads both `drafts/` trees the way the server
would: it parses every `INSERT`, matches each column list against `01_schema.sql`, counts
the values in every row, and checks the foreign keys inside each file set. It needs no
MySQL, so it catches drift and typos, not whether the server would accept a statement.

```
drafts/midnight-port/sql/world/03_vendors_retail.sql     39 rows
drafts/midnight-port/sql/world/04_decor_retail.sql      571 rows
drafts/midnight-port/sql/world/05_stock_retail.sql      969 rows
drafts/bfa-native/sql/world/03_decor_native.sql         350 rows
drafts/bfa-native/sql/world/04_vendors_native.sql       358 rows
0 finding(s)
```

What that proves, in the order the script checks it:

- every `INSERT` names a table and columns that exist in the schema for that draft;
- no row has a value count that disagrees with its column list;
- every `housing_decor` row has a positive `gameobject_entry`, a name, and an entry that
  appears in `generated/source-asset-index.csv` - so no row points at a game object the
  source tree does not declare;
- every `housing_vendor_stock` row has a price above zero and points at a vendor and a
  decor row that the same draft inserts;
- all 439 distinct item ids in `data/town-center-items.csv` reach `housing_decor` in draft
  A, so nothing captured in the register is missing from the module;
- both drafts carry an identical `src/game/Housing` tree, which is what makes the side by
  side in doc 10 fair.

Two real defects came out of this pass, and both were in the drafts rather than the docs:

1. **Every native decor row was named `GO_WALL_OF_ICE_2`.** The generator reused a loop
   variable from the filtering pass, so all 350 rows took the last constant it had seen,
   and the category, price band and surface of that one piece. The rows had 350 different
   game objects and 350 identical names. Fixed in `build_drafts.py`; the names are now the
   humanised constant (`Atalai Statue 3`) and the categories are spread across structural,
   storage, lighting, wall art and the rest.
2. **The draft A readme still said `gameobject_entry` was 0.** It described the state
   before the mapping pass; all 571 rows have carried a real entry since that pass, on 187
   distinct game objects. The section now describes the mapping, its four match reasons and
   the counts, instead of describing a file that no longer looks like that.

### 6.1 The columns are the build's own

`python3 tools/housing/check_sql_columns.py --repo <source>` reads every `sql/` file in the
build, harvests the columns it writes - `INSERT` lists, `UPDATE ... SET` assignments,
`WHERE` predicates and `ALTER TABLE` - and then checks the module's SQL against that
vocabulary. The point is that nothing here talks to a MySQL server, so the next best claim
is: every column I write is a column this build's own update files already write to.

```
harvested 45 tables
  quest_template            123 columns
  quest_template_addon        3 columns
  quest_objectives           12 columns
  creature_template          12 columns
  item_template               0 columns
4 statement(s) checked against the build vocabulary
0 finding(s)
```

`item_template` reading zero is the useful line: no update file in this tree inserts into
it, so there is no column list to check a guess against. That is exactly why doc 12 sec.3.1
copies a working row instead of typing an item INSERT.

The 123 columns above are why the quest rows can be written at all: `quest_template` is a
wide table, and the list came out of the build's own Duskwood POI update rather than out of
my memory. The vocabulary is saved to `generated/sql-columns.json`, so the check still
works on a machine without a source checkout.

### 6.2 What is not in the quest rows, and why

The four quests are rows in `06_quests.sql` with title, text, chain link and money. They
have no `quest_objectives` rows because none of their objectives is an objective type the
world database can express - walk a plot, claim a Cornerstone, place three pieces, reach
level 2 are all module events. A scripted completion with no objective row is honest; an
invented kill-ten-wolves objective would make the quest completable the wrong way.

The decor and shell rewards are module grants for the same reason: the pieces are
`housing_decor` rows, so the reward is a collection grant, not a piece of loot that has to
exist as an item.

### 6.3 Vendor models

Neither draft can bind a creature model in SQL, because 8.3.7 has no NPC by the retail
names. Both ship a bind script instead - `tools/sql/bind_town_center_vendors.sql` for the
39 and `tools/sql/bind_native_vendors.sql` for the 8 - that fills `creature_entry` from
`creature_template` by subname, one model per stall, and then prints every stall it could
not fill. Running it needs the world database. Until it runs, `creature_entry` is 0, the
module still lists and sells the stock, and nothing stands in the town centre.

### 6.4 The code, checked without a compiler

`python3 tools/housing/check_cpp.py` reads all 5 631 lines of the module the way a compiler
would start to: blocks have to balance, every `Class::Method` definition has to match a
declaration in a module header, every table named in a query has to be one the module
creates, every column named in a query has to exist in that table, every `fields[N]` has to
be inside the column count of the SELECT that produced it, and every include has to
resolve.

```
14 files, 8 classes with methods, 7 tables created by the module
  HousingAddon             7 methods
  HousingHouse            21 methods
  HousingMgr              78 methods
  HousingNeighborhood     12 methods
  HousingPlot              8 methods

0 finding(s)
```

A clean run only means something if the check can fail, so it was tested on a copy with
three deliberate bugs: `fields[16]` read from a 16 column SELECT, a method defined but
never declared, and `item_entri` in a query string. All three were reported, and the real
tree stayed at zero.

`python3 tools/housing/check_lua.py` does the same job for the addon: block balance with
the `do` in `for ... do` counted once, unterminated strings, globals assigned without a
`local` (SavedVariables named in the `.toc` excepted), and a `.toc` whose listed files exist.
`Core.lua` and `Menu.lua` come to 1 324 lines, 0 findings, and a missing `end` plus a stray
global in a copy were both caught.

The blueprint checksum is the one piece of addon code with a number to check, so it was
checked outside the client: the same 0xEDB88320 table and loop, run in Python, gives
`CBF43926` for `123456789`, which is the published CRC32 test value. A wrong table constant
would still produce a stable checksum - and would still reject every code written by a
correct implementation, which is exactly the failure this check rules out.

`verify_drafts.py` also had a bug worth writing down, because it is the kind that makes a
clean report worthless: its INSERT pattern required a space before `VALUES`, and it ended
the row list at the first semicolon even when that semicolon was inside a quoted comment.
The plot inserts are written `INSERT INTO` on one line, the column list on the next, and one
plot comment contains a semicolon - so 55 plot rows were being read as 13 and never
checked. Both are fixed, the parser is quote aware, and the row counts it now reports
(61 in a draft's schema file, 107 across `sql/`) match the files.

### 6.5 The addon and the server, against each other

`python3 tools/housing/check_protocol.py` is the only check that reads both halves at once.
Three questions, all answered from source:

- does every `C:` opcode the addon sends reach a branch in `HandleAddonMessage`?
- does every `S:` line the server sends have a handler in the addon, with `S:CHUNK`
  reassembled inline rather than by handler?
- does any handler read `fields[N]` beyond the last field of the line it is parsing?

```
24 C: opcodes sent by the addon, 25 dispatched by the server
19 S: opcodes sent by the server, 18 handled in the addon
0 finding(s)
```

Counting fields is the fiddly part and the check does it properly: a line is often built in
pieces, because `SendResult` appends its optional `extra` field only when it is non-zero, so
the count follows the payload variable until the chunk is sent. Written carelessly the check
reports every line as one field short, which is exactly what happened first.

It caught one real gap: `VAREA` was dispatched by the server and never sent by the addon, so
the hub amenity list had no way to reach the menu. The House tab now asks for it and lists
what comes back. An injected off-by-one - reading `fields[9]` from the six-field `S:PLOT`
line - was reported, so the check fails when it should.

What the scripts cannot settle: whether MySQL accepts the statements (no server here),
whether the C++ compiles (no boost or MySQL headers here), and whether a mapped game
object looks right in the game (needs the client). Those three stay open, and section 3
says why rather than hiding them.

### 6.6 A draft that is not one of the two SQL drafts

`drafts/mod-psychohouse/` is a whole module - SQL, C++, a config, a README - and none of the
existing tools could see it. `verify_drafts.py` reads the two SQL drafts by name;
`check_cpp.py` walked this module's `src/`; `check_sql_columns.py` reads this module's `sql/`.
A third draft would have shipped with a README and nothing else looking at it.

Two changes, both in this round:

- `check_cpp.py` gained `--src` and `--sql`, so the same structural checks run on any tree:
  braces, declarations against definitions, `fields[N]` against the SELECT that produced the
  row, includes, and every table named in a query against the CREATE TABLEs of the SQL passed
  in;
- `check_psychohouse.py` was written for the draft, and it is the first tool here that reads
  **the build's own loader queries** as the vocabulary. The column list for
  `creature_template` comes out of this tree's `WORLD_SEL_CREATURE_TEMPLATE` prepared
  statement; the list for `gameobject_template` out of `ObjectMgr::LoadGameObjectTemplate`;
  the list for `creature_template_model` out of its loader. A column the build does not read
  is a finding, and so is a column it reads that the draft forgot.

What it also checks, because these are the mistakes that would actually reach a realm: every
facility row's kind against the enum, `is_object` against the kind, a resolver for every
facility left unbound, a resolver for a facility that does not exist, the phase range against
the uint16 space and against the neighbourhood module's range, config keys in both directions,
and the draft README's two arithmetic claims against the table they describe.

Both were mutation-tested on copies. Five faults in `check_psychohouse.py`'s subject and two
more added later - seven in total - were each reported, and an unbalanced block plus a bogus
`fields` index were reported by `check_cpp.py`.

What neither can settle, and the draft's README says so first: MySQL parsing the SQL, and a
compiler accepting the C++. Neither exists here.


### 6.7 The draft, read against MariaDB 11.8.6

The third draft was written and mechanically checked, and then read line by line against the
server it will actually run on. Five defects came out of that read, four of them fatal and one
silent, and none of them would have been found by a column-and-width checker:

| Defect | Why it mattered | Now |
| --- | --- | --- |
| `Query("... {}", x)` in three places, and `{}` in every parameterised statement | this build has `Query(char const*)` and the variadic `PQuery`, and `Trinity::StringFormat` is `fmt::sprintf`. Half of them would not compile; the other half would have sent the braces to the server as SQL | `PQuery`/`PExecute` with `%u`/`%s`/`UI64FMTD`, counted by a checker |
| the house key was `int unsigned` and `uint32` | `characters.guid` is `bigint(20) unsigned` and `ObjectGuid::LowType` is `uint64`, so the key would truncate on an old realm | `bigint unsigned` / `uint64`, cross-checked against the build's own base schema |
| the `.conf.dist` carried a `[psychohouse]` header and said to put the file next to the binary | `ConfigMgr::LoadInitial` reads one file and keeps `fullTree.begin()->second`; the keys would never have been read and every value would have fallen back to its default without a log line | keys pasted into `worldserver.conf`, no header, and a checker that refuses a header |
| resolvers assigned a bare scalar subquery | on MariaDB with the default `sql_mode`, a subquery that matches nothing returns NULL and the `NOT NULL` insert aborts with error 1048 on the first missing service | every subquery wrapped in `COALESCE(..., 0)` |
| `INSERT IGNORE` on the phase allocation, and `InHouse` left set at logout | the first could hand two characters the same phase; the second meant a character that logged out inside the house never had it furnished again | plain `INSERT` with the read-back retry, and a logout hook that clears the flag |

Two smaller ones came out of the same read: `WorldSession.h` was missing in the two files that
call `GetSession()->GetAccountId()` and `GetSession()->GetPlayer()`, neither of which compiles
without it; and the four player commands were registered against a GM permission, so only a GM
could use a command meant for the house owner.

All of it is enforced now, not remembered. `check_psychohouse.py` gained five checks - printf
dialect and argument counts, the 64-bit key on both sides, `COLLATE` on every table, no
`INSERT IGNORE`, no bare scalar subquery - plus the rule that the config carries no section
header. Each was mutation-tested with a deliberate fault and each fault was reported.

Execution is still absent: there is no MariaDB server here and no compiler. As a syntax check,
all four files were parsed with a MySQL-dialect parser in one sitting; that proves the
statements parse, not that they run.


### 6.8 The draft, read against the core rather than against the headers

The MariaDB pass matched every call the draft makes against the header that declares it, which
is enough to know a signature exists and not enough to know what it does. The second read
opened the implementation of the four calls the draft leans on hardest, and found three
defects that no column, width or dialect check could see:

| Call | What the implementation says | What the draft now does |
| --- | --- | --- |
| `DatabaseWorkerPool::PExecute` | enqueues the statement on a worker thread and returns; `PQuery` runs on the calling thread (`DatabaseWorkerPool.h`) | every write to a table this module owns is `DirectPExecute`, because the insert into `psychohouse_owner` is followed by a read of that row on the same tick |
| `Map::SummonCreature` | creates the `TempSummon`, copies the summoner's phase shift, calls `InitStats`, and **adds it to the map** before returning (`Object.cpp`) | `PhasingHandler::AddPhase(creature, phase, true)`: `false` suppresses `UpdateVisibilityIfNeeded`, and the NPC would never reach the client |
| `TempSummon::InitStats` | `TEMPSUMMON_MANUAL_DESPAWN` with a duration of 0 becomes `TEMPSUMMON_DEAD_DESPAWN` (`TemporarySummon.cpp`) | NPCs are flagged `UNIT_FLAG_IMMUNE_TO_PC`, and a five-second world tick re-spawns anything the map no longer has |
| `PhasingHandler::UpdateVisibilityIfNeeded` | acts only when `changed && object->IsInWorld()` (`PhasingHandler.cpp`) | the same distinction, stated in doc 13 sec.4 rather than implied |

The arithmetic was read too: the refund multiplied a `uint32` price by a `uint8` percentage
before dividing, which is safe at the 1 000 gold default and wraps past 8 589 gold at a 50
percent rate. It is `uint64` now.

Two rules joined `check_psychohouse.py` and were mutation-tested: `PExecute` on a table the
module owns, and `AddPhase(x, ..., false)` without a following `AddToMap(x)`. Five injected
`PExecute` calls and one injected `false` were all reported; the clean draft reports nothing.

What this section still cannot say: that the module builds or runs. There is no compiler and
no server here. The repair pass in particular has never executed, and its worst case - a
creature that dies between two ticks - is a five-second gap, not a guarantee.
