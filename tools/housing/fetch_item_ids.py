#!/usr/bin/env python3
"""
mod-psycho-house -- wowhead vendor item-id scraper

Fills in the item ids for the Town Center vendor stock. Names and prices are
already transcribed in data/town-center-stock.csv; this tool resolves each name
to its wowhead item id and rewrites the row.

Why a separate tool: wowhead's NPC "Sells" list renders 50 rows per page and the
pager is JavaScript, so a plain page fetch only ever returns the first 50. The
tool therefore tries, in order:

  1. the NPC page itself (fast path, 50 rows, resolves most small vendors)
  2. the NPC page with ?sells=N paging (works on some mirrors)
  3. the /items filter endpoint with &page=N
  4. a per-name search fallback (slow but complete)

Every resolved id is written with its source strategy so a wrong answer can be
traced to the strategy that produced it.

Usage:
    python3 tools/housing/fetch_item_ids.py \
        --vendors data/vendor_npc_ids.csv \
        --stock docs/mod-psycho-house/data/town-center-stock.csv \
        --out docs/mod-psycho-house/data/town-center-items.csv
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
import urllib.parse
import urllib.request

UA = (
    "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 "
    "(KHTML, like Gecko) Chrome/125.0 Safari/537.36"
)

# <a href="https://www.wowhead.com/item=245352/sturdy-wooden-coffin">Sturdy Wooden Coffin</a>
ROW_RE = re.compile(
    r'item=(\d{4,7})/[a-z0-9\-]+"\s*>(?:<[^>]+>)*([^<>]{2,90})<', re.S
)

PRICE_AFTER_RE = re.compile(r"\|\s*([\d,]+)\s*\|")


def fetch(url: str, timeout: int = 45) -> str:
    request = urllib.request.Request(url, headers={"User-Agent": UA})
    with urllib.request.urlopen(request, timeout=timeout) as response:
        return html.unescape(response.read().decode("utf-8", errors="replace"))


def parse_rows(page: str) -> dict[str, int]:
    """name (lowercased) -> item id, from one wowhead NPC page."""
    found: dict[str, int] = {}
    for match in ROW_RE.finditer(page):
        item_id, name = int(match.group(1)), match.group(2).strip()
        if len(name) < 3:
            continue
        found.setdefault(name.lower(), item_id)
    return found


def scrape_npc(npc_id: int, slug: str, delay: float) -> dict[str, int]:
    base = f"https://www.wowhead.com/npc={npc_id}/{slug}" if slug else f"https://www.wowhead.com/npc={npc_id}"
    merged: dict[str, int] = {}

    # Strategy 1+2: the NPC page and its ?sells=N variants.
    for url in (base, f"{base}?sells=2", f"{base}?sells=3", f"{base}?sells=4"):
        try:
            merged.update(parse_rows(fetch(url)))
        except urllib.error.URLError as error:
            print(f"  {url} -> {error}", file=sys.stderr)
        time.sleep(delay)

    # Strategy 3: the item filter endpoint.
    for page in (1, 2, 3):
        url = f"https://www.wowhead.com/items?filter=129;0;{npc_id}&page={page}"
        try:
            merged.update(parse_rows(fetch(url)))
        except urllib.error.URLError as error:
            print(f"  {url} -> {error}", file=sys.stderr)
        time.sleep(delay)

    return merged


def item_search(name: str, delay: float) -> int | None:
    """Strategy 4: resolve a single name through wowhead search."""
    query = urllib.parse.quote(name)
    url = f"https://www.wowhead.com/search?q={query}"
    try:
        page = fetch(url)
    except urllib.error.URLError:
        return None
    time.sleep(delay)
    normalised = re.sub(r"[^a-z0-9]", "", name.lower())
    for match in ROW_RE.finditer(page):
        item_id, found_name = int(match.group(1)), match.group(2)
        if re.sub(r"[^a-z0-9]", "", found_name.lower()) == normalised:
            return item_id
    return None


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--vendors", required=True, help="vendor,npc_id,slug CSV")
    parser.add_argument("--stock", required=True, help="vendor,item,price,currency,note CSV")
    parser.add_argument("--out", required=True)
    parser.add_argument("--delay", type=float, default=1.2)
    parser.add_argument("--search-fallback", action="store_true")
    args = parser.parse_args()

    npc_by_vendor: dict[str, tuple[int, str]] = {}
    with open(args.vendors, encoding="utf-8") as handle:
        for row in csv.DictReader(handle):
            if row.get("npc_id"):
                npc_by_vendor[row["vendor"]] = (int(row["npc_id"]), row.get("slug", ""))

    unresolved = 0
    out_rows = []
    cache: dict[str, dict[str, int]] = {}

    with open(args.stock, encoding="utf-8") as handle:
        for row in csv.DictReader(handle):
            vendor, name = row["vendor"], row["item"]
            if vendor not in npc_by_vendor:
                print(f"no npc id for vendor {vendor!r}, skipping", file=sys.stderr)
                unresolved += 1
                continue

            if vendor not in cache:
                npc_id, slug = npc_by_vendor[vendor]
                print(f"scraping {vendor} (npc {npc_id})")
                cache[vendor] = scrape_npc(npc_id, slug, args.delay)

            item_id = cache[vendor].get(name.lower())
            strategy = "npc-page"
            if item_id is None and args.search_fallback:
                item_id = item_search(name, args.delay)
                strategy = "search"
            if item_id is None:
                unresolved += 1
                strategy = "UNRESOLVED"

            out_rows.append(
                [vendor, item_id or "", name, row.get("price", ""), row.get("currency", ""), strategy]
            )

    os.makedirs(os.path.dirname(args.out) or ".", exist_ok=True)
    with open(args.out, "w", newline="", encoding="utf-8") as handle:
        writer = csv.writer(handle)
        writer.writerow(["vendor", "item_id", "item_name", "price", "currency", "source"])
        writer.writerows(out_rows)

    resolved = len(out_rows) - unresolved
    print(f"wrote {args.out}: {resolved} resolved, {unresolved} unresolved")
    if unresolved:
        print("re-run with --search-fallback for the slow per-name resolver", file=sys.stderr)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
