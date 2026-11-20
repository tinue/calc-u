#!/usr/bin/env bash
# Runs the web client's Playwright tests against frontends/web/dist/ (build it
# first: frontends/web/build.sh; CALCU_WEB_DIST: another dist/, as
# tests/rom_missing_build.sh builds one) on the ports 8160 and 8161
# (CALCU_WEB_PORT moves both); the library's tools/playwright.sh serves it.
# Arguments go to `playwright test`.
set -euo pipefail
WEB="$(cd "$(dirname "$0")" && pwd)"
exec "$WEB/../../../../library/web/tools/playwright.sh" "$WEB" playwright.config.js \
  "${CALCU_WEB_DIST:-$WEB/dist}" "${CALCU_WEB_PORT:-8160}" "$@"
