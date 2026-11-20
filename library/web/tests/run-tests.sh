#!/usr/bin/env bash
# Runs the shell's Playwright tests (CTest
# LibraryWebPlaywright, Longer) against the test product's dist/ folders in
# build/web/library/web/test-dist/, building them first
# (tests/product/build.sh; CALCU_SKIP_BUILD: not), on the ports 8170 and
# 8171 (CALCU_LIB_WEB_PORT moves both); tools/playwright.sh serves them,
# as it does the products'. Arguments go to `playwright test`.
set -euo pipefail

TESTS="$(cd "$(dirname "$0")" && pwd)"
WEB="$(cd "$TESTS/.." && pwd)"
ROOT="$(cd "$WEB/../.." && pwd)"
export CALCU_TEST_DIST="$ROOT/build/web/library/web/test-dist"

if [ -z "${CALCU_SKIP_BUILD:-}" ]; then
  "$TESTS/product/build.sh"
fi
exec "$WEB/tools/playwright.sh" "$WEB" tests/playwright.config.js "$CALCU_TEST_DIST" "${CALCU_LIB_WEB_PORT:-8170}" "$@"
