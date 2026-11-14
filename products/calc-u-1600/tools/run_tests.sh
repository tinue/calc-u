#!/bin/sh
# Builds and runs the headless core/ + app/ test suite (CoreTests) at -O1,
# from the repository's root CMake project (ADR 0003) without the Qt apps,
# in the repository's build/tests tree. The source lists come from
# cmake/CalcU.cmake and the product file's test glob, so there is nothing
# to keep in sync here. The tests run from the product directory (they
# load roms/ via relative paths).
set -e
cd "$(dirname "$0")/.."
REPO="$(cd ../.. && pwd)"
TREE="$REPO/build/tests"
if [ ! -f "$TREE/CMakeCache.txt" ]; then
  cmake -S "$REPO" -B "$TREE" -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCALCU_FRONTEND_QT=OFF >/dev/null
fi
cmake --build "$TREE" --target CoreTests
"$TREE/products/calc-u-1600/CoreTests" "$@"
