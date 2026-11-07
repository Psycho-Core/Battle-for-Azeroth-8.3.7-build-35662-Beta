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
    """The settings table, found by section number rather than by heading text.

    It used to be found by the literal heading, which meant the count in the heading had to
    be edited every time a key was added and a stale count silently emptied the set.
    """
    text = read(DOC)
    body = section(text, "1. The")
    if not body:
        match = re.search(r"^##\s+(1\.[^\n]*)$(.*?)(?=^##\s|\Z)", text, re.S | re.M)
        body = match.group(2) if match else ""

    return set(re.findall(r"^\|\s*`(Housing\.[A-Za-z.]+)`", body, re.M))


def member_of(key):
    """The member `LoadConfig` fills for a key, from the note() call beside it."""
    body = read(MGR).split("void HousingMgr::LoadConfig()")[1].split("\nvoid HousingMgr::")[0]
    for match in re.finditer(r'note(?:Text)?\("' + re.escape(key) + r'",\s*(_[A-Za-z]+)', body):
        return match.group(1)
    return None


def applied_keys():
    """Keys whose member is read somewhere other than the loader and the constructor.

    A setting that is parsed, recorded and printed but never consulted is the quietest kind
    of dead feature there is: `.housing config` shows a value, the document explains it, and
    nothing behaves differently when it changes. This walks every member the loader fills and
    looks for a second reader anywhere in the module's header or its four implementation
    files, after removing the two places that write it.
    """
    source = read(MGR)
    body = source.split("void HousingMgr::LoadConfig()")[1].split("\nvoid HousingMgr::")[0]
    keys = re.findall(r'note(?:Text)?\("(Housing\.[A-Za-z.]+)"', body)

    # Everything except the loader itself and the constructor's initialiser list.
    rest = source.replace(body, "")
    ctor = rest.split("HousingMgr::HousingMgr()")[1].split("\nvoid HousingMgr::")[0] \
        if "HousingMgr::HousingMgr()" in rest else ""
    rest = rest.replace(ctor, "")

    # Implementation files only. The header's declaration is not a reader, and counting it
    # would make this rule pass for every key that has a member at all.
    others = ""
    for name in sorted(os.listdir(os.path.dirname(MGREXTRA))):
        if not name.endswith(".cpp"):
            continue
        path = os.path.join(os.path.dirname(MGREXTRA), name)
        if path == MGR:
            continue   # the loader is `rest`, already stripped of the two places that write
        others += read(path)

    dead = []
    for key in keys:
        member = member_of(key)
        if member is None:
            # A note() whose argument is a local - `noteText("Housing.Map.Mode", mode)` - cannot
            # be followed to a member by name. The key is still documented and still recorded;
            # this rule just has nothing to say about where it is read.
            continue
        if member.startswith("_") and member in rest:
            continue
        if member.startswith("_") and re.search(r"\b" + member + r"\b", others):
            continue
        dead.append((key, member))
    return dead


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
    for key, member in applied_keys():
        if member is None:
            fail("{} is recorded by LoadConfig with no member beside it".format(key))
        else:
            fail("{} fills {} and nothing reads that member".format(key, member))

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
