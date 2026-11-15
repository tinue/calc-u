#!/bin/sh
# LIB-12, Technical Requirements (design §6, P6; D-5): "a build of each
# product with one ROM file taken away". The ROM folder is copied without
# one file (default TMC0583.txt; another as $1), the headless library and the
# web client's dist/ are built from it, and both start without the full set:
#   - tools/headless/ti59.py ends with exit 1 and names the file (AC-7);
#   - the web client shows the lookup's text naming it and no calculator
#     (AC-5, AC-10; Playwright, tests/rom-missing.spec.js).
# The build itself succeeds (D-5). Run from products/calc-u-59 (CTest
# CalcU59RomMissingBuild, Longer); needs emcmake, node and Playwright.
set -eu
cd "$(dirname "$0")/.."
PRODUCT=$(pwd)
ROOT=$(cd ../.. && pwd)
MISSING="${1:-TMC0583.txt}"
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT

mkdir "$TMP/roms"
cp roms/*.txt roms/*.rom.yaml "$TMP/roms/"
mkdir "$TMP/roms/solid-state"
cp roms/solid-state/cuecards.txt "$TMP/roms/solid-state/"
[ -f "$TMP/roms/$MISSING" ] || { echo "no ROM file $MISSING in roms/" >&2; exit 2; }
rm "$TMP/roms/$MISSING"
echo "rom_missing_build: roms/ without $MISSING"

# The headless library: TI59HEADLESS_LIB (CTest hands the tree's), else
# tools/headless/build.sh builds it.
[ -n "${TI59HEADLESS_LIB:-}" ] || tools/headless/build.sh >/dev/null
if TI59_ROMS_DIR="$TMP/roms" python3 tools/headless/ti59.py 5 + 7 = >"$TMP/out" 2>"$TMP/err"; then
  echo "FAIL: ti59.py started without $MISSING" >&2
  exit 1
fi
grep -q "$MISSING: missing" "$TMP/err" || { echo "FAIL: ti59.py does not name $MISSING:" >&2; cat "$TMP/err" >&2; exit 1; }
echo "rom_missing_build: headless names $MISSING: ok"

# The web client, built from that folder into its own dist/.
CALCU59_ROMS_DIR="$TMP/roms" CALCU59_WEB_DIST="$TMP/dist" frontends/web/build.sh >/dev/null
[ ! -e "$TMP/dist/wasm/roms/$MISSING" ] || { echo "FAIL: dist/ has $MISSING" >&2; exit 1; }
CALCU59_WEB_DIST="$TMP/dist" CALCU59_ROM_TAKEN_AWAY="$MISSING" \
  frontends/web/run-tests.sh tests/rom-missing.spec.js -g "taken_away" --project root
echo "rom_missing_build: web names $MISSING: ok"
