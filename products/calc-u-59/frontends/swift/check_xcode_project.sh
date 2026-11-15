#!/bin/sh
# The check of the Xcode project in the build (C59-5, Technical
# Requirements; ADR 0013): Xcode compiles only Swift and the bridge and
# links CalcU59.xcframework. Fails when the project takes in a folder or a
# source file of the library, the core or the app layer, or when the
# bridge reaches into the source tree instead of the XCFramework's
# headers (<calcu59/core/...>, <calcu59/app/...>). CTest runs it in the
# Frequent group (CalcU59XcodeProjectCheck).
set -u
cd "$(dirname "$0")"
PBX=Calc-U-59.xcodeproj/project.pbxproj
status=0
fail() { echo "FAIL: $1"; status=1; }

/usr/bin/grep -nE 'path = "?(\.\./)+(core|app|library)[/";]' "$PBX" && fail "$PBX refers to core/, app/ or library/"
/usr/bin/grep -nE '\.(cpp|cc|cxx)[ "]' "$PBX" && fail "$PBX names a C++ source file"
/usr/bin/grep -q 'CalcU59.xcframework in Frameworks' "$PBX" || fail "$PBX does not link CalcU59.xcframework"
/usr/bin/grep -nE '#(include|import) "(\.\./)+(core|app|library)/' Bridge/* App/*.swift 2>/dev/null &&
  fail "the bridge includes from the source tree, not from the XCFramework"
ls Bridge App 2>/dev/null | /usr/bin/grep -E '\.(cpp|cc|cxx)$' && fail "a C++ source file in Bridge/ or App/"

# LIB-12 (M-24): the ROM resources are the full set, flat in roms/: the ten
# calculator dumps and the 14 module files, none of the 2.3.0 module dumps.
/usr/bin/grep -nE 'roms/(solid-state/TMC05|calculator/)' "$PBX" && fail "$PBX bundles a 2.3.0 ROM path (roms/calculator/, roms/solid-state/TMC05xx)"
n=$(/usr/bin/grep -cE '/\* roms/[A-Z]{2}\.rom\.yaml in Resources \*/,' "$PBX")
[ "$n" = 14 ] || fail "$PBX bundles $n module files, not the 14 .rom.yaml"
n=$(/usr/bin/grep -cE '/\* roms/[A-Z0-9-]+\.txt in Resources \*/,' "$PBX")
[ "$n" = 10 ] || fail "$PBX bundles $n calculator ROM files, not 10"

[ $status = 0 ] && echo "ok: the Xcode project compiles only Swift and the bridge, links CalcU59.xcframework and bundles the 24 ROM files"
exit $status
