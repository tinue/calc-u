#!/usr/bin/env bash
# Build the Qt6 app (Release, Ninja) into Qt6/build — the local development
# build. On macOS this is the only Qt build: it is never shipped (the Swift
# app is the Mac product); Windows and Linux packages come from CI
# (.github/workflows/build.yml).
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
if [ ! -f "$ROOT/frontends/qt6/build/build.ninja" ]; then
    cmake -S "$ROOT/frontends/qt6" -B "$ROOT/frontends/qt6/build" -G Ninja -DCMAKE_BUILD_TYPE=Release
fi
cmake --build "$ROOT/frontends/qt6/build"
