#!/bin/sh
# LIB-12 AC-13 (design §6, Q2; LIB-11 AC-20): the CLIs take their ROMs
# through the library's lookup, from roms/ relative to where they run. With
# a roms/ that lacks one file of the full set, pc1600_cli --preset ends with
# exit code 1 and names the file; with the full set it runs the preset.
# CTest CalcU1600CliRomLookup (Frequent) passes the CLI's build as
# PC1600_CLI. Run from products/calc-u-1600; skips (exit 0) without the
# fetched ROMs.
set -eu
cd "$(dirname "$0")/.."
CLI="${PC1600_CLI:?PC1600_CLI is the pc1600_cli to test}"
LEFT_OUT=PC1600-P1-B3-old.bin
if [ ! -f roms/PC1600-P0-B0-new.bin ] || [ ! -f roms/CE-502B.card.yaml ]; then
  echo "SKIP: no ROMs in roms/ (roms/fetch_roms.sh)"
  exit 0
fi
PRESET="$(pwd)/tests/fixtures/presets/hanoi.pc1600"
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT
mkdir "$TMP/roms"
for f in roms/*.ROM roms/*.bin roms/*.card.yaml; do cp "$f" "$TMP/roms/"; done
cp -R tests "$TMP/" 2>/dev/null || true

# Without one file: no machine, exit 1, the file named.
rm "$TMP/roms/$LEFT_OUT"
set +e
out=$(cd "$TMP" && "$CLI" --preset "$PRESET" 2>&1)
code=$?
set -e
if [ $code -ne 1 ]; then
  echo "FAIL: exit code $code without $LEFT_OUT (expected 1)"; echo "$out"; exit 1
fi
case "$out" in
  *"$LEFT_OUT: missing"*) ;;
  *) echo "FAIL: the message does not name $LEFT_OUT"; echo "$out"; exit 1 ;;
esac

# With the full set the same preset runs.
cp "roms/$LEFT_OUT" "$TMP/roms/"
if ! (cd "$TMP" && "$CLI" --preset "$PRESET" > /dev/null 2>&1); then
  echo "FAIL: the preset does not run with the full set"; exit 1
fi
echo "cli ROM lookup: ok"
