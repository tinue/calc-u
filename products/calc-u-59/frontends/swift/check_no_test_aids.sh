#!/bin/sh
# The Swift app holds no aid that exists only for its tests (the launch
# argument --UITesting, the hidden R/S button, the element that tells the
# state of a preset, the --setting and --module launch values); the tests
# reach the session through harness attach.
# CTest CalcU59NoTestAids (Frequent). Run from products/calc-u-59.
set -u
cd "$(dirname "$0")"
found=$(grep -rnE -- '--UITesting|isUITesting|btn-key-rs|keystroke-playback-status|"--setting"|"--module"|isKeystrokesPlaying' \
    App Bridge 2>/dev/null)
if [ -n "$found" ]; then
    echo "The Swift app keeps an aid for its tests:"
    echo "$found"
    exit 1
fi
exit 0
