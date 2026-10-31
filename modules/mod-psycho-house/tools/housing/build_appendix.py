#!/usr/bin/env python3
"""Regenerate doc 08 from the source index, in full.

    python3 tools/housing/build_appendix.py            # rewrite 08-source-asset-appendix.md
    python3 tools/housing/build_appendix.py --check    # report count mismatches only

Every decor-shaped constant in `generated/source-asset-index.csv` is listed. The earlier
version of this appendix stopped after fifteen rows per section while still printing the
section total, which made a sample look like a list. This tool writes all of them and the
`--check` mode exists so that can never pass unnoticed again.
"""

import argparse
import csv
import os
import re
import sys

HERE = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
INDEX = os.path.join(HERE, "generated", "source-asset-index.csv")
OUT = os.path.join(HERE, "08-source-asset-appendix.md")

HEADER = """# Appendix - decor candidates that already have ids in the source tree

Extracted from `src/` by `tools/housing/index_source_assets.py`; every id below appears
in the tree as a named constant. Machine-readable: `generated/source-asset-index.csv`.
Regenerate this file with `tools/housing/build_appendix.py`.

**{go_rows} game-object rows ({go_ids} distinct entries)**, **{creature_rows} creature rows
({creature_ids} distinct entries)**. Every one of them is listed below, not a sample.

Read the constant name as the label; the display name comes from the world database at
load time. Verify a row exists in `gameobject_template` before relying on it. The `Cost`
column is the placement budget band the harvest assigns from the constant's family.

"""


def load():
    with open(INDEX, newline="", encoding="ascii") as handle:
        return list(csv.DictReader(handle))


def cell(text):
    return str(text).replace("|", "/")


def build():
    rows = load()
    go = [r for r in rows if r["kind"] == "gameobject" and r["decor"] == "yes"]
    creatures = [r for r in rows if r["kind"] == "creature" and r["decor"] == "yes"]

    go.sort(key=lambda r: (r["category"], r["subcategory"], int(r["entry"]), r["constant"]))
    creatures.sort(key=lambda r: (r["subcategory"], int(r["entry"]), r["constant"]))

    out = [HEADER.format(
        go_rows=len(go), go_ids=len({r["entry"] for r in go}),
        creature_rows=len(creatures), creature_ids=len({r["entry"] for r in creatures}),
    )]

    section = None
    for row in go:
        key = (row["category"], row["subcategory"])
        if key != section:
            if section is not None:
                out.append("")
            count = sum(1 for r in go if (r["category"], r["subcategory"]) == key)
            out.append("## {} / {} ({})".format(row["category"], row["subcategory"], count))
            out.append("")
            out.append("| Id | Constant | Cost | File |")
            out.append("| --- | --- | --- | --- |")
            section = key
        out.append("| {} | `{}` | {} | `{}` |".format(
            row["entry"], cell(row["constant"]), row["cost"], cell(row["file"])))

    out.append("")
    out.append("## Creatures usable as neighbourhood NPCs, pets or ambient life ({})".format(len(creatures)))
    out.append("")
    out.append("| Id | Constant | Role | File |")
    out.append("| --- | --- | --- | --- |")
    for row in creatures:
        out.append("| {} | `{}` | {} | `{}` |".format(
            row["entry"], cell(row["constant"]), cell(row["subcategory"]), cell(row["file"])))
    out.append("")
    return "\n".join(out)


def check():
    with open(OUT, encoding="utf-8") as handle:
        text = handle.read()
    total = 0
    bad = 0
    for block in re.split(r"^## ", text, flags=re.M)[1:]:
        head = block.splitlines()[0]
        match = re.search(r"\((\d+)\)$", head)
        if not match:
            continue
        listed = len([l for l in block.splitlines()
                      if l.startswith("| ") and not l.startswith("| ---") and not l.startswith("| Id")])
        total += 1
        if int(match.group(1)) != listed:
            bad += 1
            print("MISMATCH {}: claims {}, lists {}".format(head, match.group(1), listed))
    print("{} sections checked, {} mismatched".format(total, bad))
    return 1 if bad else 0


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()
    if args.check:
        return check()
    text = build()
    with open(OUT, "w", encoding="utf-8") as handle:
        handle.write(text)
    print("wrote {} ({} lines)".format(os.path.relpath(OUT, HERE), text.count("\n") + 1))
    return check()


if __name__ == "__main__":
    sys.exit(main())
