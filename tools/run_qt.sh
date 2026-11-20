#!/usr/bin/env bash
# Builds a product's Qt6 app (Release, Ninja) from the root CMake project
# into build/app-<n>, the other product off (configured the
# first time only), and starts it: on macOS the .app through `open`, else
# the binary. The local development build; the release packages come
# from CI (docs/Building.md).
#
#   tools/run_qt.sh 59 [args]      # build and start; args go to the app (a preset)
#   tools/run_qt.sh 1600 --build   # build only
#   tools/run_qt.sh 1600 --app-dir # print the directory holding the built app
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
case "${1:-}" in
  59|calc-u-59) N=59 OTHER=1600 ;;
  1600|calc-u-1600) N=1600 OTHER=59 ;;
  *) echo "usage: tools/run_qt.sh 59|1600 [--build | --app-dir | <app args>...]" >&2; exit 2 ;;
esac
shift
TREE="$ROOT/build/app-$N"
APP_DIR="$TREE/products/calc-u-$N/frontends/qt6"
APP="Calc-U-$N"

if [[ "${1:-}" == "--app-dir" ]]; then echo "$APP_DIR"; exit 0; fi
if [[ ! -f "$TREE/build.ninja" ]]; then
  cmake -S "$ROOT" -B "$TREE" -G Ninja -DCMAKE_BUILD_TYPE=Release "-DCALCU_PRODUCT_$OTHER=OFF"
fi
cmake --build "$TREE"
if [[ "${1:-}" == "--build" ]]; then exit 0; fi

if [[ "$(uname -s)" == "Darwin" ]]; then
  open "$APP_DIR/$APP.app" --args "$@"
  # `open` returns once launchd has the app; a terminal closed right after
  # could otherwise warn that the session ended before its child finished.
  sleep 2
else
  exec "$APP_DIR/$APP" "$@"
fi
