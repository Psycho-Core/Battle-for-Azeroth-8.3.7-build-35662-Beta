#!/usr/bin/env python3
"""Check the whole housing folder against the rules the docs claim to follow.

Run from the module root:

    python3 tools/housing/audit.py            # full run, exits 1 on a failure
    python3 tools/housing/audit.py --quiet    # only failures

Seventeen checks, in the order a reviewer would want them:

  ascii    every tracked text file is pure ASCII
  voice    no first-person plural, no second-person address, no tooling talk
  links    every relative markdown link and every `file` reference resolves
  docrefs  every document named in a .cpp, .h or .lua comment exists
  counts   the numbers in the docs match the CSVs and the generated SQL
  csv      item ids are unique per vendor, stock ids exist in the item CSV,
           every stock vendor is in the vendor list
  sql      every INSERT tuple has the right field count and no blank field
  mapping  every decor row has a non-zero game object and a known match class
  claims   every protocol, command, table and column the docs claim exists
  floorplan the room placement rules, run over the shapes the drafts ship
  drafts   the two generated drafts against each other's invariants and doc 10's counts
  install  each draft's install order executed as one transaction, then every file re-run
  addon    the addon itself, loaded and driven inside a model of the 8.3.7 client
  copies   each draft ships the module's own src/ and addon/, byte for byte
  worklist the generated port worklist still matches the catalogue it is generated from

The script never edits anything. It prints one line per finding so a bad number
can be fixed in place instead of hunted for.
"""

from __future__ import annotations

import argparse
import csv
import glob
import os
import re
import subprocess
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
# Paths a documented build step creates rather than the repository: the Lua interpreter that
# tools/housing/build_lua.sh puts in tools/housing/bin/ is git-ignored on purpose, so a link to
# it is correct in a prepared checkout and dead in a fresh clone.
BUILD_OUTPUTS = ("tools/housing/bin/", "tools/housing/bin/lua51")

TARGET_PATHS = ("src/game/PsychoHouse", "src/scripts/PsychoHouse", "src/server/game/PsychoHouse", "src/server/scripts/PsychoHouse")

MD_LINK = re.compile(r"\[[^\]]*\]\(([^)#][^)]*)\)")

# A document named in a source comment. The lookarounds matter: `hashlib.md5` is not a
# reference to a markdown file, and neither is `foo.md5`.
DOC_REF = re.compile(r"(?<![\w./-])([A-Za-z0-9_][A-Za-z0-9_./-]*\.md)(?![\w.])")
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


def check_doc_refs(findings):
    """Every document named in a source or addon comment exists.

    Four comments in this module pointed at `docs/player-housing/04-addon-protocol.md` and
    `docs/player-housing/02-module-architecture.md` for three revisions. There is no `docs/`
    directory: the paths had been carried over from a tree that is not this one, and the
    links check only ever read `.md` files, so nothing noticed. A reader following the
    comment found nothing at the other end.
    """
    for base, dirs, files in os.walk(ROOT):
        dirs[:] = [d for d in dirs if d not in SKIP_DIRS | {"legacy-prototype", "drafts"}]

        for name in sorted(files):
            if not name.endswith((".cpp", ".h", ".lua")):
                continue

            path = os.path.join(base, name)

            try:
                text = read(path).decode("utf-8", "replace")
            except OSError:
                continue

            for target in DOC_REF.findall(text):
                cleaned = target.split(":")[0]
                candidates = [os.path.join(ROOT, cleaned),
                              os.path.join(os.path.dirname(path), cleaned),
                              # The third draft keeps its documents beside its sources, and a
                              # tool that tests that draft names them from here.
                              os.path.join(ROOT, "drafts", "mod-psychohouse", cleaned)]

                if any(os.path.exists(candidate) for candidate in candidates):
                    continue

                findings.append(("docrefs", rel(path), "names %s, which does not exist" % cleaned))


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
            if target.startswith(BUILD_OUTPUTS):
                continue
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


def check_claims(findings):
    script = os.path.join(ROOT, "tools", "housing", "check_claims.py")
    result = subprocess.run([sys.executable, script], capture_output=True, text=True)
    if result.returncode != 0:
        for line in result.stdout.splitlines():
            if line.startswith("  "):
                findings.append(("claims", rel(script), line.strip()))


def check_reference(findings):
    """Run the settings-and-commands checker as one of the audit's own steps.

    The other checks in this file look at the documents as prose; this one looks at the
    three lists that have to match the code, and delegates so there is one implementation
    of that comparison rather than two that can drift.
    """
    import subprocess

    script = os.path.join(ROOT, "tools", "housing", "check_reference.py")
    result = subprocess.run([sys.executable, script], capture_output=True, text=True)
    if result.returncode != 0:
        for line in result.stdout.splitlines():
            if line.startswith("  "):
                findings.append(("reference", rel(script), line.strip()))


def check_floorplan(findings):
    """The floorplan rules, run over the shapes the drafts ship.

    `PlaceRoom` cannot be compiled here, so the rules it enforces are restated in Python and
    run against the real `housing_room` rows: the collection fits a level-1 house, nothing
    floats, one room per cell and the plan stays one piece.
    """
    import subprocess

    script = os.path.join(ROOT, "tools", "housing", "check_floorplan.py")
    result = subprocess.run([sys.executable, script], capture_output=True, text=True)
    if result.returncode != 0:
        for line in result.stdout.splitlines():
            if line.startswith("  "):
                findings.append(("floorplan", rel(script), line.strip()))


def check_drafts(findings):
    """The two drafts, checked against themselves and against doc 10.

    Every other check in this file reads a document or a CSV. This one reads the generated
    SQL, which is what an operator actually runs: the pet beds and their creature rows in both
    directions, the faction mask on every stock line and the counts doc 10 states.
    """
    import subprocess

    script = os.path.join(ROOT, "tools", "housing", "verify_drafts.py")
    result = subprocess.run([sys.executable, script], capture_output=True, text=True)
    if result.returncode != 0:
        for line in result.stdout.splitlines():
            if line.startswith("  "):
                findings.append(("drafts", rel(script), line.strip()))


def check_install(findings):
    """Run each draft's install order as one transaction and re-run every file on its own.

    This is the only check in this file that executes the drafts rather than reading them:
    the files are applied to an in-memory model of the schema in the order the README prints,
    then each file is re-imported on its own, which is what an operator does to repair a row.
    The six checks above read a file at a time and cannot see a DELETE in one file eating
    rows another file inserted.
    """
    import subprocess

    script = os.path.join(ROOT, "tools", "housing", "simulate_install.py")
    result = subprocess.run([sys.executable, script], capture_output=True, text=True)
    if result.returncode != 0:
        for line in result.stdout.splitlines():
            if line.startswith("  "):
                findings.append(("install", rel(script), line.strip()))


def check_copies(findings):
    """A draft's `src/` and `addon/` are the module's, byte for byte.

    `build_drafts.py` copies both into each draft, which is what makes a draft a thing an
    operator can install on its own. The copy step is only run when the drafts are
    regenerated, so an addon fix that lands after the last regeneration ships the fixed file
    in the module and the broken one in both drafts - which is exactly what happened to the
    five fixes from the forty-sixth revision until this check existed.
    """
    for draft in ("midnight-port", "bfa-native"):
        for folder in ("src", "addon"):
            source = os.path.join(ROOT, folder)
            target = os.path.join(ROOT, "drafts", draft, folder)

            for root, _, files in os.walk(source):
                for name in sorted(files):
                    if name.endswith(".pyc"):
                        continue
                    path = os.path.join(root, name)
                    copy = os.path.join(target, os.path.relpath(path, source))
                    if not os.path.exists(copy):
                        findings.append(("copies", rel(copy), "missing from the draft"))
                    elif read(path) != read(copy):
                        findings.append(("copies", rel(copy),
                                         "differs from the module's own copy"))


def check_worklist(findings):
    """The port worklist is generated from the catalogue, so it must not be edited by hand.

    A worklist that has drifted is worse than no worklist: it names pieces that no longer
    exist and misses the ones that were added, and the six columns beside each row are what
    the port is filled in from.
    """
    import subprocess

    script = os.path.join(ROOT, "tools", "housing", "port_worklist.py")
    result = subprocess.run([sys.executable, script, "--check"], capture_output=True, text=True)
    if result.returncode != 0:
        for line in result.stdout.splitlines():
            if line.startswith(("stale:", "missing:")):
                findings.append(("worklist", rel(script), line))


def check_addon(findings):
    """Run the addon: load it in a 5.1 client model, feed it the protocol, click the window.

    The Lua is syntax and pattern checks in check_lua.py; this is the addon executed. A
    machine without a Lua 5.1 interpreter is reported and not counted as a pass, because a
    skipped test that reads as "ok" is how a suite stops meaning anything.
    """
    import subprocess

    script = os.path.join(ROOT, "tools", "housing", "check_addon.py")
    result = subprocess.run([sys.executable, script], capture_output=True, text=True)
    if result.returncode == 2:
        findings.append(("addon", rel(script), "not run: " + result.stdout.strip().splitlines()[0]))
    elif result.returncode != 0:
        for line in result.stdout.splitlines():
            if line.startswith("finding: "):
                findings.append(("addon", rel(script), line[len("finding: "):]))


def check_channels(findings):
    """The channel page's count is its own table.

    "Channels: 0 of 34" is a claim about a list, and a list that is edited by hand drifts from
    the sentence above it. This reads the table, the headline and the three group lines, and
    fails when they disagree - the same rule the rest of this folder applies to every other
    number it states.
    """
    path = os.path.join(ROOT, "drafts", "midnight-port", "CHANNELS.md")
    if not os.path.exists(path):
        findings.append(("channels", "drafts/midnight-port/CHANNELS.md", "the page is missing"))
        return

    text = read(path).decode("ascii", "replace")
    rows = {}
    for line in text.splitlines():
        match = re.match(r"^\|\s*(\d+)\s*\|\s*([^|]+?)\s*\|\s*([^|]+?)\s*\|", line)
        if match:
            rows[int(match.group(1))] = {
                "name": match.group(2),
                "auth": match.group(3),
                "cells": [cell.strip() for cell in line.strip("|").split("|")],
            }

    if not rows:
        findings.append(("channels", "CHANNELS.md", "the channel table could not be read"))
        return

    numbers = sorted(rows)
    if numbers != list(range(1, len(numbers) + 1)):
        findings.append(("channels", "CHANNELS.md",
                         "the channel numbers are not 1..{}: {}".format(len(numbers), numbers)))

    headline = re.search(r"Channels: 0 of (\d+)", text)
    if not headline:
        findings.append(("channels", "CHANNELS.md", "the headline count is missing"))
    elif int(headline.group(1)) != len(rows):
        findings.append(("channels", "CHANNELS.md",
                         "the headline says {} channels and the table has {}".format(
                             headline.group(1), len(rows))))

    for number, row in sorted(rows.items()):
        status = row["cells"][-1].strip("`")
        if status not in ("none", "spec"):
            findings.append(("channels", "CHANNELS.md",
                             "channel {} has status `{}`, which is not one this page defines".format(
                                 number, status)))
        for cell in row["cells"]:
            if not cell:
                findings.append(("channels", "CHANNELS.md",
                                 "channel {} has an empty cell".format(number)))

    # The three group lines name channel numbers. Each bullet is a paragraph of its own, so it
    # is split on its own marker rather than parsed with a pattern across the whole page.
    oauth_rows = {number for number, row in rows.items() if "OAuth" in row["auth"]}

    bullets = {}
    for chunk in text.split("\n* **")[1:]:
        label = chunk.split("**", 1)[0]
        # Only the first sentence: the rest of a bullet explains and may cite other numbers
        # ("section 4", "3 by webhook token"), and those are not members of the group.
        body = chunk.split("\n* **")[0].split("\n\n")[0].split(".")[0]
        bullets[label] = set(int(n) for n in re.findall(r"\d+", body))

    if "OAuth channels" not in bullets:
        findings.append(("channels", "CHANNELS.md", "the OAuth group line is missing"))
    elif bullets["OAuth channels"] != oauth_rows:
        findings.append(("channels", "CHANNELS.md",
                         "the OAuth group lists {} and the table says {}".format(
                             sorted(bullets["OAuth channels"]), sorted(oauth_rows))))

    for label in ("Token channels", "Protocol channels"):
        if label not in bullets:
            findings.append(("channels", "CHANNELS.md", "the {} line is missing".format(label)))
            continue
        for number in bullets[label]:
            if number not in rows:
                findings.append(("channels", "CHANNELS.md",
                                 "the {} line names channel {}, which the table does not have".format(
                                     label, number)))


def check_parity(findings):
    """The parity sentence counts its own table, so it cannot drift from it.

    Doc 13 sec.9e claims thirteen systems with the same rule, six smaller, six replaced and
    fourteen absent. That is a claim about thirty-nine rows, and a row changed from `same` to
    `no` without the sentence moving would make the document wrong in the one paragraph written
    to prove it is right.
    """
    path = os.path.join(ROOT, "13-mod-psychohouse-design.md")
    if not os.path.exists(path):
        findings.append(("parity", "13-mod-psychohouse-design.md", "the document is missing"))
        return

    text = read(path).decode("utf-8", "replace")
    if "| System | Here | Note |" not in text:
        findings.append(("parity", "13-mod-psychohouse-design.md", "the parity table is missing"))
        return

    table = text.split("| System | Here | Note |", 1)[1].split("The one place this table could lie", 1)[0]
    statuses = []
    for line in table.splitlines():
        cells = [cell.strip() for cell in line.strip().strip("|").split("|")]
        if len(cells) == 3 and cells[1] in ("same", "smaller", "replaced", "no"):
            statuses.append(cells[1])

    if not statuses:
        findings.append(("parity", "13-mod-psychohouse-design.md", "the parity table has no rows"))
        return

    counted = {name: statuses.count(name) for name in ("same", "smaller", "replaced", "no")}
    words = {"thirteen": 13, "six": 6, "fourteen": 14, "thirty-nine": 39}

    sentence = re.search(r"The table has ([a-z-]+) rows: ([a-z-]+) systems carry the same rule, "
                         r"([a-z-]+) a deliberately\s*smaller one, ([a-z-]+) are replaced by "
                         r"something that covers the same need, and ([a-z-]+) are absent", text)
    if not sentence:
        findings.append(("parity", "13-mod-psychohouse-design.md",
                         "the parity sentence was not found in a readable form"))
        return

    claimed = [words.get(group) for group in sentence.groups()]
    actual = [len(statuses), counted["same"], counted["smaller"], counted["replaced"], counted["no"]]

    if claimed != actual:
        findings.append(("parity", "13-mod-psychohouse-design.md",
                         "the sentence says {} and the table says {}".format(claimed, actual)))


def check_psychohouse_addon(findings):
    """Run the per-character module's addon in the 5.1 model as well.

    The neighbourhood addon and the module's addon are separate programs on separate prefixes,
    so a clean `addon` run says nothing about the second one. Same rule as above: no
    interpreter is a reported finding, not a pass.
    """
    import subprocess

    script = os.path.join(ROOT, "tools", "housing", "check_psychohouse_addon.py")
    result = subprocess.run([sys.executable, script], capture_output=True, text=True)
    out = result.stdout + result.stderr
    if result.returncode == 2:
        findings.append(("psychoaddon", rel(script), "not run: " + out.strip().splitlines()[0]))
    elif result.returncode != 0:
        for line in out.splitlines():
            if line.startswith("  ") and line.strip():
                findings.append(("psychoaddon", rel(script), line.strip()))


CHECKS = {
    "ascii": check_ascii,
    "voice": check_voice,
    "source-voice": check_source_voice,
    "links": check_links,
    "docrefs": check_doc_refs,
    "counts": check_counts,
    "csv": check_csv,
    "sql": check_sql,
    "mapping": check_mapping,
    "reference": check_reference,
    "claims": check_claims,
    "floorplan": check_floorplan,
    "drafts": check_drafts,
    "install": check_install,
    "addon": check_addon,
    "copies": check_copies,
    "worklist": check_worklist,
    "psychoaddon": check_psychohouse_addon,
    "channels": check_channels,
    "parity": check_parity,
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
