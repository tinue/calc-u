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
DIST="$WEB/dist"

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

# Every model's ROM and constants, the module catalogue and every module
# image: the files the worker hands SessionController's ResourceReader.
cp "$REPO_ROOT"/roms/calculator/*.txt "$REPO_ROOT"/roms/solid-state/*.txt "$DIST/wasm/roms/"
rm -f "$DIST/wasm/roms/files.txt"

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
