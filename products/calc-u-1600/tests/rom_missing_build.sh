#!/bin/sh
# LIB-12, Technical Requirements (design §6, Q5; D-5): "a build of each
# product with one ROM file taken away". The ROM folder is copied without
# one file (default PC1600-P1-B3-old.bin; another as $1), the web client's
# dist/ is built from it, and it starts without the full set: the page
# shows the lookup's text naming the file and no calculator (AC-5, AC-13;
# Playwright, frontends/web/tests/rom-missing.spec.js). The build itself
# succeeds (D-5). The CLIs' side is tests/cli_rom_tests.sh (Frequent); the
# Qt6 app is checked by hand (D-8, qa-report.md). Run from
# products/calc-u-1600 (CTest CalcU1600RomMissingBuild, Longer); needs
# emcmake, node and Playwright.
set -eu
cd "$(dirname "$0")/.."
MISSING="${1:-PC1600-P1-B3-old.bin}"
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT

mkdir "$TMP/roms"
for f in roms/*.ROM roms/*.bin roms/*.module.yaml; do cp "$f" "$TMP/roms/"; done
[ -f "$TMP/roms/$MISSING" ] || { echo "no ROM file $MISSING in roms/" >&2; exit 2; }
rm "$TMP/roms/$MISSING"
echo "rom_missing_build: roms/ without $MISSING"

# The web client, built from that folder into its own dist/.
CALCU1600_ROMS_DIR="$TMP/roms" CALCU1600_WEB_DIST="$TMP/dist" frontends/web/build.sh >/dev/null
[ ! -e "$TMP/dist/bundle/$MISSING" ] || { echo "FAIL: dist/ has $MISSING" >&2; exit 1; }
CALCU1600_WEB_DIST="$TMP/dist" CALCU1600_ROM_TAKEN_AWAY="$MISSING" \
  frontends/web/run-tests.sh tests/rom-missing.spec.js -g "taken_away" --project root
echo "rom_missing_build: web names $MISSING: ok"
