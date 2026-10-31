#!/usr/bin/env python3
"""Merge one vendor's hand-captured item list into the two town-centre CSVs.

    python3 tools/housing/merge_capture.py --vendor "Jehzar Starfall" \
        --file data/captures/jehzar-starfall.tsv --source "wowhead npc 255298"

The capture file is tab separated, one item per line:

    <item id or empty>  <name>  <price or empty>  <currency or empty>  [note]

Rules the merge follows, because they are the rules the register promises:

- Every stock row for that vendor is replaced, never duplicated. Re-running the same
  capture is therefore safe, and a corrected line replaces the old one.
- A row with an item id also lands in `data/town-center-items.csv`.
- A row without a price keeps an empty price. Blank means unpublished, never free.
- The `note` column of the stock CSV is preserved when a row survives by name; new rows
  start empty, so a gate note is never silently dropped for an item that already had one.
"""

import argparse
import csv
import os
import sys

HERE = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
DATA = os.path.join(HERE, "data")
STOCK = os.path.join(DATA, "town-center-stock.csv")
ITEMS = os.path.join(DATA, "town-center-items.csv")


def read_rows(path):
    with open(path, newline="", encoding="ascii") as handle:
        return list(csv.DictReader(handle))


def write_rows(path, fieldnames, rows):
    with open(path, "w", newline="", encoding="ascii") as handle:
        writer = csv.DictWriter(handle, fieldnames=fieldnames)
        writer.writeheader()
        writer.writerows(rows)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--vendor", required=True)
    parser.add_argument("--file", required=True)
    parser.add_argument("--source", default="")
    parser.add_argument(
        "--append",
        action="store_true",
        help="keep the vendor's existing rows and add these on top instead of replacing "
             "them. Use it when one vendor stands as two npcs and each page sells a "
             "different set.",
    )
    args = parser.parse_args()

    capture = []
    with open(args.file, encoding="ascii") as handle:
        for number, line in enumerate(handle, 1):
            line = line.rstrip("\n")
            if not line.strip() or line.startswith("#"):
                continue
            parts = line.split("\t")
            if len(parts) < 2:
                sys.exit("line %d is not tab separated: %r" % (number, line))
            parts += [""] * (5 - len(parts))
            item_id, name, price, currency, note = [p.strip() for p in parts[:5]]
            if not name:
                sys.exit("line %d has no name" % number)
            capture.append({
                "item_id": item_id,
                "name": name,
                "price": price,
                "currency": currency or ("Gold" if price else ""),
                "note": note,
            })

    stock = read_rows(STOCK)
    items = read_rows(ITEMS)

    old = {(r["vendor"], r["item"].strip().lower()): r for r in stock}
    if args.append:
        kept = list(stock)
        replaced = 0
    else:
        kept = [r for r in stock if r["vendor"] != args.vendor]
        replaced = len(stock) - len(kept)

    for row in capture:
        previous = old.get((args.vendor, row["name"].lower()))
        if args.append and previous is not None:
            previous["price"] = row["price"]
            previous["currency"] = row["currency"]
            if row["note"]:
                previous["note"] = row["note"]
            continue
        kept.append({
            "vendor": args.vendor,
            "item": row["name"],
            "price": row["price"],
            "currency": row["currency"],
            "note": row["note"] or (previous["note"] if previous and row["price"] else ""),
        })

    kept.sort(key=lambda r: (r["vendor"].lower(), r["item"].lower()))
    write_rows(STOCK, ["vendor", "item", "price", "currency", "note"], kept)

    known = {(r["vendor"], r["item_id"]) for r in items}
    added = 0
    for row in capture:
        if not row["item_id"]:
            continue
        key = (args.vendor, row["item_id"])
        if key in known:
            continue
        items.append({
            "vendor": args.vendor,
            "item_id": row["item_id"],
            "item_name": row["name"],
            "price": row["price"],
            "currency": row["currency"],
            "source": args.source,
        })
        known.add(key)
        added += 1

    items.sort(key=lambda r: (r["vendor"].lower(), int(r["item_id"])))
    write_rows(ITEMS, ["vendor", "item_id", "item_name", "price", "currency", "source"], items)

    with_id = sum(1 for r in capture if r["item_id"])
    blank = sum(1 for r in capture if not r["price"])
    print("{}: {} rows captured, {} with an item id, {} with no published price".format(
        args.vendor, len(capture), with_id, blank))
    print("stock: replaced {} old rows, now {}".format(replaced, len(kept)))
    print("items: added {} new ids, now {}".format(added, len(items)))


if __name__ == "__main__":
    main()
