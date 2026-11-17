#!/bin/sh
# LIB-12, Technical Requirements (design §6, P6; D-5): "a build of each
# product with one ROM file taken away". The ROM folder is copied without
# one file (default TMC0583.txt; another as $1), the web client's dist/ is
# built from it, and neither starts with it:
#   - the harness `calcu` (CALCU, CTest hands the tree's) on that folder
#     (CALCU59_ROMS_DIR) ends with exit 2 and names the file (AC-7);
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
[ -f "$TMP/roms/$MISSING" ] || { echo "no ROM file $MISSING in roms/" >&2; exit 2; }
rm "$TMP/roms/$MISSING"
echo "rom_missing_build: roms/ without $MISSING"

# The harness: CALCU (CTest hands the tree's), else the build/tests tree's.
CALCU="${CALCU:-$ROOT/build/tests/library/harness/cli/calcu}"
status=0
CALCU59_ROMS_DIR="$TMP/roms" "$CALCU" --product 59 wait 1s >"$TMP/out" 2>"$TMP/err" || status=$?
[ "$status" -eq 2 ] || { echo "FAIL: calcu started without $MISSING (exit $status)" >&2; exit 1; }
grep -q "$MISSING: missing" "$TMP/err" || { echo "FAIL: calcu does not name $MISSING:" >&2; cat "$TMP/err" >&2; exit 1; }
echo "rom_missing_build: the harness names $MISSING: ok"

# The web client, built from that folder into its own dist/.
CALCU59_ROMS_DIR="$TMP/roms" CALCU59_WEB_DIST="$TMP/dist" frontends/web/build.sh >/dev/null
[ ! -e "$TMP/dist/bundle/$MISSING" ] || { echo "FAIL: dist/ has $MISSING" >&2; exit 1; }
CALCU59_WEB_DIST="$TMP/dist" CALCU59_ROM_TAKEN_AWAY="$MISSING" \
  frontends/web/run-tests.sh tests/rom-missing.spec.js -g "taken_away" --project root
echo "rom_missing_build: web names $MISSING: ok"
