#!/usr/bin/env python3
"""Check the addon and the server agree on the BFAH protocol.

    python3 tools/housing/check_protocol.py

Three things are checked, all by reading the source of both halves:

  sends    every `C:OP` the addon sends is an op the server dispatches
  replies  every `S:OP` the server sends has a handler in the addon (or is handled
           inline, as S:CHUNK is)
  fields   every field index the addon reads is inside the field count of the line the
           server builds for that op
  window   every op the server dispatches is reachable from the housing window, not only
           from a slash command - two exceptions, HELLO and PING, are the handshake and
           have no business on a button
  orphans  every handler in the addon is for a line the server actually sends. A handler
           for an op nobody sends is dead code with a friendly message in it, and it hides
           the fact that the feature it belongs to reports nothing

That last check is the one that matters most: a handler reading fields[14] from a line
that has twelve fields prints `nil` at runtime and looks like a server bug. There is no
client here, so this is the only way it gets caught.
"""

import os
import re
import sys

HERE = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
ADDON = os.path.join(HERE, "addon", "HousingUI")
MENU = os.path.join(ADDON, "Menu.lua")

# Ops that are transport, not features: the window cannot usefully offer them.
WINDOW_EXEMPT = {"HELLO", "PING"}
PROTOCOL = os.path.join(HERE, "src", "game", "Housing", "HousingMgrProtocol.cpp")
GAME_DIR = os.path.join(HERE, "src", "game", "Housing")

findings = []


def fail(what):
    findings.append(what)


def read(path):
    with open(path, encoding="utf-8") as handle:
        return handle.read()


def addon_ops():
    """C:OPS sent by the addon, and the S:OPS it handles."""
    sends, handlers = set(), {}
    for name in sorted(os.listdir(ADDON)):
        if not name.endswith(".lua"):
            continue
        text = read(os.path.join(ADDON, name))
        sends.update(re.findall(r'"(C:[A-Z]+)', text))
        sends.update("C:" + op for op in re.findall(r'"C:([A-Z]+)\|', text))
        for match in re.finditer(r'H\.handlers\["([A-Z]+)"\]', text):
            handlers.setdefault(match.group(1), set()).add(name)
        # field indices read per handler: crude but exact for this addon's shape
        for block in re.finditer(r'H\.handlers\["([A-Z]+)"\] = function\(fields\)(.*?)\nend',
                                 text, re.S):
            op, body = block.group(1), block.group(2)
            used = [int(n) for n in re.findall(r"fields\[(\d+)\]", body)]
            if used:
                handlers.setdefault("_max_" + op, set()).add(max(used))
    return sends, handlers


def server_ops():
    """OPS the server dispatches, and the field count of each S: line it builds.

    A line is sometimes built in pieces - SendResult appends the optional `extra` field
    only when it is non-zero - so the count follows the payload variable for as long as
    the function keeps appending to it, and stops when the chunk is sent.
    """
    # Every file in the game directory: the extended opcode set lives in
    # HousingMgrExtra.cpp, and a dispatcher split across two files is still one dispatcher.
    dispatched = set()
    for name in sorted(os.listdir(GAME_DIR)):
        if name.endswith(".cpp"):
            dispatched.update(re.findall(r'op == "([A-Z]+)"', read(os.path.join(GAME_DIR, name))))
    replies = {}
    for name in sorted(os.listdir(GAME_DIR)):
        if not name.endswith(".cpp"):
            continue
        current = None
        for line in read(os.path.join(GAME_DIR, name)).splitlines():
            # A list ends with a marker line rather than a payload - `S:HUNTEND;` - so the
            # closing quote may follow a semicolon. Without that the end markers look like
            # handlers for lines nobody sends, which is exactly backwards.
            opened = re.search(r'"(S:([A-Z]+)(?:\|([^"]*))?);?"', line)
            if opened:
                current = opened.group(2)
                fields = 1 + opened.group(0).count("|")
                replies[current] = max(replies.get(current, 0), fields)
            elif current and "+=" in line:
                added = re.search(r'"([^"]*)"', line)
                if added and added.group(1).startswith("|"):
                    replies[current] = max(
                        replies[current], replies[current] + added.group(1).count("|"))
            if "SendChunk(" in line:
                current = None
    return dispatched, replies


def window_ops():
    """The C:OPS the window itself sends.

    A feature that a player can only reach by typing `.housing <verb>` is a feature with no
    interface, and the documents say the window is the interface. This is the check that
    keeps the two honest.
    """
    text = read(MENU)

    # Only an actual send counts. The Guide tab prints every opcode as text, and a check that
    # counted those would pass on the documentation alone - which is the failure mode this
    # rule exists to catch.
    ops = set(re.findall(r'H\.Send\(\s*"C:([A-Z]+)', text))
    ops.update(re.findall(r'H\.Send\(\s*string\.format\(\s*"C:([A-Z]+)', text))
    return ops


def main():
    sends, handlers = addon_ops()
    dispatched, replies = server_ops()

    sends_ops = {s[2:] for s in sends}
    for op in sorted(sends_ops - dispatched):
        fail("the addon sends C:{} which the server does not dispatch".format(op))

    inline = {"CHUNK"}
    missing = sorted(set(replies) - set(handlers) - inline)
    for op in missing:
        fail("the server sends S:{} and the addon has no handler".format(op))

    # A handler for a line the server never builds is a feature that prints its own success
    # message into the void. S:CHUNK is the transport envelope and is handled by the frame
    # reader rather than by a table entry, so it is the one legitimate exception.
    for op in sorted({o for o in handlers if not o.startswith("_max_")} - set(replies) - inline):
        fail("the addon handles S:{} and the server never sends it".format(op))

    reachable = window_ops()
    for op in sorted(dispatched - reachable - WINDOW_EXEMPT):
        fail("the server dispatches C:{} and the window never sends it - a slash command is "
             "the only way in".format(op))

    for op in sorted(set(replies) & set(handlers)):
        sent = replies[op]
        looked = handlers.get("_max_" + op)
        if not looked:
            continue
        highest = max(int(n) for n in looked)
        if highest > sent:
            fail("S:{} carries {} fields but the addon reads fields[{}]".format(
                op, sent, highest))

    print("{} C: opcodes sent by the addon, {} dispatched by the server".format(
        len(sends_ops), len(dispatched)))
    print("{} of {} dispatched opcodes reachable from the window".format(
        len(dispatched & (reachable | WINDOW_EXEMPT)), len(dispatched)))
    print("{} S: opcodes sent by the server, {} handled in the addon".format(
        len(replies), len(set(replies) & set(handlers))))
    for op in sorted(replies):
        used = handlers.get("_max_" + op)
        print("  S:{:<14} {:>2} fields{}".format(
            op, replies[op], "" if not used else ", addon reads up to fields[{}]".format(
                max(int(n) for n in used))))
    unused = sorted(dispatched - sends_ops)
    if unused:
        print("  dispatched but never sent by the addon: " + ", ".join(unused))
    print("\n{} finding(s)".format(len(findings)))
    for line in findings:
        print("  " + line)
    return 1 if findings else 0


if __name__ == "__main__":
    sys.exit(main())
