#!/bin/sh
# The file types a product's Qt6 app is opened with by the file manager, on
# macOS (Info.plist.in), Windows (the installer) and Linux (the desktop
# entry and the presets' MIME type), and no other. Each product's CTest
# <Product>OpenWithTypes runs it in the product directory:
#
#   check_open_with_types.sh <App> "<presets>" ["<alternates>"]
#
# <presets> are the product's preset extensions, in the order the files list
# them, on every platform. <alternates> are further extensions the app opens
# on macOS only as an alternative to their own app (Alternate); with them the
# presets are an exported type of the app's own (Owner).
set -e
APP=$1
PRESETS="$2 "
ALTERNATES=${3:+"$3 "}
LIB=$(cd "$(dirname "$0")/.." && pwd)   # library/qt6
app=$(echo "$APP" | tr '[:upper:]' '[:lower:]')
R=frontends/qt6/resources
PLIST=$R/macos/Info.plist.in
ISS=$R/windows/$app.iss
DESKTOP=$R/linux/$app.desktop
MIME=$R/linux/$app.xml
fail() { echo "check_open_with_types: $*" >&2; exit 1; }
for f in "$PLIST" "$ISS" "$DESKTOP" "$MIME" "$LIB/windows/CalcUApp.iss"; do
  [ -f "$f" ] || fail "$f is missing"
done

# The strings of every array after a key, one line.
arrays_after() {
  awk -v key="<key>$1</key>" '
    index($0, key) { found = 1; next }
    found && /<\/array>/ { found = 0; next }
    found && /<string>/ { s = $0; sub(/.*<string>/, "", s); sub(/<\/string>.*/, "", s); print s }
  ' "$PLIST" | tr '\n' ' '
}
owned=$(arrays_after public.filename-extension)
documents=$(arrays_after CFBundleTypeExtensions)
if [ -n "$ALTERNATES" ]; then
  [ "$owned" = "$PRESETS" ] || fail "macOS: the exported preset extensions are '$owned', not '$PRESETS'"
  [ "$documents" = "$ALTERNATES" ] || fail "macOS: the other extensions are '$documents', not '$ALTERNATES'"
  grep -q '<string>Owner</string>' "$PLIST" || fail "macOS: the presets are not the app's own (Owner)"
  grep -q '<string>Alternate</string>' "$PLIST" || fail "macOS: the other files are not Alternate"
else
  [ -z "$owned" ] || fail "macOS: exported extensions '$owned' without alternates"
  [ "$documents" = "$PRESETS" ] || fail "macOS: the extensions are '$documents', not '$PRESETS'"
fi

win=$(sed -n 's/.*Subkey: "Software\\Classes\\\.\([a-z0-9]*\)".*/\1/p' "$ISS" | tr '\n' ' ')
[ "$win" = "$PRESETS" ] || fail "Windows: the extensions are '$win', not '$PRESETS'"
grep -q '"%1"' "$LIB/windows/CalcUApp.iss" || fail "Windows: the file is not handed over as an argument"

grep -q "^Exec=$APP %f\$" "$DESKTOP" || fail "Linux: Exec does not hand over one file (%f)"
type=$(sed -n 's/^MimeType=\(.*\);$/\1/p' "$DESKTOP")
case "$type" in
  ''|*';'*) fail "Linux: the desktop entry names not exactly one MIME type: '$type'" ;;
esac
grep -q "<mime-type type=\"$type\">" "$MIME" || fail "Linux: $MIME does not define $type"
globs=$(sed -n 's/.*<glob pattern="\*\.\([a-z0-9]*\)"\/>.*/\1/p' "$MIME" | tr '\n' ' ')
[ "$globs" = "$PRESETS" ] || fail "Linux: the MIME type's extensions are '$globs', not '$PRESETS'"
echo "check_open_with_types: ok"
