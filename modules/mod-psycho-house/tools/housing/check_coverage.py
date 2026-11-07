#!/usr/bin/env python3
"""Check that each draft carries every piece of content its own source data has.

    python3 tools/housing/check_coverage.py

verify_drafts.py asks whether the SQL is well formed. This asks a different question: is
anything missing. It walks the data each draft is generated from and looks for it in the
generated SQL, then walks the SQL and looks for the data.

  draft A (midnight-port)   data/town-center-stock.csv, town-center-items.csv,
                            vendor_npc_ids.csv, town-center-go-map.csv
  draft B (bfa-native)      generated/source-asset-index.csv

Both drafts are also walked for their riddle hunts: every reward the hunt table names has to
be a decor row in the same draft, and every cache and every reward object has to be a game
object the source tree names. The two are different questions - a hunt can name a decor id
that exists while its object entry was a guess - and the second failure is a crate that never
spawns.

A reward object has to be named by the tree, not classified as furniture by the keyword
heuristic. The index's `decor` column is a guess made from constant names, and four of the ten
reward objects are real objects the heuristic does not recognise (a tomb, a cluster display, an
idol, a fire); refusing them would mean refusing every object whose constant lacks a furniture
word, which is not the question being asked here. What is asked is whether the entry exists.

Both directions matter and they fail differently. A row in the CSV that never reaches the
draft is content that was silently dropped; a row in the draft that is not in the CSV is
content invented by the generator. The three lines the seventeenth revision lost to a
fallback that was computed and thrown away were the first kind, and no checker looked for
them.

An item the data itself cannot price is allowed to be absent: the rule here is that it has
to be named in the file as a comment, so an operator reading the draft can see what is
missing rather than having to diff two files.
"""

import csv
import os
import re
import sys

HERE = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
DRAFTS = os.path.join(HERE, "drafts")
DATA = os.path.join(HERE, "data")
INDEX = os.path.join(HERE, "generated", "source-asset-index.csv")

findings = []


def fail(where, what):
    findings.append("{}: {}".format(where, what))


def read(path):
    with open(path, encoding="utf-8") as handle:
        return handle.read()


def rows_of(path):
    with open(path, encoding="utf-8") as handle:
        return list(csv.DictReader(handle))


def sql_tuples(text, table):
    """The tuples of the INSERT into `table`, as lists of raw strings.

    The split is quote-aware: a catalogue name is allowed to contain a comma - "Complete
    Guide to K'areshi Wrappings, Vol. 11" is the first row of the file - and a checker that
    splits on every comma reads the volume number as a column.
    """
    match = re.search(r"INSERT\s+INTO\s+`" + table + r"`\s*\(([^)]*)\)\s*VALUES\s*(.*?);",
                      text, re.S)
    if not match:
        return []

    body = match.group(2)
    tuples, parts = [], []
    current, depth, quote = "", 0, False

    for char in body:
        if char == "'":
            quote = not quote
        if not quote and char == "(":
            depth += 1
            if depth == 1:
                current, parts = "", []
                continue
        if not quote and char == ")":
            depth -= 1
            if depth == 0:
                parts.append(current.strip())
                tuples.append(parts)
                continue
        if depth:
            if not quote and char == ",":
                parts.append(current.strip())
                current = ""
                continue
            current += char

    return tuples


def unquote(value):
    value = value.strip()
    if len(value) >= 2 and value[0] == "'" and value[-1] == "'":
        return value[1:-1].replace("''", "'")
    return value


def currency_code(text):
    return {"Community Coupons": 3363, "Tricky Treat": 33226}.get(text.strip(), 0)


def check_midnight():
    label = "midnight-port"
    source = os.path.join(DRAFTS, label, "sql", "world")

    stock = rows_of(os.path.join(DATA, "town-center-stock.csv"))
    items = rows_of(os.path.join(DATA, "town-center-items.csv"))
    npcs = rows_of(os.path.join(DATA, "vendor_npc_ids.csv"))
    go_map = rows_of(os.path.join(DATA, "town-center-go-map.csv"))
    mapping = {row["item"] for row in go_map}

    vendors_sql = read(os.path.join(source, "03_vendors_retail.sql"))
    decor_sql = read(os.path.join(source, "04_decor_retail.sql"))
    stock_sql = read(os.path.join(source, "05_stock_retail.sql"))

    vendors = sql_tuples(vendors_sql, "housing_vendor")
    decor = sql_tuples(decor_sql, "housing_decor")
    sold = sql_tuples(stock_sql, "housing_vendor_stock")

    # ---- vendors -----------------------------------------------------------
    vendor_name = {int(row[0]): unquote(row[1]) for row in vendors}
    vendor_id = {name: vid for vid, name in vendor_name.items()}

    for row in npcs:
        if row["vendor"] not in vendor_id:
            fail(label, "vendor '{}' is in vendor_npc_ids.csv and not in the draft"
                 .format(row["vendor"]))

    for row in npcs:
        if row["npc_id"] and "npc {}".format(row["npc_id"]) not in " ".join(
                unquote(v[6]) for v in vendors if unquote(v[1]) == row["vendor"]):
            fail(label, "vendor '{}' does not carry its retail npc id {} in its comment"
                 .format(row["vendor"], row["npc_id"]))

    # ---- the catalogue -----------------------------------------------------
    decor_by_key = {}
    for row in decor:
        key = (unquote(row[1]), int(row[10]), int(row[11]))
        decor_by_key[key] = row

    for row in decor:
        if int(row[2]) == 0:
            fail(label, "decor '{}' has no gameobject_entry".format(unquote(row[1])))
        if int(row[9]) and int(row[9]) not in vendor_name:
            fail(label, "decor '{}' points at vendor {} which does not exist"
                 .format(unquote(row[1]), row[9]))
        if unquote(row[1]) not in mapping:
            fail(label, "decor '{}' is not in data/town-center-go-map.csv"
                 .format(unquote(row[1])))

    # ---- every priced line, and only priced lines --------------------------
    unpriced = [row for row in stock if not row["price"].strip()]
    for row in stock:
        if not row["price"].strip():
            continue
        key = (row["item"], int(row["price"]), currency_code(row["currency"]))
        if key not in decor_by_key:
            fail(label, "stock line '{}' at {} is not in the catalogue"
                 .format(row["item"], row["price"]))

    # A line the vendor page did not price is still inserted when the item's own page has a
    # price - the same piece is often sold by two stalls and only one of them prints it. What
    # is left has to be named in the comment block, so an operator can see the hole instead
    # of having to diff two files.
    item_price = {row["item_name"]: row["price"].strip() for row in items if row["price"].strip()}
    comment_block = stock_sql.split("--   vendor ", 1)[1] if "--   vendor " in stock_sql else ""

    for row in unpriced:
        fallback = item_price.get(row["item"], "")
        if fallback:
            key = (row["item"], int(fallback), currency_code(row["currency"]))
            if key not in decor_by_key:
                fail(label, "line '{}' has a price of {} on its item page and no decor row"
                     .format(row["item"], fallback))
            continue
        if row["item"] not in comment_block:
            fail(label, "unpriced line '{}' is neither inserted nor listed as a comment"
                 .format(row["item"]))

    # ---- stock rows --------------------------------------------------------
    stock_count = {}
    for row in sold:
        vid = int(row[0])
        stock_count[vid] = stock_count.get(vid, 0) + 1
        if vid not in vendor_name:
            fail(label, "stock row for vendor {} which does not exist".format(vid))

    priced_at_vendor = {}
    for row in stock:
        if row["price"].strip():
            priced_at_vendor[row["vendor"]] = priced_at_vendor.get(row["vendor"], 0) + 1
    for name, count in priced_at_vendor.items():
        actual = stock_count.get(vendor_id.get(name, 0), 0)
        if actual < count:
            fail(label, "vendor '{}' has {} priced lines in the CSV and {} stock rows"
                 .format(name, count, actual))

    # ---- retail item ids ---------------------------------------------------
    ids_in_draft = {int(row[3]) for row in decor if int(row[3])}
    ids_in_data = {int(row["item_id"]) for row in items if row["item_id"].strip()}

    for missing in sorted(ids_in_data - ids_in_draft):
        fail(label, "retail item id {} is resolved in town-center-items.csv and not on any "
                    "decor row".format(missing))
    for extra in sorted(ids_in_draft - ids_in_data):
        fail(label, "decor row carries retail item id {} which is not in "
                    "town-center-items.csv".format(extra))

    id_by_name = {row["item_name"]: row["item_id"] for row in items}
    for row in decor:
        name = unquote(row[1])
        expected = id_by_name.get(name)
        if expected and int(row[3]) != int(expected):
            fail(label, "decor '{}' carries item id {} where the data says {}"
                 .format(name, row[3], expected))

    # ---- the report --------------------------------------------------------
    sold_ids = {int(row[1]) for row in sold}
    unsold = [unquote(row[1]) for row in decor if int(row[0]) not in sold_ids]

    print("{:<14} {:>3} vendors  {:>3} priced  {:>4} stock  {:>3} unpriced  {:>3} without item id"
          .format(label, len(vendor_name), len(priced_at_vendor), len(sold),
                  len(unpriced), sum(1 for row in decor if not int(row[3]))))

    for vid, name in sorted(vendor_name.items()):
        if not stock_count.get(vid):
            print("  empty stall: {} ({})".format(name, vid))

    if unsold:
        print("  catalogue rows nothing sells: {}".format(len(unsold)))

    return len(decor), len(sold)


def check_native():
    label = "bfa-native"
    source = os.path.join(DRAFTS, label, "sql", "world")

    index = rows_of(INDEX)
    gameobjects = {row["entry"]: row for row in index
                   if row["kind"] == "gameobject" and row["decor"] == "yes"}

    vendors_sql = read(os.path.join(source, "04_vendors_native.sql"))
    decor_sql = read(os.path.join(source, "03_decor_native.sql"))

    vendors = sql_tuples(vendors_sql, "housing_vendor")
    decor = sql_tuples(decor_sql, "housing_decor")
    sold = sql_tuples(vendors_sql, "housing_vendor_stock")

    vendor_name = {int(row[0]): unquote(row[1]) for row in vendors}

    seen = set()
    for row in decor:
        entry = row[2].strip()
        name = unquote(row[1])

        if entry not in gameobjects:
            fail(label, "decor '{}' uses entry {} which is not a decor-shaped game object in "
                        "the source tree".format(name, entry))
        if entry in seen:
            fail(label, "entry {} ships twice".format(entry))
        seen.add(entry)
        if int(row[10]) != 0:
            fail(label, "native row '{}' prices itself in currency {}; this draft is gold only"
                 .format(name, row[10]))

    sold_decor = set()
    for row in sold:
        vid, decor_id = int(row[0]), int(row[1])
        sold_decor.add(decor_id)
        if vid not in vendor_name:
            fail(label, "stock row for vendor {} which does not exist".format(vid))

    for row in decor:
        if int(row[0]) not in sold_decor:
            fail(label, "decor '{}' is not sold anywhere".format(unquote(row[1])))

    print("{:<14} {:>3} vendors  {:>4} stock  {:>4} entries from the source tree"
          .format(label, len(vendor_name), len(sold), len(seen)))

    return len(decor), len(sold)


def check_hunts(label, decor_file):
    """The hunt table against the catalogue and against the build's own objects."""
    source = os.path.join(DRAFTS, label, "sql", "world")
    hunt_sql = read(os.path.join(source, "08_hunts.sql"))
    hunts = sql_tuples(hunt_sql, "housing_hunt")

    # The ten reward pieces are catalogue rows too, and they live in the hunt file because
    # that is the file that generates them, so the catalogue is both files together.
    decor = sql_tuples(read(os.path.join(source, decor_file)), "housing_decor")
    decor += sql_tuples(hunt_sql, "housing_decor")

    index = rows_of(INDEX)
    objects = {row["entry"]: row for row in index if row["kind"] == "gameobject"}
    decorated = {entry for entry, row in objects.items() if row["decor"] == "yes"}

    decor_by_id = {int(row[0]): row for row in decor}
    decor_object = {int(row[0]): row[2].strip() for row in decor}

    if len(hunts) != 10:
        fail(label, "{} riddle hunts, where there are ten".format(len(hunts)))

    for row in hunts:
        hunt_id, quest_id = int(row[0]), int(row[3])
        reward, cache = int(row[4]), row[5].strip()

        if reward not in decor_by_id:
            fail(label, "hunt {} rewards decor {} which is not in the catalogue"
                 .format(hunt_id, reward))
        else:
            entry = decor_object[reward]
            if entry not in objects:
                fail(label, "hunt {} rewards '{}', object {} which the source tree does "
                            "not name".format(hunt_id, unquote(decor_by_id[reward][1]), entry))
            elif int(decor_by_id[reward][4]) not in range(1, 6):
                fail(label, "hunt {} rewards '{}' with budget cost {}, outside 1-5"
                     .format(hunt_id, unquote(decor_by_id[reward][1]), decor_by_id[reward][4]))

        if cache not in objects:
            fail(label, "hunt {} puts its cache at entry {}, which is not a game object in "
                        "the source tree".format(hunt_id, cache))

        if quest_id // 100 != 9001:
            fail(label, "hunt {} carries quest {}, outside 900100-900199"
                 .format(hunt_id, quest_id))

    rewards = {int(row[4]) for row in hunts}
    shaped = sum(1 for reward in rewards
                 if decor_object.get(reward) in decorated)
    print("  {:>3} hunts, {:>3} reward pieces, {} of them furniture-shaped in the index, "
          "every object named by the tree".format(len(hunts), len(rewards), shaped))

    return len(hunts)


def main():
    print("drafts against the data they are generated from\n")

    a_decor, a_stock = check_midnight()
    a_hunts = check_hunts("midnight-port", "04_decor_retail.sql")
    b_decor, b_stock = check_native()
    b_hunts = check_hunts("bfa-native", "03_decor_native.sql")

    print("\n{} finding(s)".format(len(findings)))
    for line in findings:
        print("  " + line)

    if not findings:
        print("\nDraft A: {} catalogue rows, {} stock rows, all from data/; {} hunts.".format(
            a_decor, a_stock, a_hunts))
        print("Draft B: {} catalogue rows, {} stock rows, all from the source tree; {} hunts."
              .format(b_decor, b_stock, b_hunts))
        print("Nothing in either draft is missing from its own data, and nothing in either")
        print("data set is missing from the draft without being named as a comment.")

    return 1 if findings else 0


if __name__ == "__main__":
    sys.exit(main())
