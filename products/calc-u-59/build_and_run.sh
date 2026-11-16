#!/usr/bin/env bash
# Build and start the Qt6 app (local development). Extra arguments, e.g. a
# .ti59 preset, are passed to the app.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")" && pwd)"
APP_DIR="$ROOT/../../build/app-59/products/calc-u-59/frontends/qt6"  # tools/build_qt.sh
"$ROOT/tools/build_qt.sh"
case "$(uname)" in
    Darwin) open "$APP_DIR/Calc-U-59.app" --args "$@" ;;
    *) exec "$APP_DIR/Calc-U-59" "$@" ;;
esac
