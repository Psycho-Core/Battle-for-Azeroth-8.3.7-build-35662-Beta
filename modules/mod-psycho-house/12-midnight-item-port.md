# 12 - Porting Midnight items into the 8.3.7 client

This is the client half of the port. Doc 07 says what the unlocked client can do in
principle; this document is the actual sequence for getting a Midnight decor item out of the
Midnight client, into 8.3.7 build 35662, and into the housing catalogue so it can be bought,
owned and placed.

Two things are already true before any of this starts:

- the exe accepts modified files, so no signature or checksum work is needed here;
- the catalogue already knows every id. `data/town-center-items.csv` holds 739 rows and 439
  distinct retail item ids, and `drafts/midnight-port/sql/world/04_decor_retail.sql` puts
  each one in `housing_decor.item_entry` next to the 8.3.7 game object that stands in for it.

So the port is not "find the items". It is "make those ids real in a 8.3.7 client".

## 1. What an item has to be for the module to work

The module only needs three things from an item, in this order of importance:

| Need | Where it lives | Without it |
| --- | --- | --- |
| The id exists and can be granted to a collection | `item_template` on the server, `Item.db2` / `ItemSparse.db2` on the client | The buy fails, or the item shows as `Unknown item` |
| An icon the client can draw | icon FileDataID referenced from `ItemSparse` | The client draws a question mark, everything else still works |
| A model for the placed piece | `ItemAppearance` -> `ItemDisplayInfo` -> M2, or a game object | The piece places as its stand-in game object instead |

That third row is the one that decides how big the job is. The catalogue already maps every
piece to a real 8.3.7 game object, so a piece with no Midnight model still places, still
looks like furniture, and still costs what retail charges. Porting a Midnight model only
changes what the placed piece looks like. Nothing else in the system depends on it.

## 2. Ids: keep the retail ones

The register's ids run from **235 523** to **282 347** - all of them above 235 000, none of
them inside the range a 8.3.7-era database uses. Check that on the actual database before
committing to it, because this is the one assumption that costs the most to unwind:

```sql
SELECT MAX(`entry`) FROM `item_template`;
SELECT COUNT(*) FROM `item_template`;
```

If the answer is below 235 000, the 439 ids are free and nothing in the module has to
change: the CSV, the SQL and the client patch all agree, and a piece bought in game is the
same id as the piece in the register. If a handful collide, do not renumber the whole
register - offset just the colliding ids into a high band (3 000 000+) and update
`item_entry` in `04_decor_retail.sql` plus the id column in the CSV.

What not to do: renumber on the way in. The register is the only place the Midnight id and
the 8.3.7 stand-in are written down together, and losing that link means never being able
to tell which 8.3.7 barrel is supposed to be the Consortium one.

## 3. Route A - catalogue only, no client files

The cheapest port, and the one I would ship first:

1. Insert an `item_template` row for each id, name and price copied from the register, with
   an icon id borrowed from an existing 8.3.7 item of the same family.
2. Leave `ItemAppearance` alone.
3. The buyer gets a real item, the collection tracks it, and placing it spawns the mapped
   8.3.7 game object from `data/town-center-go-map.csv`.

The result is the retail catalogue with 8.3.7 furniture in the slots. That is what draft A
already assumes, and it is the reason the port can be useful before a single Midnight file is
converted. It also works whether the client is patched or not.

## 4. Route B - the full port, file by file

The long way. Per item:

### 4.1 Extract from the Midnight client

- Item rows: `Item.db2`, `ItemSparse.db2` (name, description, class, subclass, quality,
  icon, stack size, price, flags).
- Appearance chain: `ItemAppearance.db2` -> `ItemDisplayInfo.db2` (model name, texture,
  icon, attachment, particle colour), plus `ItemModifiedAppearance.db2` to tie the item to
  the appearance.
- Models and textures: the M2/SKIN and BLP the display info names.

Community listfiles and CASC readers are the normal way in. Note the file versions: a
Midnight M2 is not a BfA M2. Build 35662 reads the BfA generation, with `.skin` side files
and the older animation layout; Midnight ships the newer generation with `.skel`/`.anim`.
Copying the file across unchanged gives an invisible model, not an error - which is the
worst kind of failure to debug.

### 4.2 Convert the model

Three options, worst to best in effort:

| Option | What it means | Result |
| --- | --- | --- |
| Reuse a BfA model | Point the display info at an existing 8.3.7 M2 and only port the texture | Cheapest, looks close for furniture |
| Re-export and rebuild | Export the Midnight M2 to a mesh, import with a BfA-targeting Blender exporter | Faithful, an evening per family |
| Convert the M2 directly | Only if the file is already the BfA generation | Rare, verify before planning around it |

For a decor item that is a static prop, the second option is mechanical: geometry, one
texture, no skeleton. Chairs, tables, barrels and crates need nothing else. Anything with an
animation (a lantern flame, a banner) needs the animation rebuilt as well, which is where
the hours go.

### 4.3 Convert the texture

8.3.7 wants BLP2 with DXT1 or DXT5 and mipmaps, power-of-two dimensions. Midnight textures
are often newer formats or BC7; both have to be re-encoded. Alpha matters for anything with
cut-out edges - curtains, pennants, rugs with fringes - and a DXT1 conversion will make the
cut-out turn black.

### 4.4 Write the DB2 rows

For each item, in this order:

1. `ItemDisplayInfo` row: model name, texture name, icon, attachment id, particle colour.
2. `ItemAppearance` row: `DisplayID` pointing at the display info, plus its inventory type.
   Decor is not equipped, so the invtype only has to be legal, not meaningful.
3. `ItemModifiedAppearance` row: item id -> appearance id, order index 0.
4. `Item.db2` row and `ItemSparse.db2` row: name, description, class and subclass, quality,
   icon FileDataID, stack size 1, price 0 (the module charges through its own table, not
   through the item's vendor price), flags that keep it out of the auction house.
5. `item_template` on the server with the **same entry** and the same name, so `.additem`,
   the collection grant and the client tooltip all agree.

Keeping the server row and the client row on the same id is not optional: the housing table
stores the id, and a mismatch means a piece that exists on the client and not on the server,
or the reverse.

### 4.5 Icons without making icons

The fastest correct answer: reuse an existing 8.3.7 icon FileDataID that already matches the
piece (a barrel icon for barrels, a rug icon for rugs). The register's categories -
Structural, Storage, Lighting, Wall Art, Rugs, Beds, Tables, Seating, Nature, Books, Pets,
Holiday, Accents - give thirteen sensible choices, and the client already has every one of
them. New icons are worth doing for the coupon set and the Endeavor rewards, where the icon
is the only thing that distinguishes two similar pieces.

### 4.6 Package it

Everything from 4.4 and 4.5 goes into one patch, laid out the way the client expects:

```
DBFilesClient/Item.db2
DBFilesClient/ItemSparse.db2
DBFilesClient/ItemAppearance.db2
DBFilesClient/ItemModifiedAppearance.db2
DBFilesClient/ItemDisplayInfo.db2
Interface/Icons/            (only if new icons are shipped)
Creature/ or World/...      (only if new models are shipped)
```

With the exe already unlocked, a patch file dropped next to the stock archives is read at
startup. Two rules that save afternoons: never edit the stock archives, and keep one patch
per concern (one for items, one for models) so a bad model patch can be removed without
losing the item rows.

### 4.7 Verify, in this order

1. Server: `.lookup item <id>` finds the row, `.additem <id>` creates it.
2. Client: `/dump GetItemInfo(<id>)` returns the name, not `nil`.
3. Icon: the item shows a real icon, not a question mark.
4. Appearance: `/dump select(2, GetItemInfo(<id>))` and the dressing room show the model.
5. Housing: buy the piece, place it, and confirm the placed object is the ported model and
   not the stand-in game object.

Step 3 failing while 1 and 2 pass means the icon FileDataID is wrong, not the item. Step 4
failing while 3 passes means the appearance chain is wrong. Step 5 failing while 4 passes
means `housing_decor.item_entry` and the game object mapping disagree.

## 5. What I could not check here

There is no Midnight client, no CASC reader and no 8.3.7 client in this workspace, so:

- the DB2 column lists above are the shapes those tables have, not field-by-field dumps I
  verified against a live pair of clients;
- the M2 generation numbers are the reason to convert, but the exact version boundary is
  something to confirm against the build's own loader;
- the packaging paths are the conventional ones, not read out of this tree.

Everything else in this document - the ids, the item count, the mapping to game objects, the
catalogue rows - is checked against files in this folder and reproducible with the tools in
`tools/housing/`.

## 6. The housing menus, client side

The menu cannot be a copy of Midnight's frames. The 8.3.7 client does not have the Midnight
API, and its FrameXML is its own. What it can be is the same arrangement, rebuilt:

| Midnight menu | Here |
| --- | --- |
| Housing window opened from a menu button | `PsychoHouse` addon, `House` button next to the minimap, `/house` or `/housing` |
| House tab with the plot card | House tab: name, tier, indoor and outdoor budgets, homestone, lock, rename, tier upgrades |
| Catalogue with filters | Catalogue tab: name filter, buy, "owned" state, currency named per row |
| Placement mode | `/housing place`, `/housing move`, `/housing remove`, or the Placed tab |
| Neighbourhood directory | Neighbours tab: every plot with owner, tier, and a Visit button |
| Endeavor track and milestones | Endeavors tab: the four milestones and what each unlocks |
| Blueprint share codes | Blueprints tab: the `PCH1` format, with copy and paste |
| Nothing equivalent | Guide tab: the opcode list, so the transport is never a mystery |

That is `addon/HousingUI/Menu.lua` plus `Core.lua`, in the ported draft at
`drafts/midnight-port/addon/HousingUI/`, and in the native draft as well because both drafts
ship the same core. Install renames the folder and the `.toc` to `PsychoHouse`, which the
client pairs by name.

Two optional extras, both of which need the patched client and neither of which I would do
before the addon menu works:

- **A micro-menu button.** Add a housing button next to the collections tab by patching the
  menu bar Lua in `Interface/FrameXML/` inside a patch, or - far safer - creating the button
  from the addon and anchoring it to `MicroMenu`. The addon route cannot break the default
  UI; the patch route can.
- **A key binding.** `Bindings.xml` in the addon, one entry, `BINDING_HEADER_PSYCHO_HOUSE`
  and `BINDING_NAME_PSYCHO_HOUSE_TOGGLE`, calling the same toggle the button calls.

What I would not do: patch the collections journal to add a housing tab. The journal is
driven by DB2 rows and secure code, and a broken collections frame is a support ticket that
never ends.

## 7. Order of work

If the goal is a client that feels like Midnight, this is the order that spends the effort
where it shows:

| Step | Work | Why here |
| --- | --- | --- |
| 1 | Route A for all 439 ids with borrowed icons | The whole catalogue becomes real in a day |
| 2 | Icons for the 90 coupon pieces and the Endeavor rewards | Those are the pieces people look at while browsing |
| 3 | Models for one family at a time - barrels, crates, then seating | A family is one texture and one mesh reused |
| 4 | Room shells and exterior shells | The largest models, the largest payoff, the most work |
| 5 | Animated pieces last | Animation is the only part that is months rather than evenings |

## 8. Tooling

| Tool | Use |
| --- | --- |
| A CASC reader with a current listfile | Pulling Item/ItemSparse/ItemAppearance/ItemDisplayInfo and the models |
| `wow.export` | Bulk model and texture export, with textures already named after their M2 |
| Blender with a BfA-targeting M2 exporter | Rebuilding a Midnight mesh as a BfA M2 |
| A BLP encoder set to BLP2/DXT | Re-encoding textures, mipmaps on, alpha mode picked per piece |
| A DB2 editor with 8.3.7 definitions | Writing the rows, with the structure from this build's own DB2 headers |
| An MPQ or CASC patch writer | Packaging, one patch per concern |
| `tools/housing/fetch_item_ids.py` | Re-checking the register's ids against the live item pages |
| `tools/housing/check_sql_columns.py` | After any `item_template` change, checking every column exists in this build's SQL vocabulary |

## 9. Per-item checklist

- [ ] item id free on the server (`SELECT` against `item_template`)
- [ ] `item_template` row, same entry, same name as the register
- [ ] `Item.db2` + `ItemSparse.db2` rows
- [ ] icon FileDataID chosen (borrowed or new)
- [ ] `ItemDisplayInfo` row
- [ ] `ItemAppearance` row
- [ ] `ItemModifiedAppearance` row
- [ ] model converted, or the stand-in game object kept on purpose
- [ ] patch built, one per concern
- [ ] `.additem`, `GetItemInfo`, icon, dressing room, buy, place - in that order
- [ ] `housing_decor.item_entry` still matches the id that was ported
