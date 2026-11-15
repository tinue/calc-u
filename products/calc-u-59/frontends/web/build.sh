#!/usr/bin/env bash
# Build the web client into frontends/web/dist/ (gitignored): the WASM
# engine (core/ + app/ + bindings.cpp; CMake target calcu59_web of the
# repository's root project, ADR 0003, built in build/web), the ROM files it
# loads, the calculator's script, stylesheet and fonts, the presets, the
# installable PWA and a demo page. reference/WebClient.md is the contract
# for what dist/ contains; the product's CMakeLists.txt has the engine's
# Emscripten flags.
#
# Requires emcmake/cmake (e.g. `brew install emscripten`; CI pins the
# version in .github/workflows/calc-u-59.yml), ninja and node. Re-run
# whenever core/, app/, roms/ or anything in frontends/web/ changes.
set -euo pipefail

WEB="$(cd "$(dirname "$0")" && pwd)"
REPO_ROOT="$(cd "$WEB/../.." && pwd)"  # the product directory
ROOT="$(cd "$REPO_ROOT/../.." && pwd)"  # the repository's root CMake project
BUILD="$ROOT/build/web"
# CALCU59_ROMS_DIR, CALCU59_WEB_DIST: another ROM folder and another output
# (tests/rom_missing_build.sh builds a dist/ without one ROM file).
ROMS="${CALCU59_ROMS_DIR:-$REPO_ROOT/roms}"
DIST="${CALCU59_WEB_DIST:-$WEB/dist}"

for tool in emcmake cmake ninja node; do
  if ! command -v "$tool" >/dev/null 2>&1; then
    echo "error: $tool not found on PATH." >&2
    exit 1
  fi
done

rm -rf "$DIST"
mkdir -p "$DIST/wasm/roms" "$DIST/app" "$DIST/vendor" "$DIST/fonts" "$DIST/img"

if [ ! -f "$BUILD/CMakeCache.txt" ]; then
  emcmake cmake -S "$ROOT" -B "$BUILD" -G Ninja -DCMAKE_BUILD_TYPE=Release >/dev/null
fi
cmake --build "$BUILD" --target calcu59_web
cp "$BUILD/products/calc-u-59/ti59-core.js" "$BUILD/products/calc-u-59/ti59-core.wasm" "$DIST/wasm/"

# The full set of ROM files (LIB-12 AC-9: the ten calculator dumps and the 14
# module files, as roms/fetch_roms.sh fetched them) and cuecards.txt (the
# module list and cue cards): the worker writes the ROM files into /roms,
# where the lookup finds them. A missing file is not copied, and the started
# client names it (LIB-12 AC-5; tests/rom_missing_build.sh).
cp "$ROMS"/*.txt "$ROMS"/*.rom.yaml "$REPO_ROOT"/roms/solid-state/cuecards.txt "$DIST/wasm/roms/"

# The calculator artwork and the cue-card templates (one source: the Xcode
# asset catalogue, as for the Qt app).
for img in ti59_base ti58_base CueCard MagnetCard SolidStateCard; do
  cp "$REPO_ROOT/frontends/swift/App/Assets.xcassets/$img.imageset/$img.png" "$DIST/img/"
done

cp "$WEB/index.html" "$WEB/calc.css" "$WEB/calc-engine-worker.js" "$DIST/"
cp "$WEB"/fonts/* "$DIST/fonts/"
# React 18 for the PWA and the demo page; a host page brings its own.
cp "$WEB/vendor/react.production.min.js" "$WEB/vendor/react-dom.production.min.js" "$DIST/vendor/"
cp "$WEB/app/index.html" "$WEB/app/app.css" "$WEB/app/sw.js" "$WEB/app/engine-worker.js" "$WEB/app/manifest.json" "$DIST/app/"
# The app icon, derived from App/AppIcon.icon by tools/gen_qt_icons.sh.
cp "$REPO_ROOT/frontends/qt6/resources/linux/calc-u-59.png" "$DIST/app/icon.png"

node "$WEB/build.js"

echo "wrote $DIST"
