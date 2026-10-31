#!/usr/bin/env python3
"""Harvest the column names this build's own SQL uses, then check module SQL against them.

    python3 tools/housing/check_sql_columns.py --repo /path/to/source
    python3 tools/housing/check_sql_columns.py --repo /path/to/source --write

The point is that nothing here runs against a MySQL server, so the next best check is:
every column the module writes must be a column the core's own update files already write
to. The harvest reads `sql/` in the source tree - INSERT column lists, UPDATE SET
assignments and ALTER TABLE ADD/CHANGE - and builds the vocabulary. Anything the module
uses that the build never mentions is reported.

With --write the vocabulary is saved to generated/sql-columns.json, which the docs quote
and the check re-reads when no source tree is passed.
"""

import argparse
import json
import os
import re
import sys

HERE = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

sys.path.insert(0, os.path.join(HERE, "tools", "housing"))
import build_schema  # noqa: E402  - the loader queries, read by check_psychohouse too
MODULE_SQL = [os.path.join(HERE, "sql"),
              os.path.join(HERE, "drafts", "midnight-port", "sql"),
              os.path.join(HERE, "drafts", "bfa-native", "sql"),
              os.path.join(HERE, "drafts", "mod-psychohouse", "sql")]

INSERT = re.compile(r"INSERT\s+INTO\s+`(\w+)`\s*\(([^)]*)\)", re.I)
UPDATE = re.compile(r"UPDATE\s+`(\w+)`\s+SET\s+(.*?)(?:\bWHERE\b|;)", re.I | re.S)
ALTER_ADD = re.compile(r"ALTER\s+TABLE\s+`(\w+)`\s+(?:ADD|CHANGE|MODIFY)\s+(?:COLUMN\s+)?`?(\w+)`?", re.I)


def harvest(root):
    vocab = {}
    for base, _dirs, files in os.walk(root):
        for name in sorted(files):
            if not name.endswith(".sql"):
                continue
            path = os.path.join(base, name)
            with open(path, encoding="utf-8", errors="replace") as handle:
                text = handle.read()
            for match in INSERT.finditer(text):
                table = match.group(1)
                columns = {c.strip().strip("`") for c in match.group(2).split(",")}
                vocab.setdefault(table, set()).update(columns)
            for match in UPDATE.finditer(text):
                table = match.group(1)
                body = match.group(2)
                columns = {c.strip().strip("`") for c in re.findall(r"`(\w+)`\s*=", body)}
                vocab.setdefault(table, set()).update(columns)
            for match in re.finditer(r"(?:UPDATE|DELETE FROM)\s+`(\w+)`[^;]*?WHERE\s+(.*?)(?:;|$)",
                                     text, re.I | re.S):
                table = match.group(1)
                columns = {c.strip() for c in re.findall(r"`(\w+)`\s*=", match.group(2))}
                vocab.setdefault(table, set()).update(columns)
            for match in ALTER_ADD.finditer(text):
                vocab.setdefault(match.group(1), set()).add(match.group(2))
    return {table: sorted(columns) for table, columns in vocab.items()}


def module_statements(path):
    with open(path, encoding="utf-8") as handle:
        text = handle.read()
    for match in INSERT.finditer(text):
        yield path, match.group(1), [c.strip().strip("`") for c in match.group(2).split(",")]
    for match in UPDATE.finditer(text):
        yield path, match.group(1), [c.strip().strip("`") for c in
                                     re.findall(r"`(\w+)`\s*=", match.group(2))]


def module_tables():
    """Every table the module creates itself - those are not the build's to validate."""
    owned = set()
    for root in MODULE_SQL:
        for base, _dirs, files in os.walk(root):
            for name in files:
                if not name.endswith(".sql"):
                    continue
                with open(os.path.join(base, name), encoding="utf-8") as handle:
                    owned.update(re.findall(r"CREATE TABLE (?:IF NOT EXISTS )?`(\w+)`",
                                            handle.read()))
    return owned


def check(vocab):
    findings = []
    module_tables = module_tables_default()
    checked = 0
    for root in MODULE_SQL:
        for base, _dirs, files in os.walk(root):
            for name in sorted(files):
                if not name.endswith(".sql"):
                    continue
                for path, table, columns in module_statements(os.path.join(base, name)):
                    if table in module_tables:
                        continue          # the module owns these tables
                    known = vocab.get(table)
                    checked += 1
                    if known is None:
                        findings.append("{}: table `{}` is not in the build's SQL at all".format(
                            os.path.relpath(path, HERE), table))
                        continue
                    unknown = [c for c in columns if c not in known]
                    if unknown:
                        findings.append("{}: `{}` has no column {}".format(
                            os.path.relpath(path, HERE), table, unknown))
    return findings, checked


def module_tables_default():
    owned = module_tables()
    if not owned:
        owned = {"housing_decor", "housing_vendor"}
    return owned


def find_repo():
    """Walk up from the module until a directory with sql/updates turns up."""
    path = HERE
    for _ in range(4):
        if os.path.isdir(os.path.join(path, "sql", "updates")):
            return path
        path = os.path.dirname(path)
    return os.path.dirname(HERE)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--repo", default=find_repo(),
                        help="source tree to harvest sql/ from, "
                             "default: the tree this module sits in")
    parser.add_argument("--write", action="store_true",
                        help="save the vocabulary to generated/sql-columns.json")
    args = parser.parse_args()

    sql_root = os.path.join(args.repo, "sql")
    if not os.path.isdir(sql_root):
        sys.exit("no sql directory under " + args.repo)

    vocab = harvest(sql_root)
    print("harvested {} tables from {}".format(len(vocab), sql_root))

    # The build's own SQL only ever writes a handful of columns per table - an INSERT that
    # touches twelve of creature_template's seventy-eight is normal. A full-row INSERT would
    # therefore look wrong here. The loader queries are the complete list, so they are merged
    # in for the tables a house module writes to.
    from_loader = build_schema.loader_vocabulary(args.repo)
    for table, columns in from_loader.items():
        vocab[table] = sorted(set(vocab.get(table, [])) | set(columns))
    if from_loader:
        print("merged loader columns for {}".format(", ".join(sorted(from_loader))))
    for table in ("quest_template", "quest_template_addon", "quest_objectives",
                  "creature_template", "creature", "item_template", "gameobject_template"):
        print("  {:<24} {:>4} columns".format(table, len(vocab.get(table, []))))

    if args.write:
        out = os.path.join(HERE, "generated", "sql-columns.json")
        os.makedirs(os.path.dirname(out), exist_ok=True)
        with open(out, "w", encoding="utf-8") as handle:
            json.dump(vocab, handle, indent=1, sort_keys=True)
            handle.write("\n")
        print("wrote " + os.path.relpath(out, HERE))

    findings, checked = check(vocab)
    print("\n{} statement(s) checked against the build vocabulary".format(checked))
    print("{} finding(s)".format(len(findings)))
    for line in findings:
        print("  " + line)
    return 1 if findings else 0


if __name__ == "__main__":
    sys.exit(main())
