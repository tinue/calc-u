#!/bin/sh
# LIB-13 AC-14 (design §10.6, E13-5; D-19; CTest CalcU59OpenWithTypes): the
# file types the Qt6 app is opened with by the file manager are today's three
# preset extensions, on macOS (Info.plist), Windows (the installer) and Linux
# (the desktop entry), and no other. Run from the product directory (CTest does).
set -e
R=frontends/qt6/resources
fail() { echo "check_open_with_types: $*" >&2; exit 1; }
for f in $R/macos/Info.plist.in $R/windows/calc-u-59.iss $R/linux/calc-u-59.desktop; do
  [ -f "$f" ] || fail "$f is missing"
done

mac=$(awk '/<key>CFBundleTypeExtensions<\/key>/ { found = 1; next }
           found && /<\/array>/ { exit }
           found && /<string>/ { s = $0; sub(/.*<string>/, "", s); sub(/<\/string>.*/, "", s); print s }' \
      $R/macos/Info.plist.in | tr '\n' ' ')
[ "$mac" = "ti59 ti58 ti58c " ] || fail "macOS: the extensions are '$mac', not 'ti59 ti58 ti58c'"

win=$(sed -n 's/.*Subkey: "Software\\Classes\\\.\([a-z0-9]*\)".*/\1/p' $R/windows/calc-u-59.iss | tr '\n' ' ')
[ "$win" = "ti59 ti58 ti58c " ] || fail "Windows: the extensions are '$win', not 'ti59 ti58 ti58c'"
grep -q '"%1"' $R/windows/calc-u-59.iss || fail "Windows: the file is not handed over as an argument"

grep -q '^Exec=Calc-U-59 %f$' $R/linux/calc-u-59.desktop || fail "Linux: Exec does not hand over one file (%f)"
grep -q '^MimeType=application/x-calcu59-state;$' $R/linux/calc-u-59.desktop || fail "Linux: the MIME type of the presets is not the only one"
echo "check_open_with_types: ok"
