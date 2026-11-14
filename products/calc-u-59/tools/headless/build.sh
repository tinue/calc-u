#!/usr/bin/env bash
# Build the headless harness shared library used by tools/headless/ti59.py:
# the ti59headless target of the root CMake project (ADR 0003; capi.cpp on
# core/ and app/, sources from the product's CMakeLists.txt), built in the
# repository's build/tools tree (Release, no Qt) and copied to
# tools/headless/build/ (ignored).
set -euo pipefail
HERE="$(cd "$(dirname "$0")" && pwd)"
REPO="$(cd "$HERE/../../../.." && pwd)"
TREE="$REPO/build/tools"
if [ ! -f "$TREE/CMakeCache.txt" ]; then
  cmake -S "$REPO" -B "$TREE" -G Ninja -DCMAKE_BUILD_TYPE=Release -DCALCU_FRONTEND_QT=OFF >/dev/null
fi
cmake --build "$TREE" --target ti59headless
mkdir -p "$HERE/build"
cp "$TREE/products/calc-u-59/libti59headless.dylib" "$HERE/build/"
echo "built $HERE/build/libti59headless.dylib"
