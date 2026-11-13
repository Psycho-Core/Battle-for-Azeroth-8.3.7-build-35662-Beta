#!/usr/bin/env python3
"""Run a draft's install order as one transaction, in memory, and report what it loses.

Every other checker reads a draft file by file. An operator does not: the README gives an
order, the files run in it, and the database that comes out is the sum of what every file
did to every table. Three things only exist at that level, and none of them is visible in a
single file:

  * a statement that runs before the table it writes has been created (error 1146),
  * a DELETE in a later file that removes rows an earlier file inserted and nothing puts
    back - a silent data loss that a file-by-file read cannot see,
  * two files inserting the same primary key (error 1062).

    python3 tools/housing/simulate_install.py                     both drafts
    python3 tools/housing/simulate_install.py --draft midnight-port

The simulator understands the statement shapes the drafts actually use - CREATE TABLE,
DELETE with a simple predicate, INSERT ... VALUES - and it counts every statement it does
not understand instead of pretending to have run it. Those are printed by kind, because a
simulation that quietly skips half of its input is worse than no simulation.

Nothing here talks to a database. This is the install order executed as a model, which is
the part of "untested" that can be tested without a server.
"""

import argparse
import glob
import json
import os
import re
import sys

try:
    import sqlglot
    from sqlglot import exp
except ImportError:                                     # pragma: no cover - reported below
    sqlglot = None

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
DRAFTS = os.path.join(ROOT, "drafts")
VOCAB = os.path.join(ROOT, "generated", "sql-columns.json")


def build_tables():
    """Tables the realm already has and a draft writes to.

    The drafts ship rows for `quest_template`, its objectives and addon, and the bind
    statements fill `creature_template` and `gameobject_template`. Those tables are created
    by the build, never by a draft, so a statement against them is not error 1146. The
    vocabulary in `generated/sql-columns.json` is the build's own schema as this folder
    harvested it, which makes it the list to check a foreign table name against - a table
    that is neither created here nor in that vocabulary is a typo, not a realm table.
    """
    try:
        with open(VOCAB, encoding="utf-8") as handle:
            return set(json.load(handle))
    except (OSError, ValueError):
        return set()


REALM_TABLES = build_tables()

findings = []


def fail(label, message):
    findings.append("{}: {}".format(label, message))


# --------------------------------------------------------------------------- the install order

def install_order(draft):
    """The SQL block in a draft's README, in the order it is printed.

    The README is the install document: it is the only place that says which file comes
    before which, so the simulation runs what it says rather than what the file names imply.
    A README that stops printing the block fails here instead of being guessed at.
    """
    path = os.path.join(DRAFTS, draft, "README.md")
    text = open(path, encoding="utf-8").read()
    blocks = re.findall(r"```[a-z]*\n(.*?)```", text, re.S)

    order = []
    for block in blocks:
        if "sql/world/01_schema.sql" not in block:
            continue

        for line in block.splitlines():
            # `path   (a note)` - the note is commentary, the first word is the instruction.
            words = line.strip().split()
            if words and words[0].endswith(".sql"):
                order.append(words[0])

    if not order:
        fail(draft, "the README prints no SQL install order")

    return order


def resolve(draft, relative):
    candidate = os.path.join(DRAFTS, draft, relative)
    if os.path.exists(candidate):
        return candidate

    fail(draft, "the install order names {}, which does not exist".format(relative))
    return None


# --------------------------------------------------------------------------- the database model

class Database:
    def __init__(self):
        self.tables = {}         # name -> {"columns": [...], "pk": [...], "rows": {key: origin}}
        self.assumed = set()
        self.unsimulated = {}
        self.insertions = []     # (table, key, file) in execution order
        self.deletions = []      # (table, key, file) in execution order

    def create(self, name, columns, primary_key, path, auto=None):
        existing = self.tables.get(name)
        if existing:
            return existing

        table = {"columns": columns, "pk": primary_key, "rows": {}, "file": path,
                 "auto": auto, "next": 1}
        self.tables[name] = table
        return table

    def key_for(self, table, row):
        primary_key = table["pk"]

        # An AUTO_INCREMENT key that the INSERT does not supply is the server's to assign.
        # Modelling it as NULL would make every such row a duplicate of the last one.
        if len(primary_key) == 1 and table.get("auto") == primary_key[0] \
                and row.get(primary_key[0]) is None:
            row = dict(row)
            row[primary_key[0]] = table["next"]
            table["next"] += 1

        if not primary_key:
            # No primary key: fall back to the whole row, which is what a table without one
            # can distinguish anyway.
            return tuple(sorted(row.items()))
        return tuple(row.get(column) for column in primary_key)

    def insert(self, table_name, row, path):
        table = self.tables[table_name]
        key = self.key_for(table, row)

        if key in table["rows"]:
            fail(path, "inserts a duplicate primary key into {}: {} (also inserted by {})"
                 .format(table_name, key, table["rows"][key]))
            return

        table["rows"][key] = path
        self.insertions.append((table_name, key, path))

    def delete(self, table_name, row, path):
        table = self.tables[table_name]
        key = self.key_for(table, row)
        if key in table["rows"]:
            del table["rows"][key]
            self.deletions.append((table_name, key, path))


def columns_of(create):
    columns = []
    for definition in create.find_all(exp.ColumnDef):
        columns.append(definition.name)
    return columns


def primary_key_of(create):
    for constraint in create.find_all(exp.PrimaryKey):
        return [c.name for c in constraint.expressions]
    return []


def auto_increment_of(create):
    for definition in create.find_all(exp.ColumnDef):
        for constraint in definition.constraints or []:
            if "AUTO_INCREMENT" in constraint.sql().upper():
                return definition.name
    return None


def literal_value(node):
    if isinstance(node, exp.Literal):
        if node.is_string:
            return node.this

        text = str(node.this)
        try:
            return int(text)
        except ValueError:
            return float(text)

    if isinstance(node, exp.Neg):
        value = literal_value(node.this)
        return -value if isinstance(value, int) else value

    if isinstance(node, exp.Null):
        return None

    if isinstance(node, exp.Boolean):
        return 1 if node.this else 0

    return node.sql()


def predicate_matches(where, row):
    """The DELETE predicates the drafts use: BETWEEN, =, >=, <=, IN, joined by AND."""
    if where is None:
        return True

    # sqlglot wraps the condition of a DELETE in a Where node; the condition is what matters.
    if isinstance(where, exp.Where):
        return predicate_matches(where.this, row)

    if isinstance(where, exp.And):
        return predicate_matches(where.this, row) and predicate_matches(where.expression, row)

    if isinstance(where, exp.Or):
        return predicate_matches(where.this, row) or predicate_matches(where.expression, row)

    if isinstance(where, exp.Between):
        column = where.this.name
        low = literal_value(where.args["low"])
        high = literal_value(where.args["high"])
        value = row.get(column)
        return isinstance(value, int) and isinstance(low, int) and low <= value <= high

    if isinstance(where, exp.In):
        column = where.this.name
        return row.get(column) in [literal_value(v) for v in where.expressions]

    if isinstance(where, exp.EQ):
        return row.get(where.this.name) == literal_value(where.expression)

    if isinstance(where, exp.GTE):
        value = row.get(where.this.name)
        limit = literal_value(where.expression)
        return isinstance(value, int) and value >= limit

    if isinstance(where, exp.LTE):
        value = row.get(where.this.name)
        limit = literal_value(where.expression)
        return isinstance(value, int) and value <= limit

    # Anything else is a predicate this model cannot evaluate. It is counted rather than
    # treated as "true", because guessing would silently invent deletions.
    raise NotImplementedError(where.sql())


def apply_statements(statements, database, label):
    """The statement dispatch: insert, delete, create, and a count of everything else."""
    for statement in statements:
        if isinstance(statement, exp.Create):
            name = statement.this.this.name
            database.create(name, columns_of(statement), primary_key_of(statement), label,
                            auto_increment_of(statement))
            continue

        if isinstance(statement, exp.Delete):
            name = statement.this.name
            table = database.tables.get(name)
            if table is None:
                if name in REALM_TABLES:
                    database.assumed.add(name)
                    continue
                fail(label, "deletes from {}, which neither this draft nor the build's "
                            "schema defines (error 1146)".format(name))
                continue

            if isinstance(statement.this, exp.Subquery) or statement.args.get("using"):
                database.unsimulated["DELETE with a join"] = \
                    database.unsimulated.get("DELETE with a join", 0) + 1
                continue

            try:
                doomed = [key for key, _ in table["rows"].items()
                          if predicate_matches(statement.args.get("where"),
                                               dict(zip(table["pk"], key))
                                               if table["pk"] else {})]
            except NotImplementedError:
                database.unsimulated["DELETE with a predicate this model cannot read"] = \
                    database.unsimulated.get(
                        "DELETE with a predicate this model cannot read", 0) + 1
                continue

            for key in doomed:
                del table["rows"][key]
                database.deletions.append((name, key, label))
            continue

        if isinstance(statement, exp.Insert):
            name = statement.this.this.name if isinstance(statement.this, exp.Schema) \
                else statement.this.name
            table = database.tables.get(name)

            if table is None:
                if name in REALM_TABLES:
                    database.assumed.add(name)
                    continue
                fail(label, "inserts into {}, which neither this draft nor the build's "
                            "schema defines (error 1146)".format(name))
                continue

            if statement.expression is None or not isinstance(statement.expression,
                                                              exp.Values):
                database.unsimulated["INSERT ... SELECT (a bind statement)"] = \
                    database.unsimulated.get("INSERT ... SELECT (a bind statement)", 0) + 1
                continue

            columns = [c.name for c in statement.this.expressions] \
                if isinstance(statement.this, exp.Schema) else table["columns"]

            for tuple_row in statement.expression.expressions:
                values = [literal_value(v) for v in tuple_row.expressions]
                if len(values) != len(columns):
                    fail(label, "inserts {} values into {} column(s) of {}".format(
                        len(values), len(columns), name))
                    continue

                database.insert(name, dict(zip(columns, values)), label)
            continue

        if isinstance(statement, exp.Update):
            database.unsimulated["UPDATE (a bind statement)"] = \
                database.unsimulated.get("UPDATE (a bind statement)", 0) + 1
            continue

        kind = type(statement).__name__
        database.unsimulated[kind] = database.unsimulated.get(kind, 0) + 1



def parse_file(draft, relative):
    """The statements in one file of the install order, with its error reported once."""
    path = resolve(draft, relative)
    if not path:
        return None

    label = "{}/{}".format(draft, relative)
    text = open(path, encoding="utf-8").read()

    try:
        return label, [s for s in sqlglot.parse(text, read="mysql")
                       if s is not None and not isinstance(s, exp.Semicolon)]
    except Exception as exc:
        fail(label, "does not parse: {}".format(str(exc).splitlines()[0][:120]))
        return None


def run_one(draft, database, relative):
    """Execute one file of the order into the database."""
    parsed = parse_file(draft, relative)
    if not parsed:
        return False

    label, statements = parsed
    apply_statements(statements, database, label)
    return True


def snapshot(database):
    return {name: len(table["rows"]) for name, table in database.tables.items()}


def run(draft, database):
    parsed_files = 0

    for relative in install_order(draft):
        if run_one(draft, database, relative):
            parsed_files += 1

    return parsed_files


def rerun_each_file(draft, order):
    """Install the draft in its published order, then re-run each file on its own.

    Re-importing one file is the ordinary repair for a bad row, and the drafts are written to
    be re-runnable, so the second run of a file must leave the database where the first one
    did. It does not, when one file's DELETE covers ids another file owns: re-importing the
    catalogue of draft A removes the ten riddle-hunt reward pieces that `08_hunts.sql`
    inserted, and nothing puts them back until the hunts run again.
    """
    base = Database()
    run(draft, base)
    before = snapshot(base)

    losses = []

    for relative in order:
        trial = Database()
        run(draft, trial)

        if not run_one(draft, trial, relative):
            continue

        after = snapshot(trial)
        lost = {name: before[name] - after.get(name, 0)
                for name in before if before[name] > after.get(name, 0)}

        if lost:
            losses.append((relative, lost))

    return losses


def report_lost_rows(draft, database):
    """Rows an earlier file inserted that a later file removed and nothing restores."""
    restored = {}
    for table_name, key, _ in database.insertions:
        restored[(table_name, key)] = True

    lost = []
    for table_name, key, _ in database.deletions:
        if (table_name, key) in restored:
            continue
        lost.append((table_name, key))

    for table_name, key in lost[:10]:
        fail(draft, "a row of {} inserted earlier in the order is deleted by a later file and "
                    "never restored: {}".format(table_name, key))

    return len(lost)


RANGE_DELETE = re.compile(
    r"DELETE\s+FROM\s+`(\w+)`\s+WHERE\s+`(\w+)`\s+BETWEEN\s+(\d+)\s+AND\s+(\d+)", re.I)


def check_delete_ranges(draft):
    """No two files may claim the same id range of the same table.

    This is the rule the simulation above found the hard way, and it is restated without the
    parser so it holds on a machine where sqlglot is not installed: a file that deletes a band
    it does not own removes another file's rows, and the operator sees it only when a hunt
    reward stops appearing in the catalogue.
    """
    claims = {}

    for relative in install_order(draft):
        path = resolve(draft, relative)
        if not path:
            continue

        text = open(path, encoding="utf-8").read()
        for table, column, low, high in RANGE_DELETE.findall(text):
            claims.setdefault((table, column), []).append(
                (int(low), int(high), relative))

    checks = 0
    for (table, column), ranges in sorted(claims.items()):
        for i, (low_a, high_a, file_a) in enumerate(ranges):
            for low_b, high_b, file_b in ranges[i + 1:]:
                if file_a == file_b:
                    continue

                checks += 1
                if low_a <= high_b and low_b <= high_a:
                    fail(draft, "{} and {} both delete {} rows in {} {}..{} and {}..{} - the "
                                "second one to run removes the first one's rows".format(
                                    file_a, file_b, table, column, low_a, high_a,
                                    low_b, high_b))

    return checks


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--draft", choices=["midnight-port", "bfa-native"])
    args = parser.parse_args()

    drafts = [args.draft] if args.draft else ["midnight-port", "bfa-native"]

    checks = 0
    for draft in drafts:
        checks += check_delete_ranges(draft)

    if sqlglot is None:
        print("delete ranges: {} range pair(s) compared by regex; the ordered simulation "
              "was skipped because sqlglot is not installed".format(checks))
        print("\n{} finding(s)".format(len(findings)))
        for line in findings:
            print("  " + line)
        return 1 if findings else 0

    for draft in drafts:
        database = Database()
        parsed = run(draft, database)

        order = install_order(draft)
        lost = report_lost_rows(draft, database)

        print("{:<16} {} file(s) in order, {} table(s), {} row(s) at the end, {} lost"
              .format(draft, len(order), len(database.tables),
                      sum(len(t["rows"]) for t in database.tables.values()), lost))

        if database.assumed:
            print("  written to and created by the realm: {}".format(
                ", ".join(sorted(database.assumed))))

        for kind, count in sorted(database.unsimulated.items()):
            print("  not simulated: {} x{}".format(kind, count))

        for relative, lost in rerun_each_file(draft, order):
            fail(draft, "re-importing {} on its own loses {}".format(
                relative, ", ".join("{} {} row(s)".format(n, c) for n, c in
                                    sorted(lost.items()))))

        # A table that a file empties and nothing refills is a table an operator installs
        # and finds empty. The counted ones are listed so the number is never assumed.
        empty = sorted(name for name, table in database.tables.items()
                       if not table["rows"] and name.startswith("housing_"))
        if empty:
            print("  empty after the install: {}".format(", ".join(empty)))

    print("\n{} finding(s)".format(len(findings)))
    for line in findings:
        print("  " + line)

    return 1 if findings else 0


if __name__ == "__main__":
    sys.exit(main())
