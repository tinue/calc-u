#!/bin/sh
# A release build, or any build for a real iPhone or
# iPad, contains no harness attach: no AttachEndpoint or AttachedHarness and
# none of attach's texts in its code.
#
#   check_no_attach.sh --sources        what decides it in the sources (Frequent,
#                                       CTest CalcU59NoAttachSources)
#   check_no_attach.sh --binary <file>  one binary or static library; the app
#                                       target's build phase runs it on every
#                                       Release and device build
#   check_no_attach.sh                  builds the Release iOS device app, the
#                                       Release XCFramework and the Debug iOS
#                                       device app, and checks them all (Longer,
#                                       CTest CalcU59NoAttachInRelease; a few
#                                       minutes)
set -u
HERE="$(cd "$(dirname "$0")" && pwd)"
status=0
fail() { echo "FAIL: $1"; status=1; }

# One binary: no attach symbol (names survive in a static library and in an
# unstripped app) and no text only attach has (texts survive stripping).
check_binary() {
  f="$1"
  before=$status; status=0
  [ -f "$f" ] || { fail "$f: not found"; status=1; return; }
  # Read once; a file that cannot be read fails (never a silent pass).
  syms=$(nm -C "$f" 2>&1) || { fail "$f cannot be read: $(echo "$syms" | head -1)"; return; }
  if echo "$syms" | /usr/bin/grep -qE 'calcu::harness::(AttachEndpoint|AttachedHarness)'; then
    fail "$f has attach symbols (AttachEndpoint / AttachedHarness)"
  fi
  if strings -a "$f" | /usr/bin/grep -qE 'one harness is already attached|Harness attach on 127\.0\.0\.1'; then
    fail "$f has attach's texts"
  fi
  [ $status = 0 ] && echo "ok: no attach in $f"
  [ $before = 0 ] || status=1
  # A Debug app's code is in <name>.debug.dylib beside its stub executable.
  if [ -f "$f.debug.dylib" ]; then check_binary "$f.debug.dylib"; fi
}

check_sources() {
  cd "$HERE"
  # build_xcframework.sh: attach only for Debug, only in the ios-simulator
  # slice; the device slice never.
  /usr/bin/grep -q '^\[ "\$CONFIG" = Debug \] && DEV_ATTACH=ON$' build_xcframework.sh ||
    fail "build_xcframework.sh: attach is not tied to the Debug configuration"
  /usr/bin/grep -qE '^slice ios OFF ' build_xcframework.sh || fail "build_xcframework.sh: the device slice may have attach"
  [ "$(/usr/bin/grep -cE '^slice [a-z-]+ "\$DEV_ATTACH"' build_xcframework.sh)" = 1 ] &&
    /usr/bin/grep -qE '^slice ios-simulator "\$DEV_ATTACH"' build_xcframework.sh ||
    fail "build_xcframework.sh: attach in another slice than the Debug ios-simulator"
  # The call sites: the bridge's and the Swift frontend's, under the same guard.
  guard='DEBUG && TARGET_OS_SIMULATOR'
  n=$(/usr/bin/grep -c "^#if $guard" Bridge/TIAppSession.h Bridge/TIAppSession.mm | awk -F: '{s+=$2} END {print s}')
  [ "$n" -ge 3 ] || fail "TIAppSession: attach is not under #if $guard"
  /usr/bin/grep -n 'openAttach' App/*.swift | /usr/bin/grep -q . || fail "no openAttach call found in App/"
  awk '/^ *#if DEBUG && targetEnvironment\(simulator\)/ {g=1} /^ *#endif/ {g=0}
       /openAttach/ && !g {print FILENAME ":" FNR ": " $0; bad=1} END {exit bad}' App/*.swift ||
    fail "App/: openAttach outside #if DEBUG && targetEnvironment(simulator)"
  [ $status = 0 ] && echo "ok: attach only in Debug builds for the simulator"
}

case "${1:-}" in
  --sources) check_sources; exit $status ;;
  --binary) check_binary "$2"; exit $status ;;
  "") ;;
  *) echo "usage: check_no_attach.sh [--sources | --binary <file>]" >&2; exit 2 ;;
esac

# Full check: build, then look.
check_sources
REPO="$(cd "$HERE/../../../.." && pwd)"
DD="$REPO/build/xcode-no-attach"
XCF="$REPO/build/xcframework-59"
cd "$HERE"
echo "building the Release iOS device app (unsigned) ..."
xcodebuild build -project Calc-U-59.xcodeproj -scheme Calc-U-59 -configuration Release \
  -destination 'generic/platform=iOS' -derivedDataPath "$DD" CODE_SIGNING_ALLOWED=NO -quiet ||
  fail "the Release iOS device build failed"
check_binary "$DD/Build/Products/Release-iphoneos/Calc-U-59.app/Calc-U-59"
for s in ios-arm64 ios-arm64_x86_64-simulator; do check_binary "$XCF/CalcU59.xcframework/$s/libCalcU59.a"; done
echo "building the Debug iOS device app (unsigned) ..."
xcodebuild build -project Calc-U-59.xcodeproj -scheme Calc-U-59 -configuration Debug \
  -destination 'generic/platform=iOS' -derivedDataPath "$DD" CODE_SIGNING_ALLOWED=NO -quiet ||
  fail "the Debug iOS device build failed"
check_binary "$DD/Build/Products/Debug-iphoneos/Calc-U-59.app/Calc-U-59"
check_binary "$XCF/Debug/ios/libCalcU59.a"
# The XCFramework is the Debug one again for the next Xcode build of the
# Debug app (it is made for one configuration at a time).
[ $status = 0 ] && echo "ok: no attach in the Release and device builds"
exit $status
