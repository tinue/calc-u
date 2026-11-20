#!/bin/sh
# CTest CalcU1600OpenWithTypes: the file types the Qt6 app is opened with
# by the file manager, on macOS (Info.plist), Windows (the installer) and
# Linux (the desktop entry). The three preset extensions are the product's
# own; on macOS `.bas`, `.bbin` and `.bin` are Alternate, and `.rom` is no
# name of a program file and is not listed.
# Run from the product directory (CTest does).
set -e
R=frontends/qt6/resources
PLIST=${PLIST:-$R/macos/Info.plist.in}
fail() { echo "check_open_with_types: $*" >&2; exit 1; }
for f in "$PLIST" $R/windows/calc-u-1600.iss $R/linux/calc-u-1600.desktop; do
  [ -f "$f" ] || fail "$f is missing"
done

# The strings of the first array after a key, one per line.
array_after() {
  awk -v key="<key>$1</key>" '
    index($0, key) { found = 1; next }
    found && /<\/array>/ { exit }
    found && /<string>/ { s = $0; sub(/.*<string>/, "", s); sub(/<\/string>.*/, "", s); print s }
  ' "$PLIST"
}

presets=$(array_after public.filename-extension | tr '\n' ' ')
[ "$presets" = "pc1500 pc1500a pc1600 " ] || fail "macOS: preset extensions are '$presets', not 'pc1500 pc1500a pc1600'"
programs=$(array_after CFBundleTypeExtensions | tr '\n' ' ')
[ "$programs" = "bas bbin bin " ] || fail "macOS: program extensions are '$programs', not 'bas bbin bin'"
grep -q '<string>Owner</string>' "$PLIST" || fail "macOS: the presets are not the product's own (Owner)"
grep -q '<string>Alternate</string>' "$PLIST" || fail "macOS: the program files are not Alternate"
if grep -q '<string>rom</string>' "$PLIST"; then fail "macOS: .rom is listed"; fi

win=$(sed -n 's/.*Subkey: "Software\\Classes\\\.\([a-z0-9]*\)".*/\1/p' $R/windows/calc-u-1600.iss | tr '\n' ' ')
[ "$win" = "pc1500 pc1500a pc1600 " ] || fail "Windows: the extensions are '$win', not 'pc1500 pc1500a pc1600'"
grep -q '"%1"' $R/windows/calc-u-1600.iss || fail "Windows: the file is not handed over as an argument"

grep -q '^Exec=Calc-U-1600 %f$' $R/linux/calc-u-1600.desktop || fail "Linux: Exec does not hand over one file (%f)"
grep -q '^MimeType=' $R/linux/calc-u-1600.desktop || fail "Linux: the desktop entry names no MIME type of the presets"
echo "check_open_with_types: ok"
