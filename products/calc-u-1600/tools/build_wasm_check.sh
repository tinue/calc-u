#!/bin/sh
# Builds Core + Shared for WebAssembly with Emscripten and runs the node
# smoke test (tools/wasm_smoke.cpp) from the repo root: the check that the
# frontend-neutral layers stay portable to a browser GUI (C++20, no Qt,
# no threads, no blocking -- docs/developer/App-Layer.md).
#
# Needs emcc/emcmake (Homebrew `emscripten`, or an activated emsdk), node,
# and core/Basic/vendor/sharpdx/libsharpdx-wasm.a (committed; rebuild with
# tools/build_sharpdx_wasm.sh). ROMs in roms/ (roms/fetch_roms.sh) for the
# smoke run; without them it builds and skips.
#
# Builds in build/web of the repository's root CMake project (ADR 0003),
# the tree frontends/web/build.sh uses too.
set -e
cd "$(dirname "$0")/.."
BUILD=../../build/web
if [ ! -f $BUILD/CMakeCache.txt ]; then
  emcmake cmake -S ../.. -B $BUILD -G Ninja -DCMAKE_BUILD_TYPE=Release >/dev/null
fi
cmake --build $BUILD --target wasm_smoke
set +e
node $BUILD/products/calc-u-1600/wasm_smoke.js
status=$?
set -e
if [ $status -eq 2 ]; then exit 0; fi  # no ROMs: built, smoke skipped
exit $status
