#!/usr/bin/env python3
"""Run the mod-psychohouse addon in a 5.1 interpreter against the 8.3.7 model.

    python3 tools/housing/check_psychohouse_addon.py            # findings, exit 1 on any
    python3 tools/housing/check_psychohouse_addon.py --verbose  # plus every PASS line

The addon has never been in a client in this workspace and there is no client to put it in.
What can be done is run it: Lua 5.1 is the language 8.3.7 uses, the client API it touches is
the small set stated in tools/housing/wow_env.lua, and the protocol is written down in
drafts/mod-psychohouse/ADDON-PROTOCOL.md. test_psychohouse.lua loads the addon inside that
model, feeds it one line per record, clicks every button and sets every edit box.

This checks what the addon *does*. What the two sides *say* - the record names and field
counts in the document, in the C++ switch and in the Lua handlers - is check_psychohouse.py's
job; run both.

Needs a Lua 5.1 interpreter. It looks for lua5.1, lua51, luajit, then a binary built into
tools/housing/bin/. `tools/housing/build_lua.sh` builds one from source in a few seconds.
"""

import os
import shutil
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
ADDON = os.path.join(ROOT, "drafts", "mod-psychohouse", "addon", "PsychoHouse")

INTERPRETERS = ("lua5.1", "lua51", "lua-5.1", "luajit", "lua")


def find_lua():
    for name in INTERPRETERS:
        found = shutil.which(name)
        if found:
            return found

    local = os.path.join(HERE, "bin", "lua51")
    return local if os.path.exists(local) else None


def main():
    verbose = "--verbose" in sys.argv
    lua = find_lua()

    if not lua:
        print("no Lua 5.1 interpreter found - looked for %s and tools/housing/bin/lua51"
              % ", ".join(INTERPRETERS))
        print("build one with tools/housing/build_lua.sh, or install lua5.1")
        return 2

    if not os.path.isdir(ADDON):
        print("the addon is missing: " + ADDON)
        return 1

    env = dict(os.environ, PH_ADDON=ADDON)
    run = subprocess.run([lua, os.path.join(HERE, "test_psychohouse.lua")],
                         capture_output=True, text=True, env=env, cwd=ROOT)

    out = (run.stdout or "") + (run.stderr or "")

    findings, passed = [], 0
    for line in out.splitlines():
        if line.startswith("FAIL "):
            findings.append(line[5:])
        elif line.startswith("PASS "):
            passed += 1

    if run.returncode not in (0, 1):
        tail = out.strip().splitlines()
        findings.append("the run crashed (exit %d): %s"
                        % (run.returncode, tail[-1] if tail else "no output"))

    if verbose:
        print(out)

    print("{} check(s) passed, {} finding(s)".format(passed, len(findings)))
    for line in findings:
        print("  " + line)

    return 1 if findings else 0


if __name__ == "__main__":
    sys.exit(main())
