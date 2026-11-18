#!/bin/sh
# The bundled example presets of the Swift app:
# examples/bundle.yaml and exactly the files it lists, each
# `{ file: ... }`, into <repository>/build/bundle-59/examples, which the app
# target bundles as the folder `examples` (its Resources). The phase "Build
# CalcU59.xcframework" calls it; the Qt6 and Web builds take the same list
# (frontends/qt6/CMakeLists.txt, frontends/web/build.js).
set -eu
SWIFT="$(cd "$(dirname "$0")" && pwd)"
EXAMPLES="$SWIFT/../../examples"
OUT="$SWIFT/../../../../build/bundle-59/examples"
rm -rf "$OUT"
mkdir -p "$OUT"
cp "$EXAMPLES/bundle.yaml" "$OUT/bundle.yaml"
sed -n 's/.*{ *file: *\([^ }]*\) *}.*/\1/p' "$EXAMPLES/bundle.yaml" | sort -u | while read -r f; do
  [ -f "$EXAMPLES/$f" ] || { echo "error: bundle.yaml: $f is not in examples/" >&2; exit 1; }
  mkdir -p "$OUT/$(dirname "$f")"
  cp "$EXAMPLES/$f" "$OUT/$f"
done
echo "copied the files of bundle.yaml into $OUT"
