#!/usr/bin/env bash
# Builds the web client into frontends/web/dist/ (gitignored; CALCU_WEB_DIST:
# another folder): build.js names what is the product's, the library's
# library/web/build/product.js does the steps. --release builds without the
# engine's test-only commands, as the release's zip is built; without it the
# test build the Playwright tests drive.
# Needs emcmake/cmake (Homebrew `emscripten` or an activated emsdk; CI pins
# the version), ninja, node, and ROMs in roms/ (roms/fetch_roms.sh).
set -euo pipefail
exec node "$(dirname "$0")/build.js" "$@"
