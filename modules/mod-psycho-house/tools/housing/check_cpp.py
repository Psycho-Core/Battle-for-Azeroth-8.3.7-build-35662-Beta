#!/usr/bin/env python3
"""Static checks on the module's C++ that do not need a compiler.

    python3 tools/housing/check_cpp.py [--repo /path/to/source]

There is no boost, no MySQL client and no compiler in the environment this was written
in, so `cmake` cannot answer the question "is this correct". These checks answer the
questions that do not need one:

  braces      every file balances its braces, brackets and parens, ignoring strings,
              chars and comments
  declares    every method declared in a module header has a definition, and every
              Class::Method definition belongs to a class the headers declare
  fields      every `fields[N]` index is inside the column count of the SELECT that
              produced it - the classic way a loader reads the wrong column or reads past
              the end of a row
  tables      every table named in a Query() string is one the module creates
  includes    every module-local include resolves to a real file, and every core include
              is found somewhere under the source tree
  sql         every query string has a SELECT that names at least one column

Run it with --repo pointing at the source checkout so the include and core-symbol checks
have somewhere to look.
"""

import argparse
import os
import re
import sys

HERE = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
SRC = os.path.join(HERE, "src")

# Both are overridable so a draft in drafts/ can be checked with the same tool. --src picks
# the tree to read, --sql picks the directories whose CREATE TABLEs are the vocabulary.
CPP_FILES = []
SCHEMA_ROOTS = []

findings = []


def fail(where, what):
    findings.append("{}: {}".format(where, what))


def strip_code(text):
    """Remove comments and string/char literals so bracket counting is honest."""
    out = []
    i = 0
    n = len(text)
    while i < n:
        char = text[i]
        two = text[i:i + 2]
        if two == "//":
            while i < n and text[i] != "\n":
                i += 1
        elif two == "/*":
            i = text.find("*/", i + 2)
            i = n if i < 0 else i + 2
        elif char == '"':
            i += 1
            while i < n and text[i] != '"':
                i += 2 if text[i] == "\\" else 1
            i += 1
        elif char == "'":
            i += 1
            while i < n and text[i] != "'":
                i += 2 if text[i] == "\\" else 1
            i += 1
        else:
            out.append(char)
            i += 1
    return "".join(out)


def check_braces(path, text):
    body = strip_code(text)
    for opener, closer, label in (("{", "}", "brace"), ("(", ")", "paren"),
                                  ("[", "]", "bracket")):
        depth = 0
        for char in body:
            if char == opener:
                depth += 1
            elif char == closer:
                depth -= 1
                if depth < 0:
                    break
        if depth != 0:
            fail(os.path.relpath(path, HERE), "unbalanced {} (depth {})".format(label, depth))


def declarations(headers):
    """class -> set of method names declared in the module headers.

    Brace-matched rather than regex-guessed, because a declaration can span lines:
        static HousingMgr* instance();
    and a body can contain access specifiers, nested structs and default arguments.
    """
    skip = {"if", "for", "while", "switch", "return", "sizeof", "else", "do"}
    out = {}
    for path, text in headers.items():
        body = strip_code(text)
        for match in re.finditer(r"\b(?:class|struct)\s+([\w\s]*?)\s*\{", body):
            words = [w for w in match.group(1).split()
                     if not re.fullmatch(r"[A-Z_][A-Z0-9_]*", w)]
            if not words:
                continue
            name = words[-1]
            depth, index = 1, match.end()
            while index < len(body) and depth:
                if body[index] == "{":
                    depth += 1
                elif body[index] == "}":
                    depth -= 1
                index += 1
            inner = body[match.end():index]
            methods = out.setdefault(name, set())
            for found in re.finditer(r"(\w+)\s*\([^;{}]*\)\s*(?:const\s*)?(?:override\s*)?"
                                     r"(?:=\s*0\s*)?;", inner):
                if found.group(1) not in skip:
                    methods.add(found.group(1))
    return out


EXTERNAL = {"std", "Trinity", "boost", "fmt", "Housing", "ObjectGuid", "Position"}


def external_class(cls, repo):
    """Core or standard-library classes are not the module's to declare."""
    if cls in EXTERNAL or cls[:1].islower():
        return True
    if repo:
        wanted = cls + ".h"
        for base, _dirs, files in os.walk(os.path.join(repo, "src")):
            if wanted in files:
                return True
    return False


def check_definitions(cpp_files, declared, repo):
    for path, text in cpp_files.items():
        for match in re.finditer(r"^[\w:<>,\s\*&]*?\b(\w+)::(\w+)\s*\(", text, re.M):
            cls, method = match.group(1), match.group(2)
            if cls not in declared:
                if external_class(cls, repo):
                    continue
                fail(os.path.relpath(path, HERE),
                     "{}::{} defined but class {} is not in a module header".format(
                         cls, method, cls))
            elif method not in declared[cls]:
                fail(os.path.relpath(path, HERE),
                     "{}::{} is defined but not declared in {}.h".format(cls, method, cls))


QUERY = re.compile(r"(?:WorldDatabase|CharacterDatabase|LoginDatabase)\.P?Query\(\s*"
                   r"((?:\"[^\"]*\"\s*)+)\)", re.S)


def check_fields(path, text, tables):
    label = os.path.relpath(path, HERE)
    for match in QUERY.finditer(text):
        literal = "".join(re.findall(r"\"([^\"]*)\"", match.group(1)))
        select = re.search(r"SELECT\s+(.*?)\s+FROM", literal, re.I | re.S)
        if not select:
            fail(label, "query without a SELECT: {}".format(literal[:60]))
            continue
        columns = select.group(1)
        if columns.strip() in ("*", "1"):
            count = None
        else:
            count = len([c for c in columns.split(",") if c.strip()])
        table = re.search(r"FROM\s+`?(\w+)`?", literal, re.I)
        if table and table.group(1) not in tables and not table.group(1).startswith("character_"):
            fail(label, "query reads table `{}` which the module does not create".format(
                table.group(1)))
        if table and table.group(1) in tables:
            known = tables[table.group(1)]
            plain = re.sub(r"\w+\s*\(([^)]*)\)", r"\1", columns)
            named = {c.strip().strip("`") for c in plain.split(",")}
            named = {c.split(".")[-1].split(" ")[0] for c in named}
            unknown = sorted(c for c in named if c not in known and c != "*")
            if unknown:
                fail(label, "query on `{}` names columns it does not have: {}".format(
                    table.group(1), unknown))
        if count is None:
            continue
        # the fields[N] indices that follow this query, before the next query
        tail = text[match.end():]
        next_query = min([i for i in (tail.find(".Query("), tail.find(".PQuery(")) if i >= 0] or [-1])
        if next_query >= 0:
            tail = tail[:next_query]
        used = [int(n) for n in re.findall(r"fields\[(\d+)\]", tail)]
        if used and max(used) >= count:
            fail(label, "SELECT has {} columns but the loader reads fields[{}]".format(
                count, max(used)))


def check_includes(path, text, repo):
    label = os.path.relpath(path, HERE)
    local = os.path.dirname(path)
    for line in re.findall(r'#include\s+"([^"]+)"', text):
        if os.path.exists(os.path.join(local, line)):
            continue
        if os.path.exists(os.path.join(SRC, line)):
            continue
        wanted = os.path.basename(line)
        found = any(wanted in files for _base, _dirs, files in os.walk(SRC))
        if not found and repo:
            for root in (os.path.join(repo, "src"), os.path.join(repo, "dep")):
                for base, _dirs, files in os.walk(root):
                    if wanted in files:
                        found = True
                        break
                if found:
                    break
        if found:
            continue
        fail(label, "include \"{}\" does not resolve".format(line))


def collect_sources():
    del CPP_FILES[:]
    for base, _dirs, files in os.walk(SRC):
        for name in sorted(files):
            if name.endswith((".cpp", ".h")):
                CPP_FILES.append(os.path.join(base, name))


def module_schema():
    """table -> set of columns, read out of the module's own CREATE TABLE statements."""
    schema = []
    for root in (SCHEMA_ROOTS or [os.path.join(HERE, "sql")]):
        for base, _dirs, files in os.walk(root):
            for name in files:
                if name.endswith(".sql"):
                    with open(os.path.join(base, name), encoding="utf-8") as handle:
                        schema.append(handle.read())
    text = "\n".join(schema)
    tables = {}
    for match in re.finditer(r"CREATE TABLE (?:IF NOT EXISTS )?`(\w+)` \((.*?)\n\)", text, re.S):
        columns = set()
        for line in match.group(2).splitlines():
            line = line.strip()
            if line.startswith("`"):
                columns.add(line.split()[0].strip("`"))
        tables[match.group(1)] = columns
    return tables


def module_tables():
    return set(module_schema())


def find_repo():
    """Walk up from the module until a directory with src/server/game turns up."""
    path = HERE
    for _ in range(4):
        if os.path.isdir(os.path.join(path, "src", "server", "game")):
            return path
        path = os.path.dirname(path)
    return os.path.dirname(HERE)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--repo", default=find_repo(),
                        help="source checkout to resolve includes against, "
                             "default: the tree this module sits in")
    parser.add_argument("--src", default=None,
                        help="directory of .cpp/.h files to check, default: this module's src/")
    parser.add_argument("--sql", action="append", default=None,
                        help="directory of CREATE TABLE statements, repeatable, "
                             "default: this module's sql/")
    args = parser.parse_args()

    global SRC
    if args.src:
        SRC = os.path.abspath(args.src)
    del SCHEMA_ROOTS[:]
    if args.sql:
        SCHEMA_ROOTS.extend(os.path.abspath(item) for item in args.sql)
    collect_sources()

    headers, cpp_files = {}, {}
    for path in CPP_FILES:
        with open(path, encoding="utf-8") as handle:
            text = handle.read()
        if path.endswith(".h"):
            headers[path] = text
        else:
            cpp_files[path] = text

    for path in CPP_FILES:
        with open(path, encoding="utf-8") as handle:
            text = handle.read()
        check_braces(path, text)

    declared = declarations(headers)
    check_definitions(cpp_files, declared, args.repo)

    tables = module_schema()
    for path, text in list(cpp_files.items()) + list(headers.items()):
        check_fields(path, text, tables)
        check_includes(path, text, args.repo)

    print("{} files, {} classes, {} tables created by the module".format(
        len(CPP_FILES), len(declared), len(tables)))
    for name in sorted(declared):
        print("  {:<24} {} methods".format(name, len(declared[name])))
    print("\n{} finding(s)".format(len(findings)))
    for line in findings:
        print("  " + line)
    return 1 if findings else 0


if __name__ == "__main__":
    sys.exit(main())
