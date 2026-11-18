#!/bin/sh
# Builds CalcU59.xcframework: the library, the
# Calc-U-59 core and the Calc-U-59 app layer from the repository's root
# CMake project, as one static library per platform -- macOS (arm64,
# x86_64), iOS devices (arm64), the iOS simulator (arm64, x86_64) -- with
# their headers under calcu59/core/ and calcu59/app/, and the library's
# (library/**/*.hpp) under their area folders, machine/Machine.hpp
# among them.
#
#   frontends/swift/build_xcframework.sh [Debug|Release]   # default Release
#
# The Xcode project runs it as the target CalcU59Core, which the app
# depends on, so every Xcode build builds the XCFramework from the current
# sources first; run by hand it does the same. Output:
# build/xcframework-59/CalcU59.xcframework at the repository root, built
# in build/xcframework-59/<configuration>/<platform>/. Needs CMake and
# Ninja (Homebrew).
set -eu

CONFIG="${1:-${CONFIGURATION:-Release}}"
[ "$CONFIG" = Debug ] || CONFIG=Release

# From Xcode: drop its build environment (SDKROOT, deployment targets,
# ARCHS ...), which would steer CMake's compilers, and start clean.
if [ -z "${CALCU_XCF_CLEAN_ENV:-}" ]; then
  exec env -i HOME="$HOME" USER="${USER:-}" TMPDIR="${TMPDIR:-/tmp}" CALCU_XCF_CLEAN_ENV=1 \
    PATH="/opt/homebrew/bin:/usr/local/bin:/usr/bin:/bin:/usr/sbin:/sbin" \
    /bin/sh "$0" "$CONFIG"
fi

for tool in cmake ninja; do
  command -v "$tool" >/dev/null || {
    echo "error: $tool not found (brew install cmake ninja): CalcU59.xcframework cannot be built from the current sources" >&2
    exit 1
  }
done

PRODUCT="$(cd "$(dirname "$0")/../.." && pwd)"
REPO="$(cd "$PRODUCT/../.." && pwd)"
OUT="$REPO/build/xcframework-59"
TREES="$OUT/$CONFIG"
XCF="$OUT/CalcU59.xcframework"

slice() {  # slice <name> <attach ON|OFF> <cmake args...>
  name="$1"; attach="$2"; shift 2
  tree="$TREES/$name"
  # Configured every time (fast when nothing changed), so a tree made before
  # an option existed takes it too.
  cmake -S "$REPO" -B "$tree" -G Ninja -DCMAKE_BUILD_TYPE="$CONFIG" \
    -DCALCU_PRODUCT_1600=OFF -DCALCU_TESTS=OFF -DCALCU_FRONTEND_QT=OFF -DCALCU_ATTACH="$attach" "$@" >/dev/null
  targets="calcu59_shared calcu59_core calcu_library"
  # Harness attach: the attached harness needs the
  # product's harness entry (its display rule); only in the slices with attach.
  [ "$attach" = ON ] && targets="$targets calcu59_harness"
  cmake --build "$tree" --target $targets
  lib="$tree/libCalcU59.a"
  core="$tree/products/calc-u-59/libcalcu59_core.a"
  app="$tree/products/calc-u-59/libcalcu59_shared.a"
  library="$tree/library/libcalcu_library.a"
  harness=""
  [ "$attach" = ON ] && harness="$tree/products/calc-u-59/libcalcu59_harness.a"
  stamp="$tree/.calcu-attach"
  if [ ! -f "$lib" ] || [ "$core" -nt "$lib" ] || [ "$app" -nt "$lib" ] || [ "$library" -nt "$lib" ] ||
     { [ -n "$harness" ] && [ "$harness" -nt "$lib" ]; } || [ "$(cat "$stamp" 2>/dev/null)" != "$attach" ]; then
    libtool -static -no_warning_for_no_symbols -o "$lib" "$core" "$app" "$library" $harness
    echo "$attach" > "$stamp"
  fi
}

# Attach exactly in Debug for macOS and the simulator, never for a device or
# in Release.
DEV_ATTACH=OFF
[ "$CONFIG" = Debug ] && DEV_ATTACH=ON
slice macos "$DEV_ATTACH" -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64" -DCMAKE_OSX_DEPLOYMENT_TARGET=15.6
slice ios OFF -DCMAKE_SYSTEM_NAME=iOS -DCMAKE_OSX_SYSROOT=iphoneos -DCMAKE_OSX_ARCHITECTURES=arm64 \
  -DCMAKE_OSX_DEPLOYMENT_TARGET=18.0
slice ios-simulator "$DEV_ATTACH" -DCMAKE_SYSTEM_NAME=iOS -DCMAKE_OSX_SYSROOT=iphonesimulator \
  -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64" -DCMAKE_OSX_DEPLOYMENT_TARGET=18.0

# Headers: copied only when they changed, so an unchanged tree keeps the
# XCFramework as it is.
HEADERS="$TREES/headers"
for layer in core app; do
  mkdir -p "$HEADERS/calcu59/$layer"
  for h in "$PRODUCT/$layer"/*.hpp; do
    cmp -s "$h" "$HEADERS/calcu59/$layer/$(basename "$h")" || cp "$h" "$HEADERS/calcu59/$layer/"
  done
  for h in "$HEADERS/calcu59/$layer"/*.hpp; do
    [ -f "$PRODUCT/$layer/$(basename "$h")" ] || rm -f "$h"
  done
done

# The library's headers keep their area folders (headers/machine/Machine.hpp).
( cd "$REPO/library" && find . -name '*.hpp' ) | while read -r rel; do
  mkdir -p "$HEADERS/$(dirname "$rel")"
  cmp -s "$REPO/library/$rel" "$HEADERS/$rel" || cp "$REPO/library/$rel" "$HEADERS/$rel"
done
( cd "$HEADERS" && find . -name '*.hpp' -not -path './calcu59/*' ) | while read -r rel; do
  [ -f "$REPO/library/$rel" ] || rm -f "$HEADERS/$rel"
done

# The XCFramework is made again when a library or a header is newer than
# it, or it was made for the other configuration.
STAMP="$XCF/.calcu-configuration"
stale=0
if [ ! -f "$STAMP" ] || [ "$(cat "$STAMP")" != "$CONFIG" ]; then
  stale=1
elif [ -n "$(find "$TREES" \( -name libCalcU59.a -o -name '*.hpp' \) -path "$TREES/*" -newer "$STAMP" | head -1)" ]; then
  stale=1
fi
if [ "$stale" = 1 ]; then
  rm -rf "$XCF"
  xcodebuild -create-xcframework \
    -library "$TREES/macos/libCalcU59.a" -headers "$HEADERS" \
    -library "$TREES/ios/libCalcU59.a" -headers "$HEADERS" \
    -library "$TREES/ios-simulator/libCalcU59.a" -headers "$HEADERS" \
    -output "$XCF" >/dev/null
  echo "$CONFIG" > "$STAMP"
  echo "built $XCF ($CONFIG)"
else
  echo "$XCF is up to date ($CONFIG)"
fi
