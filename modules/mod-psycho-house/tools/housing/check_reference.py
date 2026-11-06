#!/usr/bin/env python3
"""Check that every setting and every command is documented, and that the three
sources agree.

    python3 tools/housing/check_reference.py

Four lists have to match, and nothing in the repo was keeping them in step:

  settings   the keys in the conf dist, the keys LoadConfig records, and the keys in
             doc 14 section 1
  commands   the rows of the command table and the `.housing <name>` rows in doc 14
             section 3
  client     the keys in the addon's DEFAULTS table and doc 14 section 4
  help       that in-game help is read out of the command table rather than typed out

A setting that is in the conf and not in the loader is a key an operator can edit and
nothing reads. A command that is dispatched and not in the doc is a command nobody can
find. Both are invisible to every other checker here, which is why this one exists.
"""

import os
import re
import sys

HERE = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
DOC = os.path.join(HERE, "14-settings-and-commands.md")
MGREXTRA = os.path.join(HERE, "src", "game", "Housing", "HousingMgrExtra.cpp")
MGREXTRA_H = os.path.join(HERE, "src", "game", "Housing", "HousingMgr.h")
MGR = os.path.join(HERE, "src", "game", "Housing", "HousingMgr.cpp")
COMMANDS = os.path.join(HERE, "src", "scripts", "Housing", "housing_commands.cpp")
CORE = os.path.join(HERE, "addon", "HousingUI", "Core.lua")
DRAFTS = os.path.join(HERE, "drafts")

findings = []


def fail(what):
    findings.append(what)


def read(path):
    with open(path, encoding="utf-8") as handle:
        return handle.read()


def section(text, heading):
    """The body of a `## heading` section, up to the next `## `."""
    match = re.search(r"^##\s+" + re.escape(heading) + r"\s*$(.*?)(?=^##\s|\Z)",
                      text, re.S | re.M)
    return match.group(1) if match else ""


def conf_keys():
    keys = {}
    for draft in sorted(os.listdir(DRAFTS)):
        path = os.path.join(DRAFTS, draft, "conf", "mod-psycho-house.conf.dist")
        if not os.path.exists(path):
            continue
        found = set(re.findall(r"^(Housing\.[A-Za-z.]+)\s*=", read(path), re.M))
        keys[draft] = found
    return keys


def recorded_keys():
    """Keys LoadConfig writes into the snapshot, from the two note() helpers."""
    text = read(MGR)
    body = text.split("void HousingMgr::LoadConfig()")[1].split("\nvoid HousingMgr::")[0]
    return set(re.findall(r'note(?:Text)?\("(Housing\.[A-Za-z.]+)"', body))


def doc_keys():
    body = section(read(DOC), "1. The 29 server settings")
    return set(re.findall(r"^\|\s*`(Housing\.[A-Za-z.]+)`", body, re.M))


def command_rows():
    text = read(COMMANDS)
    block = text.split("static std::vector<ChatCommand> housingCommandTable")[1].split("};")[0]
    return [m.group(1) for m in re.finditer(r'^\s*\{\s*"([a-z]+)"', block, re.M)]


def doc_commands():
    body = section(read(DOC), "3. Every command")
    return set(re.findall(r"`\.housing ([a-z]+)`", body))


def client_keys():
    text = read(CORE)
    body = text.split("local DEFAULTS = {")[1].split("}", 1)[0]
    return set(re.findall(r"^\s*([A-Za-z]+)\s*=", body, re.M))


def doc_client():
    body = section(read(DOC), "4. The client settings")
    return set(re.findall(r"^\|\s*`([A-Za-z]+)`", body, re.M))


def main():
    # ---- settings ---------------------------------------------------------
    conf = conf_keys()
    if len(conf) < 2:
        fail("expected a conf dist in each draft, found {}".format(sorted(conf)))
    sets = list(conf.values())
    for draft, found in conf.items():
        if sets and found != sets[0]:
            fail("draft {} documents {} settings, draft {} documents {}".format(
                draft, len(found), sorted(conf)[0], len(sets[0])))

    everything = set().union(*sets) if sets else set()
    recorded = recorded_keys()
    documented = doc_keys()

    for key in sorted(everything - recorded):
        fail("{} is in the conf dist and LoadConfig never records it".format(key))
    for key in sorted(recorded - everything):
        fail("{} is recorded by LoadConfig and absent from the conf dist".format(key))
    for key in sorted(everything - documented):
        fail("{} is a setting and doc 14 section 1 does not list it".format(key))
    for key in sorted(documented - everything):
        fail("doc 14 lists {} and there is no such setting".format(key))

    # ---- commands ---------------------------------------------------------
    rows = command_rows()
    documented_rows = doc_commands()

    if len(rows) != len(set(rows)):
        fail("the command table has duplicate rows")
    for name in rows:
        if name not in documented_rows:
            fail(".housing {} is dispatched and doc 14 section 3 does not list it".format(name))
    for name in sorted(documented_rows - set(rows)):
        fail("doc 14 lists .housing {} and the table has no such row".format(name))

    # ---- client settings --------------------------------------------------
    client = client_keys()
    documented_client = doc_client()
    for key in sorted(client - documented_client):
        fail("addon setting {} is not in doc 14 section 4".format(key))
    for key in sorted(documented_client - client):
        fail("doc 14 lists addon setting {} and the addon has no such default".format(key))

    # ---- help comes out of the table -------------------------------------
    help_body = read(COMMANDS).split("static bool HandleHousingCommand")[1].split("\n    }")[0]
    if "getCommandTable()" not in help_body:
        fail("the .housing help text is typed out rather than read from the command table")

    # ---- the config command exists ---------------------------------------
    if '"config"' not in read(COMMANDS):
        fail("LoadConfig records a snapshot and no command prints it")

    print("{} settings ({} drafts), {} recorded, {} documented".format(
        len(everything), len(conf), len(recorded), len(documented)))
    print("{} commands, {} in the doc".format(len(rows), len(documented_rows)))
    print("{} client settings, {} in the doc".format(len(client), len(documented_client)))
    print("\n{} finding(s)".format(len(findings)))
    for line in findings:
        print("  " + line)
    return 1 if findings else 0


if __name__ == "__main__":
    sys.exit(main())
