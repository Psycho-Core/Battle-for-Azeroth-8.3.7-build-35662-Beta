#!/usr/bin/env python3
"""Check that the names the documents cite are names the code actually has.

    python3 tools/housing/check_claims.py

The README section "Everything doc 01 lists, and where it lives" is the answer to the
completeness question: every row of the retail record, with the function, table, opcode,
setting or command that carries it. An answer made of names is only worth as much as the
names, and a prose claim is exactly the kind of thing that goes stale when a function is
renamed or a column is dropped. So the names are checked.

Every backticked name in that section, and in doc 02's matrix, is one of a few shapes, and
each shape has an oracle:

  C:OP / S:OP          an opcode: client-dispatched or server-sent (check_protocol.py knows)
  H.Name               an addon function or table field in addon/HousingUI/
  Housing.X.Y          a key in mod-psycho-house.conf.dist
  .housing word        a subcommand in the command table
  table.column         both halves present in the module's own schema
  snake_name           a table, a column, a script name, a conf key or a file
  Class::Member        the class and the member, both somewhere in src/
  CamelName            a symbol in src/ (function, enum, constant or class)
  file.ext             a file in the module

The corpus is the module's own source, addon, SQL, conf and tools. Nothing here reads the
network or a database, and nothing here rewrites a document: a failure names the document,
the token, and which oracle refused it.
"""

import glob
import os
import re
import sys

HERE = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import check_protocol  # noqa: E402  the opcode oracle lives there, and there is one of it

findings = []


def fail(where, what):
    findings.append("{}: {}".format(where, what))


def read(path):
    with open(path, encoding="utf-8", errors="ignore") as handle:
        return handle.read()


def corpus(patterns):
    text = []
    for pattern in patterns:
        for path in glob.glob(os.path.join(HERE, pattern), recursive=True):
            text.append(read(path))
    return "\n".join(text)


def section_of(path, start_marker, end_marker="\n## "):
    text = read(os.path.join(HERE, path))
    start = text.find(start_marker)
    if start < 0:
        fail(path, "the section starting '{}' is gone".format(start_marker[:40]))
        return ""
    end = text.find(end_marker, start + len(start_marker))
    return text[start:end if end > 0 else len(text)]


def doc_matrix(path, heading, stop="\n### "):
    """The rows of one matrix section, as whole lines."""
    text = read(os.path.join(HERE, path))
    start = text.find(heading)
    if start < 0:
        fail(path, "section '{}' is gone".format(heading[:40]))
        return ""
    end = text.find(stop, start + len(heading))
    return text[start:end if end > 0 else len(text)]


SOURCE = corpus(("src/**/*.h", "src/**/*.cpp"))
ADDON = corpus(("addon/**/*.lua",))
CONF = read(os.path.join(HERE, "conf", "mod-psycho-house.conf.dist")) \
    if os.path.exists(os.path.join(HERE, "conf", "mod-psycho-house.conf.dist")) else ""
if not CONF:
    for path in glob.glob(os.path.join(HERE, "drafts", "*", "conf", "*.conf.dist")):
        CONF += read(path)
SQL = corpus(("sql/**/*.sql", "tools/sql/*.sql"))
for path in glob.glob(os.path.join(HERE, "drafts", "*", "sql", "**", "*.sql"), recursive=True):
    # The drafts repeat the module's schema; one copy is enough and the copies are identical,
    # but reading them costs nothing and covers a table added only to a draft.
    SQL += read(path)
DOCS = SOURCE + "\n" + ADDON + "\n" + SQL + "\n" + CONF


def client_ops():
    """Ops the addon sends, and ops the server dispatches - both mean the opcode exists."""
    sends, _handlers = check_protocol.addon_ops()
    dispatched, _replies = check_protocol.server_ops()
    return {op[2:] for op in sends} | dispatched


def server_ops():
    """Ops the server answers with, read the same way the protocol checker reads them."""
    _dispatched, replies = check_protocol.server_ops()
    return set(replies)


def handlers():
    return set(re.findall(r'H\.handlers\[\s*"([A-Z]+)"', ADDON))


DOC14 = ""


def commands():
    text = read(os.path.join(HERE, "src", "scripts", "Housing", "housing_commands.cpp"))
    return set(re.findall(r'\{\s*"([a-z]+)"', text))


def conf_keys():
    return set(re.findall(r"^(Housing\.[A-Za-z.]+)\s*=", CONF, re.M))


def tables_and_columns():
    tables, columns = set(), set()
    for match in re.finditer(r"CREATE TABLE IF NOT EXISTS `(\w+)`\s*\((.*?)\n\)", SQL, re.S):
        tables.add(match.group(1))
        for line in match.group(2).splitlines():
            name = re.match(r"\s*`(\w+)`\s+\w", line)
            if name:
                columns.add(name.group(1))
    return tables, columns


def known_files():
    names = set()
    for path in glob.glob(os.path.join(HERE, "**", "*"), recursive=True):
        if os.path.isfile(path):
            names.add(os.path.basename(path))
    return names


TABLES, COLUMNS = tables_and_columns()
FILES = known_files()
CLIENT_OPS, SERVER_OPS, HANDLERS = client_ops(), server_ops(), handlers()
COMMANDS, CONF_KEYS = commands(), conf_keys()

SKIP = (" in ", "=", "(", ")", "+", "*", "/", "%")

# Core tables the module reads or writes and does not create. They are not in this module's
# schema because they are the build's, and a document is allowed to name them: the claim being
# checked is that the name exists, and these names exist in every 8.3.7 world database.
CORE_TABLES = {
    "item_template",
    "spell_template",
    "creature_template",
    "creature_template_model",
    "gameobject_template",
    "gameobject_template_addon",
    "quest_template",
    "quest_template_addon",
    "quest_objectives",
    "spell_recipe",
    "skill_line_ability",
}


def check_token(where, token):
    token = token.strip()
    if not token or any(mark in token for mark in SKIP) or token[0].isdigit():
        return

    # Opcodes.
    match = re.fullmatch(r"([CS]):([A-Z]+)", token)
    if match:
        side, op = match.groups()
        if side == "C" and op not in CLIENT_OPS:
            fail(where, "`{}` is cited and the addon never sends it".format(token))
        if side == "S" and op not in SERVER_OPS and op not in HANDLERS:
            fail(where, "`{}` is cited and the server never sends it".format(token))
        return

    # Conf keys.
    if re.fullmatch(r"Housing\.[A-Za-z.]+", token):
        if token not in CONF_KEYS:
            fail(where, "`{}` is cited and the conf file has no such key".format(token))
        return

    # Addon names.
    if token.startswith("H."):
        if token[2:] not in ADDON:
            fail(where, "`{}` is cited and the addon has no such name".format(token))
        return

    # Commands.
    if token.startswith(".housing "):
        word = token.split(None, 1)[1].strip()
        if word not in COMMANDS:
            fail(where, "`{}` is cited and the command table has no such entry".format(token))
        return

    # Files.
    if re.fullmatch(r"[\w./-]+\.(py|md|sql|cpp|h|lua|dist|toc|csv)", token):
        if os.path.basename(token) not in FILES:
            fail(where, "`{}` is cited and no such file is in the module".format(token))
        return

    # table.column, or table.column.suffix for a dotted key.
    match = re.fullmatch(r"([a-z_]+)\.([a-z_]+)(\.[a-z_]+)?", token)
    if match:
        table, column = match.group(1), match.group(2)
        if table not in TABLES:
            fail(where, "`{}` names table `{}`, which the schema does not create"
                 .format(token, table))
            return
        if column not in COLUMNS:
            fail(where, "`{}` names column `{}` on `{}`, which the schema does not create"
                 .format(token, column, table))
        return

    # Script names and table names.
    if re.fullmatch(r"[a-z][a-z0-9_]*", token):
        if token in TABLES or token in COLUMNS or token in CONF or token in CORE_TABLES:
            return
        if token in SOURCE or token in SQL or token in ADDON:
            return
        fail(where, "`{}` is cited and no schema, source or conf names it".format(token))
        return

    # Class::Member.
    if "::" in token:
        scope, member = token.split("::", 1)
        if scope not in SOURCE:
            fail(where, "`{}` names class `{}`, which is in no source file".format(token, scope))
        if member not in SOURCE:
            fail(where, "`{}` names member `{}`, which is in no source file"
                 .format(token, member))
        return

    # A bare symbol, a constant, an enum, a script name.
    if re.fullmatch(r"[A-Za-z_][A-Za-z0-9_]*:?", token):
        bare = token.rstrip(":")
        if bare in DOCS:
            return
        fail(where, "`{}` is cited and no source, addon, schema or conf names it"
             .format(token))
        return


def scan(where, text):
    # Code spans only: a name in a code span is a claim, a name in prose is a word.
    for token in re.findall(r"`([^`\n]+)`", text):
        check_token(where, token)


def guide_pairs():
    """The Guide tab's op -> verb table, as the addon writes it."""
    text = read(os.path.join(HERE, "addon", "HousingUI", "Menu.lua"))
    block = text.split("local GUIDE_ROWS = {")[1].split("\n}\n")[0]
    return re.findall(r'\{\s*"(C:[A-Z]+)"\s*,\s*"([a-z]+)"\s*\}', block)


def sent_ops():
    """Ops the addon actually sends, with the Guide tab's own list cut out first.

    check_protocol.addon_ops() reads every quoted C: string in the addon, which would make the
    Guide tab satisfy itself: an op named only in the table would count as sent. The block is
    removed here so the comparison is against the rest of the client.
    """
    sends = set()

    for name in sorted(os.listdir(os.path.join(HERE, "addon", "HousingUI"))):
        if not name.endswith(".lua"):
            continue

        text = read(os.path.join(HERE, "addon", "HousingUI", name))

        if "local GUIDE_ROWS = {" in text:
            before, rest = text.split("local GUIDE_ROWS = {", 1)
            text = before + rest.split("\n}\n", 1)[-1]

        sends.update(re.findall(r'"(C:[A-Z]+)', text))
        sends.update("C:" + op for op in re.findall(r'"C:([A-Z]+)\|', text))

    return sends


def check_guide():
    """The Guide tab lists every op the addon sends, and every op has a command.

    The tab said "Every one of these has a /housing equivalent" while three features had no
    command at all, and it listed twenty-four lines while the addon sent sixty-one ops. A
    sentence about completeness is the kind of claim that is true on the day it is written and
    wrong by the next pass, so the table is data and this reads it.
    """
    pairs = guide_pairs()

    if not pairs:
        fail("Menu.lua", "the Guide tab has no op table to check")
        return 0

    listed = [op for op, _verb in pairs]

    duplicates = {op for op in listed if listed.count(op) > 1}
    for op in sorted(duplicates):
        fail("Menu.lua", "the Guide tab lists `{}` more than once".format(op))

    sent = {op for op in sent_ops() if op not in ("C:HELLO", "C:PING")}
    missing = sorted(sent - set(listed))
    extra = sorted(set(listed) - sent)

    for op in missing:
        fail("Menu.lua", "the addon sends `{}` and the Guide tab does not list it".format(op))

    for op in extra:
        fail("Menu.lua", "the Guide tab lists `{}`, which the addon never sends".format(op))

    for op, verb in pairs:
        if verb not in COMMANDS:
            fail("Menu.lua", "`{}` is paired with `.housing {}`, which the command table has no "
                 "entry for".format(op, verb))

    # And the mapping has to be one to one in the other direction: a verb the tab claims for an
    # op but the server does not dispatch is a command a player cannot run.
    for op, verb in pairs:
        if ".housing " + verb not in DOC14:
            fail("Menu.lua", "`{}` is paired with `.housing {}`, which doc 14 does not list"
                 .format(op, verb))

    print("guide: {} ops mapped to {} verbs, every one dispatched and documented"
          .format(len(pairs), len({verb for _op, verb in pairs})))
    return len(pairs)


def main():
    global DOC14
    DOC14 = read(os.path.join(HERE, "14-settings-and-commands.md"))
    check_guide()

    inventory = section_of("README.md", "## Everything doc 01 lists")
    scan("README.md", inventory)

    matrix = doc_matrix("02-feature-matrix-and-port.md", "## 2. Feature matrix")
    scan("02-feature-matrix-and-port.md", matrix)

    tokens = len(re.findall(r"`([^`\n]+)`", inventory))
    print("claims: {} spans in the inventory, {} tables, {} columns, {} opcodes, "
          "{} commands, {} conf keys".format(tokens, len(TABLES), len(COLUMNS),
                                             len(CLIENT_OPS), len(COMMANDS), len(CONF_KEYS)))
    print("\n{} finding(s)".format(len(findings)))
    for line in findings:
        print("  " + line)

    if not findings:
        print("\nEvery name the completeness entry cites is a name the module has.")

    return 1 if findings else 0


if __name__ == "__main__":
    sys.exit(main())
