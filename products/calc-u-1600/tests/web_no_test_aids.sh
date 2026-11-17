#!/bin/sh
# LIB-15 AC-29 (release prep 4): a release Web dist/ (frontends/web/build.sh
# --release: the CMake option CALCU_WEB_TEST_AIDS off) answers none of the
# test-only commands, so its handle has no `c1600.upload` and no
# `c1600.display-text` (the Playwright tests use them on the test build).
#   tests/web_no_test_aids.sh <dist>   checks that folder (CI: the one it zips)
#   tests/web_no_test_aids.sh          builds a release dist/ into a temporary
#                                      folder first (CTest CalcU1600WebNoTestAids,
#                                      Longer; needs emcmake and node)
# Run from anywhere; the names are looked for in every file of dist/.
set -eu
cd "$(dirname "$0")/.."
if [ $# -gt 0 ]; then
  DIST=$(cd "$1" && pwd)
else
  TMP=$(mktemp -d)
  trap 'rm -rf "$TMP"' EXIT
  DIST="$TMP/dist"
  CALCU1600_WEB_DIST="$DIST" frontends/web/build.sh --release >/dev/null
fi
WASM="$DIST/wasm/calcu1600-core.wasm"
[ -f "$WASM" ] || { echo "FAIL: no $WASM" >&2; exit 1; }
# The search finds command names: one the release keeps is there.
grep -aqF "c1600.shell-product" "$WASM" || { echo "FAIL: c1600.shell-product not found in $WASM" >&2; exit 1; }
found=$(grep -rlaF -e "c1600.upload" -e "c1600.display-text" "$DIST" || true)
if [ -n "$found" ]; then
  echo "FAIL: a test-only command in the release dist/ (LIB-15 AC-29):" >&2
  echo "$found" >&2
  exit 1
fi
echo "web_no_test_aids: $DIST has no c1600.upload, no c1600.display-text: ok"
