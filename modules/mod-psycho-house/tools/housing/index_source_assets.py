#!/usr/bin/env python3
"""
mod-psycho-house -- source asset index

Walks the source tree and extracts every numeric id constant that the code
already names, then classifies the furniture-shaped ones as decor candidates.
This is the "what does my own source already contain" inventory: it needs no
database, and every id it prints is literally present in the tree.

Usage:
    python3 tools/housing/index_source_assets.py \
        --root . \
        --out-md generated/source-asset-index.md \
        --out-csv generated/source-asset-index.csv

Output columns: kind, entry, constant, name, file, line, category, decor_candidate
"""

from __future__ import annotations

import argparse
import csv
import os
import re
from collections import Counter

# enum-style constants:  SOME_NAME = 123456,
CONST_RE = re.compile(r"^\s*([A-Z][A-Z0-9_]{2,})\s*=\s*(\d{1,7})\s*[,}]?\s*(?://\s*(.*))?$")

# Which constant families matter for housing.
KIND_PREFIXES = (
    ("ITEM_", "item"),
    ("NPC_", "creature"),
    ("GO_", "gameobject"),
    ("GAMEOBJECT_", "gameobject"),
    ("GAMEOBJ_", "gameobject"),
    ("QUEST_", "quest"),
    ("SPELL_", "spell"),
    ("ACHIEVEMENT_", "achievement"),
    ("CURRENCY_", "currency"),
    ("TOY_", "toy"),
)

# Keyword -> (category, subcategory). Order matters: first hit wins.
DECOR_KEYWORDS = [
    ("Large Structures", ("Structural", "Large Structures"),
     ("TOWER", "TENT", "GAZEBO", "PAVILION", "WINDMILL", "BRIDGE", "DOCK", "WELL", "FOUNTAIN", "GATE")),
    ("Walls and Columns", ("Structural", "Walls and Columns"),
     ("WALL", "PILLAR", "COLUMN", "FENCE", "BARRICADE", "PALISADE")),
    ("Doors", ("Structural", "Doors"), ("DOOR", "GATE", "PORTCULLIS")),
    ("Windows", ("Structural", "Windows"), ("WINDOW", "SHUTTER")),
    ("Seating", ("Furnishings", "Seating"),
     ("CHAIR", "BENCH", "STOOL", "THRONE", "SEAT")),
    ("Tables", ("Furnishings", "Tables and Desks"),
     ("TABLE", "DESK", "COUNTER", "WORKBENCH")),
    ("Storage", ("Furnishings", "Storage"),
     ("CHEST", "CRATE", "BARREL", "CABINET", "BOOKSHELF", "BOOKCASE", "SHELF", "BOX")),
    ("Beds", ("Furnishings", "Beds"), ("BED_", "_BED", "COT", "HAMMOCK")),
    ("Lighting", ("Lighting", "Small Lights"),
     ("LANTERN", "LAMP", "TORCH", "CANDLE", "BRAZIER", "CAMPFIRE", "BONFIRE", "CHANDELIER")),
    ("Large Foliage", ("Nature", "Large Foliage"),
     ("TREE", "OAK", "PINE", "WILLOW", "PALM")),
    ("Foliage", ("Nature", "Small Foliage"),
     ("BUSH", "SHRUB", "FERN", "PLANT", "FLOWER", "HERB", "VINE", "GRASS", "MOSS", "MUSHROOM")),
    ("Accents", ("Accents", "Ornamental"),
     ("VASE", "URN", "BOWL", "TROPHY", "STATUE", "PAINTING", "BANNER", "RUG", "TAPESTRY", "CARPET")),
    ("Food and Drink", ("Accents", "Food and Drink"),
     ("FOOD", "MEAT", "BREAD", "KEG", "BOTTLE", "MUG", "KETTLE", "CAULDRON", "FEAST")),
    ("Utility", ("Functional", "Utility"),
     ("FORGE", "ANVIL", "OVEN", "MAILBOX")),
]

COST = {
    "Large Structures": 5,
    "Doors": 4,
    "Walls and Columns": 4,
    "Windows": 4,
    "Tables and Desks": 3,
    "Storage": 3,
    "Beds": 3,
    "Large Foliage": 5,
    "Small Foliage": 2,
    "Seating": 2,
    "Wall Hangings": 2,
    "Ornamental": 2,
    "Small Lights": 2,
    "Floor": 1,
    "Utility": 4,
}


def kind_of(constant: str) -> str | None:
    for prefix, kind in KIND_PREFIXES:
        if constant.startswith(prefix):
            return kind
    return None


def classify(constant: str):
    for _label, (category, subcategory), keywords in DECOR_KEYWORDS:
        if any(word in constant for word in keywords):
            return category, subcategory, COST.get(subcategory, 1)
    return None


def walk(root: str):
    for base, dirs, files in os.walk(root):
        dirs[:] = [d for d in dirs if d not in (".git", "build", "dep", "node_modules")]
        for name in files:
            if not name.endswith((".h", ".hpp", ".cpp", ".sql", ".lua")):
                continue
            yield os.path.join(base, name)


def scan(root: str):
    found = []
    for path in walk(root):
        try:
            with open(path, encoding="utf-8", errors="ignore") as handle:
                for lineno, line in enumerate(handle, 1):
                    match = CONST_RE.match(line)
                    if not match:
                        continue
                    constant, value, comment = match.groups()
                    kind = kind_of(constant)
                    if not kind:
                        continue
                    found.append(
                        {
                            "kind": kind,
                            "entry": int(value),
                            "constant": constant,
                            "name": (comment or "").strip(),
                            "file": path.replace("\\", "/"),
                            "line": lineno,
                            "category": "",
                            "subcategory": "",
                            "cost": "",
                            "decor": "",
                        }
                    )
        except OSError:
            continue

    # Only game objects and creatures are actually placeable in a house.
    # Spells/quests/achievements are counted for the inventory but never
    # treated as decor; an item id cannot be spawned as furniture.
    placeable = {"gameobject", "creature"}
    noise = (
        "INVISIBLE", "TRIGGER", "SPAWN", "WAYPOINT", "BUNNY", "MARKER",
        "SPELL", "SOUND", "AREA_TRIGGER", "FOCUS", "PROXY", "SERVERSIDE",
    )
    # Structural constants that are not entry ids at all.
    not_entries = (
        "GAMEOBJECT_", "GO_FLAG", "GO_STATE", "GO_TYPE", "GO_USE",
        "NPC_FLAG", "NPC_TEXT", "NPC_VENDOR",
    )
    for row in found:
        hit = None
        usable = (
            row["entry"] > 0
            and row["kind"] in placeable
            and not row["constant"].startswith(not_entries)
            and not any(word in row["constant"] for word in noise)
        )
        if usable:
            hit = classify(row["constant"])
        if hit:
            row["category"], row["subcategory"], row["cost"] = hit
            row["decor"] = "yes"
        else:
            row["decor"] = "no"
    return found


def write_csv(rows, path):
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "w", newline="", encoding="utf-8") as handle:
        writer = csv.DictWriter(
            handle,
            fieldnames=[
                "kind", "entry", "constant", "name", "file", "line",
                "category", "subcategory", "cost", "decor",
            ],
        )
        writer.writeheader()
        for row in sorted(rows, key=lambda r: (r["kind"], r["entry"])):
            writer.writerow(row)


def write_md(rows, path, root):
    by_kind = Counter(row["kind"] for row in rows)
    decor = [row for row in rows if row["decor"] == "yes"]
    unique_entries = len({row["entry"] for row in rows})

    lines = [
        "# Generated: source asset index",
        "",
        "Produced by `tools/housing/index_source_assets.py` over the source tree.",
        "Every id here is a numeric constant that exists in the code, with the",
        "constant name as the label. It is the answer to \"what does my own source",
        "already contain\" without querying a database.",
        "",
        f"- files scanned under: `{root}`",
        f"- id constants found: **{len(rows)}**",
        f"- distinct ids: **{unique_entries}**",
        "",
        "| Kind | Constants |",
        "| --- | --- |",
    ]
    for kind, count in sorted(by_kind.items(), key=lambda kv: -kv[1]):
        lines.append(f"| {kind} | {count} |")

    kinds = Counter((row["kind"], row["category"]) for row in decor)
    lines += ["", f"## Decor-shaped constants ({len(decor)})", ""]
    lines += [
        "Classified by constant name. These are the pieces that already have a",
        "known id in the tree and can be spawned as decor without inventing",
        "anything; the full machine-readable list is in the CSV next to this file.",
        "",
        "| Id | Constant | Category | Cost | File |",
        "| --- | --- | --- | --- | --- |",
    ]
    for row in sorted(decor, key=lambda r: (r["category"], r["subcategory"], r["entry"]))[:400]:
        location = row["file"].replace(os.sep, "/")
        if location.startswith("./"):
            location = location[2:]
        lines.append(
            f"| {row['entry']} | `{row['constant']}` | "
            f"{row['category']} / {row['subcategory']} | {row['cost']} | "
            f"`{location}:{row['line']}` |"
        )

    if len(decor) > 400:
        lines.append("")
        lines.append(f"... plus {len(decor) - 400} more rows in the CSV.")

    lines += [
        "",
        "## Caveat",
        "",
        "A constant name is not a guarantee that the template row exists in the",
        "world database: the code names ids, the database ships them. Run",
        "`tools/housing/harvest_decor.py` against the live world DB and treat the",
        "intersection of the two lists as the safe decor pool.",
        "",
    ]

    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "w", encoding="utf-8") as handle:
        handle.write("\n".join(lines))


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", default=".")
    parser.add_argument(
        "--out-md", default="generated/source-asset-index.md"
    )
    parser.add_argument(
        "--out-csv", default="generated/source-asset-index.csv"
    )
    args = parser.parse_args()

    rows = scan(args.root)
    if not rows:
        print("no id constants found -- check --root")
        return 1
    write_csv(rows, args.out_csv)
    write_md(rows, args.out_md, args.root)
    decor = sum(1 for row in rows if row["decor"] == "yes")
    print(f"constants: {len(rows)}  decor-shaped: {decor}")
    print(f"wrote {args.out_md}")
    print(f"wrote {args.out_csv}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
