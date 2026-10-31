# 11. Verification ledger

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
| `counts` | The headline numbers in `README.md` equal the CSVs: 600 item ids, 917 stock rows, 87 blank prices, 610 mapped decor names |
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

That is the first pass with nothing open in the mechanical checks. What is still incomplete
is incomplete in **volume**, not in consistency: the second-faction pages and the rows past
the 50 row cap, both counted exactly in section 5.

## 1. Where each number comes from

| Number in the docs | How it was obtained | Checked against |
| --- | --- | --- |
| 39 vendors, 27 + 12 split by neighborhood | UmbraMarket vendor index, cross read against the Wowhead zone pages for Founder's Point (16105) and Razorwind Shores (15524) | `data/vendor_npc_ids.csv` row count |
| 1 155 catalogue items | Sum of the per-vendor item counts on the same index | The per-vendor table in doc 09 |
| 917 stock rows | Transcribed from live vendor pages, then merged through `merge_capture.py`; the pages that render were transcribed whole, not sampled | `wc -l data/town-center-stock.csv` |
| 600 item ids | Wowhead "Sold by NPC" listings and NPC sell tabs, vendor by vendor; names diffed against the stock CSV before a single id was written | `data/town-center-items.csv`, `source` column |
| 432 distinct ids | Duplicates are items several vendors sell (the Orgrimmar furniture runs, Wooden Chamberstick, the Hallow's End set) | `data/town-center-items.csv`, sorted by id |
| 87 blank prices | Stock rows whose vendor page shows no cost. Blank means unpublished, never free | doc 09 sec.7.5 lists them by vendor |
| 610 mapped decor names | Draft A's decor count, one mapping row per name | `data/town-center-go-map.csv` |
| 906 candidate game objects | Rows of `generated/source-asset-index.csv` where `kind = gameobject`, enum constants rejected | `tools/housing/map_decor_go.py --stats` |
| 555 decor rows with an entry | Generated SQL, one tuple per mapped name | `tools/housing/audit.py` mapping check |
| 350 native decor rows | Index rows whose constant and category look like housing decor | `drafts/bfa-native/sql/world/03_decor_native.sql` |

## 2. What was checked by reading, not by script

The mechanical checks catch format and arithmetic. These were checked by re-reading the
sources themselves:

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
| The C++ compiles | The sandbox has no boost, MySQL or OpenSSL headers, so not even the core headers parse | Every file is written against the signatures in this tree, and the two headers that do parse here (`Define.h`, `Position.h`) were used to check enum and type names. Treat phase 0 as a shakedown build |
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
| 04 content spec | Quest ids 900100-900103, Endeavor tables, reward bands | Complete; ids are inside the module's reserved range |
| 05 build plan | Phase 0 to phase 6, file lists per phase | Complete; the file lists match the shipped `src/` tree |
| 06 blueprint, dye, pet bed | `PCH1` payload format, checksum, dye channels | Complete; the checksum is defined and a Lua implementation is in the addon |
| 07 client patching | `.toc`, `## Interface: 80300`, message prefix, DB2 notes | Complete; the addon ships as `addon/HousingUI/` and is renamed on install |
| 08 source appendix | Pointer to the generated index | Regenerated this round |
| 09 town centre register | 39 vendors, 675 rows, 358 ids, 87 blanks | Updated this round. Six vendors still have no stock capture - section 5 below |
| 10 draft modules | What each draft contains, how to install and switch | Updated this round with the CMake patch step and the real row counts |
| 11 this ledger | Where every number comes from | Written this round |

## 5. Open items, with exact counts

1. **238 catalogue items are not captured yet**, and every one has a named owner:
   Ren 62 rows behind npc 255325, Len 69 behind 255326, Yen 36 behind 255319, and 34 + 33
   rows past the 50 row page cap on Ren 255222 and Gronthul 255278. The routine is in
   `tools/housing/merge_capture.py`; the pages are the only work.
2. **317 of the 917 transcribed rows have no item id**. The wall is documented in doc 09
   sec.7.5: 50 rows per Wowhead listing with a JavaScript pager that ignores the query
   string, and two 12.1.7 NPCs that the "Sold by NPC" filter does not index at all.
3. **87 stock rows have no published price.** Not free, just unpublished.
4. **One npc id unconfirmed**: Throska, a service NPC outside the vendor list, so the
   register still prints no id for it. Every one of the 39 vendors is confirmed.
5. **218 of the 610 decor mappings are category fallbacks.** They are valid game objects and
   they place correctly, but the piece will not look like its retail original. The map is a
   CSV for exactly this reason: replacing one is a one-line edit and a rebuild. When a real
   `gameobject_template` export exists, `map_decor_go.py --go-names` takes it and raises the
   family match count.
6. **Compile, SQL parse and client test** are listed in section 3 and are the reason the
   drafts are called drafts.
