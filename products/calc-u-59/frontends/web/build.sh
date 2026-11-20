#!/usr/bin/env bash
# Builds the web client into frontends/web/dist/ (gitignored; CALCU_WEB_DIST:
# another folder): build.js names what is the product's, the library's
# library/web/build/product.js does the steps. --release builds without the
# engine's test-only commands (Calc-U-59's engine has none: the same dist/).
# Needs emcmake/cmake (Homebrew `emscripten` or an activated emsdk; CI pins
# the version), ninja and node. Re-run whenever core/, app/, roms/ or
# anything in frontends/web/ changes.
set -euo pipefail
exec node "$(dirname "$0")/build.js" "$@"
