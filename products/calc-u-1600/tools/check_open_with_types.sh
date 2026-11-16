#!/bin/sh
# LIB-13 AC-21 (design Part D, T16-5; CTest CalcU1600OpenWithTypes): the
# file types Finder's Open With offers for the Qt6 app on macOS. The three
# preset extensions are the product's own (Owner); `.bas`, `.bbin` and `.bin`
# are Alternate; `.rom` is no name of a program file and is not listed.
# Run from the product directory (CTest does).
set -e
PLIST=${PLIST:-frontends/qt6/resources/macos/Info.plist.in}
fail() { echo "check_open_with_types: $*" >&2; exit 1; }
[ -f "$PLIST" ] || fail "$PLIST is missing"

# The strings of the first array after a key, one per line.
array_after() {
  awk -v key="<key>$1</key>" '
    index($0, key) { found = 1; next }
    found && /<\/array>/ { exit }
    found && /<string>/ { s = $0; sub(/.*<string>/, "", s); sub(/<\/string>.*/, "", s); print s }
  ' "$PLIST"
}

presets=$(array_after public.filename-extension | tr '\n' ' ')
[ "$presets" = "pc1500 pc1500a pc1600 " ] || fail "preset extensions are '$presets', not 'pc1500 pc1500a pc1600'"
programs=$(array_after CFBundleTypeExtensions | tr '\n' ' ')
[ "$programs" = "bas bbin bin " ] || fail "program extensions are '$programs', not 'bas bbin bin'"
grep -q '<string>Owner</string>' "$PLIST" || fail "the presets are not the product's own (Owner)"
grep -q '<string>Alternate</string>' "$PLIST" || fail "the program files are not Alternate"
if grep -q '<string>rom</string>' "$PLIST"; then fail ".rom is listed"; fi
echo "check_open_with_types: ok"
