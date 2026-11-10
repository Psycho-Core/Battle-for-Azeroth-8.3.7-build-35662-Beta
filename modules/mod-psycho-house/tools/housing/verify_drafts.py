#!/usr/bin/env python3
"""Check the two draft modules against the schema they are written for.

    python3 tools/housing/verify_drafts.py

audit.py looks at the documents and the data files. This looks at the generated SQL in
drafts/: does every INSERT match its column list, does every foreign key point at a row
that exists in the same file set, is every game object entry in the source index, is every
price a number. It parses the SQL as text - no MySQL, no server - so it catches typos and
drift, not whether the server would accept the statement.
"""

import csv
import os
import re
import sys

HERE = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
DRAFTS = os.path.join(HERE, "drafts")
INDEX = os.path.join(HERE, "generated", "source-asset-index.csv")

findings = []
checks = 0


def fail(where, what):
    findings.append("{}: {}".format(where, what))


def read(path):
    with open(path, encoding="utf-8") as handle:
        return handle.read()


def statements(text):
    """Return (table, [column...], [tuple...]) for every INSERT in the file.

    Comment lines are dropped first: several files carry example statements in their
    header comments, and a parser that reads those as real SQL reports nonsense.
    """
    text = "\n".join(line for line in text.splitlines() if not line.lstrip().startswith("--"))
    out = []
    for match in re.finditer(
            r"INSERT\s+INTO\s+`(\w+)`\s*\(([^)]*)\)\s*VALUES\s*", text, re.S):
        table = match.group(1)
        columns = [c.strip().strip("`") for c in match.group(2).split(",")]
        rows = split_tuples(statement_body(text[match.end():]))
        out.append((table, columns, rows))
    return out


def statement_body(tail):
    """Everything up to the first semicolon that is not inside a string literal.

    A semicolon inside a row's comment text used to cut the statement short, which made the
    plot table look like it had 13 rows when it has 55.
    """
    out = []
    quote = False
    for char in tail:
        if quote:
            out.append(char)
            if char == "'":
                quote = False
            continue
        if char == "'":
            quote = True
            out.append(char)
        elif char == ";":
            break
        else:
            out.append(char)
    return "".join(out)


def split_tuples(body):
    rows, depth, current, quote = [], 0, [], False
    for char in body:
        if quote:
            current.append(char)
            if char == "'":
                quote = False
            continue
        if char == "'":
            quote = True
            current.append(char)
        elif char == "(":
            depth += 1
            if depth == 1:
                current = []
                continue
            current.append(char)
        elif char == ")":
            depth -= 1
            if depth == 0:
                rows.append("".join(current).strip())
                continue
            current.append(char)
        elif depth:
            current.append(char)
    return [split_fields(r) for r in rows if r.strip()]


def split_fields(row):
    fields, current, quote = [], [], False
    for char in row:
        if quote:
            current.append(char)
            if char == "'":
                quote = False
            continue
        if char == "'":
            quote = True
            current.append(char)
        elif char == ",":
            fields.append("".join(current).strip())
            current = []
        else:
            current.append(char)
    fields.append("".join(current).strip())
    return fields


def value(field):
    if field.startswith("'"):
        return field[1:-1].replace("''", "'")
    if field.upper() == "NULL":
        return None
    try:
        return int(field.split(".")[0])
    except ValueError:
        return field


def check_file(path, schema_columns, go_entries, expect):
    label = os.path.relpath(path, HERE)
    text = read(path)
    checks_run = 0

    if any(ord(char) > 127 for char in text):
        fail(label, "non-ASCII byte")

    parsed = statements(text)
    if not parsed:
        fail(label, "no INSERT statement found")
        return 0, 0

    total = 0
    for table, columns, rows in parsed:
        checks_run += 1
        known = schema_columns.get(table)
        if known is None:
            # A table the build owns rather than the module (quest_template and friends).
            # Its columns are checked against the build's own SQL vocabulary by
            # check_sql_columns.py; here only the value count can be checked.
            print("{:<52} {:>5} rows  {:>2} checks   ({})".format(
                "  " + table, len(rows), 0, "build table"))
        else:
            unknown = [c for c in columns if c not in known]
            if unknown:
                fail(label, "{} has columns the schema does not: {}".format(table, unknown))
        for number, row in enumerate(rows, 1):
            total += 1
            if len(row) != len(columns):
                fail(label, "{} row {}: {} values for {} columns".format(
                    table, number, len(row), len(columns)))
                continue
            record = {c: value(f) for c, f in zip(columns, row)}
            if table == "housing_decor":
                entry = record.get("gameobject_entry")
                if not isinstance(entry, int) or entry <= 0:
                    fail(label, "decor {} has gameobject_entry {!r}".format(
                        record.get("id"), entry))
                elif go_entries and entry not in go_entries:
                    fail(label, "decor {} entry {} is not in the source index".format(
                        record.get("id"), entry))
                if not record.get("name"):
                    fail(label, "decor {} has no name".format(record.get("id")))
            if table == "housing_vendor_stock":
                price = record.get("price")
                if not isinstance(price, int) or price <= 0:
                    fail(label, "stock row {} has price {!r}".format(number, price))
    print("{:<52} {:>5} rows  {:>2} checks".format(label, total, checks_run))
    return total, checks_run


def check_sql_against_own_schema(root, label_root):
    """Every INSERT into a table this tree defines, with the right number of values.

    Used for the module's own SQL and for draft C, which are written independently of the
    two content drafts and therefore cannot go through the loop above.
    """
    schema = {}
    files = []
    for base, _dirs, names in os.walk(root):
        for name in sorted(names):
            if not name.endswith(".sql"):
                continue
            path = os.path.join(base, name)
            files.append(path)
            text = read(path)
            for match in re.finditer(r"CREATE TABLE (?:IF NOT EXISTS )?`(\w+)` \((.*?)\n\)",
                                     text, re.S):
                schema[match.group(1)] = [
                    l.strip().split()[0].strip("`") for l in match.group(2).splitlines()
                    if l.strip().startswith("`")]

    rows = 0
    for path in files:
        label = os.path.join(label_root, os.path.relpath(path, root))
        if any(ord(char) > 127 for char in read(path)):
            fail(label, "non-ASCII byte")
        for table, columns, tuples in statements(read(path)):
            known = schema.get(table)
            if known is None:
                # A table the build owns. Its columns are checked against the build's own
                # loader queries by check_psychohouse.py; the value count is all this can see.
                print("{:<52} {:>5} rows  {:>2} checks   ({})".format(
                    "  " + table, len(tuples), 0, "build table"))
            else:
                unknown = [c for c in columns if c not in known]
                if unknown:
                    fail(label, "{} has columns the schema does not: {}".format(table, unknown))
            for number, row in enumerate(tuples, 1):
                rows += 1
                if len(row) != len(columns):
                    fail(label, "{} row {}: {} values for {} columns".format(
                        table, number, len(row), len(columns)))
    return rows


def check_dead_columns(root):
    """Every column of every table this module creates has a reader somewhere.

    A column that nothing selects and nothing updates is a column an operator will fill in
    and then wonder why nothing happens. The reference set is the module's own C++ plus the
    bind files: a column whose only job is to carry a search name to `tools/sql/*.sql` counts
    as used, because that is where its reader is.

    Matching is on the backticked name, so `armillary` in a comment does not count as a
    reader for `` `armillary` `` - the query or the UPDATE has to be there.
    """
    schema = {}
    for base, _dirs, names in os.walk(root):
        for name in sorted(names):
            if not name.endswith(".sql"):
                continue
            text = read(os.path.join(base, name))
            for match in re.finditer(r"CREATE TABLE (?:IF NOT EXISTS )?`(\w+)` \((.*?)\n\)",
                                     text, re.S):
                columns = [l.strip().split()[0].strip("`") for l in match.group(2).splitlines()
                           if l.strip().startswith("`")]
                schema[match.group(1)] = [c for c in columns
                                          if c not in ("PRIMARY", "KEY", "UNIQUE", "INDEX")]

    references = ""
    module = os.path.dirname(root)
    for base, _dirs, names in os.walk(os.path.join(module, "src")):
        for name in sorted(names):
            if name.endswith((".cpp", ".h")):
                references += read(os.path.join(base, name))
    for name in sorted(os.listdir(os.path.join(module, "tools", "sql"))):
        if name.endswith(".sql"):
            references += read(os.path.join(module, "tools", "sql", name))

    checks = 0
    for table, columns in sorted(schema.items()):
        for column in columns:
            checks += 1
            if "`{}`".format(column) not in references:
                fail("sql", "{} .{} is created and never read or written".format(table, column))
    print("{:<52} {:>5} rows  {:>2} checks   (columns with a reader)".format(
        "module sql columns", checks, checks))
    return checks


DECOR_FLAG_PET_BED = 0x010


def check_creatures(base, draft, tables, findings_before=0):
    """The pieces that own a creature, against the pieces the draft ships.

    `housing_decor_creature` is what makes a pet bed a bed: `AssignPet` refuses a placement
    whose decor row is not flagged, and `SpawnDecorCreature` looks the row up by decor id. The
    two have to agree in both directions, and a row pointing at a decor id the draft does not
    ship is a bed on a piece that does not exist - which is exactly what draft B shipped until
    this check existed, as a single placeholder row for decor 0.
    """
    checks = 0
    flags = {}
    beds = {}
    hunts = []

    for name in sorted(os.listdir(base)):
        if not name.endswith(".sql"):
            continue

        for table, columns, tuples in statements(read(os.path.join(base, name))):
            for tuple_row in tuples:
                record = {c: value(f) for c, f in zip(columns, tuple_row)}

                if table == "housing_decor":
                    flags[record.get("id")] = record.get("flags") or 0
                elif table == "housing_decor_creature":
                    beds[record.get("decor_id")] = record.get("kind")
                elif table == "housing_hunt":
                    hunts.append((record.get("id"), record.get("reward_decor")))

    checks += 1
    for decor_id in sorted(beds):
        if decor_id not in flags:
            fail(draft, "housing_decor_creature row for decor {} does not exist in the "
                        "catalogue".format(decor_id))

    checks += 1
    for decor_id, kind in sorted(flags.items()):
        if kind & DECOR_FLAG_PET_BED and decor_id not in beds:
            fail(draft, "decor {} is flagged as a pet bed and has no housing_decor_creature "
                        "row".format(decor_id))

    checks += 1
    for decor_id in sorted(beds):
        if decor_id in flags and not flags[decor_id] & DECOR_FLAG_PET_BED:
            fail(draft, "decor {} owns a creature and is not flagged as a pet bed".format(
                decor_id))

    # Both drafts say pet beds are in (doc 10 sec.4), so one with none is a feature the
    # documents promise and the data cannot deliver.
    checks += 1
    if not beds:
        fail(draft, "no pet bed at all: doc 10 says both drafts ship them")

    checks += 1
    for hunt_id, reward in hunts:
        if reward and reward not in flags:
            fail(draft, "hunt {} rewards decor {} which the draft does not ship".format(
                hunt_id, reward))

    return checks


def draft_stats(draft):
    """Counts doc 10 states, taken from the draft itself."""
    base = os.path.join(DRAFTS, draft, "sql", "world")
    stats = {"decor": 0, "stock": 0, "vendors": 0, "horde": 0, "alliance": 0,
             "gated": 0, "items": set(), "item_rows": 0, "beds": 0, "both": 0}

    for name in sorted(os.listdir(base)):
        # The hunt file carries the ten reward pieces, which doc 10 counts on its own line
        # rather than in the catalogue. Counting them here would make the catalogue look ten
        # pieces larger than the number the document states.
        if not name.endswith(".sql") or name == "08_hunts.sql":
            continue

        for table, columns, tuples in statements(read(os.path.join(base, name))):
            for tuple_row in tuples:
                record = {c: value(f) for c, f in zip(columns, tuple_row)}

                if table == "housing_vendor":
                    stats["vendors"] += 1
                elif table == "housing_vendor_stock":
                    stats["stock"] += 1
                    if record.get("milestone"):
                        stats["gated"] += 1
                elif table == "housing_decor":
                    stats["decor"] += 1
                    if str(record.get("faction_mask")) == "1":
                        stats["alliance"] += 1
                    elif str(record.get("faction_mask")) == "2":
                        stats["horde"] += 1
                    elif str(record.get("faction_mask")) == "3":
                        stats["both"] += 1
                    if record.get("item_entry"):
                        stats["items"].add(record["item_entry"])
                        stats["item_rows"] += 1
                elif table == "housing_decor_creature":
                    stats["beds"] += 1

    return stats


def native_rank():
    """Entry to position, over every decor-shaped game object in the source index."""
    path = os.path.join(HERE, "generated", "source-asset-index.csv")
    entries = set()

    for row in csv.DictReader(open(path, newline="", encoding="utf-8")):
        if row["kind"] == "gameobject" and row["decor"] == "yes":
            entries.add(int(row["entry"]))

    return {entry: i for i, entry in enumerate(sorted(entries))}


NATIVE_RANK = native_rank()


def check_ids(base, draft, band, findings_before=0):
    """Decor ids: unique, inside the draft's band, and the same piece never given twice.

    Draft B's ids used to be the row's position in the filtered harvest, so adding one word to
    the reject tuple renumbered every piece after the first rejected one - and a piece's id is
    what a placement, a stock row and a saved blueprint point at. They are the entry's position
    in the whole decor-shaped set now, which means gaps in the band and no shift.
    """
    seen = {}
    checks = 1

    for name in sorted(os.listdir(base)):
        if not name.endswith(".sql"):
            continue

        for table, columns, tuples in statements(read(os.path.join(base, name))):
            if table != "housing_decor":
                continue

            for tuple_row in tuples:
                record = {c: value(f) for c, f in zip(columns, tuple_row)}
                decor_id = record.get("id")

                if decor_id in seen:
                    fail(draft, "decor {} appears twice: {} and {}".format(
                        decor_id, seen[decor_id], name))
                seen[decor_id] = name

                if not band[0] <= decor_id <= band[1]:
                    fail(draft, "decor {} is outside the {}-{} band".format(
                        decor_id, band[0], band[1]))

                if draft == "bfa-native" and decor_id < 2900:
                    expected = 2000 + NATIVE_RANK.get(record.get("gameobject_entry"))
                    if expected != decor_id:
                        fail(draft, "decor {} should be {} for game object {} - native ids "
                                    "are the entry's position in the whole decor set, not in "
                                    "the harvest".format(decor_id, expected,
                                                         record.get("gameobject_entry")))

    return checks


HORDE = 0x02
ALLIANCE = 0x01


def check_stock_side(base, draft, findings_before=0):
    """Every stock line against the mask on the piece it sells.

    A decor row's `faction_mask` is what `BuyDecor` and the shop listing test, so a row whose
    mask does not carry the buying side is a line on a vendor's shelf that nobody can buy.
    Draft A shipped 233 of them: a piece both neighbourhoods stock at the same price becomes
    one row, and the mask was taken from whichever hub was read first - so 50 pieces Faarden
    the Builder sells in Founder's Point were Horde-only, the four pet beds among them.
    """
    vendor_side = {}
    masks = {}
    bed_ids = set()
    stock = []

    for name in sorted(os.listdir(base)):
        if not name.endswith(".sql"):
            continue

        for table, columns, tuples in statements(read(os.path.join(base, name))):
            for tuple_row in tuples:
                record = {c: value(f) for c, f in zip(columns, tuple_row)}

                if table == "housing_vendor":
                    comment = record.get("comment") or ""
                    if "Horde" in comment:
                        vendor_side[record.get("id")] = HORDE
                    elif "Alliance" in comment:
                        vendor_side[record.get("id")] = ALLIANCE
                elif table == "housing_decor":
                    masks[record.get("id")] = record.get("faction_mask") or 0
                    if (record.get("flags") or 0) & DECOR_FLAG_PET_BED:
                        bed_ids.add(record.get("id"))
                elif table == "housing_vendor_stock":
                    stock.append((record.get("vendor_id"), record.get("decor_id")))

    checks = 0
    checks += 1
    for vendor_id, decor_id in stock:
        side = vendor_side.get(vendor_id)
        if side is None or decor_id not in masks:
            continue
        if not masks[decor_id] & side:
            fail(draft, "vendor {} stocks decor {} which is masked {} - no character on that "
                        "side can buy it".format(vendor_id, decor_id, masks[decor_id]))

    # Every side must be able to buy a pet bed, or the feature is one faction's.
    checks += 1
    for side, label in ((HORDE, "Horde"), (ALLIANCE, "Alliance")):
        if not any(mask & side for mask in
                   (masks[d] for v, d in stock if d in masks and d in bed_ids)):
            fail(draft, "no pet bed is buyable by {}".format(label))

    return checks


def check_documented_counts():
    """Doc 10's numbers against the two drafts.

    Doc 10 is the document that says what each draft ships, and every number in it was typed
    by hand: the decor count, the stock count, the vendor count, the faction split, how many
    stock rows an Endeavor gate holds back, how many retail item ids the rows carry and how
    many pet beds the progression file carries. Nothing compared them, because doc 10 was not
    on any checker's list - the side-by-side table still said 350 decor pieces when the draft
    shipped 336, and the pet-bed line said three when both drafts had shipped four.
    """
    doc = read(os.path.join(HERE, "10-draft-modules.md"))
    a = draft_stats("midnight-port")
    b = draft_stats("bfa-native")

    def stated(pattern, what):
        match = re.search(pattern, doc, re.S)
        if not match:
            fail("10-draft-modules.md", "no longer states {}".format(what))
            return None
        return int(match.group(1))

    checks = 0

    checks += 1
    value = stated(r"\| Vendors \| (\d+), each with the retail npc id", "the vendor count")
    if value is not None and value != a["vendors"]:
        fail("10-draft-modules.md", "says {} vendors in draft A, the SQL has {}".format(
            value, a["vendors"]))

    checks += 1
    value = stated(r"\| Decor rows \| (\d+), every one carrying", "draft A's decor count")
    if value is not None and value != a["decor"]:
        fail("10-draft-modules.md", "says {} decor rows in draft A, the SQL has {}".format(
            value, a["decor"]))

    checks += 1
    value = stated(r"\| Stock lines \| (\d+) \|", "draft A's stock count")
    if value is not None and value != a["stock"]:
        fail("10-draft-modules.md", "says {} stock lines in draft A, the SQL has {}".format(
            value, a["stock"]))

    checks += 1
    match = re.search(r"\| Faction split \| (\d+) pieces are sold only in Razorwind Shores "
                      r"\(Horde\) and (\d+) only in Founder's Point \(Alliance\); the other "
                      r"(\d+) are sold in both", doc)
    if not match:
        fail("10-draft-modules.md", "no longer states draft A's faction split")
    elif (int(match.group(1)) != a["horde"] or int(match.group(2)) != a["alliance"]
          or int(match.group(3)) != a["both"]):
        fail("10-draft-modules.md", "says {} Horde, {} Alliance and {} both, the SQL has {}, "
             "{} and {}".format(match.group(1), match.group(2), match.group(3), a["horde"],
                                a["alliance"], a["both"]))

    checks += 1
    match = re.search(r"\| Gated stock \| (\d+) stock lines on six vendors are locked behind "
                      r"Endeavor milestones at retail.*?(\d+) of them are shipped as gated "
                      r"rows", doc, re.S)
    if not match:
        fail("10-draft-modules.md", "no longer states how many stock rows are gated")
    elif int(match.group(2)) != a["gated"]:
        fail("10-draft-modules.md", "says {} milestone-gated stock rows in draft A, the SQL "
             "has {}".format(match.group(2), a["gated"]))

    checks += 1
    match = re.search(r"\| Retail item ids on those rows \| (\d+) rows carry one; between them "
                      r"they cover all (\d+) distinct ids", doc)
    if not match:
        fail("10-draft-modules.md", "no longer states how many rows carry a retail item id")
    elif int(match.group(1)) != a["item_rows"] or int(match.group(2)) != len(a["items"]):
        fail("10-draft-modules.md", "says {} rows carry {} ids, the SQL has {} rows and {} ids"
             .format(match.group(1), match.group(2), a["item_rows"], len(a["items"])))

    checks += 1
    match = re.search(r"\| Decor rows \| (\d+): (\d+) game objects that already exist", doc)
    if not match:
        fail("10-draft-modules.md", "no longer states draft B's decor count")
    elif int(match.group(1)) != b["decor"]:
        fail("10-draft-modules.md", "says {} decor rows in draft B, the SQL has {}".format(
            match.group(1), b["decor"]))
    elif int(match.group(2)) != b["decor"] - b["beds"]:
        fail("10-draft-modules.md", "says {} harvested decor rows in draft B, the SQL has {} "
             "besides the {}".format(match.group(2), b["decor"] - b["beds"], b["beds"]))

    checks += 1
    value = stated(r"\| Stock lines \| (\d+), every piece sold somewhere", "draft B's stock "
                                                                             "count")
    if value is not None and value != b["stock"]:
        fail("10-draft-modules.md", "says {} stock lines in draft B, the SQL has {}".format(
            value, b["stock"]))

    checks += 1
    match = re.search(r"\| Number of vendors \| \d+ \| (\d+) \|", doc)
    if not match:
        fail("10-draft-modules.md", "the side-by-side table no longer states draft B's vendors")
    elif int(match.group(1)) != b["vendors"]:
        fail("10-draft-modules.md", "the side-by-side table says {} vendors in draft B, the SQL "
             "has {}".format(match.group(1), b["vendors"]))

    checks += 1
    match = re.search(r"\| Number of decor pieces \| \d+, split \d+ Horde / \d+ Alliance \| "
                      r"(\d+), both factions \|", doc)
    if not match:
        fail("10-draft-modules.md", "the side-by-side table no longer states draft B's decor")
    elif int(match.group(1)) != b["decor"]:
        fail("10-draft-modules.md", "the side-by-side table says {} decor pieces in draft B, the "
             "SQL has {}".format(match.group(1), b["decor"]))

    checks += 1
    value = stated(r"(\d+) pet beds? ", "how many pet beds the progression file carries")
    if value is not None and (value != a["beds"] or value != b["beds"]):
        fail("10-draft-modules.md", "says {} pet beds, draft A has {} and draft B has {}".format(
            value, a["beds"], b["beds"]))

    print("{:<52} {:>2} checks".format("doc 10's numbers against the drafts", checks))
    return checks


def main():
    index = list(csv.DictReader(open(INDEX, newline="", encoding="utf-8")))
    go_entries = {int(r["entry"]) for r in index if r["kind"] == "gameobject" and r["entry"]}

    draft_results = {}
    for draft in ("midnight-port", "bfa-native"):
        base = os.path.join(DRAFTS, draft, "sql", "world")
        schema_text = read(os.path.join(base, "01_schema.sql"))
        schema_columns = {}
        for match in re.finditer(r"CREATE TABLE (?:IF NOT EXISTS )?`(\w+)` \((.*?)\n\)",
                                 schema_text, re.S):
            table = match.group(1)
            columns = [l.strip().split()[0].strip("`") for l in match.group(2).splitlines()
                       if l.strip().startswith("`")]
            schema_columns[table] = columns
        if not schema_columns:
            fail(draft, "no CREATE TABLE found in 01_schema.sql")

        tables = {"housing_decor": set(), "housing_vendor": set()}
        rows_total = 0
        for name in sorted(os.listdir(base)):
            if not name.endswith(".sql"):
                continue
            rows, run = check_file(os.path.join(base, name), schema_columns, go_entries, tables)
            rows_total += rows
            for table, columns, tuples in statements(read(os.path.join(base, name))):
                for tuple_row in tuples:
                    record = {c: value(f) for c, f in zip(columns, tuple_row)}
                    if table == "housing_vendor":
                        tables["housing_vendor"].add(record.get("id"))
                    if table == "housing_decor":
                        tables["housing_decor"].add(record.get("id"))
        draft_results[draft] = rows_total

        # foreign keys across the file set
        for name in sorted(os.listdir(base)):
            if not name.endswith(".sql"):
                continue
            for table, columns, tuples in statements(read(os.path.join(base, name))):
                if table != "housing_vendor_stock":
                    continue
                for tuple_row in tuples:
                    record = {c: value(f) for c, f in zip(columns, tuple_row)}
                    if "vendor_id" in record and record["vendor_id"] not in tables["housing_vendor"]:
                        fail(name, "stock row points at missing vendor {}".format(record["vendor_id"]))
                    if "decor_id" in record and record["decor_id"] not in tables["housing_decor"]:
                        fail(name, "stock row points at missing decor {}".format(record["decor_id"]))

        checks_run = 1
        for need in ("housing_decor", "housing_vendor"):
            checks_run += 1
            if not tables[need]:
                fail(draft, "no rows in " + need)

        checks_run += check_creatures(base, draft, tables, findings_before=len(findings))
        checks_run += check_stock_side(base, draft, findings_before=len(findings))
        checks_run += check_ids(base, draft,
                                (1000, 1999) if draft == "midnight-port" else (2000, 2999))

    # every item id the town centre catalogue knows must reach draft A
    items_csv = os.path.join(HERE, "data", "town-center-items.csv")
    catalogue = {r["item_id"].strip() for r in csv.DictReader(open(items_csv, newline="",
                                                                 encoding="utf-8"))
                 if r["item_id"].strip()}
    decor_sql = read(os.path.join(DRAFTS, "midnight-port", "sql", "world",
                                  "04_decor_retail.sql"))
    carried = {v[3] for v in re.findall(
        r"^\((\d+), '((?:[^']|'')*)', (\d+), (\d+),", decor_sql, re.M)} - {"0"}
    if catalogue - carried:
        fail("midnight-port", "{} catalogue item ids never reach housing_decor, e.g. {}".format(
            len(catalogue - carried), sorted(catalogue - carried)[:5]))

    # the module's own SQL (not a draft) against its own CREATE TABLEs
    module_rows = check_sql_against_own_schema(os.path.join(HERE, "sql"), "sql/")
    check_dead_columns(os.path.join(HERE, "sql"))
    print("{:<52} {:>5} rows".format("sql/world + sql/characters", module_rows))

    # draft C keeps its own SQL in its own shape, so it is checked the same way
    draft_c = check_sql_against_own_schema(
        os.path.join(DRAFTS, "mod-psychohouse", "sql"), "drafts/mod-psychohouse/sql")
    print("{:<52} {:>5} rows".format("drafts/mod-psychohouse/sql", draft_c))
    if not draft_c:
        fail("mod-psychohouse", "its SQL inserts no rows at all")

    # every config key the module reads is a key both drafts document, and the other way
    # round: a key in the file that nothing reads is a key an operator will set for nothing.
    read_keys = set()
    src_root = os.path.join(HERE, "src")
    for folder, _, names in os.walk(src_root):
        for name in names:
            if not name.endswith((".cpp", ".h")):
                continue
            for match in re.finditer(r'Default\("([^"]+)"', read(os.path.join(folder, name))):
                read_keys.add(match.group(1))

    for draft in ("midnight-port", "bfa-native"):
        dist = os.path.join(DRAFTS, draft, "conf", "mod-psycho-house.conf.dist")
        text = read(dist)
        shipped = set(re.findall(r"^(Housing\.[A-Za-z.]+)\s*=", text, re.M))

        for key in sorted(read_keys - shipped):
            fail(draft + "/conf", "the C++ reads {} and the file never sets it".format(key))
        for key in sorted(shipped - read_keys):
            fail(draft + "/conf", "the file sets {} and no C++ reads it".format(key))

    print("{:<52} {:>5} keys".format("conf, both drafts", len(read_keys)))

    # both drafts ship the same core, so their C++ headers must agree
    a = sorted(os.listdir(os.path.join(DRAFTS, "midnight-port", "src", "game", "Housing")))
    b = sorted(os.listdir(os.path.join(DRAFTS, "bfa-native", "src", "game", "Housing")))
    if a != b:
        fail("drafts", "the two src/game/Housing trees differ: {}".format(
            set(a) ^ set(b)))

    check_documented_counts()

    print("\n{} finding(s)".format(len(findings)))
    for line in findings:
        print("  " + line)
    return 1 if findings else 0


if __name__ == "__main__":
    sys.exit(main())
