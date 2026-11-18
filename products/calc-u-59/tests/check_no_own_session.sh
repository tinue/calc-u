#!/bin/sh
# The Qt6, Swift and Web frontends host the
# library's app session and use none of the product's own session classes; the
# app layer has no thread but the Swift frontend's session thread.
# Run from products/calc-u-59. Exit 1 with the lines found.
set -u
fail=0
found=$(grep -rnE '\b(EmulatorSession|EmulationRunner|SessionController::create|std::make_unique<(cu59::)?SessionController>|TISessionController)\b' \
    frontends/qt6/app frontends/swift/App frontends/swift/Bridge frontends/web/bindings.cpp frontends/web/src 2>/dev/null \
    | grep -vE '^[^:]+:[0-9]+:\s*(//|\*|///)')
if [ -n "$found" ]; then
    echo "A frontend uses the product's own session classes:"
    echo "$found"
    fail=1
fi
threads=$(grep -rnE 'std::thread\b' app | grep -v '^app/SessionThread\.' | grep -vE '^[^:]+:[0-9]+:\s*//')
if [ -n "$threads" ]; then
    echo "A thread in the app layer outside SessionThread (the core holds no lock):"
    echo "$threads"
    fail=1
fi
# The bridge reaches the session only through cu59::SessionThread: it includes no SessionController.hpp and calls no tick.
if grep -nE '#include .*SessionController\.hpp|->tick\(|\.tick\(' frontends/swift/Bridge/* >/dev/null 2>&1; then
    echo "The Swift bridge reaches past the session thread:"
    grep -nE '#include .*SessionController\.hpp|->tick\(|\.tick\(' frontends/swift/Bridge/*
    fail=1
fi
# Qt6 sends neither suspend nor continue: the desktop app keeps running in
# the background, Swift and the Web pause.
if grep -rnE 'kSuspendCommand|kContinueCommand' frontends/qt6/app >/dev/null 2>&1; then
    echo "The Qt6 frontend sends suspend or continue."
    fail=1
fi
[ $fail -eq 0 ] && echo "ok"
exit $fail
