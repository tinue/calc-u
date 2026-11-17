#!/usr/bin/env bash
# Runs the web client's Playwright tests against frontends/web/dist/ (build it
# first: frontends/web/build.sh). Two HTTP servers (library/web/tools/serve.py):
# one serves dist/ at its root, the other only under /anything/calc/
# (everything else a 404) -- the sub-path that proves the client is
# location-independent.
set -euo pipefail

WEB="$(cd "$(dirname "$0")" && pwd)"
SERVE="$WEB/../../../../library/web/tools/serve.py"   # the library's test server
DIST="${CALCU1600_WEB_DIST:-$WEB/dist}"
export CALCU_WEB_PORT="${CALCU_WEB_PORT:-8160}"
SUBPATH_PORT=$((CALCU_WEB_PORT + 1))

if [ ! -f "$DIST/MANIFEST.json" ]; then
  echo "error: $DIST is not built. Run frontends/web/build.sh first." >&2
  exit 1
fi
for port in "$CALCU_WEB_PORT" "$SUBPATH_PORT"; do
  if lsof -ti:"$port" > /dev/null 2>&1; then
    echo "error: port $port is in use (set CALCU_WEB_PORT)." >&2
    exit 1
  fi
done

PIDS=()
trap 'kill "${PIDS[@]}" 2>/dev/null' EXIT
python3 "$SERVE" "$DIST" "$CALCU_WEB_PORT" &
PIDS+=($!)
python3 "$SERVE" "$DIST" "$SUBPATH_PORT" /anything/calc &
PIDS+=($!)
sleep 1

cd "$WEB"
# CI installs these itself (with the browsers' system dependencies).
if [ -z "${CI:-}" ]; then
  npm install --no-audit --no-fund
  npx playwright install chromium webkit
fi
npx playwright test "$@"
