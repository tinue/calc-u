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

# C59-5 AC-10, AC-13: only the Debug configuration may take incoming
# connections (harness attach); Release keeps the plain entitlements.
/usr/bin/grep -q 'com.apple.security.network.server' Calc-U-59-Debug.entitlements ||
  fail "Calc-U-59-Debug.entitlements has no com.apple.security.network.server (attach, C59-5 AC-10)"
/usr/bin/grep -q 'com.apple.security.network.server' Calc-U-59.entitlements &&
  fail "Calc-U-59.entitlements (Release) has com.apple.security.network.server (C59-5 AC-13)"
n=$(/usr/bin/grep -c 'ENABLE_INCOMING_NETWORK_CONNECTIONS = YES;' "$PBX")
[ "$n" = 1 ] || fail "$PBX allows incoming connections in $n configurations, not in Debug only"
/usr/bin/grep -c 'CODE_SIGN_ENTITLEMENTS = "Calc-U-59-Debug.entitlements";' "$PBX" | /usr/bin/grep -qx 1 ||
  fail "$PBX: the Debug entitlements file is not the app's Debug configuration's alone"

# C59-5 AC-2, AC-13 (M-33): the script phase that makes the XCFramework
# declares it as its output, so that Xcode copies it only after the script;
# without it a Release build right after a Debug one linked the Debug
# XCFramework, attach included.
/usr/bin/grep -A3 'outputPaths = (' "$PBX" | /usr/bin/grep -q 'build/xcframework-59/CalcU59.xcframework"' ||
  fail "$PBX: the phase 'Build CalcU59.xcframework' does not declare the XCFramework as its output"

[ $status = 0 ] && echo "ok: the Xcode project compiles only Swift and the bridge, links CalcU59.xcframework and bundles the 24 ROM files"
exit $status
