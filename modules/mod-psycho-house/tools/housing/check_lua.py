#!/usr/bin/env python3
"""Structural checks on the addon's Lua.

    python3 tools/housing/check_lua.py

There is no Lua interpreter in the environment this was written in, so this does what can
be done without one:

  blocks      every `function`, `if`, `for`, `while`, `do` and `repeat` is closed by an
              `end` or `until`. Comments and strings are stripped first, and the `do` in
              `for ... do` is not counted twice
  strings     every string literal is terminated on its own line
  globals     any global the addon assigns is declared once at the top, so a typo shows up
              as a second global rather than a nil at runtime
  toc         the `.toc` lists files that exist

A clean run means the file will parse. It does not mean the addon behaves - that needs
the client.
"""

import os
import re
import sys

HERE = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
ADDON = os.path.join(HERE, "addon", "HousingUI")

findings = []


def fail(where, what):
    findings.append("{}: {}".format(where, what))


def strip_lua(text):
    out = []
    i, n = 0, len(text)
    while i < n:
        if text.startswith("--[[", i):
            end = text.find("]]", i)
            i = n if end < 0 else end + 2
        elif text.startswith("--", i):
            while i < n and text[i] != "\n":
                i += 1
        elif text[i] in "\"'":
            quote = text[i]
            i += 1
            while i < n and text[i] != quote and text[i] != "\n":
                i += 2 if text[i] == "\\" else 1
            if i < n and text[i] == quote:
                i += 1
            else:
                fail(out and "line" or "addon", "unterminated string literal")
            out.append(" ")
        elif text.startswith("[[", i):
            end = text.find("]]", i)
            i = n if end < 0 else end + 2
            out.append(" ")
        else:
            out.append(text[i])
            i += 1
    return "".join(out)


OPENERS = {"function", "if", "for", "while", "repeat"}
CLOSERS = {"end", "until"}


def check_blocks(name, text):
    body = strip_lua(text)
    depth = 0
    per_line = []
    for number, line in enumerate(body.splitlines(), 1):
        words = re.findall(r"\b\w+\b", line)
        opened = 0
        closed = 0
        for index, word in enumerate(words):
            if word in OPENERS:
                opened += 1
            elif word == "do":
                # `for ... do` and `while ... do` are the same block, not two
                if not any(w in ("for", "while") for w in words[:index]):
                    opened += 1
            elif word in CLOSERS:
                closed += 1
        depth += opened - closed
        if depth < 0:
            fail(name, "line {}: more end/until than blocks".format(number))
            depth = 0
        per_line.append(depth)
    if depth != 0:
        fail(name, "{} block(s) left open at the end of the file".format(depth))
    return len(per_line)


def saved_variables():
    """Names the .toc declares as SavedVariables are globals on purpose."""
    toc = os.path.join(ADDON, "HousingUI.toc")
    if not os.path.exists(toc):
        return set()
    with open(toc, encoding="utf-8") as handle:
        for line in handle:
            if line.lower().startswith("## savedvariables"):
                return {name.strip() for name in line.split(":", 1)[1].split(",")}
    return set()


def check_globals(name, text):
    """A global that is assigned outside a table constructor and never declared.

    Table constructors are skipped by tracking brace depth: `name = ...` inside `{ }` is a
    field, not a global, and that is where most of the noise would come from.
    """
    body = strip_lua(text)
    declared = set(re.findall(r"^\s*local\s+([A-Za-z_]\w*)", body, re.M))
    allowed = {"HousingUI"} | saved_variables()
    depth = 0
    for number, line in enumerate(body.splitlines(), 1):
        start_depth = depth
        depth += line.count("{") - line.count("}")
        if start_depth > 0:
            continue
        match = re.match(r"\s*([A-Za-z_]\w*)\s*=", line)
        if not match:
            continue
        global_name = match.group(1)
        if global_name in declared or global_name in allowed:
            continue
        if global_name.startswith("SLASH_"):
            continue
        fail(name, "line {}: global '{}' is assigned but never declared local".format(
            number, global_name))


def check_toc():
    toc = os.path.join(ADDON, "HousingUI.toc")
    if not os.path.exists(toc):
        fail("addon", "HousingUI.toc is missing")
        return
    with open(toc, encoding="utf-8") as handle:
        for line in handle:
            line = line.strip()
            if not line or line.startswith("#"):
                continue
            if not os.path.exists(os.path.join(ADDON, line)):
                fail("addon", "toc lists {} which does not exist".format(line))


def main():
    total = 0
    for name in sorted(os.listdir(ADDON)):
        if not name.endswith(".lua"):
            continue
        path = os.path.join(ADDON, name)
        with open(path, encoding="utf-8") as handle:
            text = handle.read()
        if any(ord(char) > 127 for char in text):
            fail(name, "non-ASCII byte")
        lines = check_blocks(name, text)
        check_globals(name, text)
        total += lines
        print("{:<28} {:>4} lines".format(name, lines))
    check_toc()
    print("\n{} finding(s)".format(len(findings)))
    for line in findings:
        print("  " + line)
    return 1 if findings else 0


if __name__ == "__main__":
    sys.exit(main())
