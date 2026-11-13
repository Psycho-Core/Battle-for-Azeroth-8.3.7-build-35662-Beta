#!/bin/sh
# Build a Lua 5.1 interpreter into tools/housing/bin/lua51.
#
# The addon runs in a 5.1 client, so the tests run it in 5.1 rather than a newer Lua that
# would accept syntax and behave differently around numbers. Nothing here is shipped to
# players: the binary lands in tools/housing/bin/, which is ignored by git, and the checker
# uses any lua5.1 already on the machine before it looks for this one.
#
#   sh tools/housing/build_lua.sh
#
# Sources come from lua.org, falling back to the lua/lua mirror on GitHub.

set -e

here=$(cd "$(dirname "$0")" && pwd)
work=${TMPDIR:-/tmp}/housing-lua-build
version=5.1.5

mkdir -p "$here/bin"
rm -rf "$work"
mkdir -p "$work"

got=0
for url in \
    "https://www.lua.org/ftp/lua-$version.tar.gz" \
    "https://codeload.github.com/lua/lua/tar.gz/refs/tags/v5.1.1"
do
    if curl -fsSL "$url" -o "$work/lua.tar.gz" 2>/dev/null && tar xzf "$work/lua.tar.gz" -C "$work" 2>/dev/null; then
        got=1
        break
    fi
done

if [ "$got" -eq 0 ]; then
    echo "could not download a Lua 5.1 source tree" >&2
    exit 1
fi

src=$(find "$work" -maxdepth 2 -name lapi.c -printf '%h\n' | head -1)
if [ -z "$src" ]; then
    echo "the archive did not contain the Lua sources" >&2
    exit 1
fi

cd "$src"
cc=${CC:-gcc}
$cc -O2 -DLUA_USE_POSIX -o "$here/bin/lua51" \
    lua.c lapi.c lcode.c ldebug.c ldo.c ldump.c lfunc.c lgc.c llex.c lmem.c lobject.c \
    lopcodes.c lparser.c lstate.c lstring.c ltable.c ltm.c lundump.c lvm.c lzio.c \
    lauxlib.c lbaselib.c ldblib.c liolib.c lmathlib.c loslib.c ltablib.c lstrlib.c \
    loadlib.c linit.c -lm

"$here/bin/lua51" -v
