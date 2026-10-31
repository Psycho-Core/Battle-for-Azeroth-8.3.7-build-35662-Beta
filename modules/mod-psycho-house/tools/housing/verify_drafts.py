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
    """Return (table, [column...], [tuple...]) for every INSERT in the file."""
    out = []
    for match in re.finditer(
            r"INSERT INTO `(\w+)` \(([^)]*)\) VALUES\s*(.*?);", text, re.S):
        table = match.group(1)
        columns = [c.strip().strip("`") for c in match.group(2).split(",")]
        rows = split_tuples(match.group(3))
        out.append((table, columns, rows))
    return out


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
            if name in ("01_schema.sql", "02_world_content.sql") or not name.endswith(".sql"):
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

    # both drafts ship the same core, so their C++ headers must agree
    a = sorted(os.listdir(os.path.join(DRAFTS, "midnight-port", "src", "game", "Housing")))
    b = sorted(os.listdir(os.path.join(DRAFTS, "bfa-native", "src", "game", "Housing")))
    if a != b:
        fail("drafts", "the two src/game/Housing trees differ: {}".format(
            set(a) ^ set(b)))

    print("\n{} finding(s)".format(len(findings)))
    for line in findings:
        print("  " + line)
    return 1 if findings else 0


if __name__ == "__main__":
    sys.exit(main())
