#!/usr/bin/env python3
"""Copy one of the two draft modules into a source checkout.

    python3 tools/housing/install.py --draft midnight-port --root /path/to/source
    python3 tools/housing/install.py --draft bfa-native --root . --dry-run

What it moves:

    src/game/Housing      -> <root>/src/server/game/Housing
    src/scripts/Housing   -> <root>/modules/PsychoHouse             (layout A)
                             <root>/src/server/scripts/PsychoHouse   (layout B)
    addon/HousingUI       -> <root>/modules/PsychoHouse/addon       (layout A)
                             <root>/addon/PsychoHouse               (layout B)
    sql/world/*.sql       -> <root>/sql/updates/world/  with a 2026_10_31_ prefix
    sql/characters/*.sql  -> <root>/sql/updates/characters/

Layout A is a tree that globs a root `modules/` directory. The directory name is
`PsychoHouse` and not `mod-psycho-house` on purpose: the loader generator builds the
export name out of the directory name (`Add##<dir>##Scripts`), so a hyphen would produce
an identifier that does not compile. Run `tools/housing/patch_cmake.py` once on that tree
so the glob reaches the root `modules/` folder at all. Layout B is the BfaCore
Reforged tree, whose script modules are directories under `src/server/scripts/` and whose
loader export name has to match the directory name - hence the rename to `PsychoHouse`
and the loader file rename below.

Nothing is overwritten silently: an existing file is moved to `<name>.bak-install`.
"""

import argparse
import os
import shutil
import sys

HERE = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
DRAFTS = os.path.join(HERE, "drafts")
LOADER_OLD = "housing_script_loader.cpp"
LOADER_NEW = "psycho_house_loader.cpp"
LOADER_EXPORT_OLD = "AddHousingScripts"
LOADER_EXPORT_NEW = "AddPsychoHouseScripts"


def copy_tree(src, dst, dry):
    if not os.path.isdir(src):
        sys.exit("missing source tree: " + src)
    for base, _dirs, files in os.walk(src):
        rel = os.path.relpath(base, src)
        target = dst if rel == "." else os.path.join(dst, rel)
        if not dry:
            os.makedirs(target, exist_ok=True)
        for name in files:
            out = os.path.join(target, name)
            print(("would copy " if dry else "copy ") + out)
            if not dry:
                if os.path.exists(out):
                    os.replace(out, out + ".bak-install")
                shutil.copy2(os.path.join(base, name), out)


def copy_file(src, dst, dry):
    print(("would copy " if dry else "copy ") + dst)
    if dry:
        return
    os.makedirs(os.path.dirname(dst), exist_ok=True)
    if os.path.exists(dst):
        os.replace(dst, dst + ".bak-install")
    shutil.copy2(src, dst)


def detect_layout(root):
    return "A" if os.path.isdir(os.path.join(root, "modules")) else "B"


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--draft", required=True, choices=["midnight-port", "bfa-native"])
    ap.add_argument("--root", default=".")
    ap.add_argument("--dry-run", action="store_true")
    args = ap.parse_args()

    src = os.path.join(DRAFTS, args.draft)
    root = os.path.abspath(args.root)
    layout = detect_layout(root)
    print("draft {} -> {} (layout {})".format(args.draft, root, layout))

    copy_tree(os.path.join(src, "src", "game", "Housing"),
              os.path.join(root, "src", "server", "game", "Housing"), args.dry_run)

    if layout == "A":
        scripts_dst = os.path.join(root, "modules", "PsychoHouse")
        addon_dst = os.path.join(scripts_dst, "addon", "PsychoHouse")
    else:
        scripts_dst = os.path.join(root, "src", "server", "scripts", "PsychoHouse")
        addon_dst = os.path.join(root, "addon", "PsychoHouse")

    copy_tree(os.path.join(src, "src", "scripts", "Housing"), scripts_dst, args.dry_run)

    old_loader = os.path.join(scripts_dst, LOADER_OLD)
    if not args.dry_run and os.path.exists(old_loader):
        os.replace(old_loader, os.path.join(scripts_dst, LOADER_NEW))
        new_loader = os.path.join(scripts_dst, LOADER_NEW)
        with open(new_loader, encoding="utf-8") as fh:
            text = fh.read()
        with open(new_loader, "w", encoding="utf-8") as fh:
            fh.write(text.replace(LOADER_EXPORT_OLD, LOADER_EXPORT_NEW))
        print("renamed loader -> " + LOADER_NEW)

    copy_tree(os.path.join(src, "addon", "HousingUI"), addon_dst, args.dry_run)

    # The client matches an addon folder to its .toc by name, so the file is renamed with
    # the folder. The Lua inside keeps its own global, which is harmless.
    old_toc = os.path.join(addon_dst, "HousingUI.toc")
    new_toc = os.path.join(addon_dst, "PsychoHouse.toc")
    print(("would rename " if args.dry_run else "rename ") + new_toc)
    if not args.dry_run and os.path.exists(old_toc):
        os.replace(old_toc, new_toc)

    sql_world = os.path.join(root, "sql", "updates", "world")
    sql_char = os.path.join(root, "sql", "updates", "characters")
    for folder, target in (("world", sql_world), ("characters", sql_char)):
        base = os.path.join(src, "sql", folder)
        if not os.path.isdir(base):
            continue
        for name in sorted(os.listdir(base)):
            if not name.endswith(".sql"):
                continue
            stamp = "2026_10_31_1" + name.split("_")[0]
            copy_file(os.path.join(base, name),
                      os.path.join(target, stamp + "_" + folder + "_" + name), args.dry_run)

    print()
    print("Next:")
    print("  1. merge %s into worldserver.conf"
          % os.path.join(src, "conf", "mod-psycho-house.conf.dist"))
    print("  2. apply the SQL in the order printed above")
    print("  3. run tools/housing/patch_cmake.py --root %s once, then re-run CMake" % root)
    print("  4. %s" % ("AddPsychoHouseScripts() is generated by the loader - no manual step on layout A"
                       if layout == "A" else
                       "AddPsychoHouseScripts() is what the loader must export on layout B"))
    print("  5. the addon is staged in the source tree at %s - copy that folder into the"
          " client's Interface/AddOns/, because a client only reads addons from its own"
          " data directory" % addon_dst)


if __name__ == "__main__":
    main()
