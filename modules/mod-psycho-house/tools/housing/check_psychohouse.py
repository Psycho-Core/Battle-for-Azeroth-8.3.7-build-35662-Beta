#!/usr/bin/env python3
"""Checks on the mod-psychohouse draft that the other tools do not cover.

    python3 tools/housing/check_psychohouse.py [--repo /path/to/source]

`check_cpp.py --src drafts/mod-psychohouse/src --sql drafts/mod-psychohouse/sql` covers the
C++ side of the draft. This covers the SQL and the catalogue:

  tables    every column an INSERT / REPLACE / UPDATE writes is a column of the table it
            names, and for the three build tables (creature_template, creature_template_model,
            gameobject_template) the column list comes from this build's own loader queries
            read out of the source, not from memory
  rows      every multi-row INSERT has the same number of values as the column list, and
            every value column lines up
  catalogue every facility row has a kind this build knows, a faction in 0..2, a price, an
            is_object flag that matches its kind, and an entry that is either 0 (unbound) or
            positive
  bindings  every facility left at entry 0 has a resolver statement in 03_bind_facilities.sql
            keyed to its id, and every resolver targets a facility that exists
  phase     the phase range in the header is inside the uint16 phase space, does not collide
            with the neighbourhood module's range, and has room for the number of houses the
            documentation claims
  conf      every key the C++ reads is a key the .conf.dist ships, and the reverse

No database and no compiler are involved. Anything that needs either is listed in the
draft's README instead of being claimed here.
"""

import argparse
import os
import re
import sys

HERE = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
DRAFT = os.path.join(HERE, "drafts", "mod-psychohouse")

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import build_schema  # noqa: E402  - shares the loader-query parsers with check_sql_columns

findings = []


def fail(where, what):
    findings.append("{}: {}".format(where, what))


def read(path):
    with open(path, encoding="utf-8") as handle:
        return handle.read()


def sql_files():
    out = []
    for base, _dirs, files in os.walk(os.path.join(DRAFT, "sql")):
        for name in sorted(files):
            if name.endswith(".sql"):
                out.append(os.path.join(base, name))
    return out


def strip_comments(text):
    text = re.sub(r"/\*.*?\*/", " ", text, flags=re.S)
    return re.sub(r"^\s*--.*$", " ", text, flags=re.M)


def split_top(text, sep=","):
    """Split on a separator that is not inside quotes, parentheses or backticks."""
    parts, depth, quote, start = [], 0, None, 0
    for index, char in enumerate(text):
        if quote:
            if char == quote:
                quote = None
            continue
        if char in "'\"`":
            quote = char
        elif char == "(":
            depth += 1
        elif char == ")":
            depth -= 1
        elif char == sep and depth == 0:
            parts.append(text[start:index])
            start = index + 1
    parts.append(text[start:])
    return [part.strip() for part in parts if part.strip()]


def created_tables(text):
    tables = {}
    for match in re.finditer(r"CREATE TABLE (?:IF NOT EXISTS )?`(\w+)` \((.*?)\n\)", text, re.S):
        columns = []
        for line in match.group(2).splitlines():
            line = line.strip()
            if line.startswith("`"):
                columns.append(line.split()[0].strip("`"))
        tables[match.group(1)] = columns
    return tables


def build_vocabulary(repo):
    """Columns of the three build tables, taken from this build's own loader queries.

    Parsed in build_schema.py, which check_sql_columns.py also imports, so the two tools
    cannot drift apart on what a column list is.
    """
    return build_schema.loader_vocabulary(repo)


def check_statements(path, label, tables, vocab):
    text = strip_comments(read(path))
    known = dict(tables)
    known.update(vocab)

    for match in re.finditer(r"(INSERT|REPLACE)\s+(?:IGNORE\s+)?INTO\s+`(\w+)`\s*\(([^)]*)\)\s*VALUES\s*"
                             r"(.*?);", text, re.S | re.I):
        table, column_text, values = match.group(2), match.group(3), match.group(4)
        columns = [c.strip().strip("`") for c in column_text.split(",")]
        if table not in known:
            fail(label, "unknown table `{}`".format(table))
            continue
        missing = [c for c in columns if c not in known[table]]
        for column in missing:
            fail(label, "`{}` has no column `{}`".format(table, column))

        rows = split_top(values)
        for number, row in enumerate(rows, 1):
            if not row.startswith("("):
                continue
            inner = row.strip()
            if not inner.endswith(")"):
                fail(label, "{} row {} is not closed".format(table, number))
                continue
            fields = split_top(inner[1:-1])
            if len(fields) != len(columns):
                fail(label, "{} row {}: {} values for {} columns".format(
                    table, number, len(fields), len(columns)))

    for match in re.finditer(r"UPDATE\s+`(\w+)`\s+SET\s+`(\w+)`\s*=", text, re.I):
        table, column = match.group(1), match.group(2)
        if table not in known:
            fail(label, "unknown table `{}` in UPDATE".format(table))
        elif column not in known[table]:
            fail(label, "UPDATE `{}` sets unknown column `{}`".format(table, column))


def facility_rows():
    path = os.path.join(DRAFT, "sql", "world", "01_schema.sql")
    text = strip_comments(read(path))
    rows = []
    match = re.search(r"INSERT INTO `psychohouse_facility`\s*\(([^)]*)\)\s*VALUES\s*(.*?);", text, re.S)
    if not match:
        fail("01_schema.sql", "no facility INSERT found")
        return rows

    columns = [c.strip().strip("`") for c in match.group(1).split(",")]
    for row in split_top(match.group(2)):
        fields = split_top(row.strip()[1:-1])
        if len(fields) != len(columns):
            fail("01_schema.sql", "facility row has {} values for {} columns".format(
                len(fields), len(columns)))
            continue
        rows.append(dict(zip(columns, [f.strip() for f in fields])))
    return rows


def check_catalogue(rows):
    kinds = {1: "bank", 2: "auctioneer", 3: "mailbox", 4: "innkeeper", 5: "stable",
             6: "spirit", 7: "forge", 8: "anvil", 9: "barber", 10: "vault",
             11: "portal", 12: "repair"}
    # kinds that are game objects rather than creatures
    object_kinds = {3, 7, 8, 9, 10, 11}

    ids = set()
    for row in rows:
        ident = int(row["id"])
        if ident in ids:
            fail("catalogue", "duplicate facility id {}".format(ident))
        ids.add(ident)

        kind = int(row["kind"])
        if kind not in kinds:
            fail("catalogue", "facility {} has unknown kind {}".format(ident, kind))

        if int(row["faction"]) not in (0, 1, 2):
            fail("catalogue", "facility {} has faction {}".format(ident, row["faction"]))

        if int(row["price"]) <= 0:
            fail("catalogue", "facility {} is free".format(ident))

        if int(row["entry"]) not in (0,) and int(row["entry"]) < 1:
            fail("catalogue", "facility {} has entry {}".format(ident, row["entry"]))

        expected = 1 if kind in object_kinds else 0
        if int(row["is_object"]) != expected:
            fail("catalogue", "facility {} kind {} should have is_object {}".format(
                ident, kind, expected))

        name = row["name"].strip("'")
        if not name:
            fail("catalogue", "facility {} has no name".format(ident))

    # both factions must be able to travel: one portal row per faction
    portals = [row for row in rows if int(row["kind"]) == 11]
    factions = {int(row["faction"]) for row in portals}
    if factions != {0, 1}:
        fail("catalogue", "portal rows cover factions {}, expected one per faction".format(
            sorted(factions)))


def check_bindings(rows):
    path = os.path.join(DRAFT, "sql", "world", "03_bind_facilities.sql")
    text = strip_comments(read(path))

    bound_ids = {int(m) for m in re.findall(r"WHERE `id` = (\d+) AND `entry` = 0", text)}
    unbound = {int(row["id"]) for row in rows if int(row["entry"]) == 0}

    for ident in sorted(unbound - bound_ids):
        fail("03_bind_facilities.sql", "facility {} ships unbound with no resolver".format(ident))

    for ident in sorted(bound_ids - {int(row["id"]) for row in rows}):
        fail("03_bind_facilities.sql", "resolver for facility {} which does not exist".format(ident))

    # the two module NPCs need models, or they are invisible
    for entry in ("900200", "900201"):
        if not re.search(r"SELECT\s+{}\s*,\s*0\s*,\s*m\.`CreatureDisplayID`".format(entry), text):
            fail("03_bind_facilities.sql", "no model resolver for creature {}".format(entry))


def check_phase(repo):
    header = read(os.path.join(DRAFT, "src", "mod_psychohouse.h"))
    base = re.search(r"kPhaseBase\s*=\s*(\d+)", header)
    last = re.search(r"kPhaseLast\s*=\s*(\d+)", header)
    if not base or not last:
        fail("mod_psychohouse.h", "phase range constants not found")
        return

    base_value, last_value = int(base.group(1)), int(last.group(1))
    if not (1 <= base_value <= last_value <= 65535):
        fail("mod_psychohouse.h", "phase range {}-{} is outside uint16".format(base_value, last_value))

    # the neighbourhood module owns these, read from its own header
    other = os.path.join(HERE, "src", "game", "Housing", "HousingDefines.h")
    if os.path.exists(other):
        text = read(other)
        plot = re.search(r"kPlotPhaseBase\s*=\s*(\d+)", text)
        interior = re.search(r"kInteriorPhaseBase\s*=\s*(\d+)", text)
        slots = re.search(r"kNeighborhoodSlotMax\s*=\s*(\d+)", text)
        per_slot = re.search(r"kPhasesPerSlot\s*=\s*(\d+)", text)
        if plot and interior and slots and per_slot:
            top = max(int(plot.group(1)), int(interior.group(1))) + int(slots.group(1)) * int(per_slot.group(1))
            if base_value <= top:
                fail("mod_psychohouse.h", "phase base {} collides with the neighbourhood range (ends {})".format(
                    base_value, top))

    readme = os.path.join(DRAFT, "README.md")
    if os.path.exists(readme):
        text = read(readme)
        claimed = re.search(r"(\d[\d ]*)\s+houses", text)
        if claimed:
            houses = int(claimed.group(1).replace(" ", ""))
            room = last_value - base_value + 1
            if houses != room:
                fail("README.md", "claims {} houses, the range holds {}".format(houses, room))


def check_readme_numbers(rows):
    """The README's two arithmetic claims, against the catalogue itself."""
    readme = os.path.join(DRAFT, "README.md")
    if not os.path.exists(readme):
        return

    text = read(readme)
    prices = [int(row["price"]) for row in rows]
    total = sum(prices) // 10000

    # a character can only reach one of the two faction portals
    portals = [int(row["price"]) for row in rows if int(row["kind"]) == 11]
    one_faction = (sum(prices) - (sum(portals) - (max(portals) if portals else 0))) // 10000

    for value, label in ((total, "every row"), (one_faction, "one faction")):
        if str(value) not in text:
            fail("README.md", "catalogue totals {} gold for {} but the README does not say so".format(
                value, label))

    creatures = os.path.join(DRAFT, "sql", "world", "02_creatures.sql")
    match = re.search(r"INSERT INTO `creature_template`\s*\(([^)]*)\)", read(creatures), re.S)
    if match:
        count = len([c for c in match.group(1).split(",") if c.strip()])
        if "{} columns".format(count) not in text:
            fail("README.md", "creature_template INSERT writes {} columns, the README says another number".format(
                count))


# Facts doc 13 sec.8 states about this build. Each is a (label, pattern, present) triple: the
# label is how the document names it, the pattern is what to look for in the source tree, and
# present says whether the document claims it is there. If the build gains or loses one of
# these, the document is stale and this check says so.
BUILD_CLAIMS = [
    ("PhasingHandler::AddPhase", "PhasingHandler::AddPhase",
     r"void PhasingHandler::AddPhase", True),
    ("SetPhaseMask", "SetPhaseMask",
     r"^[^/\n]*SetPhaseMask", False),
    ("GetPhaseByAuras", "GetPhaseByAuras", r"GetPhaseByAuras", False),
    ("DataMap::Base", "DataMap::Base", r"class DataMap", False),
    ("GlobalScript", "GlobalScript",
     r"class TC_GAME_API GlobalScript", False),
    ("OnPlayerBeforeTeleport", "OnPlayerBeforeTeleport",
     r"OnPlayerBeforeTeleport", False),
    ("OnUpdateZone", "OnUpdateZone", r"void OnUpdateZone\(Player", True),
    ("Map::SummonCreature", "Map::SummonCreature",
     r"SummonCreature\(uint32 entry, Position const", True),
    ("GameObject::CreateGameObject", "GameObject::CreateGameObject",
     r"static GameObject. CreateGameObject", True),
    ("Map::AddToMap", "Map::AddToMap", r"bool AddToMap\(T .\)", True),
    ("Creature::DespawnOrUnsummon", "Creature::DespawnOrUnsummon",
     r"void DespawnOrUnsummon\(uint32", True),
    ("SetRestState(0)", "SetRestState", r"void SetRestState\(uint32", False),
    ("GetOption", "a templated config accessor", r"GetOption<", False),
    ("SaveToDB with difficulties", "SaveToDB(mapid, std::vector<Difficulty> const&)",
     r"void SaveToDB\(uint32 mapid, std::vector<Difficulty> const& spawnDifficulties\)", True),
]


def check_build_claims(repo):
    doc = os.path.join(HERE, "13-mod-psychohouse-design.md")
    if not os.path.exists(doc):
        return

    text = read(doc)
    if "What this build gives a house module" not in text:
        fail("doc 13", "the capability table is gone; the claims below have no home")
        return

    corpus = []
    for base, _dirs, files in os.walk(os.path.join(repo, "src")):
        for name in files:
            if not name.endswith((".h", ".cpp")):
                continue
            with open(os.path.join(base, name), encoding="utf-8", errors="replace") as handle:
                corpus.append(handle.read())
    blob = "\n".join(corpus)
    # A commented-out call is not a capability. Strip block comments and whole-line
    # line comments so that only live code is searched.
    blob = re.sub(r"/\*.*?\*/", "", blob, flags=re.S)
    blob = "\n".join(line for line in blob.split("\n") if not line.lstrip().startswith("//"))

    for label, doc_text, pattern, expected in BUILD_CLAIMS:
        if doc_text not in text:
            fail("doc 13 sec.8", "`{}` is checked here but not named in the table".format(label))
            continue
        found = re.search(pattern, blob, re.M) is not None
        if found != expected:
            fail("doc 13 sec.8", "`{}` is {} in this tree, the document says {}".format(
                label, "present" if found else "absent", "present" if expected else "absent"))


def check_conf():
    dist = os.path.join(DRAFT, "conf", "mod_psychohouse.conf.dist")
    if not os.path.exists(dist):
        fail("conf", "mod_psychohouse.conf.dist is missing")
        return

    shipped = set(re.findall(r"^(PsychoHouse\.[\w.]+)\s*=", read(dist), re.M))

    read_keys = set()
    for base, _dirs, files in os.walk(os.path.join(DRAFT, "src")):
        for name in sorted(files):
            if not name.endswith(".cpp"):
                continue
            text = read(os.path.join(base, name))
            read_keys.update(re.findall(r"Get(?:Bool|Int|Float)Default\(\"([\w.]+)\"", text))

    for key in sorted(read_keys - shipped):
        fail("conf", "code reads `{}` but the .conf.dist does not ship it".format(key))
    for key in sorted(shipped - read_keys):
        fail("conf", "the .conf.dist ships `{}` but nothing reads it".format(key))


def main():
    global DRAFT
    parser = argparse.ArgumentParser()
    parser.add_argument("--repo", default=os.path.dirname(os.path.dirname(HERE)),
                        help="source checkout to read the build's loader queries from")
    parser.add_argument("--draft", default=DRAFT,
                        help="draft directory to check, default: drafts/mod-psychohouse")
    args = parser.parse_args()
    DRAFT = os.path.abspath(args.draft)

    if not os.path.isdir(DRAFT):
        print("draft not found at " + DRAFT)
        return 1

    texts = {path: strip_comments(read(path)) for path in sql_files()}
    tables = {}
    for text in texts.values():
        tables.update(created_tables(text))

    vocab = build_vocabulary(args.repo)
    for name in ("creature_template", "creature_template_model", "gameobject_template"):
        if name not in vocab:
            fail("vocabulary", "could not read the column list for `{}` from the source".format(name))

    for path, text in sorted(texts.items()):
        check_statements(path, os.path.relpath(path, DRAFT), tables, vocab)

    rows = facility_rows()
    check_catalogue(rows)
    check_bindings(rows)
    check_phase(args.repo)
    check_readme_numbers(rows)
    check_build_claims(args.repo)
    check_conf()

    print("{} SQL files, {} tables, {} facilities, {} build tables cross-checked".format(
        len(texts), len(tables), len(rows), len(vocab)))
    print("phase range and conf keys cross-checked against the code")
    print("\n{} finding(s)".format(len(findings)))
    for line in findings:
        print("  " + line)
    return 1 if findings else 0


if __name__ == "__main__":
    sys.exit(main())
