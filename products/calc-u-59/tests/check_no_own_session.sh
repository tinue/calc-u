#!/bin/sh
# LIB-3 AC-28 (design P59-1; C59-5 AC-4): the Qt6 and Swift frontends host the
# library's app session and use none of the product's own session classes; the
# app layer has no thread but the Swift frontend's session thread (C59-5 D-11).
# Run from products/calc-u-59. Exit 1 with the lines found.
set -u
fail=0
found=$(grep -rnE '\b(EmulatorSession|EmulationRunner|SessionController::create|std::make_unique<(cu59::)?SessionController>|TISessionController)\b' \
    frontends/qt6/app frontends/swift/App frontends/swift/Bridge 2>/dev/null \
    | grep -vE '^[^:]+:[0-9]+:\s*(//|\*|///)')
if [ -n "$found" ]; then
    echo "A frontend uses the product's own session classes (LIB-3 AC-28):"
    echo "$found"
    fail=1
fi
threads=$(grep -rnE 'std::thread\b' app | grep -v '^app/SessionThread\.' | grep -vE '^[^:]+:[0-9]+:\s*//')
if [ -n "$threads" ]; then
    echo "A thread in the app layer outside SessionThread (ADR 0009; C59-5 D-11):"
    echo "$threads"
    fail=1
fi
# Qt6 sends neither suspend nor continue (LIB-3 AC-31; ODD59-25).
if grep -rnE 'kSuspendCommand|kContinueCommand' frontends/qt6/app >/dev/null 2>&1; then
    echo "The Qt6 frontend sends suspend or continue (LIB-3 AC-31)."
    fail=1
fi
[ $fail -eq 0 ] && echo "ok"
exit $fail
