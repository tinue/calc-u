#!/usr/bin/env bash
# Build the Qt6 app (Release, Ninja) from the root CMake project (ADR 0003)
# into the repository's build/app-59 tree — the local development build;
# the app lands in build/app-59/products/calc-u-59/frontends/qt6/. On macOS
# this is the only Qt build: it is never shipped (the Swift app is the Mac
# product); Windows and Linux packages come from CI
# (.github/workflows/calc-u-59.yml at the repository root).
set -euo pipefail
REPO="$(cd "$(dirname "$0")/../../.." && pwd)"
TREE="$REPO/build/app-59"
if [ ! -f "$TREE/build.ninja" ]; then
    cmake -S "$REPO" -B "$TREE" -G Ninja -DCMAKE_BUILD_TYPE=Release -DCALCU_PRODUCT_1600=OFF
fi
cmake --build "$TREE"
