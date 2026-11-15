#!/usr/bin/env bash
# Builds the web client into frontends/web/dist/ (gitignored): the WASM engine (core/ +
# app/ + frontends/web/bindings.cpp, CMake target calcu_web of the repository's
# root project, ADR 0003, built in build/web), the files it loads
# (ROMs, bundled cards and disks, the curated presets), the faceplates, the
# client script and stylesheet, the installable app (app/) and a demo page.
# docs/developer/Web-Client.md is the contract for what dist/ contains.
#
# Needs emcmake/cmake (Homebrew `emscripten` or an activated emsdk; CI pins
# the version), ninja, node, and ROMs in roms/ (roms/fetch_roms.sh).
set -euo pipefail

WEB="$(cd "$(dirname "$0")" && pwd)"
REPO="$(cd "$WEB/../.." && pwd)"  # the product directory
ROOT="$(cd "$REPO/../.." && pwd)"  # the repository's root CMake project
DIST="${CALCU1600_WEB_DIST:-$WEB/dist}"
BUILD="$ROOT/build/web"

for tool in emcmake cmake ninja node; do
  if ! command -v "$tool" >/dev/null 2>&1; then
    echo "error: $tool not found on PATH." >&2
    exit 1
  fi
done
if ! ls "${CALCU1600_ROMS_DIR:-$REPO/roms}"/*.bin >/dev/null 2>&1; then
  echo "error: no ROMs in roms/ -- run roms/fetch_roms.sh." >&2
  exit 1
fi

if [ ! -f "$BUILD/CMakeCache.txt" ]; then
  emcmake cmake -S "$ROOT" -B "$BUILD" -G Ninja -DCMAKE_BUILD_TYPE=Release >/dev/null
fi
cmake --build "$BUILD" --target calcu_web

rm -rf "$DIST"
mkdir -p "$DIST/wasm" "$DIST/bundle" "$DIST/img" "$DIST/app" "$DIST/vendor"

cp "$BUILD/products/calc-u-1600/calcu1600-core.js" "$BUILD/products/calc-u-1600/calcu1600-core.wasm" "$DIST/wasm/"
# What the engine reads as bundled resources (AppPaths::bundledResourcesDir):
# the ROM full set (the dumps and the CE-502B's module definition; LIB-12
# AC-12, AC-14) and the bundled card and disk templates. The host-drive ROM
# stays out: the web has no host drive. CALCU1600_ROMS_DIR: another ROM
# folder (tests/rom_missing_build.sh).
ROMS="${CALCU1600_ROMS_DIR:-$REPO/roms}"
find "$ROMS" -maxdepth 1 -type f \( -iname '*.bin' -o -iname '*.rom' -o -name '*.card.yaml' \) -exec cp {} "$DIST/bundle/" \;
cp "$REPO"/frontends/qt6/resources/cards/*.yaml "$DIST/bundle/"

cp "$REPO"/frontends/qt6/resources/faceplates/*.png "$DIST/img/"
cp "$REPO/frontends/qt6/resources/linux/calc-u-1600.png" "$DIST/img/icon.png"
cp "$REPO/frontends/qt6/resources/linux/calc-u-1600.png" "$DIST/app/icon.png"

cp "$WEB/index.html" "$WEB/calc.css" "$WEB/calc-engine-worker.js" "$WEB/audio-worklet.js" "$DIST/"
# React 18 for the app and the demo page; a host page brings its own.
cp "$WEB/vendor/react.production.min.js" "$WEB/vendor/react-dom.production.min.js" "$DIST/vendor/"
cp "$WEB/app/index.html" "$WEB/app/app.css" "$WEB/app/sw.js" "$WEB/app/engine-worker.js" "$WEB/app/manifest.json" "$DIST/app/"

node "$WEB/build.js"

echo "wrote $DIST"
