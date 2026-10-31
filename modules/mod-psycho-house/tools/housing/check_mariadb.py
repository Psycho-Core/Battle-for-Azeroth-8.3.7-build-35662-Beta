#!/usr/bin/env python3
"""Check the three drafts' SQL the way MariaDB 11.8.6 would reject it.

Every other checker in this folder reads the SQL as text: columns against the schema, value
counts against column lists, foreign keys inside one draft. None of them asks the question a
real server asks on the first import - *would this statement be accepted, and would the row
that came out be the row that went in?* MariaDB rejects more than MySQL of the same vintage
by default (`sql_mode` has included STRICT_TRANS_TABLES since 10.2), so the checks below are
the ones that turn into an error 1048/1062/1264/1364/1406 halfway through an install.

    python3 tools/housing/check_mariadb.py                     the module and all three drafts
    python3 tools/housing/check_mariadb.py --draft bfa-native   one of them

What is checked, per draft:

  tables     every CREATE TABLE pins ENGINE=InnoDB and a utf8mb4 charset; a missing COLLATE is
             reported, because MariaDB and MySQL default utf8mb4 to different collations and
             a table created today should not change behaviour after a server move;
             `utf8` (utf8mb3) is reported - MariaDB 11.x still accepts it and warns.
  columns    identifiers that MariaDB reserves and that are not backticked;
             NOT NULL columns with no DEFAULT that an INSERT does not supply (error 1364);
             columns an INSERT names that the schema does not define.
  rows       value count against the column list (error 1136, cheap to re-check here);
             integer literals wider than the column (error 1264 in strict mode);
             string literals longer than the column (error 1406 in strict mode).
  keys       duplicate PRIMARY KEY and UNIQUE KEY values inside one file's data (error 1062);
             the module writes `IF NOT EXISTS` tables and then bulk INSERTs, so a duplicate
             here is a broken install, not a warning.
  surfaces   a decor category that straddles two surface bands, and a surface outside
             0-3 (floor, wall, ceiling, any) - a category with two answers means one of
             its rows is wrong, and nothing else in the pipeline would notice.
  syntax     the tuples of a VALUES list are separated by commas, and - when sqlglot is
             installed - every statement parses as MySQL. Neither is redundant: a missing
             separator comma is invisible to every checker here that splits on parentheses,
             and it is error 1064 on the server.
  dialect    `INSERT IGNORE`, `REPLACE INTO`, `ADD COLUMN IF NOT EXISTS`, `DELIMITER`,
             stored routines and events - either they change what a duplicate does, or they
             are MariaDB-only (and therefore not portable), or they make an install
             unrepeatable.

Nothing here executes SQL. There is no server in this workspace, and this script says so
loudly rather than implying otherwise.
"""

import argparse
import glob
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
DRAFTS = os.path.join(os.path.dirname(os.path.dirname(HERE)), "drafts")

findings = []
BUILD_TABLES = set()

# MariaDB reserved words that appear, or could plausibly appear, as identifiers in this
# schema. The list is the intersection of "reserved in MariaDB 10.2 through 11.8" with
# "words a housing schema would reach for"; a name outside it is not proof of safety, which
# is why every table in these files backticks its columns anyway.
RESERVED = {
    "accessible", "add", "all", "alter", "analyze", "and", "as", "asc", "before", "between",
    "bigint", "both", "by", "call", "cascade", "case", "change", "char", "character", "check",
    "collate", "column", "condition", "constraint", "continue", "convert", "create", "cross",
    "current_date", "current_time", "current_timestamp", "current_user", "cursor", "database",
    "databases", "day_hour", "day_microsecond", "day_minute", "day_second", "dec", "decimal",
    "declare", "default", "delayed", "delete", "desc", "describe", "deterministic", "distinct",
    "distinctrow", "div", "double", "drop", "dual", "each", "else", "elseif", "enclosed",
    "escaped", "exists", "exit", "explain", "false", "fetch", "float", "for", "force",
    "foreign", "from", "fulltext", "grant", "group", "having", "high_priority", "hour_microsecond",
    "hour_minute", "hour_second", "if", "ignore", "in", "index", "infile", "inner", "insert",
    "int", "integer", "interval", "into", "is", "iterate", "join", "key", "keys", "kill",
    "leading", "leave", "left", "like", "limit", "linear", "lines", "load", "localtime",
    "localtimestamp", "lock", "long", "longblob", "longtext", "loop", "low_priority",
    "master_ssl_verify_server_cert", "match", "maxvalue", "mediumblob", "mediumint",
    "mediumtext", "middleint", "minute_microsecond", "minute_second", "mod", "modifies",
    "natural", "not", "no_write_to_binlog", "null", "numeric", "on", "optimize", "option",
    "optionally", "or", "order", "out", "outer", "outfile", "precision", "primary", "procedure",
    "purge", "range", "read", "reads", "real", "references", "regexp", "release", "rename",
    "repeat", "replace", "require", "restrict", "return", "revoke", "right", "rlike", "schema",
    "schemas", "second_microsecond", "select", "sensitive", "separator", "set", "show",
    "smallint", "spatial", "specific", "sql", "sqlexception", "sqlstate", "sqlwarning",
    "sql_big_result", "sql_calc_found_rows", "sql_small_result", "ssl", "starting", "straight_join",
    "table", "terminated", "then", "tinyblob", "tinyint", "tinytext", "to", "trailing",
    "trigger", "true", "undo", "union", "unique", "unlock", "unsigned", "update", "usage",
    "use", "using", "utc_date", "utc_time", "utc_timestamp", "values", "varbinary", "varchar",
    "varcharacter", "varying", "when", "where", "while", "with", "write", "xor", "year_month",
    "zerofill",
}

INT_TYPES = {
    "tinyint": (0, 255),
    "smallint": (0, 65535),
    "mediumint": (0, 16777215),
    "int": (0, 4294967295),
    "integer": (0, 4294967295),
    "bigint": (0, 18446744073709551615),
}


def fail(where, what):
    findings.append("{}: {}".format(where, what))


def read(path):
    with open(path, encoding="utf-8") as handle:
        return handle.read()


def strip_comments(text):
    text = re.sub(r"/\*.*?\*/", "", text, flags=re.S)
    return "\n".join(line for line in text.splitlines() if not line.lstrip().startswith("--"))


def split_top(text, sep=","):
    out, current, depth, quote = [], [], 0, False
    for char in text:
        if quote:
            current.append(char)
            if char == "'":
                quote = False
            continue
        if char == "'":
            quote = True
            current.append(char)
        elif char == "(":
            depth += 1
            current.append(char)
        elif char == ")":
            depth -= 1
            current.append(char)
        elif char == sep and depth == 0:
            out.append("".join(current))
            current = []
        else:
            current.append(char)
    if "".join(current).strip():
        out.append("".join(current))
    return [part.strip() for part in out]


def split_tuples(body):
    rows, depth, current, quote = [], 0, [], False
    for char in body:
        if quote:
            current.append(char)
            if char == "'":
                quote = False
            continue
        if char == "'":
            quote = True
            current.append(char)
        elif char == "(":
            if depth == 0:
                current = []
            else:
                current.append(char)
            depth += 1
        elif char == ")":
            depth -= 1
            if depth == 0:
                rows.append("".join(current))
            else:
                current.append(char)
        else:
            if depth > 0:
                current.append(char)
    return rows


def statement_body(tail):
    out, quote, i = [], False, 0
    while i < len(tail):
        char = tail[i]
        if quote:
            out.append(char)
            if char == "'":
                if i + 1 < len(tail) and tail[i + 1] == "'":
                    out.append("'")
                    i += 2
                    continue
                quote = False
            i += 1
            continue
        if char == "'":
            quote = True
            out.append(char)
        elif char == ";":
            break
        else:
            out.append(char)
        i += 1
    return "".join(out)


def unquote(value):
    value = value.strip()
    if len(value) >= 2 and value[0] == "'" and value[-1] == "'":
        return value[1:-1].replace("''", "'")
    return value


def tables(text):
    """Return {table: {column: (type, not_null, has_default)}} plus key metadata."""
    out = {}
    for match in re.finditer(r"CREATE TABLE(?: IF NOT EXISTS)? `(\w+)` \((.*?)\n\)([^;]*);", text, re.S):
        name, body, options = match.group(1), match.group(2), match.group(3)
        columns, primary, uniques = {}, [], []
        for line in body.splitlines():
            line = line.strip().rstrip(",")
            if not line:
                continue
            key = re.match(r"PRIMARY KEY \((.*?)\)", line, re.I)
            if key:
                primary = [c.strip().strip("`") for c in key.group(1).split(",")]
                continue
            key = re.match(r"UNIQUE KEY `\w+` \((.*?)\)", line, re.I)
            if key:
                uniques.append([c.strip().strip("`") for c in key.group(1).split(",")])
                continue
            column = re.match(r"`(\w+)`\s+(\w+(?:\([\d,]+\))?)(.*)", line)
            if not column:
                continue
            cname, ctype = column.group(1), column.group(2).lower()
            rest = column.group(3)
            columns[cname] = {
                "type": re.sub(r"\(.*", "", ctype),
                "length": int(re.search(r"\((\d+)", ctype).group(1)) if "(" in ctype else None,
                "not_null": bool(re.search(r"\bNOT NULL\b", rest, re.I)),
                "default": bool(re.search(r"\bDEFAULT\b", rest, re.I)),
                "unsigned": "unsigned" in rest.lower() or "unsigned" in ctype,
                "auto": bool(re.search(r"\bAUTO_INCREMENT\b", rest, re.I)),
            }
        out[name] = {
            "columns": columns,
            "primary": primary,
            "uniques": uniques,
            "engine": re.search(r"ENGINE\s*=\s*(\w+)", options, re.I),
            "charset": re.search(r"CHARSET\s*=\s*(\w+)", options, re.I),
            "collate": re.search(r"COLLATE\s*=?\s*(\w+)", options, re.I),
        }
    return out


def check_tables(label, path, schema):
    base = os.path.basename(path)
    for name, info in sorted(schema.items()):
        where = "{} {} {}".format(label, base, name)
        if not info["engine"] or info["engine"].group(1).lower() != "innodb":
            fail(where, "no ENGINE=InnoDB; a default-charset or MyISAM table is not what the "
                        "rest of the schema is")
        if not info["charset"] or info["charset"].group(1).lower() != "utf8mb4":
            fail(where, "no CHARSET=utf8mb4")
        if not info["collate"]:
            fail(where, "no COLLATE; MariaDB and MySQL default utf8mb4 to different "
                        "collations, so the table's behaviour depends on which server "
                        "created it")
        for column, meta in info["columns"].items():
            if column.lower() in RESERVED:
                fail(where, "column `{}` is a MariaDB reserved word and is not backticked".format(column))


def check_rows(label, path, text, schema):
    base = os.path.basename(path)
    for match in re.finditer(r"INSERT\s+INTO\s+`(\w+)`\s*\(([^)]*)\)\s*VALUES", text, re.S):
        table = match.group(1)
        if table not in schema:
            # Core tables (quest_template, creature_template, ...) are created by the build,
            # not by the draft. Their columns are checked by check_sql_columns.py against the
            # build's own schema; naming them here would only repeat that.
            BUILD_TABLES.add(table)
            continue
        columns = [c.strip().strip("`") for c in match.group(2).split(",")]
        info = schema[table]
        where = "{} {} {}".format(label, base, table)

        unknown = [c for c in columns if c not in info["columns"]]
        if unknown:
            fail(where, "INSERT names column(s) the schema does not define: " + ", ".join(unknown))

        missing = [c for c, meta in info["columns"].items()
                   if meta["not_null"] and not meta["default"] and not meta["auto"]
                   and c not in columns]
        if missing and info["primary"]:
            fail(where, "NOT NULL column(s) with no DEFAULT are not supplied by the INSERT: "
                        + ", ".join(missing) + " (error 1364 in strict mode)")

        rows = split_tuples(statement_body(text[match.end():]))
        seen = {}
        for index, row in enumerate(rows):
            values = split_top(row, ",")
            if len(values) != len(columns):
                fail(where, "row {} has {} value(s) for {} column(s) (error 1136)".format(
                    index + 1, len(values), len(columns)))
                continue
            record = dict(zip(columns, values))
            for column, value in record.items():
                meta = info["columns"].get(column)
                if not meta:
                    continue
                raw = unquote(value)
                if raw.upper() == "NULL":
                    if meta["not_null"] and not meta["default"]:
                        fail(where, "row {}: NULL for NOT NULL `{}` (error 1048)".format(index + 1, column))
                    continue
                if meta["type"] in INT_TYPES and re.fullmatch(r"-?\d+", raw):
                    low, high = INT_TYPES[meta["type"]]
                    if not meta["unsigned"]:
                        high //= 2
                    if int(raw) > high or int(raw) < 0:
                        fail(where, "row {}: {} is out of range for `{} {}` (error 1264)".format(
                            index + 1, raw, meta["type"], "unsigned" if meta["unsigned"] else "signed"))
                if meta["type"] in ("varchar", "char") and meta["length"] and len(raw) > meta["length"]:
                    fail(where, "row {}: {} characters in `{}` VARCHAR({}) (error 1406)".format(
                        index + 1, len(raw), column, meta["length"]))
            keys = [info["primary"]] + info["uniques"]
            for key in keys:
                if not key or any(c not in columns for c in key):
                    continue
                fingerprint = tuple(unquote(record.get(c, "")) for c in key)
                if any(v == "" for v in fingerprint):
                    continue
                if fingerprint in seen:
                    fail(where, "rows {} and {} share the key ({}): duplicate key (error 1062)".format(
                        seen[fingerprint], index + 1, ", ".join(key)))
                seen[fingerprint] = index + 1


def check_dialect(label, path, text):
    base = os.path.basename(path)
    for pattern, what in (
            (r"\bINSERT\s+IGNORE\b", "INSERT IGNORE silently turns a duplicate key into a warning"),
            (r"\bREPLACE\s+INTO\b", "REPLACE INTO deletes and re-inserts, which discards columns "
                                    "the statement does not name"),
            (r"\bADD\s+COLUMN\s+IF\s+NOT\s+EXISTS\b", "ADD COLUMN IF NOT EXISTS is MariaDB-only"),
            (r"\bDELIMITER\b", "DELIMITER is a client directive and is not part of a statement"),
            (r"\bCREATE\s+(?:DEFINER\s*=\s*\S+\s+)?(?:PROCEDURE|FUNCTION|TRIGGER|EVENT)\b",
             "a stored routine or trigger makes the install unrepeatable and the module "
             "harder to remove")):
        for match in re.finditer(pattern, text, re.I):
            line = text.count("\n", 0, match.start()) + 1
            fail("{} {}:{}".format(label, base, line), what)
    if re.search(r"\butf8\b(?!mb)", text):
        fail("{} {}".format(label, base), "`utf8` is utf8mb3 in MariaDB 11.x: character set "
                                          "warns on creation, and the columns hold three bytes")


SURFACES = {"0", "1", "2", "3"}


def check_surfaces(label, path, text):
    """A category belongs to one surface band, and a band is one of the four values."""
    base = os.path.basename(path)
    if "housing_decor" not in text:
        return
    for match in re.finditer(r"INSERT\s+INTO\s+`housing_decor`\s*\(([^)]*)\)\s*VALUES", text, re.S):
        columns = [c.strip().strip("`") for c in match.group(1).split(",")]
        if "surface" not in columns or "category" not in columns:
            continue
        bands = {}
        for row in split_tuples(statement_body(text[match.end():])):
            values = split_top(row, ",")
            if len(values) != len(columns):
                continue
            record = dict(zip(columns, values))
            surface = unquote(record["surface"])
            category = unquote(record["category"])
            if surface not in SURFACES:
                fail("{} {}".format(label, base), "category {} has surface {}, which is not "
                                                  "floor/wall/ceiling/any".format(category, surface))
                continue
            bands.setdefault(category, set()).add(surface)
        for category, found in sorted(bands.items()):
            if len(found) > 1:
                fail("{} {}".format(label, base), "category {} straddles surface bands {}".format(
                    category, ", ".join(sorted(found))))


def check_separators(label, path, text):
    """Every tuple after the first must be preceded by a comma.

    `INSERT INTO t (a) VALUES (1) (2);` is a syntax error, and it is exactly the shape a
    generator produces when it joins rows with a newline instead of a comma. Nothing else in
    this folder can see it: the row checkers split on parentheses.
    """
    base = os.path.basename(path)
    for match in re.finditer(r"INSERT\s+INTO\s+`\w+`\s*\([^)]*\)\s*VALUES", text, re.S):
        body = statement_body(text[match.end():])
        depth, quote, previous_close = 0, False, None
        index = 0
        while index < len(body):
            char = body[index]
            if quote:
                if char == "'":
                    if index + 1 < len(body) and body[index + 1] == "'":
                        index += 2
                        continue
                    quote = False
                index += 1
                continue
            if char == "'":
                quote = True
            elif char == "(":
                if depth == 0 and previous_close is not None:
                    between = body[previous_close:index].strip()
                    if between != ",":
                        line = text.count("\n", 0, match.start()) + 1
                        fail("{} {}:{}".format(label, base, line),
                             "two tuples of a VALUES list have {!r} between them instead of a "
                             "comma - syntax error 1064".format(between))
                        break
                depth += 1
            elif char == ")":
                depth -= 1
                if depth == 0:
                    previous_close = index + 1
            index += 1


def check_parses(label, path, text):
    """Parse each file with sqlglot when it is installed; skip quietly when it is not."""
    try:
        import sqlglot
    except ImportError:
        return False
    base = os.path.basename(path)
    try:
        sqlglot.parse(text, read="mysql")
    except Exception as exc:  # sqlglot raises its own ParseError subclasses
        first = str(exc).splitlines()[0]
        fail("{} {}".format(label, base),
             "does not parse as MySQL: {}".format(first[:160]))
    return True


def draft_files(draft):
    root = os.path.join(DRAFTS, draft)
    return sorted(glob.glob(os.path.join(root, "sql", "*", "*.sql")))


def module_files():
    root = os.path.dirname(os.path.dirname(HERE))
    return sorted(glob.glob(os.path.join(root, "sql", "*", "*.sql")))


def main():
    global DRAFTS
    parser = argparse.ArgumentParser()
    parser.add_argument("--draft", action="append",
                        help="draft directory name; repeatable, default: every draft")
    parser.add_argument("--drafts", default=DRAFTS,
                        help="directory holding the drafts, default: this module's drafts/")
    args = parser.parse_args()

    DRAFTS = os.path.abspath(args.drafts)

    names = args.draft or sorted(name for name in os.listdir(DRAFTS)
                                 if os.path.isdir(os.path.join(DRAFTS, name)))
    groups = []
    if not args.draft and DRAFTS == os.path.abspath(os.path.join(os.path.dirname(HERE), "drafts")):
        groups.append(("module", module_files()))
    groups += [(name, draft_files(name)) for name in names]
    checked = 0
    parsed = False
    for draft, files in groups:
        if not files:
            continue
        schema = {}
        texts = {}
        for path in files:
            text = strip_comments(read(path))
            texts[path] = text
            if "CREATE TABLE" in text:
                for name, info in tables(text).items():
                    if name in schema:
                        fail("{}".format(draft), "table `{}` is created twice".format(name))
                    schema[name] = info
        for path, text in texts.items():
            check_dialect(draft, path, text)
            if "CREATE TABLE" in text:
                check_tables(draft, path, tables(text))
            check_rows(draft, path, text, schema)
            check_surfaces(draft, path, text)
            check_separators(draft, path, text)
            parsed = check_parses(draft, path, text) or parsed
        rows = 0
        for text in texts.values():
            for insert in re.finditer(r"INSERT\s+INTO\s+`\w+`\s*\([^)]*\)\s*VALUES", text, re.S):
                rows += len(split_tuples(statement_body(text[insert.end():])))
        print("{:<16} {:>2} file(s)  {:>2} table(s)  {:>6} rows".format(
            draft, len(files), len(schema), rows))
        checked += 1

    if not checked:
        print("no draft SQL found under " + DRAFTS)
        return 1

    if BUILD_TABLES:
        print("\n{} core table(s) written to and not created here: {}".format(
            len(BUILD_TABLES), ", ".join(sorted(BUILD_TABLES))))
    print("\n{} finding(s)".format(len(findings)))
    for line in findings:
        print("  " + line)
    print("\nparsed with sqlglot (MySQL dialect): {}".format(
        "every file" if parsed else "skipped, sqlglot is not installed"))

    if not findings:
        print("\nRead as statements only. No server exists in this workspace, so nothing here")
        print("was executed; every rule above is one MariaDB 11.8.6 enforces on import.")
    return 1 if findings else 0


if __name__ == "__main__":
    sys.exit(main())
