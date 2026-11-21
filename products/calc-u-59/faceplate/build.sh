#!/bin/sh
# build.sh: regenerate the faceplates, the cue card templates and the key rectangles from the generators.
#   frontends/swift/App/Assets.xcassets/<name>.imageset/<name>.png   (used by Swift, Qt and Web):
#     ti59_base, ti58_base, CueCard, MagnetCard, SolidStateCard
#   app/KeyboardLayout.hpp kKeyRects                                  (the key table)
# Run by hand after changing a generator, and commit the outputs: the builds do not render.
# Needs uv, Google Chrome (render.sh) and the tools/faceplate/ kit.
set -e
HERE=$(cd "$(dirname "$0")" && pwd)
ASSETS=$(dirname "$HERE")/frontends/swift/App/Assets.xcassets
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT

uv run "$HERE/build_cards.py" -o "$TMP"
uv run "$HERE/build_svg_ti59.py" --keep-svg -o "$TMP/ti59.svg"
uv run "$HERE/build_svg_ti59.py" --model 58 --keep-svg -o "$TMP/ti58.svg"
for name in CueCard MagnetCard SolidStateCard; do
    cp "$TMP/$name.png" "$ASSETS/$name.imageset/$name.png"
done
cp "$TMP/ti59.png" "$ASSETS/ti59_base.imageset/ti59_base.png"
cp "$TMP/ti58.png" "$ASSETS/ti58_base.imageset/ti58_base.png"
python3 "$HERE/keys.py" "$TMP"
