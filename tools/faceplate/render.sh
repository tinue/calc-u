#!/bin/sh
# render.sh in.svg out.png [scale]  -- renders the SVG with headless Chrome (transparent background)
# The page size is taken from the width/height attributes of the <svg> element.
# CHROME names the browser when it is not Google Chrome in /Applications (macOS).
set -e
IN=$(cd "$(dirname "$1")" && pwd)/$(basename "$1"); OUT=$2; S=${3:-1}
W=$(head -c 400 "$IN" | sed -n 's/.*<svg[^>]* width="\([0-9]*\)".*/\1/p')
H=$(head -c 400 "$IN" | sed -n 's/.*<svg[^>]* height="\([0-9]*\)".*/\1/p')
TMP=$(mktemp -d)
printf '<html><body style="margin:0;background:transparent"><img src="file://%s" style="display:block;width:%spx;height:%spx"></body></html>' "$IN" "$W" "$H" > "$TMP/p.html"
"${CHROME:-/Applications/Google Chrome.app/Contents/MacOS/Google Chrome}" --headless=new --disable-gpu --hide-scrollbars \
  --default-background-color=00000000 --force-device-scale-factor="$S" --window-size="$W,$H" \
  --allow-file-access-from-files --screenshot="$OUT" "file://$TMP/p.html" >/dev/null 2>&1
rm -rf "$TMP"
