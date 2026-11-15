#!/usr/bin/env python3
"""Write the model-port worklist for the retail draft.

Doc 12 is the procedure for getting a Midnight item into the 8.3.7 client. A procedure is not
a job list: it says how to port one item, not which of the 581 pieces in the catalogue are
port candidates, what each one currently draws as, or which retail item id each one carries.

This writes that list, twice:

    generated/midnight-port-worklist.md   grouped by category, one row per piece
    generated/midnight-port-worklist.csv  the same rows flat, for a spreadsheet

Every column that can be filled from this folder is filled. The columns that need a client -
the display id, the model and texture FileDataIDs, the icon, the patch name - are left empty
for the port, which is the point: the worklist is the answer to "how many are there, and which
ones".

    python3 tools/housing/port_worklist.py           # write both files
    python3 tools/housing/port_worklist.py --check   # fail if they are stale

Nothing here invents an id. A piece with no captured retail item id is marked catalogue-only,
because there is no retail row to port: it already places as its stand-in game object.
"""

from __future__ import annotations

import argparse
import csv
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
DRAFT = os.path.join(ROOT, "drafts", "midnight-port")
DECOR_SQL = os.path.join(DRAFT, "sql", "world", "04_decor_retail.sql")
STOCK_SQL = os.path.join(DRAFT, "sql", "world", "05_stock_retail.sql")
VENDOR_SQL = os.path.join(DRAFT, "sql", "world", "03_vendors_retail.sql")
GO_MAP = os.path.join(ROOT, "data", "town-center-go-map.csv")
PROGRESSION_SQL = os.path.join(DRAFT, "sql", "world", "07_progression.sql")
WORLD_SQL = os.path.join(DRAFT, "sql", "world", "02_world_content.sql")
OUT_DIR = os.path.join(ROOT, "generated")
OUT_MD = os.path.join(OUT_DIR, "midnight-port-worklist.md")
OUT_CSV = os.path.join(OUT_DIR, "midnight-port-worklist.csv")

SURFACE = {0: "floor", 1: "wall", 2: "ceiling", 3: "any"}
PLACEMENT = {0: "outdoor", 1: "indoor", 2: "both"}
CURRENCY = {0: "gold", 3363: "Community Coupons", 33226: "Tricky Treats"}
FACTION = {1: "Horde", 2: "Alliance", 3: "both"}

# The columns the port fills in. Kept in one tuple so the markdown table, the CSV header and
# the doc can be checked against each other.
CLIENT_COLUMNS = [
    ("display_id", "ItemDisplayInfo"),
    ("m2", "model FileDataID"),
    ("blp", "texture FileDataID"),
    ("icon", "icon FileDataID"),
    ("patch", "patch"),
    ("status", "status"),
]

ROWS = re.compile(r"^\((\d+), '(.*?)', (\d+), (\d+), (\d+), (\d+), (\d+), '(.*?)', "
                  r"(\d+), (\d+), (\d+), (\d+), (\d+), ([\d.]+), ([\d.]+), (\d+)\)[,;]?$")


def unescape(text):
    return text.replace("''", "'")


def parse_decor():
    pieces = []
    with open(DECOR_SQL, encoding="utf-8") as handle:
        for line in handle:
            match = ROWS.match(line.strip())
            if not match:
                continue
            (decor_id, name, gameobject, item, budget, surface, placement, category,
             source_type, source_id, price, currency, faction, scale_min, scale_max, flags) = match.groups()
            pieces.append({
                "id": int(decor_id),
                "name": unescape(name),
                "gameobject": int(gameobject),
                "item": int(item),
                "budget": int(budget),
                "surface": SURFACE.get(int(surface), surface),
                "placement": PLACEMENT.get(int(placement), placement),
                "category": category,
                "price": int(price),
                "currency": CURRENCY.get(int(currency), "currency " + currency),
                "faction": FACTION.get(int(faction), faction),
                "vendor": int(source_id),
            })
    return pieces


def parse_vendors():
    """vendor id -> name, read from the vendor file's own rows."""
    names = {}
    row = re.compile(r"^\((\d+), '(.*?)',")
    with open(VENDOR_SQL, encoding="utf-8") as handle:
        for line in handle:
            match = row.match(line.strip())
            if match:
                names[int(match.group(1))] = unescape(match.group(2))
    return names


def parse_stock():
    """decor id -> set of vendor ids that sell it."""
    sellers = {}
    row = re.compile(r"^\((\d+), (\d+),")
    with open(STOCK_SQL, encoding="utf-8") as handle:
        for line in handle:
            match = row.match(line.strip())
            if match:
                sellers.setdefault(int(match.group(2)), set()).add(int(match.group(1)))
    return sellers


def parse_go_map():
    """item name -> (constant name, match class) for the stand-in game object."""
    mapping = {}
    with open(GO_MAP, newline="", encoding="ascii") as handle:
        for row in csv.DictReader(handle):
            mapping[row["item"]] = (row["go_constant"], row["match"])
    return mapping


def build():
    pieces = parse_decor()
    vendors = parse_vendors()
    sellers = parse_stock()
    go_map = parse_go_map()

    for piece in pieces:
        constant, how = go_map.get(piece["name"], ("", ""))
        piece["stand_in"] = constant
        piece["matched"] = how
        sold_by = sorted(sellers.get(piece["id"], set()))
        piece["vendors"] = ", ".join(vendors.get(vendor, "vendor " + str(vendor)) for vendor in sold_by)
        piece["portable"] = "yes" if piece["item"] else "catalogue only"

    return pieces


def count_rows(path, table):
    """Rows in one INSERT for a table, counted the way the SQL file writes them.

    Written for the art inventory rather than for the catalogue: the shells, rooms, amenities
    and dyes each need art of their own, and the count has to come from the draft rather than
    from a number in a document that can drift away from it.
    """
    text = open(path, encoding="utf-8").read()
    match = re.search(r"INSERT INTO `" + table + r"`[^;]*?;", text, re.S)
    if not match:
        return None
    # Rows are one per line except where a short table packs several - the dyes do - so a row
    # counts at the start of a line or after the previous row's closing bracket.
    return len(re.findall(r"(?:^|[\)]\s*,)\s*\(", match.group(0), re.M))


def art_inventory():
    """Everything outside the decor catalogue that needs a model, a texture or a map."""
    return [
        ("Exterior shells", count_rows(PROGRESSION_SQL, "housing_shell"),
         "one building model and an interior per shell"),
        ("Rooms", count_rows(PROGRESSION_SQL, "housing_room"),
         "a room shell is geometry placed on the plan; the draft spends cells, not models"),
        ("Amenities", count_rows(WORLD_SQL, "housing_amenity"),
         "town-centre and biome props, each a game object"),
        ("Dye colours", count_rows(PROGRESSION_SQL, "housing_dye"),
         "a dye is a texture variant of a piece, not a new model"),
        ("Pet beds", count_rows(PROGRESSION_SQL, "housing_decor_creature"),
         "already real game objects - the only art they need is the companion, which is the player's"),
        ("Portal destinations", count_rows(PROGRESSION_SQL, "housing_portal"),
         "existing world locations; nothing to make"),
    ]


def summary(pieces):
    with_item = [piece for piece in pieces if piece["item"]]
    without = [piece for piece in pieces if not piece["item"]]

    by_category = {}
    for piece in pieces:
        entry = by_category.setdefault(piece["category"], {"total": 0, "item": 0, "objects": set()})
        entry["total"] += 1
        entry["objects"].add(piece["gameobject"])
        if piece["item"]:
            entry["item"] += 1

    objects = {piece["gameobject"] for piece in pieces}
    return {
        "total": len(pieces),
        "with_item": len(with_item),
        "without_item": len(without),
        "objects": len(objects),
        "by_category": by_category,
        "portable_pieces": with_item,
    }


def markdown(pieces):
    stats = summary(pieces)
    lines = []
    add = lines.append

    add("# The Midnight port, piece by piece")
    add("")
    add("Generated by `tools/housing/port_worklist.py` from")
    add("`drafts/midnight-port/sql/world/04_decor_retail.sql`. Regenerate it rather than edit it:")
    add("the catalogue is the source and this is the view of it.")
    add("")
    add("Doc 12 is *how* to port an item - extract, convert, write the DB2 rows, package, verify.")
    add("This is *which* items, what each one currently draws as, and which columns are still")
    add("empty. A piece with a retail item id is a port candidate. A piece without one is")
    add("catalogue-only: there is no retail row behind it, so it is already as ported as it gets")
    add("and it will keep drawing its stand-in game object.")
    add("")
    add("| | Count |")
    add("| --- | --- |")
    add("| Decor rows in the catalogue, `04_decor_retail.sql` | %d |" % stats["total"])
    add("| Carrying a retail item id - port candidates | %d |" % stats["with_item"])
    add("| No retail item id - catalogue only | %d |" % stats["without_item"])
    add("| Distinct stand-in game objects | %d |" % stats["objects"])
    add("| Riddle-hunt reward pieces, `08_hunts.sql` (not in this list) | 10 |")
    add("")
    add("The ten hunt rewards are the module's own pieces - ids 1900-1909, quest rewards for the")
    add("riddles - so there is no retail item behind them and no model to port.")
    add("")
    add("## By category")
    add("")
    add("| Category | Pieces | With a retail id | Distinct stand-ins |")
    add("| --- | --- | --- | --- |")
    for category in sorted(stats["by_category"]):
        entry = stats["by_category"][category]
        add("| %s | %d | %d | %d |" % (category, entry["total"], entry["item"], len(entry["objects"])))
    add("")
    add("## The worklist")
    add("")
    add("The six empty columns on the right are what the port fills in. Doc 12 sec.4.4 says what")
    add("each one is: the `ItemDisplayInfo` row, the M2 and BLP FileDataIDs it names, the icon")
    add("FileDataID, the patch the row ships in, and where the piece has got to.")
    add("")
    add("`stand-in` is the 8.3.7 game object that places today; `match` is how that object was")
    add("chosen - `family`, `category`, `keyword` or `manual` in doc 03's terms. A `category`")
    add("match is the loudest one to revisit when a model arrives, because it is the piece whose")
    add("stand-in was picked by category rather than by likeness.")
    add("")

    header = ("| id | piece | category | surface | where | price | retail item | stand-in | match "
              "| display | m2 | blp | icon | patch | status |")
    rule = "| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |"
    add(header)
    add(rule)

    for category in sorted(stats["by_category"]):
        for piece in [p for p in pieces if p["category"] == category]:
            price = "%d %s" % (piece["price"], "g" if piece["currency"] == "gold" else piece["currency"])
            item = str(piece["item"]) if piece["item"] else "-"
            add("| %d | %s | %s | %s | %s | %s | %s | %d %s | %s |  |  |  |  |  | %s |" % (
                piece["id"], piece["name"].replace("|", "\\|"), piece["category"], piece["surface"],
                piece["placement"], price, item, piece["gameobject"],
                piece["stand_in"] or "-", piece["matched"] or "-", piece["portable"]))

    add("")
    add("## Everything else that needs art")
    add("")
    add("The catalogue is the largest job, not the whole one. Counted from this draft's own SQL:")
    add("")
    add("| Asset | Count | What the art is |")
    add("| --- | --- | --- |")
    for name, count, note in art_inventory():
        add("| %s | %s | %s |" % (name, "-" if count is None else count, note))
    add("")
    add("Doc 07 sec.4 is the effort estimate and doc 06 sec.1.3 is the honest statement of what")
    add("the room system does without room art: cells, slots and the plan are real, the interior")
    add("belongs to the shell.")
    add("")
    add("## What the six columns want")
    add("")
    add("| Column | Filled from | Doc 12 |")
    add("| --- | --- | --- |")
    add("| display | a new `ItemDisplayInfo.db2` row | sec.4.4 step 1 |")
    add("| m2 | the model FileDataID that row names, after conversion to the BfA generation | sec.4.2 |")
    add("| blp | the texture FileDataID, BLP2 with DXT1/DXT5 and mipmaps | sec.4.3 |")
    add("| icon | an existing 8.3.7 icon FileDataID, borrowed by category | sec.4.5 |")
    add("| patch | the patch file the rows ship in - one per concern | sec.4.6 |")
    add("| status | `todo`, `catalogue`, `model`, `texture`, `packaged`, `verified` | sec.4.7 |")
    add("")
    return "\n".join(lines) + "\n"


def csv_rows(pieces):
    header = ["decor_id", "name", "category", "surface", "placement", "price", "currency",
              "faction", "retail_item", "stand_in_entry", "stand_in_constant", "match",
              "sold_by", "portable"] + [name for name, _ in CLIENT_COLUMNS]
    rows = []
    for piece in sorted(pieces, key=lambda p: p["id"]):
        rows.append([
            piece["id"], piece["name"], piece["category"], piece["surface"], piece["placement"],
            piece["price"], piece["currency"], piece["faction"], piece["item"] or "",
            piece["gameobject"], piece["stand_in"], piece["matched"], piece["vendors"],
            piece["portable"],
        ] + [""] * len(CLIENT_COLUMNS))
    return header, rows


def render_csv(pieces):
    header, rows = csv_rows(pieces)
    out = []
    out.append(",".join(header))
    for row in rows:
        cells = []
        for value in row:
            text = str(value)
            if any(char in text for char in ',"\n'):
                text = '"' + text.replace('"', '""') + '"'
            cells.append(text)
        out.append(",".join(cells))
    return "\n".join(out) + "\n"


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--check", action="store_true",
                        help="compare the two files with what the SQL says; do not write")
    args = parser.parse_args()

    pieces = build()
    if not pieces:
        print("no decor rows found in %s" % DECOR_SQL)
        return 1

    markdown_text = markdown(pieces)
    csv_text = render_csv(pieces)

    if args.check:
        findings = 0
        for path, expected in ((OUT_MD, markdown_text), (OUT_CSV, csv_text)):
            if not os.path.exists(path):
                print("missing: %s - run python3 tools/housing/port_worklist.py" % path)
                findings += 1
            elif open(path, encoding="utf-8").read() != expected:
                print("stale: %s - run python3 tools/housing/port_worklist.py" % path)
                findings += 1
        print("%d finding(s)" % findings)
        return 1 if findings else 0

    os.makedirs(OUT_DIR, exist_ok=True)
    with open(OUT_MD, "w", encoding="utf-8", newline="\n") as handle:
        handle.write(markdown_text)
    with open(OUT_CSV, "w", encoding="utf-8", newline="\n") as handle:
        handle.write(csv_text)

    stats = summary(pieces)
    print("wrote %s (%d pieces, %d with a retail id)" % (OUT_MD, stats["total"], stats["with_item"]))
    print("wrote %s" % OUT_CSV)
    return 0


if __name__ == "__main__":
    sys.exit(main())
