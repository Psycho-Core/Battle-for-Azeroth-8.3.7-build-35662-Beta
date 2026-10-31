#!/usr/bin/env python3
"""Read the column vocabulary of the build's own tables out of the build's loader queries.

Two checkers need the same lists, and neither can ask a MySQL server for them:

- `check_psychohouse.py` validates the draft's INSERT statements against them;
- `check_sql_columns.py` validates every statement in this module against the build's SQL,
  which is only a partial vocabulary, so the loader queries fill the gaps.

The parsers below read the C++ prepared statements rather than any documentation. If a query
is reformatted, a parser returns nothing and the caller says so; it never returns a short list
that would quietly pass a wrong INSERT.

    from build_schema import loader_vocabulary
    vocab = loader_vocabulary("/path/to/source")   # {table: [column, ...]}

Only tables a house module actually writes to are parsed. Adding another is a new function
and a new entry in `loader_vocabulary`.
"""

import os
import re


def _read(path):
    with open(path, encoding="utf-8", errors="replace") as handle:
        return handle.read()


def _columns(select):
    """Split the column list of a SELECT into names, dropping `backticks` and whitespace."""
    return [c.strip().strip("`") for c in select.split(",") if c.strip()]


def creature_template(repo):
    """`WORLD_SEL_CREATURE_TEMPLATE` - the query `ObjectMgr::LoadCreatureTemplates` uses."""
    path = os.path.join(repo, "src", "server", "database", "Database", "Implementation",
                        "WorldDatabase.cpp")
    if not os.path.exists(path):
        return []
    match = re.search(r'WORLD_SEL_CREATURE_TEMPLATE,\s*"SELECT (.*?) FROM creature_template\b',
                      _read(path), re.S)
    return _columns(match.group(1)) if match else []


def gameobject_template(repo):
    """`ObjectMgr::LoadGameObjectTemplate` - one SELECT, written as adjacent literals."""
    path = os.path.join(repo, "src", "server", "game", "Globals", "ObjectMgr.cpp")
    if not os.path.exists(path):
        return []
    text = _read(path)
    start = text.find('"SELECT entry, type, displayId')
    if start < 0:
        return []
    end = text.find('"FROM gameobject_template"', start)
    if end < 0:
        return []
    literals = re.findall(r'"([^"]*)"', text[start:end])
    select = "".join(literals)
    if not select.startswith("SELECT "):
        return []
    return _columns(select[len("SELECT "):])


def creature_template_model(repo):
    """`ObjectMgr::LoadCreatureTemplateModels` - four columns, ordered by a fifth."""
    path = os.path.join(repo, "src", "server", "game", "Globals", "ObjectMgr.cpp")
    if not os.path.exists(path):
        return []
    match = re.search(r"SELECT (CreatureID, CreatureDisplayID, DisplayScale, Probability) "
                      r"FROM creature_template_model", _read(path))
    if not match:
        return []
    columns = [c.strip() for c in match.group(1).split(",")]
    columns.append("Idx")       # the loader orders by it, so the column exists
    return columns


def loader_vocabulary(repo):
    """{table: [column, ...]} for every table a house module writes to."""
    vocabulary = {}
    for table, reader in (("creature_template", creature_template),
                          ("gameobject_template", gameobject_template),
                          ("creature_template_model", creature_template_model)):
        columns = reader(repo)
        if columns:
            vocabulary[table] = columns
    return vocabulary


if __name__ == "__main__":
    import argparse
    import sys

    parser = argparse.ArgumentParser(description="Print the loader vocabulary as JSON.")
    parser.add_argument("--repo", default=".", help="source tree containing src/server")
    arguments = parser.parse_args()
    import json
    found = loader_vocabulary(os.path.abspath(arguments.repo))
    if not found:
        sys.exit("no loader query found under " + arguments.repo)
    print(json.dumps(found, indent=2, sort_keys=True))
