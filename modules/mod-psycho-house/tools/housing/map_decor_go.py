#!/usr/bin/env python3
"""Pick a 8.3.7 game object for every ported retail decor piece.

Retail decor has an item id and no game object. 8.3.7 has game objects and no decor
table. This tool closes that gap with real entries only: every candidate is a named
`gameobject` constant that exists in the tree, so nothing here is an invented id.

Matching runs in three passes:

1. **family** - the item's head noun is looked up in FAMILIES, which maps a word to the
   constant words that stand in for it ("STOOL" -> CHAIR, "CASK" -> KEG). Every member of
   that family in the candidate pool is a match, and the one used is chosen by a stable
   hash of the item name, so ten different chairs get ten different chair models rather
   than the same one ten times.
2. **keyword** - no family hit, so score by shared words with inverse document frequency.
3. **fallback** - nothing shared. The piece takes the first game object in its own
   category and the row says `fallback`, which is the honest label for "wrong model, right
   family".

Output: `data/town-center-go-map.csv` -> item, category, gameobject_entry, go_constant,
go_subcategory, match, confidence. `tools/housing/build_drafts.py` reads it.

    python3 tools/housing/map_decor_go.py
    python3 tools/housing/map_decor_go.py --go-names dump.csv   # your own DB export
"""

import argparse
import csv
import hashlib
import math
import os
import re
import sys
from collections import Counter

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
DATA = os.path.join(ROOT, "data")
GEN = os.path.join(ROOT, "generated")

REJECT = ("DOOR", "GATE", "TRAP", "TRIGGER", "PORTAL", "INSTANCE", "CORPSE", "SPAWN",
          "SOUND", "SPELL", "AURA", "DUMMY", "BOSS", "GOSSIP", "MARKER", "EXPLOSION",
          "MISSILE", "PROJECTILE", "GRAVEYARD", "ZONE", "BARRIER", "INVISIBLE", "ARENA",
          "QUEST", "WAYPOINT", "SMOKE", "FOG", "LIGHT_BEAM", "VEHICLE", "CAMERA")

STOP = {"GO", "THE", "OF", "A", "AN", "AND", "WITH", "SMALL", "LARGE", "LONG", "SHORT",
        "OLD", "NEW", "OPEN", "CLOSED", "TALL", "WIDE", "BIG", "LITTLE", "SINGLE",
        "DOUBLE", "PAIR", "TRIO", "SET", "PILE", "STACK", "EMPTY", "FULL", "BROKEN",
        "DAMAGED", "BURNT", "DARK", "LIGHT", "WOODEN", "STONE", "METAL", "IRON"}

# head noun in the retail item -> words that identify the same thing in a constant
FAMILIES = {
    "CHAIR": ("CHAIR", "THRONE", "STOOL"),
    "BENCH": ("BENCH",),
    "STOOL": ("CHAIR", "STOOL"),
    "THRONE": ("THRONE", "CHAIR"),
    "TABLE": ("TABLE", "DESK"),
    "DESK": ("TABLE", "DESK"),
    "BENCH_SEAT": ("BENCH", "CHAIR"),
    "BED": ("BED", "BEDROLL"),
    "RUG": ("RUG", "CARPET"),
    "CARPET": ("CARPET", "RUG"),
    "CRATE": ("CRATE", "CHEST", "BOX"),
    "CHEST": ("CHEST", "CRATE"),
    "BARREL": ("KEG", "BARREL", "CASK"),
    "KEG": ("KEG", "BARREL"),
    "BRAZIER": ("BRAZIER", "FIRE"),
    "TORCH": ("TORCH", "BRAZIER"),
    "CANDLE": ("CANDLE", "TORCH", "BRAZIER"),
    "LANTERN": ("LANTERN", "TORCH", "BRAZIER"),
    "LAMP": ("LAMP", "LANTERN", "TORCH"),
    "LIGHT": ("BRAZIER", "TORCH", "FIRE"),
    "TREE": ("TREE", "FOLIAGE"),
    "BUSH": ("BUSH", "FOLIAGE", "PLANT"),
    "FLOWER": ("FLOWER", "PLANT", "FOLIAGE"),
    "GRASS": ("GRASS", "FOLIAGE", "PLANT"),
    "IVY": ("IVY", "VINE", "FOLIAGE"),
    "PLANT": ("PLANT", "FOLIAGE", "BUSH"),
    "ROCK": ("BOULDER", "ROCK", "STONE"),
    "BOULDER": ("BOULDER", "ROCK"),
    "COLUMN": ("COLUMN", "PILLAR"),
    "PILLAR": ("COLUMN", "PILLAR"),
    "STATUE": ("STATUE", "IDOL"),
    "WALL": ("WALL", "COLUMN"),
    "WINDOW": ("WINDOW",),
    "WELL": ("WELL",),
    "FOUNTAIN": ("FOUNTAIN", "WELL"),
    "BANNER": ("BANNER", "FLAG"),
    "FLAG": ("BANNER", "FLAG"),
    "ANVIL": ("ANVIL", "FORGE"),
    "FORGE": ("FORGE", "ANVIL"),
    "OVEN": ("OVEN", "FORGE"),
    "TENT": ("TENT",),
    "CART": ("CART", "WAGON"),
    "SHELF": ("SHELF", "RACK"),
    "RACK": ("RACK", "SHELF"),
    "FENCE": ("FENCE", "POST"),
    "SIGN": ("SIGN", "BANNER"),
    "BOOK": ("BOOK", "SHELF"),
    "ALTAR": ("ALTAR", "STATUE"),
    "SHRINE": ("SHRINE", "ALTAR", "STATUE"),
    "CAULDRON": ("CAULDRON", "KEG"),
    "POT": ("POT", "CAULDRON"),
    "BASKET": ("BASKET", "CRATE"),
    "SACK": ("SACK", "BAG", "CRATE"),
    "CARPET_SQ": ("CARPET", "RUG"),
}

# retail category -> preferred game object subcategory for the fallback pass
CATEGORY_HINT = {
    "Tables": "Furnishings", "Seating": "Seating", "Beds": "Furnishings",
    "Rugs": "Furnishings", "Lighting": "Lighting", "Nature": "Nature",
    "Storage": "Storage", "Structural": "Structural", "Wall Art": "Ornamental",
    "Books": "Furnishings", "Pets": "Ornamental", "Holiday": "Ornamental",
    "Accents": "Ornamental",
}

# category -> the constant words that at least look like that category
CATEGORY_FAMILY = {
    "Tables": ("TABLE", "DESK", "COUNTER"),
    "Seating": ("CHAIR", "BENCH", "THRONE"),
    "Beds": ("BED", "BEDROLL"),
    "Rugs": ("RUG", "CARPET"),
    "Lighting": ("BRAZIER", "TORCH", "CANDLE", "LANTERN", "LAMP"),
    "Nature": ("TREE", "BUSH", "FOLIAGE", "FLOWER", "GRASS", "PLANT"),
    "Storage": ("CRATE", "CHEST", "KEG", "BARREL", "SACK"),
    "Structural": ("COLUMN", "WALL", "FENCE", "POST", "STATUE"),
    "Wall Art": ("BANNER", "STATUE", "IDOL"),
    "Books": ("SHELF", "TABLE"),
    "Pets": ("STATUE", "IDOL", "CRATE"),
    "Holiday": ("BRAZIER", "BANNER", "STATUE"),
    "Accents": ("STATUE", "IDOL", "BRAZIER"),
}


def tokens(text):
    words = re.split(r"[^A-Za-z0-9]+", text.upper())
    return [w for w in words if w and w not in STOP and not w.isdigit()]


def head_nouns(item):
    words = [w for w in re.split(r"[^A-Za-z0-9]+", item.upper()) if w]
    out = []
    for word in reversed(words):
        if word in FAMILIES or word in CATEGORY_FAMILY:
            out.append(word)
        if len(out) >= 2:
            break
    return out


def load_source_gos():
    rows = [r for r in csv.DictReader(open(os.path.join(GEN, "source-asset-index.csv"),
                                           newline="", encoding="utf-8"))
            if r["kind"] == "gameobject"]
    seen = {}
    enum_prefix = ("GAMEOBJECT_TYPE", "GAMEOBJECT_FLAGS", "GAMEOBJECT_STATE", "GO_FLAG", "GO_STATE")
    for row in rows:
        label = row["constant"] or row["name"]
        if not label or label.upper().startswith(enum_prefix):
            continue
        if any(bad in label.upper() for bad in REJECT):
            continue
        seen.setdefault(row["entry"], {
            "entry": row["entry"], "constant": label, "category": row["category"],
            "subcategory": row["subcategory"], "words": set(tokens(label)),
        })
    return sorted(seen.values(), key=lambda r: int(r["entry"]))


def load_db_gos(path):
    """Optional: an export of `SELECT entry, name FROM gameobject_template`."""
    out = []
    for row in csv.reader(open(path, newline="", encoding="utf-8")):
        if len(row) < 2 or not row[0].isdigit():
            continue
        entry, name = row[0], row[1]
        if any(bad in name.upper() for bad in REJECT):
            continue
        out.append({"entry": entry, "constant": name, "category": "", "subcategory": "",
                    "words": set(tokens(name))})
    return out


def stable_pick(item, candidates):
    digest = hashlib.md5(item.encode("utf-8")).hexdigest()
    return candidates[int(digest[:8], 16) % len(candidates)]


def build_idf(gos):
    freq = Counter()
    for go in gos:
        freq.update(go["words"])
    return {w: math.log(1 + len(gos) / (1 + n)) for w, n in freq.items()}


def match(item, category, gos, idf):
    words = set(tokens(item))

    for noun in head_nouns(item):
        keys = FAMILIES.get(noun, ())
        members = [go for go in gos if go["words"] & set(keys)]
        if members:
            chosen = stable_pick(item, members)
            return chosen, "family:" + noun, "high"

    scored = []
    for go in gos:
        shared = words & go["words"]
        if shared:
            scored.append((sum(idf.get(w, 1.0) for w in shared), len(shared), go))
    if scored:
        scored.sort(key=lambda x: (-x[0], -x[1], int(x[2]["entry"])))
        score, shared, go = scored[0]
        return go, "keyword", ("high" if shared >= 2 else "medium")

    keys = CATEGORY_FAMILY.get(category, ())
    members = [go for go in gos if go["words"] & set(keys)]
    if members:
        return stable_pick(item, members), "category", "medium"

    hint = CATEGORY_HINT.get(category)
    same = [go for go in gos if go["subcategory"] == hint] or \
           [go for go in gos if go["category"] == hint]
    if same:
        return stable_pick(item, same), "fallback", "low"
    return gos[0], "fallback", "low"


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--go-names", help="CSV of entry,name from gameobject_template")
    args = ap.parse_args()

    gos = load_source_gos()
    if args.go_names:
        extra = load_db_gos(args.go_names)
        known = {go["entry"] for go in gos}
        gos += [go for go in extra if go["entry"] not in known]
    gos = [go for go in gos if go["words"]]
    idf = build_idf(gos)

    import importlib.util
    spec = importlib.util.spec_from_file_location(
        "build_drafts", os.path.join(ROOT, "tools", "housing", "build_drafts.py"))
    bd = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(bd)

    items = sorted({row["item"] for row in csv.DictReader(
        open(os.path.join(DATA, "town-center-stock.csv"), newline="", encoding="utf-8"))})

    rows = []
    counts = Counter()
    for item in items:
        category = bd.category(item)
        go, how, confidence = match(item, category, gos, idf)
        counts[(how.split(":")[0], confidence)] += 1
        rows.append({"item": item, "category": category,
                     "gameobject_entry": go["entry"], "go_constant": go["constant"],
                     "go_subcategory": go["subcategory"] or "db", "match": how,
                     "confidence": confidence})

    path = os.path.join(DATA, "town-center-go-map.csv")
    with open(path, "w", newline="", encoding="utf-8") as fh:
        writer = csv.DictWriter(fh, fieldnames=["item", "category", "gameobject_entry",
                                                "go_constant", "go_subcategory", "match",
                                                "confidence"])
        writer.writeheader()
        writer.writerows(rows)

    print("candidates: {} game objects".format(len(gos)))
    print("mapped {} decor names".format(len(rows)))
    for key, n in sorted(counts.items()):
        print("  {:<10} {:<8} {}".format(key[0], key[1], n))
    print("written to data/town-center-go-map.csv")


if __name__ == "__main__":
    main()
