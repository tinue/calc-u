#!/usr/bin/env bash
# Runs the shell's Playwright tests (LIB-15 design §6; CTest
# LibraryWebPlaywright, Longer) against the test product's dist/ folders in
# build/web/library/web/test-dist/, building them first
# (tests/product/build.sh). Two HTTP servers (tools/serve.py): one serves
# test-dist/ at its root, the other only under /anything/calc/ (AC-13).
# Arguments go to `playwright test`.
set -euo pipefail

TESTS="$(cd "$(dirname "$0")" && pwd)"
WEB="$(cd "$TESTS/.." && pwd)"
ROOT="$(cd "$WEB/../.." && pwd)"
export CALCU_TEST_DIST="$ROOT/build/web/library/web/test-dist"
export CALCU_LIB_WEB_PORT="${CALCU_LIB_WEB_PORT:-8170}"
SUBPATH_PORT=$((CALCU_LIB_WEB_PORT + 1))

if [ -z "${CALCU_SKIP_BUILD:-}" ]; then
  "$TESTS/product/build.sh"
fi
for port in "$CALCU_LIB_WEB_PORT" "$SUBPATH_PORT"; do
  if lsof -ti:"$port" > /dev/null 2>&1; then
    echo "error: port $port is in use (set CALCU_LIB_WEB_PORT)." >&2
    exit 1
  fi
done

PIDS=()
trap 'kill "${PIDS[@]}" 2>/dev/null' EXIT
python3 "$WEB/tools/serve.py" "$CALCU_TEST_DIST" "$CALCU_LIB_WEB_PORT" &
PIDS+=($!)
python3 "$WEB/tools/serve.py" "$CALCU_TEST_DIST" "$SUBPATH_PORT" /anything/calc &
PIDS+=($!)
sleep 1

cd "$WEB"
# CI installs these itself (with the browsers' system dependencies).
if [ -z "${CI:-}" ]; then
  npm install --no-audit --no-fund
  npx playwright install chromium webkit
fi
npx playwright test --config tests/playwright.config.js "$@"
