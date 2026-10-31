#!/usr/bin/env python3
"""Make a root `modules/` folder visible to the script build.

    python3 tools/housing/patch_cmake.py --root /path/to/source --dry-run
    python3 tools/housing/patch_cmake.py --root /path/to/source

Why this is needed at all: this core globs `<source>/src/server/scripts/*` for script
modules. A module that sits in `<source>/modules/` is simply never seen, so CMake
configures happily, the worldserver links happily, and the module does nothing. There is
no error message - that is the trap this script removes.

What it changes: one function, `GetScriptModuleList`, in
`cmake/macros/ConfigureScripts.cmake`. The glob is widened to include the root `modules`
folder, and every subdirectory found there is appended the same way a normal script module
is. Nothing else in the file is touched, the original is saved as
`ConfigureScripts.cmake.bak-cmake-patch`, and running it twice is a no-op.

The script module directory name has to be a valid C++ identifier because the loader
generator turns it into `Add<dir>Scripts()`. `PsychoHouse` is fine, `mod-psycho-house` is
not, which is why `install.py` names the directory `PsychoHouse`.
"""

import argparse
import os
import shutil
import sys

MARK_BEGIN = "# --- psycho-house: root modules folder ---"
MARK_END = "# --- end psycho-house patch ---"

BLOCK = """  # --- psycho-house: root modules folder ---
  # Modules dropped into <source>/modules/ are built alongside the stock script modules.
  file(GLOB ROOT_MODULE_DIRS RELATIVE
    ${CMAKE_SOURCE_DIR}/modules
    ${CMAKE_SOURCE_DIR}/modules/*)
  foreach(ROOT_MODULE ${ROOT_MODULE_DIRS})
    if (IS_DIRECTORY ${CMAKE_SOURCE_DIR}/modules/${ROOT_MODULE})
      if (EXISTS ${CMAKE_SOURCE_DIR}/modules/${ROOT_MODULE}/CMakeLists.txt)
        message("")
        message("WARNING: modules/${ROOT_MODULE} has its own CMakeLists.txt.")
        message("         Script modules are collected by the script glob and must not")
        message("         carry one. Rename or remove it, or the build will be")
        message("         confused about which project owns the directory.")
        message("")
      endif()
      list(APPEND ${variable} ${ROOT_MODULE})
      set(ROOT_MODULE_PATH_${ROOT_MODULE} ${CMAKE_SOURCE_DIR}/modules/${ROOT_MODULE} PARENT_SCOPE)
    endif()
  endforeach()
  # --- end psycho-house patch ---
"""

ANCHOR = """  endforeach()
  set(${variable} ${${variable}} PARENT_SCOPE)
endfunction()"""

# The stock file needs one more change: GetPathToScriptModule must resolve the modules that
# came from the root folder to an absolute path, otherwise CollectSourceFiles finds nothing.
PATH_HOOK = """  # --- psycho-house: root modules folder ---
  if (DEFINED ROOT_MODULE_PATH_${module})
    set(${variable} ${ROOT_MODULE_PATH_${module}} PARENT_SCOPE)
    return()
  endif()
  # --- end psycho-house patch ---
"""


def find(path):
    if not os.path.isfile(path):
        sys.exit("not found: " + path + " (is --root pointing at the source tree?)")
    with open(path, encoding="utf-8") as handle:
        return handle.read()


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--root", default=".")
    parser.add_argument("--dry-run", action="store_true")
    args = parser.parse_args()

    root = os.path.abspath(args.root)
    target = os.path.join(root, "cmake", "macros", "ConfigureScripts.cmake")
    text = find(target)

    if MARK_BEGIN in text:
        print("already patched: " + target)
        return 0

    if ANCHOR not in text:
        sys.exit(
            "this ConfigureScripts.cmake does not look like the expected BfaCore file:\n"
            "  anchor missing in " + target + "\n"
            "Patch it by hand: add `file(GLOB ... modules/*)` to GetScriptModuleList."
        )

    patched = text.replace(ANCHOR, ANCHOR.replace("  set(${variable}", BLOCK + "  set(${variable}"), 1)

    hook_anchor = 'function(GetPathToScriptModule module variable)\n'
    if hook_anchor not in patched:
        sys.exit("GetPathToScriptModule missing - patch by hand.")
    patched = patched.replace(
        hook_anchor, hook_anchor + PATH_HOOK, 1
    )

    relative = os.path.relpath(target, root)
    if args.dry_run:
        print("would back up " + relative + " -> " + relative + ".bak-cmake-patch")
        print("would add the root modules glob to GetScriptModuleList")
        print("would add the absolute-path hook to GetPathToScriptModule")
        return 0

    shutil.copy2(target, target + ".bak-cmake-patch")
    with open(target, "w", encoding="utf-8") as handle:
        handle.write(patched)

    modules_dir = os.path.join(root, "modules")
    print("patched " + relative)
    print("backup  " + relative + ".bak-cmake-patch")
    print("modules folder on disk: " + ("yes" if os.path.isdir(modules_dir) else "NO - create it"))
    print()
    print("Next: re-run CMake. A root module shows up in the script graph as")
    print("      +- worldserver / libscripts_PsychoHouse.so")
    print("If it is missing, check the spelling of the folder name: the loader export")
    print("is Add<folder>Scripts(), so the folder has to be a legal C++ identifier.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
