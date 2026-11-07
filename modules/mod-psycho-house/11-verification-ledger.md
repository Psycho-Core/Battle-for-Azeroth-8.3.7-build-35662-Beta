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
| 06 blueprint, dye, pet bed | `PHP1` payload format, checksum, dye channels | Complete. The checksum is no longer only specified: `Crc32`, `Base62`, `Checksum` and `ValidateBlueprint` are in `Core.lua`, called by the Blueprints tab and by `/housing bp`, and the CRC matches the standard check value |
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
3. **87 captured lines have no published price on their vendor page.** Three of them carry
   a price on their item page and ship at it; the other 84 are listed as comments and cannot
   be bought. Not free, just unpublished.
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
drafts/midnight-port/sql/world/05_stock_retail.sql      972 rows
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

### 6.9 All three drafts, checked the way MariaDB takes them

The MariaDB pass in sec.6.7 was run against the third draft, because that was the newest file
in the folder. Running the same questions against the other two found that they had never been
asked: every one of the 39 tables outside draft C carried no collation at all, and both older
drafts carried catalogue bugs the column checkers could not see.

`tools/housing/check_mariadb.py` now reads the module's own SQL and every draft, and it asks
only the questions a server asks on import:

| Rule | The error it prevents |
| --- | --- |
| InnoDB, `utf8mb4`, and an explicit `COLLATE` on every table | a table whose collation depends on which server created it, and a join between two tables that disagree |
| every `NOT NULL` column with no default is supplied | 1364, halfway through an import |
| integer and string literals inside the column's range | 1264 and 1406, in strict mode |
| no duplicate primary or unique key inside one file | 1062, which is how a phase can be handed out twice |
| a decor category belongs to one surface band, and the band is 0-3 | a catalogue that tells the module a wall painting stands on the floor |
| no `INSERT IGNORE`, no `REPLACE INTO`, no `ADD COLUMN IF NOT EXISTS`, no `DELIMITER`, no stored routines or events | a duplicate demoted to a warning, a column silently dropped, a MariaDB-only statement, an install that cannot be re-run |

What came out of it in the data rather than the dialect: draft A's 571 decor rows all said
`surface = 0`, which is floor, including 23 wall pieces and 50 lights; draft B's band table
had tables, seating and beds on walls and accents on ceilings. One band table in
`build_drafts.py` now serves both drafts, and the regenerated files differ from the old ones
in the `surface` column only - 352 of draft A's 571 rows and 176 of draft B's 350.

Seven faults were injected into copies to prove the checker reads what it claims to read: a
dropped `COLLATE`, a duplicate facility id, an `INSERT IGNORE`, an out-of-range price, a value
count off by one, a surface of 7, and a VALUES list with one comma removed. All seven were
reported, and the clean tree reports none.

That last one is not a hypothetical. The first run of the parse rule failed on
`06_quests.sql` in both drafts, and the reason was in `build_quests.py`: it joined the four
`quest_template` tuples with a newline instead of a comma, so the statement read
`VALUES (...) (...) (...) (...)` and MariaDB would have answered error 1064. The file had been
shipped in both drafts since it was written, and every checker in this folder splits rows on
parentheses, so not one of them could see it. It is fixed in the generator, regenerated in
both drafts, and now caught twice: by the separator rule and by sqlglot.

Still absent, and still said out loud: there is no MariaDB server in this workspace. Nothing
in this section was executed. What is claimed is that every statement parses (sqlglot, once),
that every column exists in the build's own loader or schema, and that none of the six error
classes above can fire on the data as written.

### 6.10 The core calls, re-checked by a script instead of by eye

Round 17 matched the draft's core calls to their declarations by reading headers one at a
time, which is how the missing `WorldSession.h` was found. That was two days of reading that
would have to happen again after any source update, so it is a table now: 34 tuples of call,
header and declaration fragment, each searched for in this tree, plus the four calls the
build does not have (`SetPhaseMask`, `GetPhaseByAuras`, `CustomData`, `SetRestState`) which
must not appear in the draft at all.

A second rule reads the same list backwards: a file that calls a symbol must include the
header that declares it. That is the `WorldSession.h` defect as a check rather than a story.

Both were mutation-tested: a `SetPhaseMask(...)` call pasted into the manager was reported,
and deleting `#include "WorldSession.h"` was reported. Neither can be satisfied by the module
header pulling in the world, which is what makes the include rule worth having.

What is still true and still written down: no compiler has seen this code. The table proves
that the functions the draft calls exist with the arguments it passes, not that the file
compiles.

### 6.11 The vendor comments that described the vendor page, not the file

Every `housing_vendor` row carries a comment with its npc id and how many stock lines it
sells. Twelve of them were wrong in draft A, by between one and sixteen lines, and the two
that were wrong by sixteen were Winter Veil stalls whose whole catalogue is unpriced, so they
shipped with no stock at all. The count had been taken from the captured rows, before the
unpriced ones were dropped for being free.

The comment is now the number of rows the file actually inserts, and when the two numbers
differ it prints both - `8 of 9 stock lines priced` - so a stall that lost lines cannot look
complete. That is one comment string per vendor row, regenerated, with the stock file unchanged:
972 lines over 37 vendors, and 84 unpriced lines named at the end of it.

The gap that remains is content, not code: Burlee Jinglepocket and Roz Copperpinch need a
price before their sixteen lines can be sold, and the other nine stalls are short by between
one and seventeen.

### 6.12 The catalogue said the hubs were separate and the data did not

Draft A's 571 decor rows all had `faction_mask = 3`. The vendor rows beside them name their
side in the comment - Razorwind Shores is Horde, Founder's Point is Alliance - and the retail
prices for the same piece differ sharply between the two: Autumn Leaf Pile is 50 gold on the
Horde side and 150 on the Alliance side, and eight other pieces follow the same pattern.

With a mask of 3 on both copies, an Alliance player's shop offered the 50 gold copy, so the
Alliance vendor's row was never the cheapest thing on the page and the separation the module
claims existed only in prose. `HousingMgr::BuyDecor` checks `FactionMask & factionBit` before
it takes money, and the shop listing filters on the same field, so the fix is in the data and
it takes effect on the buy path and in the window.

The mask now comes from the vendor that sells the piece: 397 Horde only, 174 Alliance only.
The ten pieces both hubs stock are unchanged as two rows; what changed is that each side sees
one of them. Two pieces, Granite Cobblestone Long Path and Granite Cobblestone Path, are sold
by two Horde vendors at 50 and 75 gold and remain a genuine double listing, which draft A's
README says out loud rather than hides.

What this costs: an Alliance player now has 174 pieces to furnish a house with instead of 571.
That is the number the retail hub actually sells, and it is written down in the draft A README
so nobody reads the catalogue total as per-player stock.

### 6.13 The three round-17 fixes, re-read rather than re-argued

Round 17 changed the draft in three places: `DirectPExecute` on the write path, `AddPhase(..., true)`
on creatures that were already on the map, and the immune-plus-repair treatment of temporary
summons. This round re-read all three against the core and none of them moved. Recorded here
because a fix that was never re-read is a fix that is assumed.

- `DatabaseWorkerPool::DirectPExecute` is the call that returns after the statement is handled;
  `PExecute` queues it. The read-back that follows the ownership insert is only meaningful
  with the former, and the file uses the former at all five write sites.
- `PhasingHandler::AddPhase(WorldObject*, uint32 phaseId, bool update = true)`. `Map::SummonCreature`
  has already put the creature on the map, so the phase change has to send the visibility
  update or the NPC stands there invisible. The two summon helpers and the facility path pass
  `true`; the game object path passes `false` and calls `AddToMap` itself, which is correct
  because that object is not on a map yet.
- `TempSummon` on this core lives with `DEAD_DESPAWN`, so a facility that dies is gone for the
  session. `RepairSpawns` refills from the owner's `Bought` set, `WorldScript::OnUpdate` calls
  it on a timer, and the three summon paths set `UNIT_FLAG_IMMUNE_TO_PC`.

The one thing this cannot prove: that `DirectPExecute` behaves as its declaration reads on a live
server. No server in this workspace has executed a line of the module.

### 6.14 The eighteenth revision: what was added, and what was checked about it

The seventeenth revision ended with a list of systems that were "designed but absent". This
one closes eleven of them in the shared core, so both drafts get them at once, and records
where each one was checked.

| System | Where it lives | What was checked |
| --- | --- | --- |
| 12-level house ladder and XP | `housing_house_level`, `HousingMgr::GrantHouseXp`, `LevelForXp` | The ladder is a table, not a constant in a header; XP is written before the level is announced; the level raises all three budgets |
| Decor storage | `character_housing_storage`, `LoadStorage` / `ConsumeStorage` | A placement consumes a copy and a removal returns one - `PlaceDecor` writes the placement before it consumes, so a failed placement cannot eat a copy |
| Endeavors | `housing_endeavor` / `_task` / `_reward`, `HousingEndeavor.cpp` | Progress is per neighbourhood, not per player; the milestone is the fraction of tasks completed; a claim is keyed on neighbourhood, milestone and account so two players cannot claim the same step twice |
| Milestone gating | `housing_vendor_stock.milestone`, shop listing | A row above the neighbourhood's milestone is not listed and not buyable |
| Blueprints | `HousingBlueprint.cpp` | The checksum is verified before the payload is read; the faction byte is checked; a truncated payload is rejected rather than half-applied |
| Dyes | `housing_decor_variant`, `DyeDecor`, `SpawnDecorObject` | The variant entry is what actually spawns - this round found that the row was written and never read |
| Pet beds | `housing_decor_creature`, `SpawnDecorCreature` | The creature is a temp summon in the plot's phase and is removed with the furniture, so a bed cannot leave a cat behind |
| Finder, visit, reservation | `GetPublicNeighborhoods`, `VisitNeighborhood`, `ReservePlot` | A reservation is an account plus an expiry, not a lock on the plot: it only stops the same account reserving twice |
| Regret window | `character_housing_archive`, `RestoreArchivedHouse` | The house is packed before the plot is freed, and the archive expires rather than lingering for ever |
| Inactivity | `RunLifecycle` | Off unless `Housing.Evict.Days` is set; the letter is sent once (`evict_warned`), and the sale goes through the same path a manual sale uses |
| Population cap | `JoinNeighborhood`, `BuyPlot` | Counted from owned plots, checked at both doors, 0 means no cap |

Eleven defects were fixed in the same pass and are recorded as findings 87 to 96 in the module
README. Two of them were not visible to any checker that existed when they were written:

- `character_housing_decor` and `housing_vendor_stock` each had a column after the index
  definition. MariaDB rejects that with error 1064. The column-count check counts columns and
  the tuple check counts commas; neither looks at the order inside the block. `check_mariadb.py`
  now refuses a column after a key line and a trailing comma before the close, and the whole
  folder is parsed with sqlglot as a second opinion when it is installed.
- The dye path read a column that `LoadDecor` never selected. A query that names a column and a
  loader that selects a different set will never disagree loudly; the only reason this was
  found is that `variant_entry` was traced from the `UPDATE` to the spawn call.

What is still open after this pass, with the reason:

- **The custom map.** Plot rows carry an X/Y lay-out and a NULL Z, and `Housing.Map.Mode =
  "anchor"` places the 55 plots on rings around the town centre for a realm that has no map
  900. Native mode still needs a real map in `Map.db2`; no amount of SQL creates terrain.
- **Dye variants.** The palette table ships with six colours and the bind statement finds the
  pairs the realm's `gameobject_template` actually has. A piece with no pair is not dyable.
  Nothing here invents a recolour that the client cannot render.
- **Endeavor targets.** The 24 tasks carry a target name, not a target id, because a creature
  or item id belongs to the realm. `bind_housing_content.sql` resolves them and reports what
  stayed at 0; an unbound task never completes, which is the safe failure.
- **Vendor milestones.** The gated rows are known per vendor, not per item, except Hordranin.
  The stock that is locked is therefore assigned by price order until the per-item list exists,
  and the draft README says so.

### 6.15 Both drafts against their own data, in both directions

`python3 tools/housing/check_coverage.py` asks a question the other checkers do not: is
anything missing. It walks the data each draft is generated from and finds it in the SQL,
then walks the SQL and finds it in the data.

```
midnight-port   39 vendors   37 priced   972 stock   87 unpriced   121 without item id
  empty stall: Burlee Jinglepocket (107)
  empty stall: Roz Copperpinch (131)
bfa-native       8 vendors   350 stock   350 entries from the source tree
0 finding(s)
```

Draft A, checked item by item:

- every one of the 39 vendors has a `housing_vendor` row and carries its retail npc id in
  the comment;
- all 972 priced captured lines land on a catalogue row at the row's own price and currency,
  matched on name, price and currency rather than on a generated id;
- all 439 retail item ids in `data/town-center-items.csv` appear on a decor row, and no decor
  row invents an id the data does not have;
- every catalogue row names a game object that is in `data/town-center-go-map.csv`, and every
  one has a non-zero entry;
- the 84 lines that ship nothing are named in the comment block at the end of the stock file.

Draft B, checked item by item:

- all 350 entries are `kind = gameobject` with `decor = yes` in
  `generated/source-asset-index.csv`, which is built from the constants this tree declares,
  and no entry appears twice;
- every piece is sold by one of the 8 stalls, and every stall a stock row names exists;
- no row carries a currency, so the gold-only claim is a property of the file rather than of
  the readme.

The checker was mutation-tested before it was trusted, three faults, one per direction:
a deleted stock tuple was reported against the vendor whose count no longer matched, a decor
row with its `item_entry` zeroed was reported twice (once as an id in the data with no row,
once as a row with the wrong id), and a native entry changed to 999999 was reported as not
being in the source tree.

What the checker cannot see: whether a game object entry exists in the server's
`gameobject_template`. The source tree declares the constant; only a world database says
whether the row is there. That is the same wall every other check in this folder hits, and
`bind_housing_content.sql` is the tool that reports it on a real realm.

### 6.16 The twentieth revision: a matrix read against the source, row by row

The question was whether everything is there, so the method was to take doc 02's feature
matrix - 62 rows across eight subsystems - and ask each row for a function. A row with a file
and a symbol is implemented; a row with only prose was the finding. Eleven subsystems
answered "no", and `HousingResult` had one defect that no reading of the documents could
have found because it is only wrong to a compiler.

What was checked, and how:

- **Enumerator names.** Read out of `HousingDefines.h` with `grep -o`, not by eye: one
  duplicate (`NeighborhoodFull` at 17 and 33), now a gap at 33 and nothing at 33.
- **Every symbol the new code calls.** 33 names - `CheckAccess`, `ReleaseViewer`,
  `ForceSellPlot`, `CanBuyRoom`, `GetRoomsForPlot`, `AnchorForAmenity`, `AddStorage`,
  `HasUnlocked`, `GenerateNeighborhoodName`, `RunDissolutionSweep`, `GetOrCreateGuildWing`,
  `ScaledRequirement`, `SpawnEndeavorTheming` and the rest - grepped in `src/game/Housing`
  for both a declaration and a `HousingMgr::` definition. Two did not exist when the code was
  written and both were fixed rather than documented: `HousingMgr::FindPlotAt` and the
  `RoomInfo` accessor path.
- **Core API, not memory.** `AddGossipItemFor` in `ScriptedGossip.h`, `CreatureScript::OnGossipHello`
  and `OnGossipSelect` at `ScriptMgr.h:459/462`, `GameObjectScript::OnGossipHello` and `OnGossipSelect`
  at `ScriptMgr.h:498/501`, `Player::HasAchieved` at `Player.h:2516`,
  `WorldSession::SendNotification` at `WorldSession.h:1079`. Four things the matrix wanted are
  not in this core and the documents say so rather than pretending: no achievement-complete
  hook (reconciled at login), no item-gained hook (loot and craft tasks are defined and never
  emitted), no reachable `Weather` setter (the Armillary phases and relights), and no
  `SendListInventory` on a creature that is not a vendor (the vendor menu is gossip).
- **The conf files, against the code.** Nine keys the C++ reads were in neither draft; eight
  keys were listed as "not read yet" and read; four `PsychoHouse.*` keys from a different
  draft were presented as this module's. `verify_drafts.py` compares the two sets in both
  directions now and was mutation-tested by deleting one key, which it named.
- **The tables themselves.** Every pipe table in doc 02 and the readme was parsed for cell
  count against its header after the rewrite - one row was wrong (`/housing snap on|off`
  inside a cell) and was fixed, because a broken table is a document that renders as one long
  line.

The checkers, all of them, on the tree as it stands:

| checker | result |
| --- | --- |
| `check_cpp.py` | 0 findings; 21 files in `src/game/Housing`, every query against a column the schema has |
| `check_lua.py` | 0 findings; the addon parses, two files, nine tabs |
| `check_protocol.py` | 0 findings; every `C:` the addon sends is dispatched and every `S:` it sends is read |
| `check_sql_columns.py` | 0 findings; 10 statements against the build's own loader queries |
| `check_mariadb.py` | 0 findings; every file parsed with sqlglot, read as MariaDB 11.8.6 takes it |
| `verify_drafts.py` | 0 findings; both drafts, both conf files, the shared `src` trees agree |
| `check_coverage.py` | 0 findings; 571 + 350 catalogue rows and their stock, in both directions |
| `check_psychohouse.py` | 0 findings; draft C, its phases, its SQL and its conf shape |
| `audit.py` | 0 findings across 8 checks |

What none of them can say: that this compiles, or that a realm accepts it. Both remain the
limit named in section 3, and nothing in this revision claims otherwise - the new C++ was
read against the headers it calls and the new SQL was parsed as text, and that is all.

### 6.17 The twenty-first revision: from "does it exist" to "can it be reached"

The twentieth revision gave every feature-matrix row a function. That is a weaker claim than
it sounds, and this revision is the proof: nine things were written, correct, and connected to
nothing.

How it was found, because "check again" is not a method:

- **Call-site census.** Every `Class::Method` definition in `src/` was counted against its
  uses across the whole module. Eleven methods had a definition and no caller:
  `GetNeighborhoodBySlot`, `GetOrCreateGuildWing`, `HasVariants`, `InviteToNeighborhood`,
  `IsGuest`, `RemoveFromNeighborhood`, `ScaledRequirement`, `SendLine`, `SetMemberRank`,
  `SpawnEndeavorTheming`, `GetPlotTemplate`. Five were deleted, four were wired, and
  `HandleAddonMessage` turned out to be called from the core's `ChatHandler.cpp` - the one
  place outside the module that knows it exists.
- **Config members.** All eighteen `HousingMgr` config fields were grepped for a second
  reference; every one is read somewhere other than its assignment, so no key is decorative.
- **Tables.** All 29 tables were counted against the C++ that queries them. Every one is
  read; `housing_dye` was written by the progression generator and read by nothing, which is
  how the missing dye station surfaced.
- **Content that cannot start.** The four introduction quests were traced from row to giver:
  none. `quest_template` rows 900100-900103 with no `creature_template` reference, no
  `quest_objectives`, and no code that offers or completes them.
- **Doc claims against code again, in the other direction:** the achievement list in doc 04,
  the scaling formula in doc 02, the dye station in doc 09, the guild wing table in doc 02.
  All four had documentation and no behaviour.

A new checker rule came out of this, and it is the only one that would have caught findings
113, 114 and 115: `check_cpp.py` now fails when a `HousingMgr` method is defined and never
called, reading the core's `src/server/game/Handlers/` as a second call corpus for the entry
point the core itself calls. It was mutation-tested by deleting the single call site of
`InviteToNeighborhood` in a copy of the tree, and it named exactly that method.

| checker | result |
| --- | --- |
| `check_cpp.py` | 0 findings; braces, declarations, the new reachability rule, column counts, includes |
| `check_lua.py` | 0 findings; the addon parses, nine tabs |
| `check_protocol.py` | 0 findings; `C:DYES` and `S:DYE` are paired, every opcode dispatched |
| `check_sql_columns.py` | 0 findings; 17 statements against the build's own loader queries |
| `check_mariadb.py` | 0 findings; every file parsed with sqlglot as MariaDB 11.8.6 reads it |
| `verify_drafts.py` | 0 findings; both drafts, both conf files, the shared trees |
| `check_coverage.py` | 0 findings; 571 + 350 catalogue rows and their stock, both directions |
| `check_psychohouse.py` | 0 findings; draft C |
| `audit.py` | 0 findings across 8 checks |

Still true, and still the limit of every claim in this folder: nothing here was compiled and
no SQL was executed against a server. The new C++ was read against the headers it calls -
`QuestStatus`, `Player::AddQuest`, `AreaExploredOrEventHappens`, `RewardQuest`,
`MotionMaster::MoveRandom`, `UNIT_FLAG_IMMUNE_TO_NPC` - and the new SQL was parsed as text.

### 6.18 The twenty-second revision: settings, and the three places a setting lives

The twenty-first revision asked whether a feature had been written. This one asked the
question behind the request - is the package complete - and the answer was that the feature
layer was in better shape than the settings layer, for a structural reason: a setting lives in
three places at once.

1. a line in a conf file, which an operator edits;
2. a field in `LoadConfig`, which the server reads;
3. a sentence in a document, which explains it.

Nothing here compared the three. Doc 14 is that document, `.housing config` makes the second
one observable, and `check_reference.py` compares all three plus the command and client lists,
which is how the following were found:

- **Fourteen of the 29 settings** appeared in no document at all. The conf file's own comments
  were the only explanation of `Housing.SaveInterval`, `Housing.RateLimit.Messages`,
  `Housing.Sell.RefundPercent`, `Housing.Phasing.PlotScoped`, the four interior coordinates and
  the four Town Center coordinates.
- **No way to see what the server had actually kept.** Six settings are clamped - `SaveInterval`
  to a floor of 5, `RateLimit.Messages` to 1, `Sell.RefundPercent` to 0-100, the three device
  levels to 1 - so the value in the file is not always the value in use, and there was no way
  to tell. `LoadConfig` now records every key with the value it kept and `.housing config`
  prints the record.
- **Help that described six of 44 commands.** The six were typed by hand, so they were the six
  somebody remembered. The help now walks the command table.
- **A client preference that lasted one session.** Grid snapping was a table literal rebuilt at
  every login, so `/housing snap off` survived until the next reload. It is now one of six
  persisted settings, with no mirrored copy for the UI to fall out of step with.
- **A window that could not be configured at all.** The client half had no settings surface;
  the Settings tab is now the tenth, and the window's own position and scale are preferences
  like the rest.

| checker | result |
| --- | --- |
| `check_reference.py` | 0 findings; 29 settings in conf = 29 recorded = 29 documented, 44 commands = 44 documented, 6 client settings = 6 documented, help read from the table. Mutation-tested four ways (dropped setting row, renamed command, renamed client default, deleted `note` call) and each was named |
| `check_cpp.py` | 0 findings; the snapshot, the `note` helpers and the new command included |
| `check_lua.py` | 0 findings; ten tabs, the settings pane and the persisted table parse |
| `check_protocol.py` | 0 findings; no opcode changed this round |
| `check_sql_columns.py` | 0 findings; no new SQL |
| `check_mariadb.py` | 0 findings; no new SQL |
| `verify_drafts.py` | 0 findings; both drafts rebuilt after the addon change |
| `check_coverage.py` | 0 findings; catalogue and stock unchanged |
| `check_psychohouse.py` | 0 findings; draft C untouched by this round |
| `audit.py` | 0 findings across 9 checks, the reference check now among them |

Unchanged and still true: nothing here was compiled and no SQL was executed. The new C++ is
`std::vector` of string pairs, a lambda that formats through `ostringstream`, and a reader of
`ChatHandler::getCommandTable()` - all of it read against the headers in this tree, none of it
built.

### 6.19 The twenty-third revision: back to doc 01, row by row

The last two revisions worked outward from the code: what exists, what can be reached, what
can be configured. This one worked inward from the specification - doc 01 is the retail record,
doc 02 decides what of it is buildable - and compared each row against what the code actually
does. Three things came out of it.

**A row that overstated the code.** Doc 02's advanced-mode row read "free placement, rotate,
scale" and the rotation was yaw only. Retail rotates on three axes. Nothing in the module
contradicted the row, because no checker compares a sentence about rotation with the number of
angles in the struct. Pitch and roll are now carried end to end, with the tilt clamped rather
than refused, and the two blueprint consequences written out in doc 06.

**A system that had been specified and not built.** Doc 06 sec.4 describes the retail pet bed
in detail - assignment, two modes, two caps - and the implementation spawned a fixed creature
per decor row. That is a bed with a cat in it that nobody chose. The assignment path now exists:
the summoned companion, a journal check, a per-bed mode, and the two caps as settings.

**A semantic argument the player was made to supply.** The dye group. Retail's slot decides it,
so the player chooses a colour and the piece contributes the group; the module asked for both
and defaulted the group to 1, which silently failed for any piece whose variants are elsewhere.

| checker | result |
| --- | --- |
| `check_cpp.py` | 0 findings; braces, declarations, queries, column counts, includes, the reachability rule and the **new gossip rule**: every `GOSSIP_ACTION_INFO_DEF + n` reaches a menu call and a `case`. Mutation-tested by pointing one menu row at another action |
| `check_lua.py` | 0 findings; ten tabs, the tilt boxes, the pet buttons |
| `check_protocol.py` | 0 findings; `C:PET` dispatched, `S:PLACEDENTRY` grew to 13 fields and the addon reads all 13 |
| `check_sql_columns.py` | 0 findings; `pitch`, `roll`, `pet_entry`, `pet_roaming` against the module's own schema |
| `check_reference.py` | 0 findings; 31 settings, 46 commands, 6 client settings, all documented |
| `check_mariadb.py` | 0 findings; the four new columns parsed as MariaDB reads them |
| `verify_drafts.py` | 0 findings; both drafts rebuilt from the shared trees |
| `check_coverage.py` | 0 findings; catalogue and stock untouched |
| `check_psychohouse.py` | 0 findings; draft C untouched |
| `audit.py` | 0 findings across 9 checks |

What is still not verified, and has not been at any point in this folder: no compiler ran over
this code and no server executed this SQL. The new core calls were read against the headers -
`Player::GetSummonedBattlePet`, `Player::GetBattlePetCountForSpecies`, `sBattlePetSpeciesStore`,
`BattlePetSpeciesEntry::CreatureID`, `QuaternionData::fromEulerAnglesZYX` - and the column
widths against the schema, but "read against the header" is not "linked and run".

### 6.20 The twenty-fourth revision: two rows that described a data structure

The twenty-third pass compared behaviour against doc 01. This one checked the two rows in doc 02
and doc 06 that describe a *shape* rather than a behaviour - "two spaces, five audiences, one bit
per audience, stored on `character_housing`" - and the shape was not there. Neither document was
lying on purpose; both were written from the retail model, and nothing compared a sentence about
a bitfield with the columns in the schema. That is the same failure mode as the rotation angles
in the previous revision, which suggests the general rule: a document sentence that names a data
structure has to be checked against the schema, not against the prose beside it.

What was actually in the code: `locked`, a per-account guest list, and neighbourhood membership.
So a house could be open to neighbours or closed to everyone, and nothing in between. There was
no guild audience, no party audience and no friends audience, and no way for the yard and the
interior to differ.

What is there now:

| Piece | Where |
| --- | --- |
| `plotMask`, `houseMask` on `character_housing`, `HousingAudience` bits | schema, `HousingDefines.h` |
| `AudienceBits` (six relationships resolved per visitor), `CheckSpace` (one space against its own mask) | `HousingMgr.cpp` |
| `ResolvePlot` asks the yard, `EnterHouse` asks the interior | `HousingMgr.cpp` |
| `SetAccess` persists and re-runs `EnforceAccess` on the same call | `HousingMgr.cpp` |
| `.housing access`, ten Steward checkboxes, six toggles per space on the House tab | `housing_commands.cpp`, `HousingNpc.cpp`, `Menu.lua` |
| Masks on `S:PLOTINFO`, `C:ACCESS` to change them | `HousingMgrProtocol.cpp`, `HousingMgrExtra.cpp` |

Three asymmetries were decided rather than discovered, and all three are in doc 14 sec.5: the
guest bit is always stored, the friends audience falls back to the visitor's list when the owner
is offline, and `locked` is checked before the mask rather than being a seventh audience.

| checker | result |
| --- | --- |
| `check_cpp.py` | 0 findings; the new methods, the ten gossip ids (each with a named `case` arm, which is what the gossip rule demands), the widened `S:PLOTINFO` |
| `check_lua.py` | 0 findings; the permission buttons, the audience table, the `PLOTINFO` handler |
| `check_protocol.py` | 0 findings; `C:ACCESS` dispatched, `S:PLOTINFO` at 16 fields and the addon reads 16 |
| `check_sql_columns.py` | 0 findings; `plotMask` and `houseMask` against the module's own schema |
| `check_reference.py` | 0 findings; 31 settings, 47 commands, all listed |
| `check_mariadb.py` | 0 findings; the two new columns parsed |
| `verify_drafts.py` | 0 findings; both drafts rebuilt |
| `check_coverage.py` | 0 findings |
| `check_psychohouse.py` | 0 findings; draft C untouched |
| `audit.py` | 0 findings across 9 checks |

And the standing caveat, unchanged in every revision: no compiler ran over this code and no
server executed this SQL. `PlayerSocial::HasFriend`, `Player::GetGroup`, `Player::GetGuildId`
and `ObjectAccessor::FindPlayerByLowGUID` were each read against this tree's headers before being
used; that is the strongest claim available here and it is weaker than a build.

### 6.21 The twenty-eighth revision: identifiers, and the data behind the prose

The twenty-seventh pass gave permissions a way in. This one stopped trusting prose and parsed
every backticked identifier in the documents against the files, then asked the same question of
every data-shaped claim: is the column created, is it selected, and is the number in the
document the number in the table.

- **Names that pointed at nothing.** `psycho_house_vendor_stock` (doc 02 sec.4, doc 09 sec.7)
  and `RestorePlot` (doc 02 sec.2) do not exist; the world schema creates
  `housing_vendor_stock` and the restore is `RestoreArchivedHouse`. Three more tables in doc
  02's "new table" list - `housing_blueprint`, `housing_dye_color`, `housing_pet_bed` - are
  not created either: blueprints are `character_housing_blueprint` in the characters
  database, a dyed appearance is `housing_decor_variant` beside `housing_dye`, and a pet bed
  is a row in `housing_decor_creature`.
- **A column that existed and was never read.** `housing_decor.dye_group` was in the schema,
  absent from the `SELECT` and absent from `DecorInfo`, so every piece reported group 0 and
  `GetVariant` could never match a colour. It is selected now, in a column order
  `check_sql_columns.py` compares against the schema. `subcategory` did not exist at all; it
  does now, is filtered by `C:DECOR`, and rides on `S:DECORENTRY` so the window can print a
  heading.
- **A documented impossibility that was not one.** The Armillary row said a script cannot
  reach weather. `Weather::SetWeather` is public and `Weather::SendFineWeatherUpdateToPlayer`
  is static. The row is rewritten as five presets over two axes a server really has - lit
  variants and a clear sky - with the two presets that differ only in the client number
  stated as such (doc 06 sec.6) rather than dressed up.
- **Content documents describing other content.** Doc 04 sec.4.1 sketched a schema with
  `taskType`/`rotationStart`/`rotationEnd`/`itemOrDecorId`; the shipped tables are
  `type` (0-6), `duration_days`, `started`, `ends`, `item_entry`, `item_count`. Sec.4.2
  described six Endeavors where `07_progression.sql` has four, with 22 task rows between them.
  Both sections now describe the data, and the six retail themes are named as the shape to
  grow into rather than as shipped content.
- **A reward path that dropped rows.** `EndeavorMgr::Claim` paid currency and XP and ignored
  `item_entry`/`item_count`. A row that names an item is paid now, with a notification if the
  bag is full; the shipped rows still name none, and the comment in the SQL says why.
- **A pane drawn twice.** The house tab put the amenity list and the permission matrix in the
  same 200 pixels and the second block ran off the bottom. An eleventh tab, Access, carries
  both; the Armillary's five preset buttons took the space they freed.

| Piece | Where |
| --- | --- |
| `housing_decor.subcategory`, `night_entry`; `character_housing.armillary` | both schema files |
| `DecorInfo::Subcategory`, `DecorInfo::DyeGroup`, `DecorInfo::NightEntry`, loader at 19 columns | `HousingDefines.h`, `HousingMgr.cpp` |
| `ArmillaryPreset` 0-4, `IsArmillaryDark`, `IsArmillaryClearSky` | `HousingDefines.h` |
| `SetArmillary`, `ToggleArmillary`, weather for the two clear-sky presets, world state 61202 | `HousingMgrExtra.cpp` |
| The lit entry chosen at spawn, dye winning over lit | `HousingHouse.cpp`, `HousingDecor.cpp` |
| `C:ARMILLARY [0-4]`, `C:DECOR <category> <page> <subcategory>` | `HousingMgrProtocol.cpp`, `Core.lua` |
| `.housing armillary`, Access tab with both matrices and the amenity list | `housing_commands.cpp`, `Menu.lua` |
| `tools/sql/bind_night_variants.sql` | paired lit twins from `gameobject_template`, shipped in both drafts |
| `EndeavorMgr::Claim` paying item rewards | `HousingEndeavor.cpp` |

| checker | result |
| --- | --- |
| `check_cpp.py` | 0 findings; the new preset enum, `SetArmillary`, the spawn path, the reward branch |
| `check_lua.py` | 0 findings; the Access tab, the Armillary buttons, the catalogue heading |
| `check_protocol.py` | 0 findings; `S:PLOTINFO` at 17 fields and the addon reads 17, `S:DECORENTRY` at 10 and the addon reads 10 |
| `check_sql_columns.py` | 0 findings; `subcategory`, `night_entry`, `armillary` against the module's own schema |
| `check_reference.py` | 0 findings; 48 commands, all listed |
| `check_mariadb.py` | 0 findings; the new bind file parsed as statements |
| `verify_drafts.py` | 0 findings; both drafts rebuilt, 31 conf keys |
| `check_coverage.py` | 0 findings |
| `check_psychohouse.py` | 0 findings; draft C untouched |
| `audit.py` | 0 findings across 9 checks |

Unchanged caveat: no compiler ran over this code and no server executed this SQL.

### 6.22 The twenty-ninth revision: the window, opcode by opcode

The twenty-eighth pass made the documents agree with the files. This one took the sentence
"the addon is the interface" literally and asked it of every opcode: the window sent nineteen
of the fifty-seven the server dispatches, so thirty-eight features were reachable only by typing
`.housing <verb>`.

- **The window is now the primary surface.** Rooms (shells and interior rooms), Storage (copies),
  Travel (homestone, gryphon, arcanostorage, portals) are new tabs. Neighbours gained buying,
  reserving, moving, founding, leaving, the favour display and the public-neighbourhood finder
  with a Join button per street. Endeavors, Blueprints and Dyes were rebuilt from static text
  into working panes. House gained sell and level-up; Placed gained build mode, place-in-front
  and exact placement; Access gained the guest list.
- **The rule that keeps it true.** `check_protocol.py`'s `window` rule parses only actual
  `H.Send("C:...")` calls in `Menu.lua` - not the Guide tab's prose, which names every opcode -
  and fails on any dispatched opcode that is not sent from the window. `HELLO` and `PING` are
  the two exemptions, and they are the handshake. Mutation-tested by renaming one send: the
  check named both the unknown opcode and the feature that had lost its button.
- **Two panes were lying about what they could do.** The Dyes tab called a function that did
  not exist, so it threw on open; the Blueprints pane described a `PCH1-kinded` format the
  server has never emitted, so its validator rejected every real code and the pane advertised
  import as future work while `C:BPEXP`/`C:BPIMP` were implemented. Both now do the thing.
- **Layout defects are defects.** The Placed tab drew its buttons over its own list and had one
  button above the top edge of the pane, where a click cannot reach it. Four control rows, list
  below. The Map row's click sent a decor id where the plot index belonged.
- **A result read as a boolean.** `S:ROOM`'s fifth field is a `HousingResult`; 0 means the room
  can be bought. Read as a flag, the pane hid every room a player could actually buy.
- **A class of bug, not an instance.** Adding sixteen buttons meant adding handlers that run
  later but are written earlier, and Lua resolves locals at compile time: a handler defined
  above the label it updates reads a nil global, silently. `check_lua.py` gained a `forward`
  rule for exactly that, plus duplicate file-scope locals. Mutation-tested with a probe
  function, which it reported with both line numbers.

| Piece | Where |
| --- | --- |
| `window` rule | `tools/housing/check_protocol.py` |
| Four new tabs, three rebuilt, guest list, sell, level-up, build mode, exact placement | `addon/HousingUI/Menu.lua` |
| Palette from server data, real `PHP1` envelope check, shell/portal counts, `S:ENDEAVOR` name and end | `addon/HousingUI/Core.lua` |
| `EndeavorMgr::GetEnds` and the two extra `S:ENDEAVOR` fields | `HousingEndeavor.h/.cpp`, `HousingMgrExtra.cpp` |
| Addon version 0.9.0 | `HousingUI.toc`, `Core.lua` |

| checker | result |
| --- | --- |
| `check_protocol.py` | 0 findings; 57 of 57 dispatched opcodes reachable from the window, `S:ENDEAVOR` at 6 fields and the addon reads 6, `S:PLOTINFO` at 17 and reads 17 |
| `check_lua.py` | 0 findings; fourteen tabs, the new panes, the redraw chain |
| `check_cpp.py` | 0 findings; `GetEnds` declared, defined and called |
| `check_sql_columns.py` | 0 findings |
| `check_reference.py` | 0 findings; 48 commands |
| `check_mariadb.py` | 0 findings |
| `verify_drafts.py` | 0 findings; both drafts rebuilt |
| `check_coverage.py` | 0 findings |
| `check_psychohouse.py` | 0 findings; draft C untouched |
| `audit.py` | 0 findings across 9 checks |

Unchanged caveat, and it matters more here than anywhere else: no Lua interpreter and no client
ran over this addon. The checks prove the file parses, that the frames are created in a sane
order, and that every opcode has a sender; they cannot prove a button is where a player expects
it or that the frame is not clipped. That needs the client.
