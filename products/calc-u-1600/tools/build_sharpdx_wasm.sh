#!/bin/sh
# Builds libsharpdx (Sharp-Data-Exchange's C ABI) for WebAssembly into
# core/Basic/vendor/sharpdx/libsharpdx-wasm.a, from a local checkout, for
# the Emscripten build of Core + Shared (tools/build_wasm_check.sh).
#
# Needs the Rust target: rustup target add wasm32-unknown-emscripten
# (and emcc on the PATH, which cargo uses for this target).
#
# Same feature set as the native libs (refresh_sharpdx.sh): --lib
# --no-default-features, so no serial or audio code. Only the static
# library is built (the crate's cdylib output is skipped).
set -e
cd "$(dirname "$0")/.."
REPO_ROOT=$(pwd)
SRC=${SHARPDX_SRC:-../../../../sharp/common/Sharp-Data-Exchange}
DST="$REPO_ROOT/core/Basic/vendor/sharpdx"

if [ ! -d "$SRC" ]; then
  echo "Sharp-Data-Exchange checkout not found at $SRC" >&2
  echo "(set SHARPDX_SRC=/path/to/Sharp-Data-Exchange to override)" >&2
  exit 1
fi

cd "$SRC"
cargo rustc --release --lib --no-default-features --target wasm32-unknown-emscripten --crate-type staticlib
cp target/wasm32-unknown-emscripten/release/libsharpdx.a "$DST/libsharpdx-wasm.a"
cmp -s include/sharpdx.h "$DST/sharpdx.h" || echo "note: $DST/sharpdx.h differs from $SRC -- run tools/refresh_sharpdx.sh" >&2
ls -l "$DST/libsharpdx-wasm.a"
