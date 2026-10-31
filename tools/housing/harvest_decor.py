#!/usr/bin/env python3
"""
mod-psycho-house -- decor harvest

Scans the world database's gameobject_template table and emits a ready-to-run
INSERT file for psycho_house_decor.

Read-only against the database. Never writes to MySQL; only writes a .sql file.

Usage:
    python3 tools/housing/harvest_decor.py \
        --host 127.0.0.1 --user root --password x --database world \
        --out sql/generated/psycho_house_decor.sql \
        --overrides tools/housing/decor_overrides.csv

Requires: pip install pymysql

Rules implemented (see docs/mod-psycho-house/03-bfa-asset-catalogue.md):
  * category/subcategory by keyword, first match wins
  * budget cost 1..5 by category weight, overridable per entry or name family
  * displayId 0 rows dropped (they spawn nothing visible)
  * near-duplicate names collapsed (digits/punctuation stripped), one entry kept
  * ids start at 100000, assigned in name order for stable diffs
"""

from __future__ import annotations

import argparse
import csv
import re
import sys
from collections import OrderedDict
from typing import Iterable

try:
    import pymysql
except ImportError:  # pragma: no cover
    print("pymysql is required: pip install pymysql", file=sys.stderr)
    raise SystemExit(2)

FIRST_ID = 100000

# (category, subcategory, regex)
RULES: list[tuple[str, str, str]] = [
    ("Structural", "Large Structures",
     r"\b(tower|gazebo|tent|pavilion|windmill|bridge|dock|well|fountain)\b"),
    ("Structural", "Walls and Columns",
     r"\b(wall|pillar|column|fence|barricade|palisade|railing)\b"),
    ("Structural", "Doors", r"\b(door|gate|portcullis|archway)\b"),
    ("Structural", "Windows", r"\b(window|shutter|lattice|trellis)\b"),

    ("Furnishings", "Seating", r"\b(chair|stool|bench|seat|throne|sofa|couch)\b"),
    ("Furnishings", "Tables and Desks", r"\b(table|desk|counter|workbench)\b"),
    ("Furnishings", "Storage",
     r"\b(chest|crate|barrel|box|cabinet|bookcase|bookshelf|shelf|coffer)\b"),
    ("Furnishings", "Beds", r"\b(bed|cot|hammock|bunk|mattress)\b"),

    ("Lighting", "Large Lights", r"\b(bonfire|pyre|beacon)\b"),
    ("Lighting", "Wall Lights", r"\b(wall lamp|sconce)\b"),
    ("Lighting", "Ceiling Lights", r"\b(chandelier|hanging lamp)\b"),
    ("Lighting", "Small Lights",
     r"\b(lantern|lamp|torch|candle|brazier|campfire|light)\b"),

    ("Nature", "Large Foliage", r"\b(tree|oak|pine|willow|palm|ancient)\b"),
    ("Nature", "Small Foliage",
     r"\b(bush|shrub|fern|plant|flower|herb|vine|ivy|grass|moss)\b"),
    ("Nature", "Ground Cover", r"\b(lilypad|coral|kelp|mushroom|leaves)\b"),

    ("Accents", "Wall Hangings", r"\b(rug|tapestry|curtain|drape)\b"),
    ("Accents", "Ornamental",
     r"\b(vase|urn|bowl|goblet|trophy|statue|painting|banner)\b"),
    ("Accents", "Food and Drink",
     r"\b(food|fruit|meat|bread|keg|bottle|mug|kettle|cauldron|feast)\b"),
    ("Accents", "Floor", r"\b(carpet|mat|rug)\b"),

    ("Functional", "Utility", r"\b(forge|anvil|oven|mailbox|bank)\b"),
]

COST_BY_SUB = {
    ("Structural", "Large Structures"): 5,
    ("Structural", "Walls and Columns"): 4,
    ("Structural", "Doors"): 4,
    ("Structural", "Windows"): 4,
    ("Furnishings", "Tables and Desks"): 3,
    ("Furnishings", "Storage"): 3,
    ("Furnishings", "Beds"): 3,
    ("Nature", "Large Foliage"): 5,
    ("Nature", "Small Foliage"): 2,
    ("Nature", "Ground Cover"): 1,
    ("Furnishings", "Seating"): 2,
    ("Accents", "Wall Hangings"): 2,
    ("Lighting", "Large Lights"): 4,
    ("Lighting", "Wall Lights"): 3,
    ("Lighting", "Ceiling Lights"): 4,
    ("Lighting", "Small Lights"): 2,
    ("Functional", "Utility"): 4,
}

# Subcategories that make no sense indoors -- surfaced as outdoor (surface 1)
OUTDOOR_ONLY = {
    ("Nature", "Large Foliage"),
    ("Nature", "Small Foliage"),
    ("Nature", "Ground Cover"),
    ("Structural", "Large Structures"),
}

NAME_JUNK = re.compile(r"[^a-z ]+")


def normalise(name: str) -> str:
    """Name family key: lowercase, digits and punctuation stripped."""
    s = NAME_JUNK.sub(" ", name.lower())
    return " ".join(s.split())


def categorise(name: str) -> tuple[str, str] | None:
    lowered = name.lower()
    for category, subcategory, pattern in RULES:
        if re.search(pattern, lowered):
            return category, subcategory
    return None


def cost_of(category: str, subcategory: str) -> int:
    return COST_BY_SUB.get((category, subcategory), 1)


def sql_quote(value: str) -> str:
    return "'" + value.replace("\\", "\\\\").replace("'", "''") + "'"


def load_overrides(path: str | None) -> dict[str, dict]:
    """CSV: name_family,cost,surface,skip[,renamed]"""
    out: dict[str, dict] = {}
    if not path:
        return out
    with open(path, newline="", encoding="utf-8") as handle:
        reader = csv.DictReader(handle)
        for row in reader:
            key = normalise(row.get("name_family") or row.get("name") or "")
            if key:
                out[key] = row
    return out


def fetch_rows(conn) -> Iterable[tuple[int, str, int]]:
    with conn.cursor() as cur:
        cur.execute(
            "SELECT entry, name, type, displayId FROM gameobject_template "
            "WHERE displayId <> 0 AND name IS NOT NULL AND name <> ''"
        )
        for entry, name, go_type, display_id in cur:
            yield int(entry), str(name), int(go_type)


def build(rows: Iterable[tuple[int, str, int]], overrides: dict[str, dict]):
    scored: dict[str, dict] = OrderedDict()

    for entry, name, go_type in rows:
        cat = categorise(name)
        if not cat:
            continue
        category, subcategory = cat
        family = normalise(name)
        if not family:
            continue

        override = overrides.get(family)
        if override and str(override.get("skip", "")).lower() in ("1", "true", "yes"):
            continue

        candidate = {
            "name": name,
            "entry": entry,
            "go_type": go_type,
            "category": category,
            "subcategory": subcategory,
            "cost": cost_of(category, subcategory),
            "surface": 1 if (category, subcategory) in OUTDOOR_ONLY else 0,
            "family": family,
        }
        if override:
            if override.get("cost"):
                candidate["cost"] = max(1, min(5, int(override["cost"])))
            if override.get("surface") not in (None, ""):
                candidate["surface"] = int(override["surface"])
            if override.get("renamed"):
                candidate["name"] = override["renamed"]

        previous = scored.get(family)
        if previous is None:
            scored[family] = candidate
            continue
        # Keep the more useful variant: shorter name wins, then lower entry id.
        if (len(candidate["name"]), candidate["entry"]) < (
            len(previous["name"]), previous["entry"]
        ):
            scored[family] = candidate

    return sorted(scored.values(), key=lambda row: row["family"])


def emit(records: list[dict], out_path: str) -> None:
    lines = [
        "-- generated by tools/housing/harvest_decor.py -- do not edit by hand",
        "-- Override individual rows in tools/housing/decor_overrides.csv instead.",
        "",
        "DELETE FROM `psycho_house_decor` WHERE `id` >= 100000;",
        "",
        "INSERT INTO `psycho_house_decor`",
        "(`id`, `name`, `gameobject_entry`, `gameobject_type`, `budget_cost`,",
        " `surface`, `placement`, `category`, `subcategory`, `enabled`)",
        "VALUES",
    ]

    values = []
    for index, record in enumerate(records):
        values.append(
            "({id}, {name}, {entry}, {gotype}, {cost}, {surface}, {placement}, "
            "{cat}, {sub}, 1)  -- from {rawname}".format(
                id=FIRST_ID + index,
                name=sql_quote(record["name"]),
                entry=record["entry"],
                gotype=record["go_type"],
                cost=record["cost"],
                surface=record["surface"],
                placement=2,
                cat=sql_quote(record["category"]),
                sub=sql_quote(record["subcategory"]),
                rawname=record["name"].replace("*/", ""),
            )
        )

    lines.append(",\n".join(values) + ";")
    lines.append("")
    lines.append("-- Summary")
    lines.append("-- total rows: %d" % len(records))
    by_cat: dict[str, int] = {}
    for record in records:
        key = f'{record["category"]} / {record["subcategory"]}'
        by_cat[key] = by_cat.get(key, 0) + 1
    for key in sorted(by_cat):
        lines.append(f"--   {key}: {by_cat[key]}")

    with open(out_path, "w", encoding="utf-8") as handle:
        handle.write("\n".join(lines) + "\n")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--host", default="127.0.0.1")
    parser.add_argument("--port", type=int, default=3306)
    parser.add_argument("--user", default="root")
    parser.add_argument("--password", default="")
    parser.add_argument("--database", default="world")
    parser.add_argument("--out", default="sql/generated/psycho_house_decor.sql")
    parser.add_argument("--overrides", default=None)
    args = parser.parse_args()

    overrides = load_overrides(args.overrides)
    conn = pymysql.connect(
        host=args.host,
        port=args.port,
        user=args.user,
        password=args.password,
        database=args.database,
        charset="utf8mb4",
        read_timeout=120,
    )
    try:
        rows = list(fetch_rows(conn))
    finally:
        conn.close()

    records = build(rows, overrides)
    if not records:
        print("no candidates matched -- check the gameobject_template table", file=sys.stderr)
        return 1

    emit(records, args.out)
    print(f"scanned {len(rows)} game objects, kept {len(records)} decor rows")
    print(f"wrote {args.out}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
