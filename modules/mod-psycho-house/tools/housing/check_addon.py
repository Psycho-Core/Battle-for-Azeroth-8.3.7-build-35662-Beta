#!/usr/bin/env python3
"""Run the shipped addon in a 5.1 interpreter against a model of the 8.3.7 client.

The addon has never been in a client in this workspace, and no client exists to put it in.
What can be done is run it: Lua 5.1 is the language 8.3.7 uses, the client API the addon
touches is small and stated in wow_env.lua, and the protocol is written down in doc 15. So
this runs Core.lua and Menu.lua inside that model, feeds them one line per server opcode from
the document, drives every slash command and clicks every button on every tab, and compares
what the addon actually did with what doc 15 says it should.

    python3 tools/housing/check_addon.py            # findings, exit 1 on any
    python3 tools/housing/check_addon.py --verbose  # plus the coverage report

Needs a Lua 5.1 interpreter. It looks for lua5.1, lua51, luajit, then a binary built into
tools/housing/bin/. `tools/housing/build_lua.sh` builds one from source in a second.
"""

import os
import re
import shutil
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
ADDON = os.path.join(ROOT, "addon", "HousingUI")
DOC = os.path.join(ROOT, "15-addon-protocol.md")

INTERPRETERS = ("lua5.1", "lua51", "lua-5.1", "luajit", "lua")

# How a field in doc 15's tables turns into a value on the wire. The document already says
# which field is a name and which is a number; this only decides what a plausible value is.
TEXT_HINTS = ("name", "text", "code", "riddle", "fragment", "owner", "op", "house name")
NUMBER_HINTS = ("id", "index", "count", "price", "cost", "budget", "tier", "level", "slot",
                "state", "biome", "scale", "x", "y", "z", "o", "pitch", "roll", "flags",
                "page", "more", "mask", "type", "preset", "protocol", "version", "total",
                "seq", "frame", "ms", "milliseconds", "cooldown", "progress", "required",
                "entry", "roaming", "selected", "size", "unlocked", "locked", "enabled",
                "indoor", "outdoor", "colour", "currency", "placement", "surface",
                "subcategory", "cells", "floor", "ends", "milestone", "quest", "owned",
                "code")


def find_lua():
    for name in INTERPRETERS:
        path = shutil.which(name)
        if path:
            return path
    local = os.path.join(HERE, "bin", "lua51")
    if os.path.exists(local):
        return local
    return None


def doc_server_ops():
    """Every `S:` opcode in doc 15 section 7, with the field list it documents."""
    ops = []
    with open(DOC, encoding="utf-8") as handle:
        for line in handle:
            match = re.match(r"^\| `S:([A-Z]+)` \| (.*?) \| ", line)
            if match:
                ops.append((match.group(1), match.group(2)))
    return ops


def doc_client_ops():
    """Every `C:` opcode in doc 15 section 6."""
    ops = []
    with open(DOC, encoding="utf-8") as handle:
        for line in handle:
            match = re.match(r"^\| `C:([A-Z]+)` \| (.*?) \| ", line)
            if match:
                ops.append((match.group(1), match.group(2)))
    return ops


def sample_value(op, position, description):
    lowered = description.lower()
    if description.strip() == "-":
        return None

    if position == 2 and op == "RESULT":
        return "BUY"
    if "->" in lowered or lowered.startswith("code in"):
        return None
    if "server name" in lowered:
        return "Test"

    for hint in TEXT_HINTS:
        if re.search(r"\b" + re.escape(hint), lowered):
            if position == 5 and op == "CHUNK":
                return "S:PONG|21"
            return "Test"
    return "7"


def sample_line(op, spec):
    """A wire line for one opcode: op, then one value per documented field."""
    fields = [op]
    for index, chunk in enumerate(spec.split(",")):
        match = re.match(r"^\s*(\d+):\s*(.*?)(?:\s*\[.*\])?(?:\s*\(.*\))?\s*$", chunk)
        if not match:
            continue
        position = int(match.group(1))
        description = match.group(2)
        value = sample_value(op, position, description)
        if value is None:
            continue
        while len(fields) < position:
            fields.append("0")
        fields.append(value)

    if op == "CHUNK":
        # A single-chunk frame whose fragment is the whole PONG line.
        return "S:CHUNK|1|1|1|S:PONG|21"
    if op == "RESULT":
        return "S:RESULT|BUY|0|31"
    if op == "BPCODE":
        return "S:BPCODE|PHP1:0000000000abcd"
    if op == "PLOTINFO":
        return "S:PLOTINFO|3|1|250000|1|2|4|Thordekk|Stormhold|0|300|600|120|240|3|5|2"
    if op == "HUNT":
        return "S:HUNT|3|1|The Lost Ladle|It hangs where the hearth is cold|Ladle Rack|9001"
    if op == "DECORENTRY":
        return "S:DECORENTRY|1500|Orgrimmar Banner|5|2|1|1|120|0|Banners"
    if op == "PLACEDENTRY":
        return "S:PLACEDENTRY|44|1500|1.5|2.5|0.25|90.0|1.0|1|0.1|-0.2|0|0"
    if op == "ROOM":
        return "S:ROOM|4|Great Hall|3|1|0|2|2"
    if op == "ENDEAVOR":
        return "S:ENDEAVOR|12|2|1|3|Hearth and Home|1900000000"
    if op == "HOMES":
        return "S:HOMES|77|3|12|Stormhold|1|2"
    if op == "HELLO":
        return "S:HELLO|3|1041|1|1"
    if op == "NBR":
        return "S:NBR|12|1|Stormhold|3|8|24|1"

    return "|".join(fields)


def write_plan(ops, path, client_ops):
    lines = ["-- generated by check_addon.py from 15-addon-protocol.md", "PLAN = {"]
    for op, spec in ops:
        lines.append('    { op = "%s", line = "%s" },' % (op, sample_line(op, spec)))

    # Which client opcodes have an argument the document does not bracket as optional. The
    # test uses this to demand a usage line rather than an empty field.
    lines.append("}")
    lines.append("CLIENT = {")
    for op, spec in client_ops:
        first = spec.split(",")[0].strip()
        optional = first.startswith("-") or "[" in first
        lines.append('    { op = "%s", optional = %s },' % (op, "true" if optional else "false"))
    lines.append("}")
    with open(path, "w", encoding="utf-8") as handle:
        handle.write("\n".join(lines) + "\n")


def main():
    verbose = "--verbose" in sys.argv
    lua = find_lua()

    if not lua:
        print("no Lua 5.1 interpreter found - looked for %s and tools/housing/bin/lua51"
              % ", ".join(INTERPRETERS))
        print("build one with tools/housing/build_lua.sh, or install lua5.1")
        return 2

    version = subprocess.run([lua, "-v"], capture_output=True, text=True).stderr
    ops = doc_server_ops()
    client_ops = doc_client_ops()

    plan_path = os.path.join("/tmp", "housing_plan.lua")
    write_plan(ops, plan_path, client_ops)

    env = dict(os.environ, HOUSING_PLAN=plan_path, HOUSING_ADDON=ADDON)
    run = subprocess.run([lua, os.path.join(HERE, "test_addon.lua")],
                         capture_output=True, text=True, env=env, cwd=ROOT)

    out = run.stdout + run.stderr
    if verbose:
        print(out)

    findings = []
    passed = 0
    for line in out.splitlines():
        if line.startswith("FAIL "):
            findings.append(line[5:])
        elif line.startswith("PASS "):
            passed += 1

    if run.returncode not in (0, 1):
        findings.append("the addon run crashed: " + out.strip().splitlines()[-1])

    sent = set()
    handled = set()
    for line in out.splitlines():
        if line.startswith("SENT "):
            sent.update(line[5:].split())
        elif line.startswith("HANDLED "):
            handled.update(line[8:].split())

    documented_client = {op for op, _ in client_ops}
    documented_server = {op for op, _ in ops}

    # Execution against the document, both ways.
    for op in sorted(sent - documented_client):
        findings.append("the addon sends C:%s, which doc 15 does not document" % op)
    for op in sorted(handled - documented_server):
        findings.append("the addon handles S:%s, which doc 15 does not document" % op)

    unreached = sorted(documented_client - sent)
    unhandled = sorted(documented_server - handled)

    print("lua: %s" % version.strip())
    print("%d checks passed, %d finding(s)" % (passed, len(findings)))
    print("client opcodes exercised: %d/%d" % (len(sent & documented_client), len(documented_client)))
    print("server opcodes dispatched: %d/%d" % (len(handled & documented_server), len(documented_server)))

    if unreached:
        print("documented, not sent by the sweep: " + ", ".join("C:" + op for op in unreached))
    if unhandled:
        print("documented, never dispatched: " + ", ".join("S:" + op for op in unhandled))

    for finding in findings:
        print("finding: " + finding)

    return 1 if findings else 0


if __name__ == "__main__":
    sys.exit(main())
