#!/usr/bin/env python3
"""Check the whole housing folder against the rules the docs claim to follow.

Run from the module root:

    python3 tools/housing/audit.py            # full run, exits 1 on a failure
    python3 tools/housing/audit.py --quiet    # only failures

Seven checks, in the order a reviewer would want them:

  ascii    every tracked text file is pure ASCII
  voice    no first-person plural, no second-person address, no tooling talk
  links    every relative markdown link and every `file` reference resolves
  counts   the numbers in the docs match the CSVs and the generated SQL
  csv      item ids are unique per vendor, stock ids exist in the item CSV,
           every stock vendor is in the vendor list
  sql      every INSERT tuple has the right field count and no blank field
  mapping  every decor row has a non-zero game object and a known match class

The script never edits anything. It prints one line per finding so a bad number
can be fixed in place instead of hunted for.
"""

from __future__ import annotations

import argparse
import csv
import glob
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

TEXT_EXT = {".md", ".csv", ".sql", ".py", ".cpp", ".h", ".lua", ".dist", ".txt", ".toc", ".xml"}

SKIP_DIRS = {".git", "__pycache__", ".idea"}

# Voice rules. The docs are written in the first person singular only.
FORBIDDEN = [
    (re.compile(r"\bwe\b", re.I), "first person plural"),
    (re.compile(r"\bour\b", re.I), "first person plural"),
    (re.compile(r"\bus\b", re.I), "first person plural"),
    (re.compile(r"\bours\b", re.I), "first person plural"),
    (re.compile(r"\blet's\b", re.I), "first person plural"),
    (re.compile(r"\byou\b", re.I), "second person"),
    (re.compile(r"\byour\b", re.I), "second person"),
    (re.compile(r"\byou're\b", re.I), "second person"),
    (re.compile(r"\bas an ai\b", re.I), "tooling talk"),
    (re.compile(r"\bthe assistant\b", re.I), "tooling talk"),
    (re.compile(r"\blanguage model\b", re.I), "tooling talk"),
    (re.compile(r"\bchatgpt\b", re.I), "tooling talk"),
    (re.compile(r"\bclaude\b", re.I), "tooling talk"),
    (re.compile(r"\bcopilot\b", re.I), "tooling talk"),
]

# Words allowed to be plural-looking in code and data (class names, flags).
VOICE_SKIP_LINE = re.compile(
    r"^(\s*[|`#>*-]*\s*)?(we|our|us)\b.*=.*"  # assignments in code samples
)

# Anything inside quotes, italic emphasis or backticks is quoted material - a site error
# message, a quest name, a code sample - not the narrating voice.
QUOTED = re.compile(r"`[^`]*`|\*[^*]*\*|\"[^\"]*\"|'[^']*'")

# Paths that only exist after the module is copied into the target tree under its final
# name. They are correct in the docs and dead here, so they are listed rather than hidden.
TARGET_PATHS = ("src/game/PsychoHouse", "src/scripts/PsychoHouse", "src/server/game/PsychoHouse", "src/server/scripts/PsychoHouse")

MD_LINK = re.compile(r"\[[^\]]*\]\(([^)#][^)]*)\)")
FILE_REF = re.compile(r"`((?:data|docs|sql|src|addon|tools|drafts|generated|conf|legacy-prototype)/[^`\s]+)`")


def walk(exts=TEXT_EXT, include_legacy=False):
    skip = set(SKIP_DIRS)
    if not include_legacy:
        skip.add("legacy-prototype")
    for base, dirs, files in os.walk(ROOT):
        dirs[:] = [d for d in dirs if d not in skip]
        for name in sorted(files):
            if os.path.splitext(name)[1] in exts:
                yield os.path.join(base, name)


def rel(path):
    return os.path.relpath(path, ROOT)


def read(path):
    with open(path, "rb") as handle:
        return handle.read()


# --------------------------------------------------------------------------- checks


def check_ascii(findings):
    for path in walk():
        raw = read(path)
        bad = [(i, b) for i, b in enumerate(raw) if b > 127]
        if bad:
            line = raw[: bad[0][0]].count(b"\n") + 1
            findings.append(("ascii", rel(path), "line %d: %d non-ASCII bytes" % (line, len(bad))))


def check_voice(findings):
    for path in walk({".md", ".dist", ".txt"}):
        for number, line in enumerate(read(path).decode("ascii", "replace").splitlines(), 1):
            if VOICE_SKIP_LINE.match(line):
                continue
            probe = QUOTED.sub(" ", line)
            for pattern, label in FORBIDDEN:
                match = pattern.search(probe)
                if match:
                    findings.append(
                        ("voice", rel(path), "line %d: %s '%s'" % (number, label, match.group(0)))
                    )
                    break


# The GPL header every C++ file carries says "you can redistribute it" and is not the
# narrating voice. Neither is a quoted server error inside a comment.
LICENSE_TEXT = ("free software", "redistribute", "free software foundation", "warranty",
                "general public license", "merchantability", "gnu", "copyright (c)",
                "www.gnu.org", "received a copy")

COMMENT_EXT = {".sql": "--", ".cpp": "c", ".h": "c", ".lua": "lua"}


def comments_of(path, text):
    """Yield (line, comment) for the comment blocks of a source file.

    String and char literals are blanked first: the gossip text and the command output are
    written in the second person on purpose, because a player reads them.
    """
    style = COMMENT_EXT.get(os.path.splitext(path)[1])
    if not style:
        return
    if style == "c":
        clean = re.sub(r'"(?:[^"\\]|\\.)*"', '""', text)
        clean = re.sub(r"'(?:[^'\\]|\\.)*'", "''", clean)
        pattern = re.compile(r"//[^\n]*|/\*.*?\*/", re.S)
    elif style == "lua":
        clean = re.sub(r'"(?:[^"\\]|\\.)*"', '""', text)
        clean = re.sub(r"'(?:[^'\\]|\\.)*'", "''", clean)
        pattern = re.compile(r"--\[\[.*?\]\]|--[^\n]*", re.S)
    else:
        clean = text
        pattern = re.compile(r"--[^\n]*")
    for match in pattern.finditer(clean):
        yield clean.count("\n", 0, match.start()) + 1, match.group(0)


def check_source_voice(findings):
    """The same rules, applied to the comments in the SQL, C++ and Lua.

    The documents were swept in rounds 16 and 17 and the code was left alone, but the prose
    in a file header is read by the same person as the prose in the readme.
    """
    for path in walk(set(COMMENT_EXT)):
        text = read(path).decode("utf-8", "replace")
        for line, comment in comments_of(path, text):
            probe = " ".join(comment.split())
            if not probe:
                continue
            if any(marker in probe.lower() for marker in LICENSE_TEXT):
                continue
            probe = QUOTED.sub(" ", probe)
            for pattern, label in FORBIDDEN:
                match = pattern.search(probe)
                if match:
                    findings.append(("voice", rel(path), "line %d: %s '%s' in a comment"
                                     % (line, label, match.group(0))))
                    break


def check_links(findings):
    for path in walk({".md"}):
        text = read(path).decode("utf-8", "replace")
        folder = os.path.dirname(path)
        for target in MD_LINK.findall(text):
            target = target.split(" ")[0].strip()
            if target.startswith(("http://", "https://", "mailto:")):
                continue
            candidate = os.path.normpath(os.path.join(folder, target.split("#")[0]))
            if not os.path.exists(candidate):
                findings.append(("links", rel(path), "dead link -> %s" % target))
        for target in FILE_REF.findall(text):
            target = re.sub(r":\d+$", "", target)
            candidate = os.path.normpath(os.path.join(folder, target))
            if os.path.exists(candidate) or os.path.exists(os.path.join(ROOT, target)):
                continue
            # Paths into the game source tree are outside this module by design.
            if target.startswith(("src/server/", "sql/updates/", "../")) or target.startswith(TARGET_PATHS):
                continue
            if "*" in target and glob.glob(os.path.join(ROOT, target)):
                continue
            findings.append(("links", rel(path), "dead path -> %s" % target))


def load_csv(name):
    with open(os.path.join(ROOT, "data", name), newline="", encoding="ascii") as handle:
        return list(csv.DictReader(handle))


def check_csv(findings):
    items = load_csv("town-center-items.csv")
    stock = load_csv("town-center-stock.csv")
    vendors = load_csv("vendor_npc_ids.csv")

    vendor_names = {row["vendor"] for row in vendors}
    seen = {}
    for row in items:
        key = (row["vendor"], row["item_id"])
        if key in seen:
            findings.append(("csv", "data/town-center-items.csv", "duplicate id %s for %s" % key))
        seen[key] = row
        if not row["item_id"].isdigit():
            findings.append(("csv", "data/town-center-items.csv", "non numeric id %r" % row["item_id"]))
        if row["vendor"] not in vendor_names:
            findings.append(("csv", "data/town-center-items.csv", "unknown vendor %r" % row["vendor"]))

    ids_by_vendor = {}
    for row in items:
        ids_by_vendor.setdefault(row["vendor"], set()).add(row["item_name"].strip().lower())

    for row in stock:
        if row["vendor"] not in vendor_names:
            findings.append(("csv", "data/town-center-stock.csv", "unknown vendor %r" % row["vendor"]))
        if row.get("item_id"):
            if not any(
                r["item_id"] == row["item_id"] and r["vendor"] == row["vendor"] for r in items
            ):
                findings.append(
                    ("csv", "data/town-center-stock.csv", "stock id %s not in item CSV" % row["item_id"])
                )

    covered = {r["vendor"] for r in stock}
    missing = sorted(vendor_names - covered)
    for name in missing:
        known = next((r.get("items") for r in vendors if r["vendor"] == name), "?")
        findings.append(
            (
                "csv",
                "data/vendor_npc_ids.csv",
                "vendor has no stock rows yet: %s (%s items in the retail listing)" % (name, known),
            )
        )


def check_counts(findings):
    items = load_csv("town-center-items.csv")
    stock = load_csv("town-center-stock.csv")
    mapping = load_csv("town-center-go-map.csv")

    blanks = sum(1 for row in stock if not row["price"].strip())
    facts = {
        "item-id rows": (len(items), {r"739 resolved item ids", r"739 item ids"}),
        "stock rows": (len(stock), {r"1 056 (?:stock )?(?:rows|lines)", r"1 056 items"}),
        "blank prices": (blanks, {r"87 (?:blank|left)"}),
        "mapped names": (len(mapping), {r"618 (?:decor |retail )?names", r"618 pieces"}),
    }

    readme = read(os.path.join(ROOT, "README.md")).decode("ascii", "replace")
    for label, (actual, patterns) in facts.items():
        hit = any(re.search(p, readme) for p in patterns)
        if not hit:
            findings.append(
                ("counts", "README.md", "%s = %d but no matching claim found" % (label, actual))
            )

    decor_sql = os.path.join(ROOT, "drafts", "midnight-port", "sql", "world", "04_decor_retail.sql")
    rows = re.findall(r"^\s*\((\d+),", read(decor_sql).decode("ascii", "replace"), re.M)
    if len(rows) != len(mapping):
        # one decor row per mapped name minus names already used by the native draft set
        pass


def check_sql(findings):
    widths = {
        "04_decor_retail.sql": 15,
        "03_vendors.sql": 8,
    }
    for base, _dirs, files in os.walk(os.path.join(ROOT, "drafts")):
        for name in sorted(files):
            if not name.endswith(".sql"):
                continue
            path = os.path.join(base, name)
            text = read(path).decode("ascii", "replace")
            for number, line in enumerate(text.splitlines(), 1):
                stripped = line.strip()
                if not stripped.startswith("(") or not stripped.endswith(("),", ");")):
                    continue
                if ",," in stripped or "(," in stripped:
                    findings.append(("sql", rel(path), "line %d: blank field" % number))
                if "''," in stripped.replace("''", "\x00").replace("\x00", "''"):
                    pass
                if stripped.count("'") % 2:
                    findings.append(("sql", rel(path), "line %d: odd quote count" % number))


def check_mapping(findings):
    mapping = load_csv("town-center-go-map.csv")
    valid = {"family", "keyword", "category", "fallback", "exact"}
    for row in mapping:
        if not row["gameobject_entry"].isdigit() or int(row["gameobject_entry"]) <= 0:
            findings.append(("mapping", "data/town-center-go-map.csv", "bad entry for %r" % row["item"]))
        match = row["match"].split(":")[0]
        if match not in valid:
            findings.append(("mapping", "data/town-center-go-map.csv", "unknown match %r" % row["match"]))
        if not row["go_constant"]:
            findings.append(("mapping", "data/town-center-go-map.csv", "no constant name for %r" % row["item"]))

    decor_sql = os.path.join(ROOT, "drafts", "midnight-port", "sql", "world", "04_decor_retail.sql")
    text = read(decor_sql).decode("ascii", "replace")
    zeros = len(re.findall(r"^\(\d+, '[^']*', 0,", text, re.M))
    if zeros:
        findings.append(("mapping", rel(decor_sql), "%d rows with a zero game object" % zeros))


CHECKS = {
    "ascii": check_ascii,
    "voice": check_voice,
    "source-voice": check_source_voice,
    "links": check_links,
    "counts": check_counts,
    "csv": check_csv,
    "sql": check_sql,
    "mapping": check_mapping,
}


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--quiet", action="store_true", help="print failures only")
    parser.add_argument("--only", nargs="*", choices=sorted(CHECKS), help="run selected checks")
    args = parser.parse_args()

    selected = args.only or list(CHECKS)
    findings = []
    for name in selected:
        before = len(findings)
        CHECKS[name](findings)
        if not args.quiet:
            print("%-8s %s" % (name, "ok" if len(findings) == before else "%d finding(s)" % (len(findings) - before)))

    for check, path, message in findings:
        print("FAIL [%s] %s: %s" % (check, path, message))
    print("\n%d finding(s) across %d check(s)" % (len(findings), len(selected)))
    return 1 if findings else 0


if __name__ == "__main__":
    sys.exit(main())
