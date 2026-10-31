#!/usr/bin/env python3
"""
mod-psycho-house -- vendor stock scraper

Pulls the decor stock of every housing vendor into one CSV, so the vendor tables
can be loaded into psycho_house_vendor_stock without retyping 1,742 items.

Output columns:
    vendor,item,price,currency,coords,npc_id

Usage:
    python3 tools/housing/fetch_vendor_stock.py \
        --vendors docs/mod-psycho-house/vendor-slugs.txt \
        --out sql/generated/vendor_stock.csv

The slug file is one vendor slug per line, e.g.:
    faarden-the-builder
    xiao-dan
    argan-hammerfist

Slugs come from the housing vendor index (166 vendors). Only public HTML is
fetched, one request per vendor, with a delay between requests. Nothing is
written except the CSV.
"""

from __future__ import annotations

import argparse
import csv
import html
import os
import re
import sys
import time
import urllib.error
import urllib.request

BASE = "https://umbralmarket.com/housing-hub/vendors/"

UA = (
    "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 "
    "(KHTML, like Gecko) Chrome/125.0 Safari/537.36"
)

COORDS_RE = re.compile(r"Coords:\s*<strong>([\d.]+),\s*([\d.]+)</strong>")
NPC_RE = re.compile(r"wowhead\.com/npc=(\d+)")

# An item card looks like:  ...item-slug)\n...\nName\n\n25g
ITEM_RE = re.compile(
    r'housing-hub/([a-z0-9\-]+)\)'          # item slug link
    r'.{0,400}?'                            # thumbnail markup
    r'>([^<>]{2,80})<'                      # item name in anchor text
    r'(?:.{0,80}?(\d+)\s*(k|g|s))?',        # optional price
    re.S,
)

PRICE_RE = re.compile(r"(?<![\w>])(\d{1,4})\s*(k|g|s)\b")


def fetch(url: str) -> str:
    request = urllib.request.Request(url, headers={"User-Agent": UA})
    with urllib.request.urlopen(request, timeout=45) as response:
        return response.read().decode("utf-8", errors="replace")


def parse(page: str, slug: str) -> list[dict]:
    text = html.unescape(page)

    coords = ""
    match = COORDS_RE.search(text)
    if match:
        coords = f"{match.group(1)} / {match.group(2)}"

    npc_id = ""
    match = NPC_RE.search(text)
    if match:
        npc_id = match.group(1)

    # Vendor display name: first heading on the page.
    name_match = re.search(r"<h1[^>]*>(.*?)</h1>", text, re.S)
    vendor = (
        re.sub(r"<[^>]+>", "", name_match.group(1)).strip() if name_match else slug
    )

    rows = []
    seen = set()
    for match in ITEM_RE.finditer(text):
        item_slug, name = match.group(1), match.group(2).strip()
        if not name or item_slug in seen:
            continue
        if len(name) < 3 or name.lower().startswith(("wow housing", "decor sold")):
            continue
        seen.add(item_slug)

        # Price appears shortly after the name in the card text.
        tail = text[match.end() : match.end() + 120]
        price_match = PRICE_RE.search(tail)
        price = ""
        currency = "Gold"
        if price_match:
            amount, unit = int(price_match.group(1)), price_match.group(2)
            if unit == "k":
                amount *= 1000
            price = str(amount)
            currency = "Gold"
        rows.append(
            {
                "vendor": vendor,
                "item": name,
                "price": price,
                "currency": currency,
                "coords": coords,
                "npc_id": npc_id,
            }
        )
    return rows


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--vendors", required=True, help="one vendor slug per line")
    parser.add_argument("--out", default="sql/generated/vendor_stock.csv")
    parser.add_argument("--delay", type=float, default=1.5)
    args = parser.parse_args()

    with open(args.vendors, encoding="utf-8") as handle:
        slugs = [line.strip() for line in handle if line.strip() and not line.startswith("#")]

    os.makedirs(os.path.dirname(args.out) or ".", exist_ok=True)
    total = 0
    with open(args.out, "w", newline="", encoding="utf-8") as handle:
        writer = csv.DictWriter(
            handle,
            fieldnames=["vendor", "item", "price", "currency", "coords", "npc_id"],
        )
        writer.writeheader()
        for slug in slugs:
            try:
                page = fetch(BASE + slug)
            except urllib.error.URLError as error:
                print(f"skip {slug}: {error}", file=sys.stderr)
                continue
            rows = parse(page, slug)
            for row in rows:
                writer.writerow(row)
            total += len(rows)
            print(f"{slug}: {len(rows)} items")
            time.sleep(args.delay)

    print(f"wrote {args.out} ({total} rows)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
