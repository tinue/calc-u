#!/bin/sh
# build.sh: regenerate the faceplates (LCD off) next to this script:
#   pc1401.png, pc1402.png, pc1403h.png, pc1421.png
# Run by hand after changing the generator, and commit the outputs.
# Needs uv, Google Chrome (render.sh) and the tools/faceplate/ kit.
set -e
HERE=$(cd "$(dirname "$0")" && pwd)
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT

for model in 1401 1402 1403h 1421; do
    uv run "$HERE/build_svg.py" --model "$model" -o "$TMP/pc$model.svg"
    cp "$TMP/pc$model.png" "$HERE/pc$model.png"
done
