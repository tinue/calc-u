#!/usr/bin/env bash
# Builds the product made for the shell's tests: its
# engine, CMake target calcu_web_test_product of the repository's root
# project in build/web (as the products' Web engines), then four
# dist/ folders in build/web/library/web/test-dist/:
#
#   Test-A/    product Test-A, release 1.0.0
#   Test-B/    product Test-B, release 1.0.0 (two products, one origin)
#   Test-A-2/  product Test-A, release 2.0.0 (a newer release)
#   Test-M/    product Test-M, release 1.0.0 (media.spec.js: the
#              session hosts the library's MediaPart)
#
# Needs emcmake/cmake (Homebrew `emscripten` or an activated emsdk), ninja
# and node.
set -euo pipefail

HERE="$(cd "$(dirname "$0")" && pwd)"
ROOT="$(cd "$HERE/../../../.." && pwd)"   # the repository's root CMake project
BUILD="$ROOT/build/web"
OUT="$BUILD/library/web/test-dist"

for tool in emcmake cmake ninja node; do
  if ! command -v "$tool" >/dev/null 2>&1; then
    echo "error: $tool not found on PATH." >&2
    exit 1
  fi
done

if [ ! -f "$BUILD/CMakeCache.txt" ]; then
  emcmake cmake -S "$ROOT" -B "$BUILD" -G Ninja -DCMAKE_BUILD_TYPE=Release >/dev/null
fi
cmake --build "$BUILD" --target calcu_web_test_product

WASM="$BUILD/library/web"
node "$HERE/build.js" --name Test-A --version 1.0.0 --wasm "$WASM" --out "$OUT/Test-A"
node "$HERE/build.js" --name Test-B --version 1.0.0 --wasm "$WASM" --out "$OUT/Test-B"
node "$HERE/build.js" --name Test-A --version 2.0.0 --wasm "$WASM" --out "$OUT/Test-A-2"
node "$HERE/build.js" --name Test-M --version 1.0.0 --wasm "$WASM" --out "$OUT/Test-M"
echo "wrote $OUT"
