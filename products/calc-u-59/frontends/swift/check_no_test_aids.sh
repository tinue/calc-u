#!/bin/sh
# C59-8 AC-21, LIB-4 AC-15 (design §2, T11, T12): the Swift app holds no aid
# that exists only for its tests (the launch argument --UITesting, the
# hidden R/S button, the element that tells the state of a preset, the
# --setting and --module launch values) and no copy of the model facts
# (MachineModel); the tests reach the session through harness attach.
# CTest CalcU59NoTestAids (Frequent). Run from products/calc-u-59.
set -u
cd "$(dirname "$0")"
status=0
found=$(grep -rnE -- '--UITesting|isUITesting|btn-key-rs|keystroke-playback-status|"--setting"|"--module"|MachineModel|isKeystrokesPlaying' \
    App Bridge 2>/dev/null)
if [ -n "$found" ]; then
    echo "The Swift app keeps an aid for its tests or the model facts (C59-8 AC-21, LIB-4 AC-15):"
    echo "$found"
    status=1
fi
if [ -e App/MachineModel.swift ]; then
    echo "App/MachineModel.swift is still there (LIB-4 AC-15)"
    status=1
fi
exit $status
