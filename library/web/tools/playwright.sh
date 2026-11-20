#!/usr/bin/env bash
# Runs Playwright tests against built dist/ folders, the same way for both
# products' frontends/web/run-tests.sh and the library's tests/run-tests.sh:
#
#   playwright.sh <package dir> <config> <served folder> <port> [playwright args]
#
# Two HTTP servers (serve.py): one serves the folder at the root of <port>,
# the other on the next port only under /anything/calc/ (everything else a
# 404) -- the sub-path that proves a client is location-independent. The
# config reads the port from CALCU_WEB_PORT. <package dir> holds the
# package.json with the Playwright version; outside CI (which installs it
# with the browsers' system dependencies) it and the browsers are installed.
set -euo pipefail

TOOLS="$(cd "$(dirname "$0")" && pwd)"
PACKAGE="$1" CONFIG="$2" SERVED="$3"
export CALCU_WEB_PORT="$4"
shift 4
SUBPATH_PORT=$((CALCU_WEB_PORT + 1))

# A dist/ has its MANIFEST.json last; the library's test-dist/ holds several.
if [ ! -f "$SERVED/MANIFEST.json" ] && ! ls "$SERVED"/*/MANIFEST.json >/dev/null 2>&1; then
  echo "error: $SERVED is not built. Run its build.sh first." >&2
  exit 1
fi
for port in "$CALCU_WEB_PORT" "$SUBPATH_PORT"; do
  if lsof -ti:"$port" > /dev/null 2>&1; then
    echo "error: port $port is in use (the caller's run-tests.sh names the variable that moves it)." >&2
    exit 1
  fi
done

PIDS=()
trap 'kill "${PIDS[@]}" 2>/dev/null' EXIT
python3 "$TOOLS/serve.py" "$SERVED" "$CALCU_WEB_PORT" &
PIDS+=($!)
python3 "$TOOLS/serve.py" "$SERVED" "$SUBPATH_PORT" /anything/calc &
PIDS+=($!)
sleep 1

cd "$PACKAGE"
if [ -z "${CI:-}" ]; then
  npm install --no-audit --no-fund
  npx playwright install chromium webkit
fi
npx playwright test --config "$CONFIG" "$@"
