# Getting the Midnight data into this draft

This is the practical page: where each number in `drafts/midnight-port` comes from, how to
capture it again, and where the actual Midnight files come from when the point is the art
rather than the numbers. It is deliberately short and in order. Doc 12 is the long version
of the second half; this page is the one to read first.

Two kinds of data are involved, and they are separate jobs:

| Kind | What it is | Needed for | Where it lands |
| --- | --- | --- | --- |
| The numbers | vendor, item name, price, currency | a catalogue that charges retail's prices with 8.3.7 furniture in the slots - what this draft already has | `data/*.csv`, then `sql/world/*.sql` |
| The files | DB2 rows, M2 meshes, BLP textures | the pieces drawing as Midnight's own models instead of 8.3.7 stand-ins | a client patch; nothing in this folder |

Nothing in this folder is a game asset. The draft ships because of the first row, and it
works without a single file from the second.

## 1. The numbers

### 1.1 The five files they live in

| File | Columns | Rows now |
| --- | --- | --- |
| `data/vendor_npc_ids.csv` | `vendor,npc_id,items,slug` | 39 - one per stall, with the retail npc id |
| `data/town-center-stock.csv` | `vendor,item,price,currency,note` | 1 056 captured lines, the raw capture |
| `data/town-center-items.csv` | `vendor,item_id,item_name,price,currency,source` | 739 - item name to retail item id, with how it was resolved |
| `data/town-center-go-map.csv` | `item,category,gameobject_entry,go_constant,go_subcategory,match,confidence` | 618 - the 8.3.7 stand-in for every decor name |
| `data/captures/*.tsv` | item id, name, price, currency, note | 11 hand captures, one vendor each |

Two rules the whole pipeline depends on:

- an empty price means **unpublished**, never free. A line with no price is never inserted;
  a row with price 0 would sell for nothing;
- an item id is looked up by **vendor and item name together**, never by name alone. Two
  stalls sell the same piece at different prices, and a name-only lookup hands one stall the
  other's id.

### 1.2 Capturing one vendor

1. Open the vendor's page. The npc id is in `data/vendor_npc_ids.csv`, so the address is
   `https://www.wowhead.com/npc=<npc_id>`, and the stock is on the page's *Sells* tab.
2. Read the table: item name, price, and what the price is in - gold, Community Coupons,
   Tricky Treats. Write one line per item.
3. Watch two things that cost me the most time:
   - the page prints **50 rows and stops**. A vendor with more than 50 items needs the paging
     controls; a plain fetch silently returns the first page only. That cap is the reason 234
     of the 1 056 lines have no item id;
   - where a vendor gates part of its stock behind an Endeavor, the page often prints **one
     count for the whole set** ("11 of 17 gated") and not which eleven. Record that count in
     the `note` column and leave the lines ungated. A guess here sells a gated piece early.
4. Save the lines as a new file in `data/captures/`, named after the vendor slug
   (`den-nightshade.tsv`), tab separated, five columns:

```
280625	Framed Moonbrook Quilt	10	Community Coupons	10 of 27 gated by Endeavor milestone
```

5. Merge it into the two CSVs. This replaces that vendor's lines, never duplicates them, so
   re-running a corrected capture is safe:

```
python3 tools/housing/merge_capture.py --vendor "\"Den\" Nightshade" \
    --file data/captures/den-nightshade.tsv --source "wowhead npc 257303"
```

A vendor that stands as two npcs takes `--append` on the second capture.

### 1.3 Resolving the retail item ids

```
python3 tools/housing/fetch_item_ids.py --vendors data/vendor_npc_ids.csv \
    --stock data/town-center-stock.csv --out data/town-center-items.csv
```

This is the only step that needs a network connection. It tries the npc page first, then the
page with `?sells=N` paging, then the item filter endpoint with `&page=N`, then a per-name
search, and records which strategy answered in the `source` column so a wrong id can be
traced. Add `--search-fallback` for the slow complete pass.

An id that will not resolve stays empty. That is not a failure: 121 of the 571 catalogue
pieces ship with no retail id, place fine, and cost what the capture says.

### 1.4 Rebuilding the draft

Order matters - the mapping reads the stock capture, the draft reads the mapping:

```
python3 tools/housing/index_source_assets.py --root . \
    --out-md generated/source-asset-index.md --out-csv generated/source-asset-index.csv
python3 tools/housing/map_decor_go.py
python3 tools/housing/build_drafts.py
python3 tools/housing/verify_drafts.py
```

Expected output, in order:

```
candidates: 256 game objects
mapped 618 decor names
midnight-port: 39 vendors, 571 decor, 972 stock
0 finding(s)
```

The first step re-reads the source tree, so it only needs re-running after the tree changes.
`map_decor_go.py` picks a stand-in for every decor name; `build_drafts.py` writes both drafts
from the CSVs. Running the last one is not optional: `verify_drafts.py` fails if a catalogue
row stands on a game object the index calls non-decor, or if a number in a document has
drifted from the SQL.

### 1.5 Improving a stand-in by hand

Every choice is a line in `data/town-center-go-map.csv`, so the fix is an edit and a rebuild:

```
grep "^Amani Water Well," data/town-center-go-map.csv
Amani Water Well,Nature,203035,GO_BURNING_TREE_2,Large Foliage,category,medium
```

Change `gameobject_entry` and `go_constant` to something better, change `match` to `manual`,
then re-run `build_drafts.py`. The only rule is that the entry must exist in
`gameobject_template` and be an object the source index calls decor; a piece whose entry is
missing fails at placement instead of spawning nothing, which is why a rough stand-in beats
a zero.

If a database export is on hand, the map tool takes it and prefers real names over constants:

```
python3 tools/housing/map_decor_go.py --go-names go-export.csv   # entry,name rows
```

## 2. The files

This is the part that needs a client, and none of it is in this folder. The whole sequence,
with three routes of increasing effort, is doc 12 sec.3 and sec.4; this section is the map of
it so the shape is clear before the detail.

### 2.1 Where the files are

Every file lives in the client's own CASC storage, under `DBFilesClient/` for the data tables
and under `Creature/`, `World/`, `Interface/` for the art. Reading it needs three things:

| What | What it is | Why it is needed |
| --- | --- | --- |
| The client build | the installed game, or its CDN archive | the only lawful source of the files |
| A listfile | community-maintained names for the storage's file ids | without it the storage is an unordered pile of numbered blobs |
| A CASC reader | CASCExplorer, wow.export, or the community archives | turns the storage into files on disk |

The mirrors that carry the listfile and per-build DB2 exports are `wago.tools` (the CDN
archive and the DB2 tables browsable with a CSV export per table) and `wow.tools` (the same
archive, plus the tooling projects it is built from). `casc.wago.tools` is the mirror that is
maintained. For DB2 to CSV in bulk, `DBC2CSV` from the wow.tools project does the conversion
stand-alone, and `WoWDBDefs` is the table-definition set it needs for a build newer than the
one bundled with it.

### 2.2 What to pull, and what each file answers

| File | Answers | Feeds |
| --- | --- | --- |
| `ItemSparse.db2` | the name, the description, the quality, the icon FileDataID | the tooltip and the icon of a ported piece |
| `Item.db2` | class, subclass, stack size, price, flags | the row that makes the id exist |
| `ItemAppearance.db2` | item appearance to display id | the link from item to model |
| `ItemModifiedAppearance.db2` | item id to appearance id | the same link, from the item's side |
| `ItemDisplayInfo.db2` | model name, texture name, icon, attachment | the actual model |
| the M2/SKIN and BLP the display info names | geometry and colour | what the piece looks like |

The housing catalogue itself is not a table to pull: the pieces, their prices and their
gates come from the vendor pages, which is section 1 of this page. A DB2 export does not
replace the capture - it names the things the capture lists.

### 2.3 The three routes, shortest first

1. **Catalogue only, no client files.** Insert an `item_template` row per retail id with a
   borrowed 8.3.7 icon, leave the appearance chain alone, and the piece places as the 8.3.7
   stand-in. This is what the draft assumes today, and it needs no Midnight file at all.
   Doc 12 sec.3 has the SQL.
2. **Model reuse.** Point `ItemDisplayInfo` at an existing 8.3.7 M2 and port only the
   texture. Cheapest real improvement; most furniture is a static prop, so it holds up.
3. **Full port.** Convert each M2 to the BfA generation, re-encode each texture to BLP2
   DXT1/DXT5, write the five DB2 rows, ship one patch. Doc 12 sec.4, file by file.

The trap in route 3 is the file generation, not the packing: a Midnight M2 uses
`.skel`/`.anim` side files and a newer animation layout than build 35662 reads, and copying
one across unchanged gives an invisible model rather than an error.

### 2.4 Packaging

Whatever gets ported goes into one patch laid out the way the client expects, dropped next
to the stock archives: `DBFilesClient/*.db2`, plus `Interface/Icons/` and the model folders
if new art ships. Never edit the stock archives, and keep one patch per concern so a bad
model patch can be removed without losing the item rows.

## 3. What is not in this folder, and will not be

- No DB2, M2, BLP or ADT file. Nothing here may ship a retail asset.
- No credential, token or session. Nothing in the module talks to a retail service; the
  channels page is a specification of what such an integration would need, not an
  implementation.
- No automated capture of the vendor pages at build time. `build_drafts.py` reads the CSVs
  and never the network; `fetch_item_ids.py` is the only tool that fetches anything, and it
  writes a CSV.

## 4. When something looks wrong

| Symptom | Cause | Fix |
| --- | --- | --- |
| a stall imports with no stock | every one of its lines lacked a price | fill the price in `data/town-center-stock.csv` and re-run `build_drafts.py`; the 84 skipped lines are listed as comments at the end of `05_stock_retail.sql` |
| a piece sells for far less than retail | the capture has the wrong price, or two stalls were merged by name | check the row in `data/town-center-stock.csv`; ids and prices are keyed on vendor plus item, not item alone |
| the furniture is a boulder, a banner or a keg where a bed should be | that is the stand-in: 8.3.7 has no bed, rug, table or lantern constant at all | edit `data/town-center-go-map.csv` and rebuild; the `match` column says how the row was chosen |
| a placed piece is invisible | the entry is not in `gameobject_template`, or has no model | check the entry with `SELECT entry, name, displayId FROM gameobject_template WHERE entry = <n>` |
| an item shows as `Unknown item` in game | no `item_template` row for the retail id | doc 12 sec.3, one donor row per id |
| an item shows a question mark | wrong or missing icon FileDataID | doc 12 sec.4.5 |
| a number in a document disagrees with the SQL | the document was typed, the SQL was generated | `python3 tools/housing/verify_drafts.py`; it checks the documented counts against the tables |
| the whole draft looks stale | the CSVs changed and the draft was not rebuilt | `python3 tools/housing/build_drafts.py` then `verify_drafts.py` |
