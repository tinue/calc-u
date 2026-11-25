#!/bin/sh
# build.sh: regenerate the faceplates (LCD off) and the key rectangles from the generators.
#   frontends/qt6/resources/faceplates/pc1500.png, pc1500A.png, pc1600.png   (used by Qt and Web)
#   frontends/qt6/resources/faceplates/pc1600K.png                          (the PC-1600K; shares the PC-1600's key table)
#   app/KeyLayout/PC1500KeyLayout.cpp, PC1600KeyLayout.cpp                   (the key tables)
# Run by hand after changing a generator, and commit the outputs: the builds do not render.
# Needs uv, Google Chrome (render.sh) and the tools/faceplate/ kit.
set -e
HERE=$(cd "$(dirname "$0")" && pwd)
PRODUCT=$(dirname "$HERE")
OUT=$PRODUCT/frontends/qt6/resources/faceplates
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT

uv run "$HERE/build_svg_1500.py" --keep-svg -o "$TMP/pc1500.svg"
uv run "$HERE/build_svg_1500.py" --model 1500a --keep-svg -o "$TMP/pc1500a.svg"
uv run "$HERE/build_svg_1600.py" --keep-svg -o "$TMP/pc1600.svg"
uv run "$HERE/build_svg_1600.py" --model 1600k --keep-svg -o "$TMP/pc1600k.svg"
cp "$TMP/pc1500.png" "$OUT/pc1500.png"
cp "$TMP/pc1500a.png" "$OUT/pc1500A.png"
cp "$TMP/pc1600.png" "$OUT/pc1600.png"
cp "$TMP/pc1600k.png" "$OUT/pc1600K.png"
python3 "$HERE/keys.py" "$TMP"
